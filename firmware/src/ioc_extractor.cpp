#include "ioc_extractor.h"
#include <regex>

IOCExtractor::IOCExtractor() {
    storage = nullptr;
    filter_private_ips = true;
    filter_localhost = true;
    min_confidence = 50;
}

IOCExtractor::~IOCExtractor() {
}

void IOCExtractor::begin(FRFDStorage* storage_ptr) {
    storage = storage_ptr;
    initializeWhitelists();
}

void IOCExtractor::initializeWhitelists() {
    // Common legitimate domains to filter out
    domain_whitelist.insert("microsoft.com");
    domain_whitelist.insert("windows.com");
    domain_whitelist.insert("apple.com");
    domain_whitelist.insert("google.com");
    domain_whitelist.insert("mozilla.org");
    domain_whitelist.insert("ubuntu.com");
    domain_whitelist.insert("debian.org");
    domain_whitelist.insert("redhat.com");
    domain_whitelist.insert("localhost");
    domain_whitelist.insert("local");

    // Localhost IPs
    ip_whitelist.insert("127.0.0.1");
    ip_whitelist.insert("::1");
    ip_whitelist.insert("0.0.0.0");
}

bool IOCExtractor::extractFromFile(const String& file_path) {
    if (!storage || !storage->fileExists(file_path)) {
        return false;
    }

    String content = storage->readFile(file_path);
    if (content.isEmpty()) {
        return false;
    }

    // Extract all IOC types
    auto ips = extractIPAddresses(content, file_path);
    auto domains = extractDomains(content, file_path);
    auto urls = extractURLs(content, file_path);
    auto hashes = extractFileHashes(content, file_path);
    auto emails = extractEmails(content, file_path);
    auto registry = extractRegistryKeys(content, file_path);
    auto paths = extractFilePaths(content, file_path);
    auto mutexes = extractMutexes(content, file_path);
    auto cves = extractCVEs(content, file_path);

    // Add all to master list
    for (const auto& ioc : ips) addIOC(ioc);
    for (const auto& ioc : domains) addIOC(ioc);
    for (const auto& ioc : urls) addIOC(ioc);
    for (const auto& ioc : hashes) addIOC(ioc);
    for (const auto& ioc : emails) addIOC(ioc);
    for (const auto& ioc : registry) addIOC(ioc);
    for (const auto& ioc : paths) addIOC(ioc);
    for (const auto& ioc : mutexes) addIOC(ioc);
    for (const auto& ioc : cves) addIOC(ioc);

    return true;
}

bool IOCExtractor::extractFromDirectory(const String& dir_path) {
    if (!storage) return false;

    std::vector<String> files = storage->getFileList(dir_path);

    for (const auto& file : files) {
        String full_path = dir_path + "/" + file;
        extractFromFile(full_path);
    }

    return true;
}

bool IOCExtractor::extractFromAllArtifacts() {
    if (!storage) return false;

    String case_dir = storage->getCaseDirectory();
    if (case_dir.isEmpty()) return false;

    return extractFromDirectory(case_dir);
}

std::vector<IOC> IOCExtractor::extractIPAddresses(const String& content, const String& source) {
    std::vector<IOC> found_iocs;

    // IPv4 pattern: xxx.xxx.xxx.xxx
    int pos = 0;
    while (pos < content.length()) {
        // Find potential IP start (digit after non-alphanumeric)
        while (pos < content.length() && !isdigit(content[pos])) pos++;
        if (pos >= content.length()) break;

        int start = pos;
        String potential_ip = "";

        // Extract potential IP (digits and dots)
        while (pos < content.length() && (isdigit(content[pos]) || content[pos] == '.')) {
            potential_ip += content[pos];
            pos++;
        }

        // Validate IP
        if (isValidIPv4(potential_ip)) {
            // Apply filters
            if (filter_localhost && (potential_ip == "127.0.0.1" || potential_ip.startsWith("127."))) {
                continue;
            }
            if (filter_private_ips && isPrivateIP(potential_ip)) {
                continue;
            }
            if (isInWhitelist(potential_ip, IOC_IP_ADDRESS)) {
                continue;
            }

            IOC ioc;
            ioc.type = IOC_IP_ADDRESS;
            ioc.value = potential_ip;
            ioc.source_artifact = source;
            ioc.context = content.substring(max(0, start - 20), min((int)content.length(), pos + 20));
            ioc.timestamp = millis();
            ioc.confidence = calculateConfidence(potential_ip, IOC_IP_ADDRESS);

            if (ioc.confidence >= min_confidence) {
                found_iocs.push_back(ioc);
            }
        }
    }

    return found_iocs;
}

std::vector<IOC> IOCExtractor::extractDomains(const String& content, const String& source) {
    std::vector<IOC> found_iocs;

    // Domain pattern: word.word.tld
    int pos = 0;
    while (pos < content.length()) {
        // Find potential domain start
        if (isalnum(content[pos])) {
            int start = pos;
            String potential_domain = "";

            // Extract potential domain (alphanumeric, dots, hyphens)
            while (pos < content.length() &&
                   (isalnum(content[pos]) || content[pos] == '.' || content[pos] == '-')) {
                potential_domain += content[pos];
                pos++;
            }

            // Validate domain
            if (isValidDomain(potential_domain) && !isInWhitelist(potential_domain, IOC_DOMAIN)) {
                IOC ioc;
                ioc.type = IOC_DOMAIN;
                ioc.value = potential_domain;
                ioc.source_artifact = source;
                ioc.context = content.substring(max(0, start - 20), min((int)content.length(), pos + 20));
                ioc.timestamp = millis();
                ioc.confidence = calculateConfidence(potential_domain, IOC_DOMAIN);

                if (ioc.confidence >= min_confidence) {
                    found_iocs.push_back(ioc);
                }
            }
        } else {
            pos++;
        }
    }

    return found_iocs;
}

std::vector<IOC> IOCExtractor::extractURLs(const String& content, const String& source) {
    std::vector<IOC> found_iocs;

    // URL patterns
    const char* protocols[] = {"http://", "https://", "ftp://", "file://"};

    for (const char* proto : protocols) {
        int pos = content.indexOf(proto);
        while (pos >= 0) {
            int start = pos;
            int end = pos + strlen(proto);

            // Extract URL until whitespace or quote
            while (end < content.length() &&
                   content[end] != ' ' && content[end] != '\n' &&
                   content[end] != '\r' && content[end] != '"' &&
                   content[end] != '\'' && content[end] != '<' && content[end] != '>') {
                end++;
            }

            String url = content.substring(start, end);

            IOC ioc;
            ioc.type = IOC_URL;
            ioc.value = url;
            ioc.source_artifact = source;
            ioc.context = content.substring(max(0, start - 20), min((int)content.length(), end + 20));
            ioc.timestamp = millis();
            ioc.confidence = calculateConfidence(url, IOC_URL);

            if (ioc.confidence >= min_confidence) {
                found_iocs.push_back(ioc);
            }

            pos = content.indexOf(proto, end);
        }
    }

    return found_iocs;
}

std::vector<IOC> IOCExtractor::extractFileHashes(const String& content, const String& source) {
    std::vector<IOC> found_iocs;

    // Look for hash patterns (hex strings of specific lengths)
    int pos = 0;
    while (pos < content.length()) {
        if (isxdigit(content[pos])) {
            int start = pos;
            String hex_string = "";

            // Extract hex characters
            while (pos < content.length() && isxdigit(content[pos])) {
                hex_string += content[pos];
                pos++;
            }

            IOCType hash_type = IOC_UNKNOWN;

            // Determine hash type by length
            if (hex_string.length() == 32 && isValidMD5(hex_string)) {
                hash_type = IOC_FILE_HASH_MD5;
            } else if (hex_string.length() == 40 && isValidSHA1(hex_string)) {
                hash_type = IOC_FILE_HASH_SHA1;
            } else if (hex_string.length() == 64 && isValidSHA256(hex_string)) {
                hash_type = IOC_FILE_HASH_SHA256;
            }

            if (hash_type != IOC_UNKNOWN) {
                IOC ioc;
                ioc.type = hash_type;
                ioc.value = hex_string;
                ioc.source_artifact = source;
                ioc.context = content.substring(max(0, start - 20), min((int)content.length(), pos + 20));
                ioc.timestamp = millis();
                ioc.confidence = calculateConfidence(hex_string, hash_type);

                if (ioc.confidence >= min_confidence) {
                    found_iocs.push_back(ioc);
                }
            }
        } else {
            pos++;
        }
    }

    return found_iocs;
}

std::vector<IOC> IOCExtractor::extractEmails(const String& content, const String& source) {
    std::vector<IOC> found_iocs;

    // Email pattern: word@domain.tld
    int pos = 0;
    while (pos < content.length()) {
        int at_pos = content.indexOf('@', pos);
        if (at_pos < 0) break;

        // Extract backwards to start
        int start = at_pos - 1;
        while (start >= 0 && (isalnum(content[start]) || content[start] == '.' ||
                              content[start] == '_' || content[start] == '-')) {
            start--;
        }
        start++;

        // Extract forwards to end
        int end = at_pos + 1;
        while (end < content.length() && (isalnum(content[end]) || content[end] == '.' ||
                                          content[end] == '-')) {
            end++;
        }

        String email = content.substring(start, end);

        if (isValidEmail(email)) {
            IOC ioc;
            ioc.type = IOC_EMAIL;
            ioc.value = email;
            ioc.source_artifact = source;
            ioc.context = content.substring(max(0, start - 20), min((int)content.length(), end + 20));
            ioc.timestamp = millis();
            ioc.confidence = calculateConfidence(email, IOC_EMAIL);

            if (ioc.confidence >= min_confidence) {
                found_iocs.push_back(ioc);
            }
        }

        pos = at_pos + 1;
    }

    return found_iocs;
}

std::vector<IOC> IOCExtractor::extractRegistryKeys(const String& content, const String& source) {
    std::vector<IOC> found_iocs;

    // Registry key patterns: HKLM\, HKCU\, HKEY_
    const char* prefixes[] = {"HKLM\\", "HKCU\\", "HKCR\\", "HKU\\", "HKEY_LOCAL_MACHINE\\", "HKEY_CURRENT_USER\\"};

    for (const char* prefix : prefixes) {
        int pos = content.indexOf(prefix);
        while (pos >= 0) {
            int start = pos;
            int end = pos + strlen(prefix);

            // Extract until newline or certain characters
            while (end < content.length() && content[end] != '\n' && content[end] != '\r' &&
                   content[end] != '"' && content[end] != '<' && content[end] != '>') {
                end++;
            }

            String reg_key = content.substring(start, end);

            IOC ioc;
            ioc.type = IOC_REGISTRY_KEY;
            ioc.value = reg_key;
            ioc.source_artifact = source;
            ioc.context = content.substring(max(0, start - 20), min((int)content.length(), end + 20));
            ioc.timestamp = millis();
            ioc.confidence = calculateConfidence(reg_key, IOC_REGISTRY_KEY);

            if (ioc.confidence >= min_confidence) {
                found_iocs.push_back(ioc);
            }

            pos = content.indexOf(prefix, end);
        }
    }

    return found_iocs;
}

std::vector<IOC> IOCExtractor::extractFilePaths(const String& content, const String& source) {
    std::vector<IOC> found_iocs;

    // Windows paths: C:\, D:\, etc.
    // Linux paths: /usr/, /etc/, /tmp/, etc.
    const char* path_indicators[] = {"C:\\", "D:\\", "E:\\", "F:\\", "/usr/", "/etc/", "/tmp/", "/var/", "/home/"};

    for (const char* indicator : path_indicators) {
        int pos = content.indexOf(indicator);
        while (pos >= 0) {
            int start = pos;
            int end = pos + strlen(indicator);

            // Extract path
            while (end < content.length() && content[end] != '\n' && content[end] != '\r' &&
                   content[end] != '"' && content[end] != '<' && content[end] != '>' &&
                   content[end] != ' ') {
                end++;
            }

            String file_path = content.substring(start, end);

            // Filter out very short paths
            if (file_path.length() > strlen(indicator) + 3) {
                IOC ioc;
                ioc.type = IOC_FILE_PATH;
                ioc.value = file_path;
                ioc.source_artifact = source;
                ioc.context = content.substring(max(0, start - 20), min((int)content.length(), end + 20));
                ioc.timestamp = millis();
                ioc.confidence = calculateConfidence(file_path, IOC_FILE_PATH);

                if (ioc.confidence >= min_confidence) {
                    found_iocs.push_back(ioc);
                }
            }

            pos = content.indexOf(indicator, end);
        }
    }

    return found_iocs;
}

std::vector<IOC> IOCExtractor::extractMutexes(const String& content, const String& source) {
    std::vector<IOC> found_iocs;

    // Look for mutex patterns (often have specific prefixes)
    const char* mutex_indicators[] = {"Global\\", "Local\\", "Session\\", "BaseNamedObjects\\"};

    for (const char* indicator : mutex_indicators) {
        int pos = content.indexOf(indicator);
        while (pos >= 0) {
            int start = pos;
            int end = pos + strlen(indicator);

            // Extract mutex name
            while (end < content.length() && (isalnum(content[end]) || content[end] == '_' ||
                                               content[end] == '-' || content[end] == '{' ||
                                               content[end] == '}')) {
                end++;
            }

            String mutex = content.substring(start, end);

            if (mutex.length() > strlen(indicator) + 3) {
                IOC ioc;
                ioc.type = IOC_MUTEX;
                ioc.value = mutex;
                ioc.source_artifact = source;
                ioc.context = content.substring(max(0, start - 20), min((int)content.length(), end + 20));
                ioc.timestamp = millis();
                ioc.confidence = calculateConfidence(mutex, IOC_MUTEX);

                if (ioc.confidence >= min_confidence) {
                    found_iocs.push_back(ioc);
                }
            }

            pos = content.indexOf(indicator, end);
        }
    }

    return found_iocs;
}

std::vector<IOC> IOCExtractor::extractCVEs(const String& content, const String& source) {
    std::vector<IOC> found_iocs;

    // CVE pattern: CVE-YYYY-NNNNN
    int pos = content.indexOf("CVE-");
    while (pos >= 0) {
        int start = pos;
        int end = pos + 4; // "CVE-"

        // Extract year (4 digits)
        if (end + 4 < content.length() && isdigit(content[end])) {
            end += 4;

            if (content[end] == '-') {
                end++;

                // Extract number (at least 4 digits)
                int num_start = end;
                while (end < content.length() && isdigit(content[end])) {
                    end++;
                }

                if (end - num_start >= 4) {
                    String cve = content.substring(start, end);

                    IOC ioc;
                    ioc.type = IOC_CVE;
                    ioc.value = cve;
                    ioc.source_artifact = source;
                    ioc.context = content.substring(max(0, start - 20), min((int)content.length(), end + 20));
                    ioc.timestamp = millis();
                    ioc.confidence = 95; // CVEs are highly structured

                    found_iocs.push_back(ioc);
                }
            }
        }

        pos = content.indexOf("CVE-", end);
    }

    return found_iocs;
}

void IOCExtractor::addIOC(const IOC& ioc) {
    iocs.push_back(ioc);
}

void IOCExtractor::clearIOCs() {
    iocs.clear();
}

std::vector<IOC> IOCExtractor::getIOCsByType(IOCType type) const {
    std::vector<IOC> filtered;
    for (const auto& ioc : iocs) {
        if (ioc.type == type) {
            filtered.push_back(ioc);
        }
    }
    return filtered;
}

std::vector<IOC> IOCExtractor::getUniqueIOCs() const {
    std::vector<IOC> unique;
    std::set<String> seen;

    for (const auto& ioc : iocs) {
        String key = String(ioc.type) + ":" + ioc.value;
        if (seen.find(key) == seen.end()) {
            unique.push_back(ioc);
            seen.insert(key);
        }
    }

    return unique;
}

String IOCExtractor::exportToJSON() {
    String json = "{";
    json += "\"extraction_timestamp\":" + String(millis()) + ",";
    json += "\"total_iocs\":" + String(iocs.size()) + ",";

    // Statistics
    auto stats = getStatistics();
    json += "\"statistics\":{";
    json += "\"ip_addresses\":" + String(stats.ip_addresses) + ",";
    json += "\"domains\":" + String(stats.domains) + ",";
    json += "\"urls\":" + String(stats.urls) + ",";
    json += "\"file_hashes\":" + String(stats.file_hashes) + ",";
    json += "\"emails\":" + String(stats.emails) + ",";
    json += "\"registry_keys\":" + String(stats.registry_keys) + ",";
    json += "\"file_paths\":" + String(stats.file_paths) + ",";
    json += "\"unique_iocs\":" + String(stats.unique_iocs);
    json += "},";

    // IOC list
    json += "\"iocs\":[";
    for (size_t i = 0; i < iocs.size(); i++) {
        if (i > 0) json += ",";

        json += "{";
        json += "\"type\":\"" + getIOCTypeName(iocs[i].type) + "\",";

        String escapedValue = iocs[i].value;
        escapedValue.replace("\\", "\\\\");
        escapedValue.replace("\"", "\\\"");
        json += "\"value\":\"" + escapedValue + "\",";

        json += "\"source\":\"" + iocs[i].source_artifact + "\",";
        json += "\"confidence\":" + String(iocs[i].confidence) + ",";
        json += "\"timestamp\":" + String(iocs[i].timestamp);
        json += "}";
    }
    json += "]";

    json += "}";
    return json;
}

String IOCExtractor::exportToCSV() {
    String csv = "Type,Value,Source,Confidence,Timestamp\n";

    for (const auto& ioc : iocs) {
        String escapedValue = ioc.value;
        escapedValue.replace("\"", "\"\"");

        csv += getIOCTypeName(ioc.type) + ",";
        csv += "\"" + escapedValue + "\",";
        csv += "\"" + ioc.source_artifact + "\",";
        csv += String(ioc.confidence) + ",";
        csv += String(ioc.timestamp) + "\n";
    }

    return csv;
}

String IOCExtractor::exportToSTIX() {
    // Simplified STIX format (STIX 2.1)
    String stix = "{\"type\":\"bundle\",\"id\":\"bundle--" + String(millis()) + "\",";
    stix += "\"objects\":[";

    for (size_t i = 0; i < iocs.size(); i++) {
        if (i > 0) stix += ",";

        stix += "{\"type\":\"indicator\",";
        stix += "\"id\":\"indicator--" + String(iocs[i].timestamp) + "-" + String(i) + "\",";
        stix += "\"pattern\":\"" + getIOCTypeName(iocs[i].type) + ":" + iocs[i].value + "\",";
        stix += "\"confidence\":" + String(iocs[i].confidence) + "}";
    }

    stix += "]}";
    return stix;
}

String IOCExtractor::exportToOpenIOC() {
    // Simplified OpenIOC XML format
    String xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml += "<ioc>\n";
    xml += "  <metadata>\n";
    xml += "    <timestamp>" + String(millis()) + "</timestamp>\n";
    xml += "    <count>" + String(iocs.size()) + "</count>\n";
    xml += "  </metadata>\n";
    xml += "  <indicators>\n";

    for (const auto& ioc : iocs) {
        xml += "    <indicator>\n";
        xml += "      <type>" + getIOCTypeName(ioc.type) + "</type>\n";
        xml += "      <value>" + ioc.value + "</value>\n";
        xml += "      <confidence>" + String(ioc.confidence) + "</confidence>\n";
        xml += "    </indicator>\n";
    }

    xml += "  </indicators>\n";
    xml += "</ioc>\n";
    return xml;
}

bool IOCExtractor::saveToFile(const String& filename, const String& format) {
    if (!storage) return false;

    String content;
    if (format == "json") {
        content = exportToJSON();
    } else if (format == "csv") {
        content = exportToCSV();
    } else if (format == "stix") {
        content = exportToSTIX();
    } else if (format == "openioc") {
        content = exportToOpenIOC();
    } else {
        return false;
    }

    return storage->writeFile(filename, content);
}

IOCStatistics IOCExtractor::getStatistics() const {
    IOCStatistics stats = {0};
    stats.total_iocs = iocs.size();

    for (const auto& ioc : iocs) {
        switch (ioc.type) {
            case IOC_IP_ADDRESS:
                stats.ip_addresses++;
                break;
            case IOC_DOMAIN:
                stats.domains++;
                break;
            case IOC_URL:
                stats.urls++;
                break;
            case IOC_FILE_HASH_MD5:
            case IOC_FILE_HASH_SHA1:
            case IOC_FILE_HASH_SHA256:
                stats.file_hashes++;
                break;
            case IOC_EMAIL:
                stats.emails++;
                break;
            case IOC_REGISTRY_KEY:
                stats.registry_keys++;
                break;
            case IOC_FILE_PATH:
                stats.file_paths++;
                break;
            default:
                break;
        }
    }

    stats.unique_iocs = getUniqueIOCCount();
    return stats;
}

uint16_t IOCExtractor::getUniqueIOCCount() const {
    return getUniqueIOCs().size();
}

// Validation methods
bool IOCExtractor::isValidIPv4(const String& ip) {
    int octets = 0;
    int num = 0;
    bool has_digit = false;

    for (size_t i = 0; i < ip.length(); i++) {
        if (isdigit(ip[i])) {
            num = num * 10 + (ip[i] - '0');
            has_digit = true;
            if (num > 255) return false;
        } else if (ip[i] == '.') {
            if (!has_digit) return false;
            octets++;
            num = 0;
            has_digit = false;
        } else {
            return false;
        }
    }

    return octets == 3 && has_digit;
}

bool IOCExtractor::isValidIPv6(const String& ip) {
    // Simplified IPv6 validation (contains colons and hex chars)
    return ip.indexOf(':') >= 0 && ip.length() >= 3;
}

bool IOCExtractor::isValidDomain(const String& domain) {
    // Must contain at least one dot
    if (domain.indexOf('.') < 0) return false;

    // Must be at least 4 chars (e.g., "a.co")
    if (domain.length() < 4) return false;

    // Must not start or end with dot or hyphen
    if (domain.startsWith(".") || domain.endsWith(".") ||
        domain.startsWith("-") || domain.endsWith("-")) {
        return false;
    }

    // Must end with known TLD or have multiple segments
    return domain.lastIndexOf('.') < domain.length() - 2;
}

bool IOCExtractor::isValidMD5(const String& hash) {
    return hash.length() == 32;
}

bool IOCExtractor::isValidSHA1(const String& hash) {
    return hash.length() == 40;
}

bool IOCExtractor::isValidSHA256(const String& hash) {
    return hash.length() == 64;
}

bool IOCExtractor::isValidEmail(const String& email) {
    int at_pos = email.indexOf('@');
    if (at_pos <= 0) return false;

    int dot_pos = email.lastIndexOf('.');
    if (dot_pos <= at_pos + 1) return false;

    if (dot_pos >= email.length() - 2) return false;

    return true;
}

bool IOCExtractor::isPrivateIP(const String& ip) {
    // 10.0.0.0/8
    if (ip.startsWith("10.")) return true;

    // 172.16.0.0/12
    if (ip.startsWith("172.")) {
        int second_octet = ip.substring(4, ip.indexOf('.', 4)).toInt();
        if (second_octet >= 16 && second_octet <= 31) return true;
    }

    // 192.168.0.0/16
    if (ip.startsWith("192.168.")) return true;

    // 169.254.0.0/16 (APIPA)
    if (ip.startsWith("169.254.")) return true;

    return false;
}

String IOCExtractor::getIOCTypeName(IOCType type) const {
    switch (type) {
        case IOC_IP_ADDRESS: return "ip_address";
        case IOC_DOMAIN: return "domain";
        case IOC_URL: return "url";
        case IOC_FILE_HASH_MD5: return "md5";
        case IOC_FILE_HASH_SHA1: return "sha1";
        case IOC_FILE_HASH_SHA256: return "sha256";
        case IOC_EMAIL: return "email";
        case IOC_REGISTRY_KEY: return "registry_key";
        case IOC_FILE_PATH: return "file_path";
        case IOC_MUTEX: return "mutex";
        case IOC_USER_AGENT: return "user_agent";
        case IOC_CVE: return "cve";
        default: return "unknown";
    }
}

uint8_t IOCExtractor::calculateConfidence(const String& value, IOCType type) {
    // Base confidence by type
    uint8_t confidence = 50;

    switch (type) {
        case IOC_FILE_HASH_MD5:
        case IOC_FILE_HASH_SHA1:
        case IOC_FILE_HASH_SHA256:
            confidence = 95; // Hashes are highly specific
            break;
        case IOC_CVE:
            confidence = 95; // CVEs are structured
            break;
        case IOC_IP_ADDRESS:
            confidence = !isPrivateIP(value) ? 80 : 60;
            break;
        case IOC_EMAIL:
            confidence = 75;
            break;
        case IOC_URL:
            confidence = 85;
            break;
        case IOC_DOMAIN:
            confidence = value.length() > 10 ? 70 : 60;
            break;
        case IOC_REGISTRY_KEY:
            confidence = 70;
            break;
        case IOC_MUTEX:
            confidence = 80; // Mutexes are fairly unique
            break;
        default:
            confidence = 50;
    }

    return confidence;
}

bool IOCExtractor::isInWhitelist(const String& value, IOCType type) {
    if (type == IOC_DOMAIN) {
        // Check if domain ends with whitelisted domain
        for (const auto& whitelisted : domain_whitelist) {
            if (value.endsWith(whitelisted) || value == whitelisted) {
                return true;
            }
        }
    } else if (type == IOC_IP_ADDRESS) {
        if (ip_whitelist.find(value) != ip_whitelist.end()) {
            return true;
        }
    }

    return false;
}

#ifndef IOC_EXTRACTOR_H
#define IOC_EXTRACTOR_H

#include <Arduino.h>
#include <vector>
#include <set>
#include "storage.h"

/**
 * @brief IOC (Indicator of Compromise) Types
 */
enum IOCType {
    IOC_IP_ADDRESS,
    IOC_DOMAIN,
    IOC_URL,
    IOC_FILE_HASH_MD5,
    IOC_FILE_HASH_SHA1,
    IOC_FILE_HASH_SHA256,
    IOC_EMAIL,
    IOC_REGISTRY_KEY,
    IOC_FILE_PATH,
    IOC_MUTEX,
    IOC_USER_AGENT,
    IOC_CVE,
    IOC_UNKNOWN
};

/**
 * @brief IOC Entry
 */
struct IOC {
    IOCType type;
    String value;
    String source_artifact;      // Which artifact it was found in
    String context;              // Surrounding context
    unsigned long timestamp;     // When it was extracted
    uint8_t confidence;          // Confidence score 0-100
};

/**
 * @brief IOC Statistics
 */
struct IOCStatistics {
    uint16_t total_iocs;
    uint16_t ip_addresses;
    uint16_t domains;
    uint16_t urls;
    uint16_t file_hashes;
    uint16_t emails;
    uint16_t registry_keys;
    uint16_t file_paths;
    uint16_t unique_iocs;
};

/**
 * @brief IOC Extractor
 *
 * Extracts indicators of compromise from forensic artifacts
 * Supports multiple output formats (CSV, JSON, STIX, OpenIOC)
 */
class IOCExtractor {
public:
    IOCExtractor();
    ~IOCExtractor();

    // Initialization
    void begin(FRFDStorage* storage_ptr);

    // Extraction Methods
    bool extractFromFile(const String& file_path);
    bool extractFromDirectory(const String& dir_path);
    bool extractFromAllArtifacts();

    // Pattern Matching
    std::vector<IOC> extractIPAddresses(const String& content, const String& source);
    std::vector<IOC> extractDomains(const String& content, const String& source);
    std::vector<IOC> extractURLs(const String& content, const String& source);
    std::vector<IOC> extractFileHashes(const String& content, const String& source);
    std::vector<IOC> extractEmails(const String& content, const String& source);
    std::vector<IOC> extractRegistryKeys(const String& content, const String& source);
    std::vector<IOC> extractFilePaths(const String& content, const String& source);
    std::vector<IOC> extractMutexes(const String& content, const String& source);
    std::vector<IOC> extractCVEs(const String& content, const String& source);

    // IOC Management
    void addIOC(const IOC& ioc);
    void clearIOCs();
    std::vector<IOC> getIOCs() const { return iocs; }
    std::vector<IOC> getIOCsByType(IOCType type) const;
    std::vector<IOC> getUniqueIOCs() const;

    // Filtering
    void filterPrivateIPs(bool enable) { filter_private_ips = enable; }
    void filterLocalhost(bool enable) { filter_localhost = enable; }
    void setMinConfidence(uint8_t confidence) { min_confidence = confidence; }

    // Export Methods
    String exportToJSON();
    String exportToCSV();
    String exportToSTIX();
    String exportToOpenIOC();
    bool saveToFile(const String& filename, const String& format);

    // Statistics
    IOCStatistics getStatistics() const;
    uint16_t getIOCCount() const { return iocs.size(); }
    uint16_t getUniqueIOCCount() const;

    // Validation
    bool isValidIPv4(const String& ip);
    bool isValidIPv6(const String& ip);
    bool isValidDomain(const String& domain);
    bool isValidMD5(const String& hash);
    bool isValidSHA1(const String& hash);
    bool isValidSHA256(const String& hash);
    bool isValidEmail(const String& email);
    bool isPrivateIP(const String& ip);

private:
    FRFDStorage* storage;
    std::vector<IOC> iocs;

    // Filtering options
    bool filter_private_ips;
    bool filter_localhost;
    uint8_t min_confidence;

    // Helper methods
    String getIOCTypeName(IOCType type) const;
    uint8_t calculateConfidence(const String& value, IOCType type);
    bool isInWhitelist(const String& value, IOCType type);
    void deduplicateIOCs();

    // Pattern matching helpers
    bool matchesPattern(const String& text, const String& pattern);
    std::vector<String> findMatches(const String& content, const String& pattern);

    // Whitelists
    std::set<String> domain_whitelist;
    std::set<String> ip_whitelist;

    void initializeWhitelists();
};

#endif // IOC_EXTRACTOR_H

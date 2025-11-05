#include "correlation_engine.h"

CorrelationEngine::CorrelationEngine() {
    ioc_extractor = nullptr;
    timeline_generator = nullptr;
}

CorrelationEngine::~CorrelationEngine() {
}

void CorrelationEngine::begin() {
    clearCorrelations();
    detected_patterns.clear();
}

void CorrelationEngine::setIOCExtractor(IOCExtractor* ioc_ptr) {
    ioc_extractor = ioc_ptr;
}

void CorrelationEngine::setTimelineGenerator(TimelineGenerator* timeline_ptr) {
    timeline_generator = timeline_ptr;
}

bool CorrelationEngine::analyzeAll() {
    begin(); // Clear previous results

    // Run all correlation analyses
    analyzeTemporalCorrelations();
    analyzeNetworkCorrelations();
    analyzeProcessCorrelations();
    analyzeFileCorrelations();
    analyzeUserCorrelations();
    analyzeIOCCorrelations();

    // Detect attack patterns based on correlations
    detectAttackPatterns();

    return true;
}

bool CorrelationEngine::analyzeTemporalCorrelations() {
    if (!timeline_generator) return false;

    // Analyze events that occurred within time windows
    correlateTemporalEvents(300000); // 5-minute window
    findSequentialEvents();
    findConcurrentEvents();

    return true;
}

bool CorrelationEngine::analyzeNetworkCorrelations() {
    if (!ioc_extractor) return false;

    correlateNetworkConnections();
    correlateDomainWithNetwork();
    correlateIPsWithProcesses();

    return true;
}

bool CorrelationEngine::analyzeProcessCorrelations() {
    if (!timeline_generator) return false;

    // Find processes that executed in sequence
    auto events = timeline_generator->getEventsByType(EVENT_PROCESS_STARTED);

    for (size_t i = 0; i < events.size(); i++) {
        for (size_t j = i + 1; j < events.size() && j < i + 10; j++) {
            if (isTimeProximate(events[i].timestamp, events[j].timestamp, 60000)) {
                Correlation corr;
                corr.type = CORR_PROCESS;
                corr.entity1 = events[i].target;
                corr.entity2 = events[j].target;
                corr.relationship = "Sequential process execution";
                corr.confidence = 70;
                corr.timestamp = events[i].timestamp;
                corr.evidence.push_back("Executed within 60s of each other");
                addCorrelation(corr);
            }
        }
    }

    return true;
}

bool CorrelationEngine::analyzeFileCorrelations() {
    if (!timeline_generator) return false;

    // Find files accessed by same user/process
    auto file_events = timeline_generator->getEventsByType(EVENT_FILE_ACCESSED);

    // Group by actor
    std::map<String, std::vector<TimelineEvent>> actor_files;
    for (const auto& event : file_events) {
        if (!event.actor.isEmpty()) {
            actor_files[event.actor].push_back(event);
        }
    }

    // Correlate files accessed by same actor
    for (const auto& pair : actor_files) {
        if (pair.second.size() > 1) {
            Correlation corr;
            corr.type = CORR_FILE;
            corr.entity1 = pair.first; // Actor
            corr.entity2 = String(pair.second.size()) + " files";
            corr.relationship = "Multiple file access by same actor";
            corr.confidence = 60;
            corr.timestamp = millis();
            addCorrelation(corr);
        }
    }

    return true;
}

bool CorrelationEngine::analyzeUserCorrelations() {
    if (!timeline_generator) return false;

    // Correlate user logins with subsequent activities
    auto logins = timeline_generator->getEventsByType(EVENT_LOGIN_SUCCESS);
    auto all_events = timeline_generator->getEvents();

    for (const auto& login : logins) {
        // Find events within 1 hour of login
        for (const auto& event : all_events) {
            if (event.timestamp > login.timestamp &&
                event.timestamp < login.timestamp + 3600000 && // 1 hour
                event.actor == login.actor) {

                Correlation corr;
                corr.type = CORR_USER;
                corr.entity1 = login.actor;
                corr.entity2 = event.description;
                corr.relationship = "Activity after login";
                corr.confidence = 75;
                corr.timestamp = login.timestamp;
                addCorrelation(corr);
            }
        }
    }

    return true;
}

bool CorrelationEngine::analyzeIOCCorrelations() {
    if (!ioc_extractor || !timeline_generator) return false;

    correlateIPsWithProcesses();
    correlateHashesWithExecutions();

    return true;
}

// Pattern Detection Methods

bool CorrelationEngine::detectAttackPatterns() {
    detectLateralMovement();
    detectDataExfiltration();
    detectPrivilegeEscalation();
    detectPersistence();
    detectReconnaissance();
    detectC2Activity();
    detectCredentialTheft();
    detectMalwareExecution();

    return true;
}

bool CorrelationEngine::detectLateralMovement() {
    if (!timeline_generator) return false;

    // Look for network connections + remote execution indicators
    auto network_events = timeline_generator->getEventsByType(EVENT_NETWORK_CONNECTION);
    auto process_events = timeline_generator->getEventsByType(EVENT_PROCESS_STARTED);

    for (const auto& net : network_events) {
        for (const auto& proc : process_events) {
            if (isTimeProximate(net.timestamp, proc.timestamp, 300000)) { // 5 min
                // Check for lateral movement keywords
                if (containsKeyword(proc.target, "psexec") ||
                    containsKeyword(proc.target, "wmic") ||
                    containsKeyword(proc.target, "schtasks") ||
                    containsKeyword(proc.target, "ssh") ||
                    containsKeyword(proc.target, "rdp")) {

                    PatternDetection pattern = createPattern(
                        PATTERN_LATERAL_MOVEMENT,
                        85,
                        "Potential lateral movement detected: Network activity + remote execution tools"
                    );

                    Correlation corr;
                    corr.type = CORR_TEMPORAL;
                    corr.entity1 = net.description;
                    corr.entity2 = proc.target;
                    corr.relationship = "Network connection followed by remote execution tool";
                    corr.confidence = 85;
                    corr.timestamp = net.timestamp;

                    addPatternCorrelation(pattern, corr);
                    addPatternIndicator(pattern, "Remote execution tool: " + proc.target);
                    pattern.recommendation = "Investigate remote execution activity. Review network connections and verify legitimacy.";

                    detected_patterns.push_back(pattern);
                }
            }
        }
    }

    return true;
}

bool CorrelationEngine::detectDataExfiltration() {
    if (!ioc_extractor || !timeline_generator) return false;

    // Look for large file operations + network connections to external IPs
    auto file_events = timeline_generator->getEventsByType(EVENT_FILE_ACCESSED);
    auto network_events = timeline_generator->getEventsByType(EVENT_NETWORK_CONNECTION);

    auto external_ips = ioc_extractor->getIOCsByType(IOC_IP_ADDRESS);

    if (file_events.size() > 10 && network_events.size() > 0 && external_ips.size() > 0) {
        PatternDetection pattern = createPattern(
            PATTERN_DATA_EXFILTRATION,
            70,
            "Potential data exfiltration: Multiple file accesses + external network connections"
        );

        addPatternIndicator(pattern, String(file_events.size()) + " file access events");
        addPatternIndicator(pattern, String(external_ips.size()) + " external IP connections");
        pattern.recommendation = "Review file access logs and network traffic. Investigate external connections.";

        detected_patterns.push_back(pattern);
    }

    return true;
}

bool CorrelationEngine::detectPrivilegeEscalation() {
    if (!timeline_generator) return false;

    // Look for failed logins followed by successful login (credential stuffing)
    auto failed_logins = timeline_generator->getEventsByType(EVENT_LOGIN_FAILURE);
    auto success_logins = timeline_generator->getEventsByType(EVENT_LOGIN_SUCCESS);

    for (const auto& failed : failed_logins) {
        for (const auto& success : success_logins) {
            if (success.timestamp > failed.timestamp &&
                success.timestamp < failed.timestamp + 3600000 && // 1 hour
                success.actor == failed.actor) {

                PatternDetection pattern = createPattern(
                    PATTERN_PRIVILEGE_ESCALATION,
                    75,
                    "Failed login attempts followed by successful login"
                );

                addPatternIndicator(pattern, "User: " + failed.actor);
                addPatternIndicator(pattern, "Multiple failed attempts before success");
                pattern.recommendation = "Investigate authentication activity for user. Review for credential compromise.";

                detected_patterns.push_back(pattern);
                break; // Only detect once per failed login
            }
        }
    }

    return true;
}

bool CorrelationEngine::detectPersistence() {
    if (!timeline_generator) return false;

    // Look for registry modifications, scheduled tasks, services
    auto registry_events = timeline_generator->getEventsByType(EVENT_REGISTRY_MODIFIED);
    auto service_events = timeline_generator->getEventsByType(EVENT_SERVICE_STARTED);

    if (registry_events.size() > 0 || service_events.size() > 0) {
        for (const auto& event : registry_events) {
            if (containsKeyword(event.target, "Run") ||
                containsKeyword(event.target, "RunOnce") ||
                containsKeyword(event.target, "Startup")) {

                PatternDetection pattern = createPattern(
                    PATTERN_PERSISTENCE,
                    80,
                    "Registry-based persistence mechanism detected"
                );

                addPatternIndicator(pattern, "Registry key: " + event.target);
                pattern.recommendation = "Review autorun registry keys. Remove unauthorized entries.";

                detected_patterns.push_back(pattern);
            }
        }

        for (const auto& event : service_events) {
            PatternDetection pattern = createPattern(
                PATTERN_PERSISTENCE,
                70,
                "Service-based persistence detected"
            );

            addPatternIndicator(pattern, "Service started: " + event.target);
            pattern.recommendation = "Review newly created or modified services.";

            detected_patterns.push_back(pattern);
        }
    }

    return true;
}

bool CorrelationEngine::detectReconnaissance() {
    if (!timeline_generator) return false;

    // Look for enumeration commands
    auto process_events = timeline_generator->getEventsByType(EVENT_PROCESS_STARTED);

    for (const auto& event : process_events) {
        if (containsKeyword(event.target, "net view") ||
            containsKeyword(event.target, "net user") ||
            containsKeyword(event.target, "whoami") ||
            containsKeyword(event.target, "ipconfig") ||
            containsKeyword(event.target, "nslookup") ||
            containsKeyword(event.target, "netstat")) {

            PatternDetection pattern = createPattern(
                PATTERN_RECONNAISSANCE,
                75,
                "Reconnaissance activity detected: System enumeration commands"
            );

            addPatternIndicator(pattern, "Command: " + event.target);
            pattern.recommendation = "Investigate enumeration activity. Review for unauthorized information gathering.";

            detected_patterns.push_back(pattern);
        }
    }

    return true;
}

bool CorrelationEngine::detectC2Activity() {
    if (!ioc_extractor || !timeline_generator) return false;

    // Look for periodic network connections (beaconing)
    auto network_events = timeline_generator->getEventsByType(EVENT_NETWORK_CONNECTION);

    // Simple beaconing detection: multiple connections to same destination
    std::map<String, int> connection_counts;

    for (const auto& event : network_events) {
        auto ips = extractIPsFromString(event.details);
        for (const auto& ip : ips) {
            connection_counts[ip]++;
        }
    }

    for (const auto& pair : connection_counts) {
        if (pair.second >= 5) { // 5+ connections to same IP
            PatternDetection pattern = createPattern(
                PATTERN_COMMAND_CONTROL,
                80,
                "Potential C2 beaconing: Repeated connections to external IP"
            );

            addPatternIndicator(pattern, "IP: " + pair.first);
            addPatternIndicator(pattern, String(pair.second) + " connections detected");
            pattern.recommendation = "Investigate repeated network connections. Block suspicious IPs.";

            detected_patterns.push_back(pattern);
        }
    }

    return true;
}

bool CorrelationEngine::detectCredentialTheft() {
    if (!timeline_generator) return false;

    // Look for credential dumping tools
    auto process_events = timeline_generator->getEventsByType(EVENT_PROCESS_STARTED);

    for (const auto& event : process_events) {
        if (containsKeyword(event.target, "mimikatz") ||
            containsKeyword(event.target, "procdump") ||
            containsKeyword(event.target, "lsass") ||
            containsKeyword(event.target, "secretsdump")) {

            PatternDetection pattern = createPattern(
                PATTERN_CREDENTIAL_THEFT,
                95,
                "Credential theft tool detected"
            );

            addPatternIndicator(pattern, "Tool: " + event.target);
            pattern.recommendation = "IMMEDIATE ACTION: Credential compromise likely. Reset credentials and investigate.";

            detected_patterns.push_back(pattern);
        }
    }

    return true;
}

bool CorrelationEngine::detectMalwareExecution() {
    if (!ioc_extractor) return false;

    // Look for suspicious hashes or execution patterns
    auto hashes = ioc_extractor->getIOCsByType(IOC_FILE_HASH_MD5);
    hashes.insert(hashes.end(),
                 ioc_extractor->getIOCsByType(IOC_FILE_HASH_SHA256).begin(),
                 ioc_extractor->getIOCsByType(IOC_FILE_HASH_SHA256).end());

    if (hashes.size() > 0) {
        PatternDetection pattern = createPattern(
            PATTERN_MALWARE_EXECUTION,
            60,
            "File hashes extracted - potential malware execution"
        );

        addPatternIndicator(pattern, String(hashes.size()) + " file hashes found");
        pattern.recommendation = "Cross-reference hashes with threat intelligence databases (VirusTotal, etc).";

        detected_patterns.push_back(pattern);
    }

    return true;
}

// Helper Methods

void CorrelationEngine::addCorrelation(const Correlation& corr) {
    correlations.push_back(corr);
}

void CorrelationEngine::clearCorrelations() {
    correlations.clear();
}

std::vector<Correlation> CorrelationEngine::getCorrelationsByType(CorrelationType type) const {
    std::vector<Correlation> filtered;
    for (const auto& corr : correlations) {
        if (corr.type == type) {
            filtered.push_back(corr);
        }
    }
    return filtered;
}

std::vector<PatternDetection> CorrelationEngine::getPatternsByType(AttackPattern pattern) const {
    std::vector<PatternDetection> filtered;
    for (const auto& p : detected_patterns) {
        if (p.pattern == pattern) {
            filtered.push_back(p);
        }
    }
    return filtered;
}

bool CorrelationEngine::isTimeProximate(unsigned long time1, unsigned long time2, unsigned long threshold_ms) {
    unsigned long diff = time1 > time2 ? time1 - time2 : time2 - time1;
    return diff <= threshold_ms;
}

PatternDetection CorrelationEngine::createPattern(AttackPattern type, uint8_t confidence, const String& desc) {
    PatternDetection pattern;
    pattern.pattern = type;
    pattern.confidence = confidence;
    pattern.description = desc;
    return pattern;
}

void CorrelationEngine::addPatternIndicator(PatternDetection& pattern, const String& indicator) {
    pattern.indicators.push_back(indicator);
}

void CorrelationEngine::addPatternCorrelation(PatternDetection& pattern, const Correlation& corr) {
    pattern.correlations.push_back(corr);
}

void CorrelationEngine::correlateIPsWithProcesses() {
    // Simplified - would need more sophisticated parsing in production
}

void CorrelationEngine::correlateHashesWithExecutions() {
    // Simplified - would correlate file hashes with process executions
}

void CorrelationEngine::correlateDomainWithNetwork() {
    // Simplified - would correlate domains with network connections
}

void CorrelationEngine::correlateTemporalEvents(unsigned long window_ms) {
    if (!timeline_generator) return;

    auto events = timeline_generator->getEvents();
    for (size_t i = 0; i < events.size(); i++) {
        for (size_t j = i + 1; j < events.size() && j < i + 20; j++) {
            if (isTimeProximate(events[i].timestamp, events[j].timestamp, window_ms)) {
                Correlation corr;
                corr.type = CORR_TEMPORAL;
                corr.entity1 = events[i].description;
                corr.entity2 = events[j].description;
                corr.relationship = "Occurred within " + String((unsigned long)(window_ms / 1000)) + "s";
                corr.confidence = 60;
                corr.timestamp = events[i].timestamp;
                addCorrelation(corr);
            }
        }
    }
}

void CorrelationEngine::findSequentialEvents() {
    // Find events that occur in a specific sequence
}

void CorrelationEngine::findConcurrentEvents() {
    // Find events that occur simultaneously
}

void CorrelationEngine::correlateNetworkConnections() {
    // Correlate network events
}

void CorrelationEngine::findC2Patterns() {
    // Detect C2 communication patterns
}

void CorrelationEngine::findExfiltrationPatterns() {
    // Detect data exfiltration patterns
}

bool CorrelationEngine::containsKeyword(const String& text, const String& keyword) {
    String lower_text = text;
    String lower_keyword = keyword;
    lower_text.toLowerCase();
    lower_keyword.toLowerCase();
    return lower_text.indexOf(lower_keyword) >= 0;
}

std::vector<String> CorrelationEngine::extractIPsFromString(const String& text) {
    std::vector<String> ips;
    // Simplified - would use proper IP extraction
    return ips;
}

std::vector<String> CorrelationEngine::extractProcessNamesFromString(const String& text) {
    std::vector<String> processes;
    // Simplified - would use proper process name extraction
    return processes;
}

String CorrelationEngine::exportCorrelationsJSON() {
    String json = "{\"correlations\":[";

    for (size_t i = 0; i < correlations.size(); i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"type\":\"" + String(correlations[i].type) + "\",";
        json += "\"entity1\":\"" + correlations[i].entity1 + "\",";
        json += "\"entity2\":\"" + correlations[i].entity2 + "\",";
        json += "\"relationship\":\"" + correlations[i].relationship + "\",";
        json += "\"confidence\":" + String(correlations[i].confidence);
        json += "}";
    }

    json += "],\"count\":" + String(correlations.size()) + "}";
    return json;
}

String CorrelationEngine::exportPatternsJSON() {
    String json = "{\"patterns\":[";

    for (size_t i = 0; i < detected_patterns.size(); i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"pattern\":\"" + String(detected_patterns[i].pattern) + "\",";
        json += "\"description\":\"" + detected_patterns[i].description + "\",";
        json += "\"confidence\":" + String(detected_patterns[i].confidence) + ",";
        json += "\"indicators\":" + String(detected_patterns[i].indicators.size());
        json += "}";
    }

    json += "],\"count\":" + String(detected_patterns.size()) + "}";
    return json;
}

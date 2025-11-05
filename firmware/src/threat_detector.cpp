#include "threat_detector.h"

ThreatDetector::ThreatDetector()
    : storage(nullptr),
      ioc_extractor(nullptr),
      timeline_generator(nullptr),
      scan_count(0) {
}

ThreatDetector::~ThreatDetector() {
}

void ThreatDetector::begin(FRFDStorage* storage_ptr, IOCExtractor* ioc_ptr, TimelineGenerator* timeline_ptr) {
    storage = storage_ptr;
    ioc_extractor = ioc_ptr;
    timeline_generator = timeline_ptr;

    Serial.println("[ThreatDetector] Initialized");
    loadDefaultRules();
}

// ===========================
// Rule Management
// ===========================

bool ThreatDetector::loadRule(const ThreatRule& rule) {
    // Check if rule already exists
    for (size_t i = 0; i < rules.size(); i++) {
        if (rules[i].rule_id == rule.rule_id) {
            Serial.println("[ThreatDetector] Rule already exists: " + rule.rule_id);
            return false;
        }
    }

    rules.push_back(rule);
    Serial.println("[ThreatDetector] Loaded rule: " + rule.name);
    return true;
}

bool ThreatDetector::loadDefaultRules() {
    Serial.println("[ThreatDetector] Loading default threat detection rules...");

    initializeDefaultRules();

    Serial.println("[ThreatDetector] Loaded " + String(rules.size()) + " default rules");
    return true;
}

void ThreatDetector::clearRules() {
    rules.clear();
}

void ThreatDetector::enableRule(const String& rule_id) {
    for (size_t i = 0; i < rules.size(); i++) {
        if (rules[i].rule_id == rule_id) {
            rules[i].enabled = true;
            return;
        }
    }
}

void ThreatDetector::disableRule(const String& rule_id) {
    for (size_t i = 0; i < rules.size(); i++) {
        if (rules[i].rule_id == rule_id) {
            rules[i].enabled = false;
            return;
        }
    }
}

ThreatRule* ThreatDetector::getRule(const String& rule_id) {
    for (size_t i = 0; i < rules.size(); i++) {
        if (rules[i].rule_id == rule_id) {
            return &rules[i];
        }
    }
    return nullptr;
}

// ===========================
// Scanning Methods
// ===========================

bool ThreatDetector::scanAllArtifacts() {
    Serial.println("[ThreatDetector] Starting comprehensive scan of all artifacts...");
    scan_count++;

    uint32_t alerts_before = alerts.size();

    // Scan behavioral patterns using correlation data
    for (const auto& rule : rules) {
        if (!rule.enabled) continue;

        if (rule.type == RULE_BEHAVIORAL || rule.type == RULE_COMPOSITE) {
            if (matchBehavioralPattern(rule) || matchCompositeRule(rule)) {
                ThreatAlert alert = createAlert(rule, "behavioral_analysis",
                                               "Pattern matched across multiple artifacts", 80);
                addAlert(alert);
            }
        }
    }

    // Scan network IOCs
    if (ioc_extractor) {
        auto iocs = ioc_extractor->getIOCs();
        for (const auto& rule : rules) {
            if (!rule.enabled) continue;

            if (rule.type == RULE_NETWORK_PATTERN || rule.type == RULE_IOC_MATCH) {
                if (matchNetworkPattern(rule, iocs)) {
                    ThreatAlert alert = createAlert(rule, "ioc_analysis",
                                                   "Malicious network indicator detected", 85);
                    addAlert(alert);
                }
            }
        }
    }

    // Scan timeline events
    if (timeline_generator) {
        auto events = timeline_generator->getEvents();
        for (const auto& rule : rules) {
            if (!rule.enabled) continue;

            if (rule.type == RULE_PROCESS_PATTERN) {
                if (matchProcessPattern(rule, events)) {
                    ThreatAlert alert = createAlert(rule, "process_analysis",
                                                   "Suspicious process activity detected", 75);
                    addAlert(alert);
                }
            }
        }
    }

    uint32_t new_alerts = alerts.size() - alerts_before;
    Serial.println("[ThreatDetector] Scan complete. Generated " + String(new_alerts) + " new alerts");

    return true;
}

bool ThreatDetector::scanFile(const String& file_path) {
    if (!storage) return false;

    // Read file content
    File file = storage->openFile(file_path, FILE_READ);
    if (!file) return false;

    String content = "";
    while (file.available()) {
        content += (char)file.read();
        if (content.length() > 50000) break; // Limit content size
    }
    file.close();

    return scanContent(content, file_path);
}

bool ThreatDetector::scanContent(const String& content, const String& source) {
    uint32_t matches = 0;

    for (const auto& rule : rules) {
        if (!rule.enabled) continue;

        bool matched = false;
        uint8_t confidence = 50;
        String matched_content = "";

        switch (rule.type) {
            case RULE_FILE_PATTERN:
                matched = matchFilePattern(rule, source);
                confidence = 70;
                matched_content = source;
                break;

            case RULE_CONTENT_PATTERN:
                matched = matchContentPattern(rule, content);
                confidence = 80;
                matched_content = "Content signature match";
                break;

            case RULE_REGISTRY_PATTERN:
                if (source.indexOf("registry") >= 0 || source.indexOf("reg_") >= 0) {
                    matched = matchRegistryPattern(rule, content);
                    confidence = 75;
                    matched_content = "Registry pattern match";
                }
                break;

            default:
                continue;
        }

        if (matched) {
            ThreatAlert alert = createAlert(rule, source, matched_content, confidence);
            addAlert(alert);
            matches++;
        }
    }

    return matches > 0;
}

// ===========================
// Rule Matching Methods
// ===========================

bool ThreatDetector::matchFilePattern(const ThreatRule& rule, const String& file_path) {
    for (const auto& pattern : rule.patterns) {
        if (containsPattern(file_path.toLowerCase(), pattern.toLowerCase())) {
            return true;
        }
    }
    return false;
}

bool ThreatDetector::matchContentPattern(const ThreatRule& rule, const String& content) {
    String content_lower = content.toLowerCase();

    uint16_t pattern_matches = 0;
    for (const auto& pattern : rule.patterns) {
        if (containsPattern(content_lower, pattern.toLowerCase())) {
            pattern_matches++;
        }
    }

    // Require multiple pattern matches for content-based rules
    return pattern_matches >= (rule.patterns.size() > 2 ? 2 : 1);
}

bool ThreatDetector::matchNetworkPattern(const ThreatRule& rule, const std::vector<IOC>& iocs) {
    for (const auto& ioc : iocs) {
        if (ioc.type != IOC_IP_ADDRESS && ioc.type != IOC_DOMAIN && ioc.type != IOC_URL) {
            continue;
        }

        for (const auto& pattern : rule.patterns) {
            if (containsPattern(ioc.value.toLowerCase(), pattern.toLowerCase())) {
                return true;
            }
        }
    }
    return false;
}

bool ThreatDetector::matchProcessPattern(const ThreatRule& rule, const std::vector<TimelineEvent>& events) {
    for (const auto& event : events) {
        if (event.type != EVENT_PROCESS_STARTED && event.type != EVENT_PROCESS_TERMINATED) {
            continue;
        }

        String combined = event.target + " " + event.description + " " + event.details;
        combined.toLowerCase();

        for (const auto& pattern : rule.patterns) {
            if (containsPattern(combined, pattern.toLowerCase())) {
                return true;
            }
        }
    }
    return false;
}

bool ThreatDetector::matchRegistryPattern(const ThreatRule& rule, const String& content) {
    String content_lower = content.toLowerCase();

    for (const auto& pattern : rule.patterns) {
        if (containsPattern(content_lower, pattern.toLowerCase())) {
            return true;
        }
    }
    return false;
}

bool ThreatDetector::matchBehavioralPattern(const ThreatRule& rule) {
    // Behavioral patterns require timeline and IOC data
    if (!timeline_generator || !ioc_extractor) return false;

    uint16_t behavior_indicators = 0;

    // Check for suspicious temporal patterns
    auto events = timeline_generator->getEvents();
    for (const auto& pattern : rule.patterns) {
        for (const auto& event : events) {
            String event_str = event.description + " " + event.target;
            if (containsPattern(event_str.toLowerCase(), pattern.toLowerCase())) {
                behavior_indicators++;
                break;
            }
        }
    }

    // Require multiple behavioral indicators
    return behavior_indicators >= 2;
}

bool ThreatDetector::matchCompositeRule(const ThreatRule& rule) {
    // Composite rules combine multiple rule types
    uint16_t condition_matches = 0;

    for (const auto& condition : rule.conditions) {
        // Parse condition (simplified)
        if (condition.indexOf("process") >= 0) {
            if (timeline_generator) {
                auto events = timeline_generator->getEvents();
                if (matchProcessPattern(rule, events)) {
                    condition_matches++;
                }
            }
        }

        if (condition.indexOf("network") >= 0) {
            if (ioc_extractor) {
                auto iocs = ioc_extractor->getIOCs();
                if (matchNetworkPattern(rule, iocs)) {
                    condition_matches++;
                }
            }
        }
    }

    // Require at least 2 conditions to match for composite rules
    return condition_matches >= 2;
}

// ===========================
// Alert Management
// ===========================

void ThreatDetector::addAlert(const ThreatAlert& alert) {
    // Check for duplicates
    for (const auto& existing : alerts) {
        if (existing.rule_id == alert.rule_id &&
            existing.artifact_source == alert.artifact_source) {
            return; // Duplicate alert
        }
    }

    alerts.push_back(alert);

    Serial.println("[ThreatDetector] ALERT: " + alert.rule_name +
                   " (" + getThreatSeverityName(alert.severity) + ")");

    // Update rule match count
    for (size_t i = 0; i < rules.size(); i++) {
        if (rules[i].rule_id == alert.rule_id) {
            rules[i].match_count++;
            break;
        }
    }
}

void ThreatDetector::clearAlerts() {
    alerts.clear();
}

std::vector<ThreatAlert> ThreatDetector::getAlertsBySeverity(ThreatSeverity severity) const {
    std::vector<ThreatAlert> filtered;
    for (const auto& alert : alerts) {
        if (alert.severity == severity && !alert.false_positive) {
            filtered.push_back(alert);
        }
    }
    return filtered;
}

std::vector<ThreatAlert> ThreatDetector::getAlertsByTactic(MITRETactic tactic) const {
    std::vector<ThreatAlert> filtered;
    for (const auto& alert : alerts) {
        for (const auto& alert_tactic : alert.tactics) {
            if (alert_tactic == tactic && !alert.false_positive) {
                filtered.push_back(alert);
                break;
            }
        }
    }
    return filtered;
}

void ThreatDetector::markFalsePositive(const String& alert_id) {
    for (size_t i = 0; i < alerts.size(); i++) {
        if (alerts[i].alert_id == alert_id) {
            alerts[i].false_positive = true;
            Serial.println("[ThreatDetector] Marked as false positive: " + alert_id);
            return;
        }
    }
}

// ===========================
// Statistics
// ===========================

DetectionStatistics ThreatDetector::getStatistics() const {
    DetectionStatistics stats;
    stats.total_rules_loaded = rules.size();
    stats.rules_enabled = 0;
    stats.total_scans_performed = scan_count;
    stats.total_alerts_generated = alerts.size();
    stats.critical_alerts = 0;
    stats.high_alerts = 0;
    stats.medium_alerts = 0;
    stats.low_alerts = 0;
    stats.false_positives = 0;

    for (const auto& rule : rules) {
        if (rule.enabled) stats.rules_enabled++;
    }

    for (const auto& alert : alerts) {
        if (alert.false_positive) {
            stats.false_positives++;
            continue;
        }

        switch (alert.severity) {
            case THREAT_CRITICAL: stats.critical_alerts++; break;
            case THREAT_HIGH: stats.high_alerts++; break;
            case THREAT_MEDIUM: stats.medium_alerts++; break;
            case THREAT_LOW: stats.low_alerts++; break;
            default: break;
        }

        // Count by rule
        stats.alerts_by_rule[alert.rule_id]++;

        // Count by tactic
        for (const auto& tactic : alert.tactics) {
            stats.alerts_by_tactic[tactic]++;
        }
    }

    return stats;
}

uint32_t ThreatDetector::getCriticalAlertCount() const {
    uint32_t count = 0;
    for (const auto& alert : alerts) {
        if (alert.severity == THREAT_CRITICAL && !alert.false_positive) {
            count++;
        }
    }
    return count;
}

// ===========================
// Export Methods
// ===========================

String ThreatDetector::exportAlertsToJSON() {
    String json = "{\n  \"alerts\": [\n";

    for (size_t i = 0; i < alerts.size(); i++) {
        const auto& alert = alerts[i];

        json += "    {\n";
        json += "      \"alert_id\": \"" + alert.alert_id + "\",\n";
        json += "      \"rule_id\": \"" + alert.rule_id + "\",\n";
        json += "      \"rule_name\": \"" + alert.rule_name + "\",\n";
        json += "      \"severity\": \"" + getThreatSeverityName(alert.severity) + "\",\n";
        json += "      \"description\": \"" + alert.description + "\",\n";
        json += "      \"artifact_source\": \"" + alert.artifact_source + "\",\n";
        json += "      \"confidence\": " + String(alert.confidence) + ",\n";
        json += "      \"false_positive\": " + String(alert.false_positive ? "true" : "false") + ",\n";
        json += "      \"timestamp\": " + String(alert.timestamp) + "\n";
        json += "    }";

        if (i < alerts.size() - 1) json += ",";
        json += "\n";
    }

    json += "  ],\n";
    json += "  \"total_alerts\": " + String(alerts.size()) + "\n";
    json += "}\n";

    return json;
}

String ThreatDetector::exportAlertsToCSV() {
    String csv = "alert_id,rule_id,rule_name,severity,description,artifact_source,confidence,false_positive,timestamp\n";

    for (const auto& alert : alerts) {
        csv += alert.alert_id + ",";
        csv += alert.rule_id + ",";
        csv += alert.rule_name + ",";
        csv += getThreatSeverityName(alert.severity) + ",";
        csv += "\"" + alert.description + "\",";
        csv += "\"" + alert.artifact_source + "\",";
        csv += String(alert.confidence) + ",";
        csv += String(alert.false_positive ? "true" : "false") + ",";
        csv += String(alert.timestamp) + "\n";
    }

    return csv;
}

// ===========================
// Default Threat Rules
// ===========================

void ThreatDetector::initializeDefaultRules() {
    loadRule(createMalwareExecutionRule());
    loadRule(createLateralMovementRule());
    loadRule(createPersistenceRule());
    loadRule(createCredentialDumpingRule());
    loadRule(createRansomwareRule());
    loadRule(createC2CommunicationRule());
    loadRule(createPrivilegeEscalationRule());
    loadRule(createDataExfiltrationRule());
    loadRule(createWebshellRule());
    loadRule(createPowerShellAbusRule());
    loadRule(createMimikatzRule());
    loadRule(createSuspiciousRegistryRule());
    loadRule(createSuspiciousScheduledTaskRule());
    loadRule(createSuspiciousNetworkConnectionRule());
}

ThreatRule ThreatDetector::createMalwareExecutionRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_001";
    rule.name = "Potential Malware Execution";
    rule.description = "Detects execution of known malware families or suspicious executables";
    rule.type = RULE_PROCESS_PATTERN;
    rule.severity = THREAT_CRITICAL;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back("mimikatz");
    rule.patterns.push_back("pwdump");
    rule.patterns.push_back("gsecdump");
    rule.patterns.push_back("wce.exe");
    rule.patterns.push_back("procdump");
    rule.patterns.push_back("psexec");

    rule.tactics.push_back(TACTIC_EXECUTION);
    rule.techniques.push_back("T1059"); // Command and Scripting Interpreter

    return rule;
}

ThreatRule ThreatDetector::createLateralMovementRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_002";
    rule.name = "Lateral Movement Activity";
    rule.description = "Detects lateral movement tools and techniques";
    rule.type = RULE_COMPOSITE;
    rule.severity = THREAT_HIGH;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back("psexec");
    rule.patterns.push_back("wmi");
    rule.patterns.push_back("schtasks");
    rule.patterns.push_back("net use");
    rule.patterns.push_back("at.exe");

    rule.conditions.push_back("process AND network");

    rule.tactics.push_back(TACTIC_LATERAL_MOVEMENT);
    rule.techniques.push_back("T1021"); // Remote Services

    return rule;
}

ThreatRule ThreatDetector::createPersistenceRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_003";
    rule.name = "Persistence Mechanism";
    rule.description = "Detects persistence mechanisms in registry, startup folders, or scheduled tasks";
    rule.type = RULE_REGISTRY_PATTERN;
    rule.severity = THREAT_HIGH;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back("\\software\\microsoft\\windows\\currentversion\\run");
    rule.patterns.push_back("\\software\\microsoft\\windows\\currentversion\\runonce");
    rule.patterns.push_back("\\currentversion\\windows\\load");
    rule.patterns.push_back("\\winlogon\\userinit");
    rule.patterns.push_back("\\winlogon\\shell");

    rule.tactics.push_back(TACTIC_PERSISTENCE);
    rule.techniques.push_back("T1547"); // Boot or Logon Autostart Execution

    return rule;
}

ThreatRule ThreatDetector::createCredentialDumpingRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_004";
    rule.name = "Credential Dumping";
    rule.description = "Detects credential dumping tools and techniques";
    rule.type = RULE_PROCESS_PATTERN;
    rule.severity = THREAT_CRITICAL;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back("lsass");
    rule.patterns.push_back("mimikatz");
    rule.patterns.push_back("procdump");
    rule.patterns.push_back("pwdump");
    rule.patterns.push_back("sekurlsa");

    rule.tactics.push_back(TACTIC_CREDENTIAL_ACCESS);
    rule.techniques.push_back("T1003"); // OS Credential Dumping

    return rule;
}

ThreatRule ThreatDetector::createRansomwareRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_005";
    rule.name = "Ransomware Indicators";
    rule.description = "Detects ransomware-related file extensions and behaviors";
    rule.type = RULE_FILE_PATTERN;
    rule.severity = THREAT_CRITICAL;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back(".encrypted");
    rule.patterns.push_back(".locked");
    rule.patterns.push_back(".crypto");
    rule.patterns.push_back("readme.txt");
    rule.patterns.push_back("decrypt_instructions");

    rule.tactics.push_back(TACTIC_IMPACT);
    rule.techniques.push_back("T1486"); // Data Encrypted for Impact

    return rule;
}

ThreatRule ThreatDetector::createC2CommunicationRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_006";
    rule.name = "Command & Control Communication";
    rule.description = "Detects potential C2 communication patterns";
    rule.type = RULE_NETWORK_PATTERN;
    rule.severity = THREAT_HIGH;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back("pastebin");
    rule.patterns.push_back("discord.com");
    rule.patterns.push_back("telegram");
    rule.patterns.push_back(".tk");
    rule.patterns.push_back(".onion");

    rule.tactics.push_back(TACTIC_COMMAND_AND_CONTROL);
    rule.techniques.push_back("T1071"); // Application Layer Protocol

    return rule;
}

ThreatRule ThreatDetector::createPrivilegeEscalationRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_007";
    rule.name = "Privilege Escalation";
    rule.description = "Detects privilege escalation attempts";
    rule.type = RULE_PROCESS_PATTERN;
    rule.severity = THREAT_HIGH;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back("runas");
    rule.patterns.push_back("elevate");
    rule.patterns.push_back("bypassuac");
    rule.patterns.push_back("fodhelper");

    rule.tactics.push_back(TACTIC_PRIVILEGE_ESCALATION);
    rule.techniques.push_back("T1548"); // Abuse Elevation Control Mechanism

    return rule;
}

ThreatRule ThreatDetector::createDataExfiltrationRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_008";
    rule.name = "Data Exfiltration";
    rule.description = "Detects potential data exfiltration activity";
    rule.type = RULE_BEHAVIORAL;
    rule.severity = THREAT_HIGH;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back("compress");
    rule.patterns.push_back("archive");
    rule.patterns.push_back("upload");
    rule.patterns.push_back("ftp");
    rule.patterns.push_back("sftp");

    rule.tactics.push_back(TACTIC_EXFILTRATION);
    rule.techniques.push_back("T1048"); // Exfiltration Over Alternative Protocol

    return rule;
}

ThreatRule ThreatDetector::createWebshellRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_009";
    rule.name = "Web Shell Detection";
    rule.description = "Detects web shells and malicious web scripts";
    rule.type = RULE_CONTENT_PATTERN;
    rule.severity = THREAT_CRITICAL;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back("eval(");
    rule.patterns.push_back("base64_decode");
    rule.patterns.push_back("system(");
    rule.patterns.push_back("exec(");
    rule.patterns.push_back("passthru");

    rule.tactics.push_back(TACTIC_PERSISTENCE);
    rule.tactics.push_back(TACTIC_COMMAND_AND_CONTROL);
    rule.techniques.push_back("T1505.003"); // Server Software Component: Web Shell

    return rule;
}

ThreatRule ThreatDetector::createPowerShellAbusRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_010";
    rule.name = "PowerShell Abuse";
    rule.description = "Detects malicious PowerShell usage";
    rule.type = RULE_PROCESS_PATTERN;
    rule.severity = THREAT_MEDIUM;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back("powershell -enc");
    rule.patterns.push_back("powershell -e ");
    rule.patterns.push_back("invoke-expression");
    rule.patterns.push_back("downloadstring");
    rule.patterns.push_back("bypass");

    rule.tactics.push_back(TACTIC_EXECUTION);
    rule.techniques.push_back("T1059.001"); // PowerShell

    return rule;
}

ThreatRule ThreatDetector::createMimikatzRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_011";
    rule.name = "Mimikatz Detection";
    rule.description = "Detects Mimikatz credential dumping tool";
    rule.type = RULE_CONTENT_PATTERN;
    rule.severity = THREAT_CRITICAL;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back("mimikatz");
    rule.patterns.push_back("sekurlsa");
    rule.patterns.push_back("logonpasswords");
    rule.patterns.push_back("lsadump");

    rule.tactics.push_back(TACTIC_CREDENTIAL_ACCESS);
    rule.techniques.push_back("T1003.001"); // LSASS Memory

    return rule;
}

ThreatRule ThreatDetector::createSuspiciousRegistryRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_012";
    rule.name = "Suspicious Registry Modification";
    rule.description = "Detects suspicious registry key modifications";
    rule.type = RULE_REGISTRY_PATTERN;
    rule.severity = THREAT_MEDIUM;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back("disabletaskmgr");
    rule.patterns.push_back("disableregistrytools");
    rule.patterns.push_back("hidedosapps");
    rule.patterns.push_back("restrictrun");

    rule.tactics.push_back(TACTIC_DEFENSE_EVASION);
    rule.techniques.push_back("T1112"); // Modify Registry

    return rule;
}

ThreatRule ThreatDetector::createSuspiciousScheduledTaskRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_013";
    rule.name = "Suspicious Scheduled Task";
    rule.description = "Detects suspicious scheduled task creation";
    rule.type = RULE_PROCESS_PATTERN;
    rule.severity = THREAT_MEDIUM;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back("schtasks /create");
    rule.patterns.push_back("at.exe");
    rule.patterns.push_back("/sc minute");
    rule.patterns.push_back("/ru system");

    rule.tactics.push_back(TACTIC_PERSISTENCE);
    rule.tactics.push_back(TACTIC_EXECUTION);
    rule.techniques.push_back("T1053.005"); // Scheduled Task

    return rule;
}

ThreatRule ThreatDetector::createSuspiciousNetworkConnectionRule() {
    ThreatRule rule;
    rule.rule_id = "THREAT_014";
    rule.name = "Suspicious Network Connection";
    rule.description = "Detects connections to suspicious ports or protocols";
    rule.type = RULE_NETWORK_PATTERN;
    rule.severity = THREAT_MEDIUM;
    rule.enabled = true;
    rule.match_count = 0;

    rule.patterns.push_back(":4444");
    rule.patterns.push_back(":1337");
    rule.patterns.push_back(":31337");
    rule.patterns.push_back(":6667");
    rule.patterns.push_back(":6666");

    rule.tactics.push_back(TACTIC_COMMAND_AND_CONTROL);
    rule.techniques.push_back("T1571"); // Non-Standard Port

    return rule;
}

// ===========================
// Helper Methods
// ===========================

bool ThreatDetector::containsPattern(const String& content, const String& pattern) {
    return content.indexOf(pattern) >= 0;
}

ThreatAlert ThreatDetector::createAlert(const ThreatRule& rule, const String& source,
                                        const String& matched_content, uint8_t confidence) {
    ThreatAlert alert;
    alert.alert_id = generateAlertId();
    alert.rule_id = rule.rule_id;
    alert.rule_name = rule.name;
    alert.severity = rule.severity;
    alert.description = rule.description;
    alert.artifact_source = source;
    alert.matched_content = matched_content;
    alert.tactics = rule.tactics;
    alert.techniques = rule.techniques;
    alert.timestamp = millis();
    alert.confidence = confidence;
    alert.false_positive = false;

    return alert;
}

String ThreatDetector::generateAlertId() {
    static uint32_t alert_counter = 0;
    return "ALERT_" + String(millis()) + "_" + String(alert_counter++);
}

String ThreatDetector::getThreatSeverityName(ThreatSeverity severity) const {
    switch (severity) {
        case THREAT_CRITICAL: return "CRITICAL";
        case THREAT_HIGH: return "HIGH";
        case THREAT_MEDIUM: return "MEDIUM";
        case THREAT_LOW: return "LOW";
        case THREAT_INFO: return "INFO";
        default: return "UNKNOWN";
    }
}

String ThreatDetector::getMITRETacticName(MITRETactic tactic) const {
    switch (tactic) {
        case TACTIC_INITIAL_ACCESS: return "Initial Access";
        case TACTIC_EXECUTION: return "Execution";
        case TACTIC_PERSISTENCE: return "Persistence";
        case TACTIC_PRIVILEGE_ESCALATION: return "Privilege Escalation";
        case TACTIC_DEFENSE_EVASION: return "Defense Evasion";
        case TACTIC_CREDENTIAL_ACCESS: return "Credential Access";
        case TACTIC_DISCOVERY: return "Discovery";
        case TACTIC_LATERAL_MOVEMENT: return "Lateral Movement";
        case TACTIC_COLLECTION: return "Collection";
        case TACTIC_COMMAND_AND_CONTROL: return "Command and Control";
        case TACTIC_EXFILTRATION: return "Exfiltration";
        case TACTIC_IMPACT: return "Impact";
        default: return "Unknown";
    }
}

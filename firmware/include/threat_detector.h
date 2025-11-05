#ifndef THREAT_DETECTOR_H
#define THREAT_DETECTOR_H

#include <Arduino.h>
#include <vector>
#include <map>
#include "storage.h"
#include "ioc_extractor.h"
#include "timeline_generator.h"

/**
 * @brief Threat Rule Types
 */
enum ThreatRuleType {
    RULE_FILE_PATTERN,       // File name/path pattern matching
    RULE_CONTENT_PATTERN,    // Content signature matching
    RULE_NETWORK_PATTERN,    // Network IOC matching
    RULE_PROCESS_PATTERN,    // Process name/behavior matching
    RULE_REGISTRY_PATTERN,   // Registry key/value matching
    RULE_BEHAVIORAL,         // Behavioral analysis
    RULE_COMPOSITE,          // Multiple conditions
    RULE_IOC_MATCH          // IOC database matching
};

/**
 * @brief Threat Severity Levels
 */
enum ThreatSeverity {
    THREAT_CRITICAL = 5,     // Confirmed active threat
    THREAT_HIGH = 4,         // High probability threat
    THREAT_MEDIUM = 3,       // Medium probability threat
    THREAT_LOW = 2,          // Low probability threat
    THREAT_INFO = 1          // Informational only
};

/**
 * @brief MITRE ATT&CK Tactics
 */
enum MITRETactic {
    TACTIC_INITIAL_ACCESS,
    TACTIC_EXECUTION,
    TACTIC_PERSISTENCE,
    TACTIC_PRIVILEGE_ESCALATION,
    TACTIC_DEFENSE_EVASION,
    TACTIC_CREDENTIAL_ACCESS,
    TACTIC_DISCOVERY,
    TACTIC_LATERAL_MOVEMENT,
    TACTIC_COLLECTION,
    TACTIC_COMMAND_AND_CONTROL,
    TACTIC_EXFILTRATION,
    TACTIC_IMPACT,
    TACTIC_UNKNOWN
};

/**
 * @brief Threat Detection Rule
 */
struct ThreatRule {
    String rule_id;                      // Unique rule identifier
    String name;                         // Rule name
    String description;                  // What this rule detects
    ThreatRuleType type;                 // Rule type
    ThreatSeverity severity;             // Threat severity
    std::vector<String> patterns;        // Patterns to match
    std::vector<String> conditions;      // Logical conditions
    std::vector<MITRETactic> tactics;    // MITRE ATT&CK tactics
    std::vector<String> techniques;      // MITRE ATT&CK technique IDs
    std::map<String, String> metadata;   // Additional metadata
    bool enabled;                        // Is rule enabled?
    uint16_t match_count;                // Number of matches found
};

/**
 * @brief Threat Detection Alert
 */
struct ThreatAlert {
    String alert_id;                     // Unique alert ID
    String rule_id;                      // Rule that triggered
    String rule_name;                    // Rule name
    ThreatSeverity severity;             // Alert severity
    String description;                  // Alert description
    String artifact_source;              // Source artifact
    String matched_content;              // What was matched
    std::vector<String> evidence;        // Evidence list
    std::vector<MITRETactic> tactics;    // MITRE tactics
    std::vector<String> techniques;      // MITRE techniques
    unsigned long timestamp;             // Detection timestamp
    uint8_t confidence;                  // Detection confidence (0-100)
    bool false_positive;                 // Marked as false positive?
};

/**
 * @brief Detection Statistics
 */
struct DetectionStatistics {
    uint32_t total_rules_loaded;
    uint32_t rules_enabled;
    uint32_t total_scans_performed;
    uint32_t total_alerts_generated;
    uint32_t critical_alerts;
    uint32_t high_alerts;
    uint32_t medium_alerts;
    uint32_t low_alerts;
    uint32_t false_positives;
    std::map<String, uint16_t> alerts_by_rule;
    std::map<MITRETactic, uint16_t> alerts_by_tactic;
};

/**
 * @brief Automated Threat Detection Engine
 *
 * YARA-like rule engine for automated threat detection
 * Scans forensic artifacts against threat intelligence rules
 */
class ThreatDetector {
public:
    ThreatDetector();
    ~ThreatDetector();

    // Initialization
    void begin(FRFDStorage* storage_ptr, IOCExtractor* ioc_ptr, TimelineGenerator* timeline_ptr);

    // Rule Management
    bool loadRule(const ThreatRule& rule);
    bool loadRulesFromFile(const String& filename);
    bool loadDefaultRules();
    void clearRules();
    void enableRule(const String& rule_id);
    void disableRule(const String& rule_id);
    std::vector<ThreatRule> getRules() const { return rules; }
    ThreatRule* getRule(const String& rule_id);

    // Scanning Methods
    bool scanAllArtifacts();
    bool scanFile(const String& file_path);
    bool scanDirectory(const String& dir_path);
    bool scanContent(const String& content, const String& source);

    // Rule Matching
    bool matchFilePattern(const ThreatRule& rule, const String& file_path);
    bool matchContentPattern(const ThreatRule& rule, const String& content);
    bool matchNetworkPattern(const ThreatRule& rule, const std::vector<IOC>& iocs);
    bool matchProcessPattern(const ThreatRule& rule, const std::vector<TimelineEvent>& events);
    bool matchRegistryPattern(const ThreatRule& rule, const String& content);
    bool matchBehavioralPattern(const ThreatRule& rule);
    bool matchCompositeRule(const ThreatRule& rule);

    // Alert Management
    void addAlert(const ThreatAlert& alert);
    void clearAlerts();
    std::vector<ThreatAlert> getAlerts() const { return alerts; }
    std::vector<ThreatAlert> getAlertsBySeverity(ThreatSeverity severity) const;
    std::vector<ThreatAlert> getAlertsByTactic(MITRETactic tactic) const;
    void markFalsePositive(const String& alert_id);

    // Statistics
    DetectionStatistics getStatistics() const;
    uint32_t getAlertCount() const { return alerts.size(); }
    uint32_t getCriticalAlertCount() const;

    // Export Methods
    String exportAlertsToJSON();
    String exportAlertsToCSV();
    String exportAlertsToSTIX();
    bool saveAlertsToFile(const String& filename, const String& format);

    // Utility
    String getThreatSeverityName(ThreatSeverity severity) const;
    String getMITRETacticName(MITRETactic tactic) const;
    uint8_t calculateConfidence(const ThreatRule& rule, const String& match);

private:
    FRFDStorage* storage;
    IOCExtractor* ioc_extractor;
    TimelineGenerator* timeline_generator;

    std::vector<ThreatRule> rules;
    std::vector<ThreatAlert> alerts;
    uint32_t scan_count;

    // Built-in threat rules
    void initializeDefaultRules();
    ThreatRule createMalwareExecutionRule();
    ThreatRule createLateralMovementRule();
    ThreatRule createPersistenceRule();
    ThreatRule createCredentialDumpingRule();
    ThreatRule createRansomwareRule();
    ThreatRule createC2CommunicationRule();
    ThreatRule createPrivilegeEscalationRule();
    ThreatRule createDataExfiltrationRule();
    ThreatRule createWebshellRule();
    ThreatRule createPowerShellAbusRule();
    ThreatRule createMimikatzRule();
    ThreatRule createSuspiciousRegistryRule();
    ThreatRule createSuspiciousScheduledTaskRule();
    ThreatRule createSuspiciousNetworkConnectionRule();

    // Pattern matching helpers
    bool containsPattern(const String& content, const String& pattern);
    bool matchesRegex(const String& content, const String& regex);
    std::vector<String> extractMatches(const String& content, const String& pattern);

    // Alert generation
    ThreatAlert createAlert(const ThreatRule& rule, const String& source,
                           const String& matched_content, uint8_t confidence);
    String generateAlertId();

    // False positive detection
    bool isLikelyFalsePositive(const ThreatAlert& alert);
};

#endif // THREAT_DETECTOR_H

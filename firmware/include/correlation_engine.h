#ifndef CORRELATION_ENGINE_H
#define CORRELATION_ENGINE_H

#include <Arduino.h>
#include <vector>
#include <map>
#include "ioc_extractor.h"
#include "timeline_generator.h"

/**
 * @brief Correlation Type
 */
enum CorrelationType {
    CORR_TEMPORAL,          // Time-based correlation
    CORR_NETWORK,           // Network connection correlation
    CORR_PROCESS,           // Process execution correlation
    CORR_FILE,              // File access correlation
    CORR_USER,              // User activity correlation
    CORR_IOC                // IOC-based correlation
};

/**
 * @brief Attack Pattern
 */
enum AttackPattern {
    PATTERN_LATERAL_MOVEMENT,
    PATTERN_DATA_EXFILTRATION,
    PATTERN_PRIVILEGE_ESCALATION,
    PATTERN_PERSISTENCE,
    PATTERN_RECONNAISSANCE,
    PATTERN_COMMAND_CONTROL,
    PATTERN_CREDENTIAL_THEFT,
    PATTERN_MALWARE_EXECUTION
};

/**
 * @brief Correlation Result
 */
struct Correlation {
    CorrelationType type;
    String entity1;             // First entity (IP, process, file, etc.)
    String entity2;             // Second entity
    String relationship;        // Description of relationship
    uint8_t confidence;         // Confidence score 0-100
    unsigned long timestamp;    // When correlation was found
    std::vector<String> evidence; // Supporting evidence
};

/**
 * @brief Attack Pattern Detection
 */
struct PatternDetection {
    AttackPattern pattern;
    String description;
    uint8_t confidence;
    std::vector<Correlation> correlations;
    std::vector<String> indicators;
    String recommendation;
};

/**
 * @brief Correlation Engine
 *
 * Analyzes relationships between artifacts to detect:
 * - Attack patterns
 * - Lateral movement
 * - Data exfiltration
 * - Privilege escalation
 * - Persistent threats
 */
class CorrelationEngine {
public:
    CorrelationEngine();
    ~CorrelationEngine();

    // Initialization
    void begin();
    void setIOCExtractor(IOCExtractor* ioc_ptr);
    void setTimelineGenerator(TimelineGenerator* timeline_ptr);

    // Correlation Analysis
    bool analyzeAll();
    bool analyzeTemporalCorrelations();
    bool analyzeNetworkCorrelations();
    bool analyzeProcessCorrelations();
    bool analyzeFileCorrelations();
    bool analyzeUserCorrelations();
    bool analyzeIOCCorrelations();

    // Pattern Detection
    bool detectAttackPatterns();
    bool detectLateralMovement();
    bool detectDataExfiltration();
    bool detectPrivilegeEscalation();
    bool detectPersistence();
    bool detectReconnaissance();
    bool detectC2Activity();
    bool detectCredentialTheft();
    bool detectMalwareExecution();

    // Correlation Management
    void addCorrelation(const Correlation& corr);
    void clearCorrelations();
    std::vector<Correlation> getCorrelations() const { return correlations; }
    std::vector<Correlation> getCorrelationsByType(CorrelationType type) const;

    // Pattern Management
    std::vector<PatternDetection> getDetectedPatterns() const { return detected_patterns; }
    std::vector<PatternDetection> getPatternsByType(AttackPattern pattern) const;

    // Statistics
    uint16_t getCorrelationCount() const { return correlations.size(); }
    uint16_t getPatternCount() const { return detected_patterns.size(); }

    // Export
    String exportCorrelationsJSON();
    String exportPatternsJSON();

private:
    IOCExtractor* ioc_extractor;
    TimelineGenerator* timeline_generator;

    std::vector<Correlation> correlations;
    std::vector<PatternDetection> detected_patterns;

    // Analysis helpers
    bool isTimeProximate(unsigned long time1, unsigned long time2, unsigned long threshold_ms);
    bool isNetworkRelated(const String& entity1, const String& entity2);
    bool isProcessRelated(const String& proc1, const String& proc2);

    // Pattern detection helpers
    PatternDetection createPattern(AttackPattern type, uint8_t confidence, const String& desc);
    void addPatternIndicator(PatternDetection& pattern, const String& indicator);
    void addPatternCorrelation(PatternDetection& pattern, const Correlation& corr);

    // Confidence scoring
    uint8_t calculateCorrelationConfidence(CorrelationType type, const String& entity1, const String& entity2);
    uint8_t calculatePatternConfidence(const PatternDetection& pattern);

    // IOC correlation
    void correlateIPsWithProcesses();
    void correlateHashesWithExecutions();
    void correlateDomainWithNetwork();

    // Timeline correlation
    void correlateTemporalEvents(unsigned long window_ms);
    void findSequentialEvents();
    void findConcurrentEvents();

    // Network correlation
    void correlateNetworkConnections();
    void findC2Patterns();
    void findExfiltrationPatterns();

    // String helpers
    bool containsKeyword(const String& text, const String& keyword);
    std::vector<String> extractIPsFromString(const String& text);
    std::vector<String> extractProcessNamesFromString(const String& text);
};

#endif // CORRELATION_ENGINE_H

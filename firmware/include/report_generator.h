#ifndef REPORT_GENERATOR_H
#define REPORT_GENERATOR_H

#include <Arduino.h>
#include <vector>
#include "storage.h"
#include "ioc_extractor.h"
#include "timeline_generator.h"

/**
 * @brief Report Type
 */
enum ReportType {
    REPORT_EXECUTIVE,    // High-level executive summary
    REPORT_TECHNICAL,    // Detailed technical report
    REPORT_INCIDENT,     // Incident response report
    REPORT_COMPLIANCE    // Compliance/audit report
};

/**
 * @brief Report Statistics
 */
struct ReportStatistics {
    // Collection stats
    uint32_t total_artifacts_collected;
    uint32_t total_files_collected;
    uint64_t total_bytes_collected;
    unsigned long collection_duration_ms;

    // Module stats
    uint16_t modules_executed;
    uint16_t modules_succeeded;
    uint16_t modules_failed;
    float success_rate;

    // IOC stats
    uint16_t total_iocs;
    uint16_t critical_iocs;
    uint16_t high_iocs;
    uint16_t medium_iocs;

    // Timeline stats
    uint32_t timeline_events;
    unsigned long earliest_event;
    unsigned long latest_event;

    // System info
    String target_os;
    String target_hostname;
    String collection_date;
};

/**
 * @brief Finding Severity
 */
enum FindingSeverity {
    SEVERITY_CRITICAL,
    SEVERITY_HIGH,
    SEVERITY_MEDIUM,
    SEVERITY_LOW,
    SEVERITY_INFO
};

/**
 * @brief Security Finding
 */
struct SecurityFinding {
    FindingSeverity severity;
    String title;
    String description;
    String artifact_source;
    String recommendation;
    std::vector<String> evidence;
};

/**
 * @brief HTML Report Generator
 *
 * Creates comprehensive HTML reports with:
 * - Executive summary
 * - Statistics and charts
 * - Security findings
 * - IOC analysis
 * - Timeline visualization
 * - Recommendations
 */
class ReportGenerator {
public:
    ReportGenerator();
    ~ReportGenerator();

    // Initialization
    void begin(FRFDStorage* storage_ptr);
    void setIOCExtractor(IOCExtractor* ioc_ptr);
    void setTimelineGenerator(TimelineGenerator* timeline_ptr);

    // Report Building
    bool generateReport(ReportType type, const String& output_path);
    bool generateExecutiveReport(const String& output_path);
    bool generateTechnicalReport(const String& output_path);
    bool generateIncidentReport(const String& output_path);
    bool generateComplianceReport(const String& output_path);

    // Statistics
    void setStatistics(const ReportStatistics& stats);
    ReportStatistics getStatistics() const { return statistics; }

    // Findings
    void addFinding(const SecurityFinding& finding);
    void clearFindings();
    std::vector<SecurityFinding> getFindings() const { return findings; }
    std::vector<SecurityFinding> getFindingsBySeverity(FindingSeverity severity) const;

    // Metadata
    void setInvestigatorName(const String& name) { investigator_name = name; }
    void setCaseNumber(const String& number) { case_number = number; }
    void setOrganization(const String& org) { organization = org; }
    void setTargetSystem(const String& system) { target_system = system; }

private:
    FRFDStorage* storage;
    IOCExtractor* ioc_extractor;
    TimelineGenerator* timeline_generator;

    ReportStatistics statistics;
    std::vector<SecurityFinding> findings;

    // Metadata
    String investigator_name;
    String case_number;
    String organization;
    String target_system;

    // HTML Generation Methods
    String generateHTML();
    String generateHTMLHeader();
    String generateHTMLStyles();
    String generateExecutiveSummary();
    String generateStatisticsSection();
    String generateFindingsSection();
    String generateIOCSection();
    String generateTimelineSection();
    String generateRecommendationsSection();
    String generateFooter();

    // Chart Generation
    String generateModuleSuccessChart();
    String generateIOCDistributionChart();
    String generateTimelineChart();
    String generateSeverityChart();

    // Helper Methods
    String getSeverityColor(FindingSeverity severity);
    String getSeverityName(FindingSeverity severity);
    String formatBytes(uint64_t bytes);
    String formatDuration(unsigned long ms);
    String formatTimestamp(unsigned long timestamp);

    // Auto-analysis
    void analyzeForFindings();
    void detectSuspiciousIOCs();
    void detectPersistence();
    void detectLateralMovement();
    void detectDataExfiltration();
};

#endif // REPORT_GENERATOR_H

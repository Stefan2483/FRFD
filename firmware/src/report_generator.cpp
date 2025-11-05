#include "report_generator.h"

ReportGenerator::ReportGenerator() {
    storage = nullptr;
    ioc_extractor = nullptr;
    timeline_generator = nullptr;

    investigator_name = "FRFD Forensic System";
    case_number = "";
    organization = "";
    target_system = "";

    memset(&statistics, 0, sizeof(statistics));
}

ReportGenerator::~ReportGenerator() {
}

void ReportGenerator::begin(FRFDStorage* storage_ptr) {
    storage = storage_ptr;
}

void ReportGenerator::setIOCExtractor(IOCExtractor* ioc_ptr) {
    ioc_extractor = ioc_ptr;
}

void ReportGenerator::setTimelineGenerator(TimelineGenerator* timeline_ptr) {
    timeline_generator = timeline_ptr;
}

void ReportGenerator::setStatistics(const ReportStatistics& stats) {
    statistics = stats;
}

void ReportGenerator::addFinding(const SecurityFinding& finding) {
    findings.push_back(finding);
}

void ReportGenerator::clearFindings() {
    findings.clear();
}

std::vector<SecurityFinding> ReportGenerator::getFindingsBySeverity(FindingSeverity severity) const {
    std::vector<SecurityFinding> filtered;
    for (const auto& finding : findings) {
        if (finding.severity == severity) {
            filtered.push_back(finding);
        }
    }
    return filtered;
}

bool ReportGenerator::generateReport(ReportType type, const String& output_path) {
    switch (type) {
        case REPORT_EXECUTIVE:
            return generateExecutiveReport(output_path);
        case REPORT_TECHNICAL:
            return generateTechnicalReport(output_path);
        case REPORT_INCIDENT:
            return generateIncidentReport(output_path);
        case REPORT_COMPLIANCE:
            return generateComplianceReport(output_path);
        default:
            return false;
    }
}

bool ReportGenerator::generateExecutiveReport(const String& output_path) {
    String html = generateHTML();

    if (storage) {
        return storage->writeFile(output_path, html);
    }

    return false;
}

bool ReportGenerator::generateTechnicalReport(const String& output_path) {
    // Same as executive for now, can be enhanced later
    return generateExecutiveReport(output_path);
}

bool ReportGenerator::generateIncidentReport(const String& output_path) {
    return generateExecutiveReport(output_path);
}

bool ReportGenerator::generateComplianceReport(const String& output_path) {
    return generateExecutiveReport(output_path);
}

String ReportGenerator::generateHTML() {
    String html = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    html += generateHTMLHeader();
    html += generateHTMLStyles();
    html += "</head>\n<body>\n";
    html += "<div class=\"container\">\n";
    html += generateExecutiveSummary();
    html += generateStatisticsSection();
    html += generateFindingsSection();
    html += generateIOCSection();
    html += generateTimelineSection();
    html += generateRecommendationsSection();
    html += generateFooter();
    html += "</div>\n</body>\n</html>";

    return html;
}

String ReportGenerator::generateHTMLHeader() {
    String header = "<meta charset=\"UTF-8\">\n";
    header += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    header += "<title>FRFD Forensic Report";
    if (!case_number.isEmpty()) {
        header += " - Case " + case_number;
    }
    header += "</title>\n";

    return header;
}

String ReportGenerator::generateHTMLStyles() {
    String styles = "<style>\n";
    styles += "* { margin: 0; padding: 0; box-sizing: border-box; }\n";
    styles += "body { font-family: 'Segoe UI', Arial, sans-serif; background: #f5f5f5; color: #333; line-height: 1.6; }\n";
    styles += ".container { max-width: 1200px; margin: 0 auto; background: white; padding: 40px; box-shadow: 0 0 20px rgba(0,0,0,0.1); }\n";
    styles += "h1 { color: #2c3e50; font-size: 2.5em; margin-bottom: 10px; border-bottom: 4px solid #3498db; padding-bottom: 10px; }\n";
    styles += "h2 { color: #34495e; font-size: 1.8em; margin-top: 30px; margin-bottom: 15px; border-left: 5px solid #3498db; padding-left: 15px; }\n";
    styles += "h3 { color: #7f8c8d; font-size: 1.3em; margin-top: 20px; margin-bottom: 10px; }\n";
    styles += ".header { text-align: center; margin-bottom: 40px; padding: 30px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border-radius: 10px; }\n";
    styles += ".header h1 { color: white; border: none; }\n";
    styles += ".metadata { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin: 20px 0; }\n";
    styles += ".metadata-item { background: #ecf0f1; padding: 15px; border-radius: 5px; }\n";
    styles += ".metadata-label { font-weight: bold; color: #7f8c8d; font-size: 0.9em; }\n";
    styles += ".metadata-value { font-size: 1.1em; color: #2c3e50; margin-top: 5px; }\n";
    styles += ".stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin: 20px 0; }\n";
    styles += ".stat-box { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 20px; border-radius: 10px; text-align: center; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }\n";
    styles += ".stat-value { font-size: 2.5em; font-weight: bold; }\n";
    styles += ".stat-label { font-size: 0.9em; opacity: 0.9; margin-top: 5px; }\n";
    styles += ".finding { background: white; border-left: 5px solid #3498db; padding: 20px; margin: 15px 0; border-radius: 5px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    styles += ".finding-critical { border-left-color: #e74c3c; background: #ffeaea; }\n";
    styles += ".finding-high { border-left-color: #e67e22; background: #fff3e6; }\n";
    styles += ".finding-medium { border-left-color: #f39c12; background: #fffaeb; }\n";
    styles += ".finding-low { border-left-color: #3498db; background: #e8f4f8; }\n";
    styles += ".finding-info { border-left-color: #95a5a6; background: #f8f9fa; }\n";
    styles += ".finding-title { font-size: 1.2em; font-weight: bold; margin-bottom: 10px; }\n";
    styles += ".finding-severity { display: inline-block; padding: 5px 15px; border-radius: 20px; font-size: 0.85em; font-weight: bold; color: white; margin-bottom: 10px; }\n";
    styles += ".severity-critical { background: #e74c3c; }\n";
    styles += ".severity-high { background: #e67e22; }\n";
    styles += ".severity-medium { background: #f39c12; }\n";
    styles += ".severity-low { background: #3498db; }\n";
    styles += ".severity-info { background: #95a5a6; }\n";
    styles += ".chart { background: white; padding: 20px; border-radius: 10px; margin: 20px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    styles += ".progress-bar { background: #ecf0f1; height: 30px; border-radius: 15px; overflow: hidden; margin: 10px 0; }\n";
    styles += ".progress-fill { background: linear-gradient(90deg, #3498db 0%, #2ecc71 100%); height: 100%; text-align: center; line-height: 30px; color: white; font-weight: bold; transition: width 0.3s; }\n";
    styles += ".ioc-table { width: 100%; border-collapse: collapse; margin: 20px 0; }\n";
    styles += ".ioc-table th { background: #34495e; color: white; padding: 12px; text-align: left; }\n";
    styles += ".ioc-table td { padding: 10px; border-bottom: 1px solid #ecf0f1; }\n";
    styles += ".ioc-table tr:hover { background: #f8f9fa; }\n";
    styles += ".footer { text-align: center; margin-top: 40px; padding-top: 20px; border-top: 2px solid #ecf0f1; color: #7f8c8d; font-size: 0.9em; }\n";
    styles += ".recommendation { background: #e8f8f5; border-left: 5px solid #1abc9c; padding: 15px; margin: 10px 0; border-radius: 5px; }\n";
    styles += "@media print { body { background: white; } .container { box-shadow: none; } }\n";
    styles += "</style>\n";

    return styles;
}

String ReportGenerator::generateExecutiveSummary() {
    String summary = "<div class=\"header\">\n";
    summary += "<h1>🔍 Forensic Investigation Report</h1>\n";
    summary += "<p style=\"font-size: 1.2em; margin-top: 10px;\">FRFD - Digital Forensics & Incident Response</p>\n";
    summary += "</div>\n";

    summary += "<div class=\"metadata\">\n";

    if (!case_number.isEmpty()) {
        summary += "<div class=\"metadata-item\"><div class=\"metadata-label\">Case Number</div><div class=\"metadata-value\">" + case_number + "</div></div>\n";
    }

    if (!organization.isEmpty()) {
        summary += "<div class=\"metadata-item\"><div class=\"metadata-label\">Organization</div><div class=\"metadata-value\">" + organization + "</div></div>\n";
    }

    if (!target_system.isEmpty()) {
        summary += "<div class=\"metadata-item\"><div class=\"metadata-label\">Target System</div><div class=\"metadata-value\">" + target_system + "</div></div>\n";
    }

    summary += "<div class=\"metadata-item\"><div class=\"metadata-label\">Investigator</div><div class=\"metadata-value\">" + investigator_name + "</div></div>\n";
    summary += "<div class=\"metadata-item\"><div class=\"metadata-label\">Report Generated</div><div class=\"metadata-value\">" + String(millis() / 1000) + "s</div></div>\n";
    summary += "<div class=\"metadata-item\"><div class=\"metadata-label\">Target OS</div><div class=\"metadata-value\">" + (statistics.target_os.isEmpty() ? "Unknown" : statistics.target_os) + "</div></div>\n";

    summary += "</div>\n";

    summary += "<h2>📋 Executive Summary</h2>\n";
    summary += "<p>This report summarizes the forensic analysis conducted on the target system. ";
    summary += "A total of <strong>" + String(statistics.modules_executed) + " forensic modules</strong> were executed, ";
    summary += "collecting <strong>" + String(statistics.total_files_collected) + " files</strong> ";
    summary += "(" + formatBytes(statistics.total_bytes_collected) + ") ";
    summary += "in " + formatDuration(statistics.collection_duration_ms) + ".</p>\n";

    // Critical findings summary
    auto critical = getFindingsBySeverity(SEVERITY_CRITICAL);
    auto high = getFindingsBySeverity(SEVERITY_HIGH);
    auto medium = getFindingsBySeverity(SEVERITY_MEDIUM);

    if (critical.size() > 0 || high.size() > 0) {
        summary += "<p style=\"background: #ffeaea; padding: 15px; border-radius: 5px; border-left: 5px solid #e74c3c; margin-top: 15px;\">";
        summary += "<strong>⚠️ Security Alert:</strong> ";
        if (critical.size() > 0) {
            summary += String(critical.size()) + " CRITICAL finding(s) identified. ";
        }
        if (high.size() > 0) {
            summary += String(high.size()) + " HIGH severity finding(s) identified. ";
        }
        summary += "Immediate action recommended.</p>\n";
    }

    return summary;
}

String ReportGenerator::generateStatisticsSection() {
    String stats = "<h2>📊 Collection Statistics</h2>\n";

    stats += "<div class=\"stats-grid\">\n";
    stats += "<div class=\"stat-box\"><div class=\"stat-value\">" + String(statistics.modules_executed) + "</div><div class=\"stat-label\">Modules Executed</div></div>\n";
    stats += "<div class=\"stat-box\"><div class=\"stat-value\">" + String(statistics.modules_succeeded) + "</div><div class=\"stat-label\">Successful</div></div>\n";
    stats += "<div class=\"stat-box\"><div class=\"stat-value\">" + String((int)statistics.success_rate) + "%</div><div class=\"stat-label\">Success Rate</div></div>\n";
    stats += "<div class=\"stat-box\"><div class=\"stat-value\">" + String(statistics.total_files_collected) + "</div><div class=\"stat-label\">Files Collected</div></div>\n";
    stats += "<div class=\"stat-box\"><div class=\"stat-value\">" + String(statistics.total_iocs) + "</div><div class=\"stat-label\">IOCs Extracted</div></div>\n";
    stats += "<div class=\"stat-box\"><div class=\"stat-value\">" + String(statistics.timeline_events) + "</div><div class=\"stat-label\">Timeline Events</div></div>\n";
    stats += "</div>\n";

    // Module success rate chart
    stats += "<div class=\"chart\">\n";
    stats += "<h3>Module Execution Success Rate</h3>\n";
    stats += "<div class=\"progress-bar\"><div class=\"progress-fill\" style=\"width: " + String((int)statistics.success_rate) + "%\">" + String((int)statistics.success_rate) + "%</div></div>\n";
    stats += "<p>" + String(statistics.modules_succeeded) + " of " + String(statistics.modules_executed) + " modules completed successfully</p>\n";
    stats += "</div>\n";

    return stats;
}

String ReportGenerator::generateFindingsSection() {
    if (findings.size() == 0) {
        return "";
    }

    String findings_html = "<h2>🔍 Security Findings</h2>\n";
    findings_html += "<p>Total findings identified: <strong>" + String(findings.size()) + "</strong></p>\n";

    // Sort by severity (critical first)
    for (int sev = SEVERITY_CRITICAL; sev <= SEVERITY_INFO; sev++) {
        auto sev_findings = getFindingsBySeverity((FindingSeverity)sev);

        for (const auto& finding : sev_findings) {
            String severity_class = "finding";
            switch (finding.severity) {
                case SEVERITY_CRITICAL: severity_class += " finding-critical"; break;
                case SEVERITY_HIGH: severity_class += " finding-high"; break;
                case SEVERITY_MEDIUM: severity_class += " finding-medium"; break;
                case SEVERITY_LOW: severity_class += " finding-low"; break;
                case SEVERITY_INFO: severity_class += " finding-info"; break;
            }

            findings_html += "<div class=\"" + severity_class + "\">\n";
            findings_html += "<span class=\"finding-severity " + String("severity-") + getSeverityName(finding.severity).toLowerCase() + "\">" + getSeverityName(finding.severity) + "</span>\n";
            findings_html += "<div class=\"finding-title\">" + finding.title + "</div>\n";
            findings_html += "<p>" + finding.description + "</p>\n";

            if (!finding.artifact_source.isEmpty()) {
                findings_html += "<p><strong>Source:</strong> " + finding.artifact_source + "</p>\n";
            }

            if (!finding.recommendation.isEmpty()) {
                findings_html += "<div class=\"recommendation\"><strong>Recommendation:</strong> " + finding.recommendation + "</div>\n";
            }

            findings_html += "</div>\n";
        }
    }

    return findings_html;
}

String ReportGenerator::generateIOCSection() {
    if (!ioc_extractor || ioc_extractor->getIOCCount() == 0) {
        return "";
    }

    String ioc_html = "<h2>🎯 Indicators of Compromise (IOCs)</h2>\n";

    auto ioc_stats = ioc_extractor->getStatistics();

    ioc_html += "<div class=\"stats-grid\">\n";
    ioc_html += "<div class=\"stat-box\"><div class=\"stat-value\">" + String(ioc_stats.total_iocs) + "</div><div class=\"stat-label\">Total IOCs</div></div>\n";
    ioc_html += "<div class=\"stat-box\"><div class=\"stat-value\">" + String(ioc_stats.ip_addresses) + "</div><div class=\"stat-label\">IP Addresses</div></div>\n";
    ioc_html += "<div class=\"stat-box\"><div class=\"stat-value\">" + String(ioc_stats.domains) + "</div><div class=\"stat-label\">Domains</div></div>\n";
    ioc_html += "<div class=\"stat-box\"><div class=\"stat-value\">" + String(ioc_stats.file_hashes) + "</div><div class=\"stat-label\">File Hashes</div></div>\n";
    ioc_html += "</div>\n";

    // Top IOCs table (first 20)
    auto iocs = ioc_extractor->getIOCs();
    if (iocs.size() > 0) {
        ioc_html += "<h3>Top Indicators</h3>\n";
        ioc_html += "<table class=\"ioc-table\">\n";
        ioc_html += "<tr><th>Type</th><th>Value</th><th>Confidence</th><th>Source</th></tr>\n";

        size_t count = 0;
        for (const auto& ioc : iocs) {
            if (count++ >= 20) break;

            ioc_html += "<tr>";
            ioc_html += "<td>" + ioc_extractor->getIOCTypeName(ioc.type) + "</td>";
            ioc_html += "<td>" + ioc.value + "</td>";
            ioc_html += "<td>" + String(ioc.confidence) + "%</td>";
            ioc_html += "<td>" + ioc.source_artifact + "</td>";
            ioc_html += "</tr>\n";
        }

        ioc_html += "</table>\n";
    }

    return ioc_html;
}

String ReportGenerator::generateTimelineSection() {
    if (!timeline_generator || timeline_generator->getEventCount() == 0) {
        return "";
    }

    String timeline_html = "<h2>⏱️ Timeline Analysis</h2>\n";

    auto timeline_stats = timeline_generator->getStatistics();

    timeline_html += "<p>Total timeline events: <strong>" + String(timeline_stats.total_events) + "</strong></p>\n";
    timeline_html += "<p>Time range: " + formatTimestamp(timeline_stats.earliest_timestamp) + " to " + formatTimestamp(timeline_stats.latest_timestamp) + "</p>\n";

    // Key events (first 10 high-significance events)
    auto events = timeline_generator->getEvents();

    // Sort by significance
    timeline_generator->sortBySignificance();

    if (events.size() > 0) {
        timeline_html += "<h3>Key Events</h3>\n";
        timeline_html += "<table class=\"ioc-table\">\n";
        timeline_html += "<tr><th>Time</th><th>Type</th><th>Description</th><th>Target</th></tr>\n";

        size_t count = 0;
        for (const auto& event : events) {
            if (event.significance >= 7 && count++ < 10) {
                timeline_html += "<tr>";
                timeline_html += "<td>" + event.datetime + "</td>";
                timeline_html += "<td>" + timeline_generator->getEventTypeName(event.type) + "</td>";
                timeline_html += "<td>" + event.description + "</td>";
                timeline_html += "<td>" + event.target + "</td>";
                timeline_html += "</tr>\n";
            }
        }

        timeline_html += "</table>\n";
    }

    return timeline_html;
}

String ReportGenerator::generateRecommendationsSection() {
    String rec = "<h2>💡 Recommendations</h2>\n";

    auto critical = getFindingsBySeverity(SEVERITY_CRITICAL);
    auto high = getFindingsBySeverity(SEVERITY_HIGH);

    if (critical.size() > 0) {
        rec += "<div class=\"recommendation\" style=\"border-color: #e74c3c; background: #ffeaea;\">\n";
        rec += "<strong>Immediate Actions Required:</strong>\n<ul>\n";
        rec += "<li>Address all CRITICAL findings immediately</li>\n";
        rec += "<li>Isolate affected systems if necessary</li>\n";
        rec += "<li>Engage incident response team</li>\n";
        rec += "</ul>\n</div>\n";
    }

    if (high.size() > 0) {
        rec += "<div class=\"recommendation\" style=\"border-color: #e67e22;\">\n";
        rec += "<strong>High Priority Actions:</strong>\n<ul>\n";
        rec += "<li>Review all HIGH severity findings within 24 hours</li>\n";
        rec += "<li>Implement recommended mitigations</li>\n";
        rec += "</ul>\n</div>\n";
    }

    rec += "<div class=\"recommendation\">\n";
    rec += "<strong>General Recommendations:</strong>\n<ul>\n";
    rec += "<li>Archive forensic evidence securely</li>\n";
    rec += "<li>Update detection signatures with discovered IOCs</li>\n";
    rec += "<li>Review and update security policies</li>\n";
    rec += "<li>Conduct post-incident review</li>\n";
    rec += "</ul>\n</div>\n";

    return rec;
}

String ReportGenerator::generateFooter() {
    String footer = "<div class=\"footer\">\n";
    footer += "<p>Report generated by FRFD Forensic System v" + String(FIRMWARE_VERSION) + "</p>\n";
    footer += "<p>Generated at: " + String(millis() / 1000) + "s uptime</p>\n";
    footer += "<p style=\"margin-top: 10px; font-size: 0.85em;\">This report is confidential and intended for authorized personnel only.</p>\n";
    footer += "</div>\n";

    return footer;
}

// Helper methods
String ReportGenerator::getSeverityColor(FindingSeverity severity) {
    switch (severity) {
        case SEVERITY_CRITICAL: return "#e74c3c";
        case SEVERITY_HIGH: return "#e67e22";
        case SEVERITY_MEDIUM: return "#f39c12";
        case SEVERITY_LOW: return "#3498db";
        case SEVERITY_INFO: return "#95a5a6";
        default: return "#95a5a6";
    }
}

String ReportGenerator::getSeverityName(FindingSeverity severity) {
    switch (severity) {
        case SEVERITY_CRITICAL: return "CRITICAL";
        case SEVERITY_HIGH: return "HIGH";
        case SEVERITY_MEDIUM: return "MEDIUM";
        case SEVERITY_LOW: return "LOW";
        case SEVERITY_INFO: return "INFO";
        default: return "UNKNOWN";
    }
}

String ReportGenerator::formatBytes(uint64_t bytes) {
    if (bytes < 1024) return String((unsigned long)bytes) + " B";
    if (bytes < 1024 * 1024) return String((unsigned long)(bytes / 1024)) + " KB";
    if (bytes < 1024 * 1024 * 1024) return String((unsigned long)(bytes / (1024 * 1024))) + " MB";
    return String((unsigned long)(bytes / (1024 * 1024 * 1024))) + " GB";
}

String ReportGenerator::formatDuration(unsigned long ms) {
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    if (hours > 0) {
        return String((unsigned long)hours) + "h " + String((unsigned long)(minutes % 60)) + "m";
    }
    if (minutes > 0) {
        return String((unsigned long)minutes) + "m " + String((unsigned long)(seconds % 60)) + "s";
    }
    return String((unsigned long)seconds) + "s";
}

String ReportGenerator::formatTimestamp(unsigned long timestamp) {
    return String(timestamp);
}

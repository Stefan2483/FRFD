#include "timeline_generator.h"

TimelineGenerator::TimelineGenerator() {
    storage = nullptr;
}

TimelineGenerator::~TimelineGenerator() {
}

void TimelineGenerator::begin(FRFDStorage* storage_ptr) {
    storage = storage_ptr;
}

bool TimelineGenerator::buildTimelineFromFile(const String& file_path) {
    if (!storage || !storage->fileExists(file_path)) {
        return false;
    }

    String content = storage->readFile(file_path);
    if (content.isEmpty()) {
        return false;
    }

    // Detect file type and parse accordingly
    String filename = file_path.substring(file_path.lastIndexOf('/') + 1);
    filename.toLowerCase();

    std::vector<TimelineEvent> parsed_events;

    // Parse based on filename patterns
    if (filename.indexOf("mft") >= 0) {
        parsed_events = parseMFTTimeline(content, file_path);
    } else if (filename.indexOf("usn") >= 0 || filename.indexOf("journal") >= 0) {
        parsed_events = parseUSNJournal(content, file_path);
    } else if (filename.indexOf("prefetch") >= 0) {
        parsed_events = parsePrefetchTimeline(content, file_path);
    } else if (filename.indexOf("event") >= 0 || filename.indexOf("log") >= 0) {
        parsed_events = parseEventLogTimeline(content, file_path);
    } else if (filename.indexOf("registry") >= 0 || filename.indexOf("reg") >= 0) {
        parsed_events = parseRegistryTimeline(content, file_path);
    } else if (filename.indexOf("browser") >= 0 || filename.indexOf("history") >= 0) {
        parsed_events = parseBrowserHistory(content, file_path);
    } else if (filename.indexOf("network") >= 0 || filename.indexOf("connection") >= 0) {
        parsed_events = parseNetworkConnections(content, file_path);
    } else if (filename.indexOf("auth") >= 0 || filename.indexOf("login") >= 0) {
        parsed_events = parseAuthLogs(content, file_path);
    } else if (filename.indexOf("process") >= 0) {
        parsed_events = parseProcessList(content, file_path);
    } else if (filename.indexOf("shimcache") >= 0) {
        parsed_events = parseShimCache(content, file_path);
    } else if (filename.indexOf("amcache") >= 0) {
        parsed_events = parseAmCache(content, file_path);
    } else if (filename.indexOf("jumplist") >= 0) {
        parsed_events = parseJumpLists(content, file_path);
    } else {
        // Generic CSV parsing
        if (isCSVFile(filename)) {
            parsed_events = parseEventLogTimeline(content, file_path);
        }
    }

    // Add all parsed events
    for (const auto& event : parsed_events) {
        addEvent(event);
    }

    return parsed_events.size() > 0;
}

bool TimelineGenerator::buildTimelineFromDirectory(const String& dir_path) {
    if (!storage) return false;

    std::vector<String> files = storage->getFileList(dir_path);
    int files_processed = 0;

    for (const auto& file : files) {
        String full_path = dir_path + "/" + file;
        if (buildTimelineFromFile(full_path)) {
            files_processed++;
        }
    }

    return files_processed > 0;
}

bool TimelineGenerator::buildTimelineFromAllArtifacts() {
    if (!storage) return false;

    String case_dir = storage->getCaseDirectory();
    if (case_dir.isEmpty()) return false;

    return buildTimelineFromDirectory(case_dir);
}

std::vector<TimelineEvent> TimelineGenerator::parseMFTTimeline(const String& content, const String& source) {
    std::vector<TimelineEvent> timeline_events;

    // Parse MFT CSV format
    // Expected columns: Filename, Created, Modified, Accessed, Changed

    std::vector<String> lines;
    int start = 0;
    int end = content.indexOf('\n');

    while (end >= 0) {
        String line = content.substring(start, end);
        lines.push_back(line);
        start = end + 1;
        end = content.indexOf('\n', start);
    }

    // Skip header if present
    int start_line = lines.size() > 0 && lines[0].indexOf("Filename") >= 0 ? 1 : 0;

    for (size_t i = start_line; i < lines.size(); i++) {
        std::vector<String> fields = parseCSVLine(lines[i]);

        if (fields.size() >= 5) {
            String filename = fields[0];
            String created = fields[1];
            String modified = fields[2];
            String accessed = fields[3];

            // Create event for file creation
            if (!created.isEmpty() && created != "N/A") {
                TimelineEvent event;
                event.timestamp = parseTimestamp(created);
                event.datetime = created;
                event.type = EVENT_FILE_CREATED;
                event.description = "File created";
                event.source_artifact = source;
                event.target = filename;
                event.significance = 5;
                timeline_events.push_back(event);
            }

            // Create event for file modification
            if (!modified.isEmpty() && modified != "N/A" && modified != created) {
                TimelineEvent event;
                event.timestamp = parseTimestamp(modified);
                event.datetime = modified;
                event.type = EVENT_FILE_MODIFIED;
                event.description = "File modified";
                event.source_artifact = source;
                event.target = filename;
                event.significance = 6;
                timeline_events.push_back(event);
            }

            // Create event for file access
            if (!accessed.isEmpty() && accessed != "N/A") {
                TimelineEvent event;
                event.timestamp = parseTimestamp(accessed);
                event.datetime = accessed;
                event.type = EVENT_FILE_ACCESSED;
                event.description = "File accessed";
                event.source_artifact = source;
                event.target = filename;
                event.significance = 4;
                timeline_events.push_back(event);
            }
        }
    }

    return timeline_events;
}

std::vector<TimelineEvent> TimelineGenerator::parseUSNJournal(const String& content, const String& source) {
    std::vector<TimelineEvent> timeline_events;

    // Parse USN Journal entries
    // Format: Timestamp, Filename, Reason

    std::vector<String> lines;
    int start = 0;
    int end = content.indexOf('\n');

    while (end >= 0) {
        String line = content.substring(start, end);
        if (line.length() > 0) lines.push_back(line);
        start = end + 1;
        end = content.indexOf('\n', start);
    }

    for (const auto& line : lines) {
        std::vector<String> fields = parseCSVLine(line);

        if (fields.size() >= 3) {
            TimelineEvent event;
            event.timestamp = parseTimestamp(fields[0]);
            event.datetime = fields[0];
            event.target = fields[1];
            event.source_artifact = source;
            event.significance = 5;

            String reason = fields[2];
            if (reason.indexOf("CREATE") >= 0) {
                event.type = EVENT_FILE_CREATED;
                event.description = "File created (USN)";
            } else if (reason.indexOf("MODIFY") >= 0) {
                event.type = EVENT_FILE_MODIFIED;
                event.description = "File modified (USN)";
            } else if (reason.indexOf("DELETE") >= 0) {
                event.type = EVENT_FILE_DELETED;
                event.description = "File deleted (USN)";
                event.significance = 7;
            } else {
                event.type = EVENT_UNKNOWN;
                event.description = "File change (USN): " + reason;
            }

            timeline_events.push_back(event);
        }
    }

    return timeline_events;
}

std::vector<TimelineEvent> TimelineGenerator::parsePrefetchTimeline(const String& content, const String& source) {
    std::vector<TimelineEvent> timeline_events;

    // Parse Prefetch CSV
    // Columns: Filename, LastExecuted, RunCount

    std::vector<String> lines;
    int start = 0;
    int end = content.indexOf('\n');

    while (end >= 0) {
        String line = content.substring(start, end);
        if (line.length() > 0) lines.push_back(line);
        start = end + 1;
        end = content.indexOf('\n', start);
    }

    int start_line = lines.size() > 0 && lines[0].indexOf("Filename") >= 0 ? 1 : 0;

    for (size_t i = start_line; i < lines.size(); i++) {
        std::vector<String> fields = parseCSVLine(lines[i]);

        if (fields.size() >= 2) {
            TimelineEvent event;
            event.timestamp = parseTimestamp(fields[1]);
            event.datetime = fields[1];
            event.type = EVENT_PROCESS_STARTED;
            event.description = "Process executed (Prefetch)";
            event.source_artifact = source;
            event.target = fields[0];
            event.significance = 7; // Process execution is significant
            timeline_events.push_back(event);
        }
    }

    return timeline_events;
}

std::vector<TimelineEvent> TimelineGenerator::parseEventLogTimeline(const String& content, const String& source) {
    std::vector<TimelineEvent> timeline_events;

    // Parse Windows Event Log CSV
    // Typical columns: TimeCreated, Id, Message, Level

    std::vector<String> lines;
    int start = 0;
    int end = content.indexOf('\n');

    while (end >= 0) {
        String line = content.substring(start, end);
        if (line.length() > 0) lines.push_back(line);
        start = end + 1;
        end = content.indexOf('\n', start);
    }

    int start_line = lines.size() > 0 && lines[0].indexOf("Time") >= 0 ? 1 : 0;

    for (size_t i = start_line; i < lines.size(); i++) {
        std::vector<String> fields = parseCSVLine(lines[i]);

        if (fields.size() >= 3) {
            TimelineEvent event;
            event.timestamp = parseTimestamp(fields[0]);
            event.datetime = fields[0];
            event.source_artifact = source;
            event.details = fields.size() > 2 ? fields[2] : "";

            // Detect event type from message
            String message = event.details.toLowerCase();

            if (message.indexOf("logon") >= 0 || message.indexOf("login") >= 0) {
                if (message.indexOf("success") >= 0 || message.indexOf("4624") >= 0) {
                    event.type = EVENT_LOGIN_SUCCESS;
                    event.description = "Successful login";
                    event.significance = 6;
                } else {
                    event.type = EVENT_LOGIN_FAILURE;
                    event.description = "Failed login attempt";
                    event.significance = 8;
                }
            } else if (message.indexOf("service") >= 0) {
                if (message.indexOf("start") >= 0) {
                    event.type = EVENT_SERVICE_STARTED;
                    event.description = "Service started";
                    event.significance = 5;
                } else if (message.indexOf("stop") >= 0) {
                    event.type = EVENT_SERVICE_STOPPED;
                    event.description = "Service stopped";
                    event.significance = 5;
                }
            } else if (message.indexOf("boot") >= 0 || message.indexOf("startup") >= 0) {
                event.type = EVENT_SYSTEM_BOOT;
                event.description = "System boot";
                event.significance = 7;
            } else if (message.indexOf("shutdown") >= 0) {
                event.type = EVENT_SYSTEM_SHUTDOWN;
                event.description = "System shutdown";
                event.significance = 7;
            } else {
                event.type = EVENT_UNKNOWN;
                event.description = "Event log entry";
                event.significance = 4;
            }

            timeline_events.push_back(event);
        }
    }

    return timeline_events;
}

std::vector<TimelineEvent> TimelineGenerator::parseRegistryTimeline(const String& content, const String& source) {
    std::vector<TimelineEvent> timeline_events;

    // Parse registry export or CSV
    // Look for LastWriteTime or modified timestamps

    std::vector<String> lines;
    int start = 0;
    int end = content.indexOf('\n');

    while (end >= 0) {
        String line = content.substring(start, end);
        if (line.length() > 0) lines.push_back(line);
        start = end + 1;
        end = content.indexOf('\n', start);
    }

    for (const auto& line : lines) {
        // Look for registry key patterns
        if (line.indexOf("HKLM\\") >= 0 || line.indexOf("HKCU\\") >= 0 || line.indexOf("HKEY_") >= 0) {
            TimelineEvent event;
            event.type = EVENT_REGISTRY_MODIFIED;
            event.description = "Registry key modified";
            event.source_artifact = source;

            // Extract key name
            int key_start = line.indexOf("HK");
            if (key_start >= 0) {
                int key_end = line.indexOf('\"', key_start);
                if (key_end < 0) key_end = line.indexOf('\n', key_start);
                if (key_end < 0) key_end = line.length();

                event.target = line.substring(key_start, key_end);
            }

            event.timestamp = millis(); // Use current time if no timestamp available
            event.datetime = formatTimestamp(event.timestamp);
            event.significance = 5;

            timeline_events.push_back(event);
        }
    }

    return timeline_events;
}

std::vector<TimelineEvent> TimelineGenerator::parseBrowserHistory(const String& content, const String& source) {
    std::vector<TimelineEvent> timeline_events;

    // Parse browser history CSV
    // Columns: URL, Title, VisitTime

    std::vector<String> lines;
    int start = 0;
    int end = content.indexOf('\n');

    while (end >= 0) {
        String line = content.substring(start, end);
        if (line.length() > 0) lines.push_back(line);
        start = end + 1;
        end = content.indexOf('\n', start);
    }

    int start_line = lines.size() > 0 && (lines[0].indexOf("URL") >= 0 || lines[0].indexOf("url") >= 0) ? 1 : 0;

    for (size_t i = start_line; i < lines.size(); i++) {
        std::vector<String> fields = parseCSVLine(lines[i]);

        if (fields.size() >= 3) {
            TimelineEvent event;
            event.timestamp = parseTimestamp(fields[2]);
            event.datetime = fields[2];
            event.type = EVENT_BROWSER_NAVIGATION;
            event.description = "Browser navigation";
            event.source_artifact = source;
            event.target = fields[0]; // URL
            event.details = fields[1]; // Title
            event.significance = 5;
            timeline_events.push_back(event);
        }
    }

    return timeline_events;
}

std::vector<TimelineEvent> TimelineGenerator::parseNetworkConnections(const String& content, const String& source) {
    std::vector<TimelineEvent> timeline_events;

    // Parse network connection CSV
    // Look for RemoteAddress, State

    std::vector<String> lines;
    int start = 0;
    int end = content.indexOf('\n');

    while (end >= 0) {
        String line = content.substring(start, end);
        if (line.length() > 0) lines.push_back(line);
        start = end + 1;
        end = content.indexOf('\n', start);
    }

    for (const auto& line : lines) {
        // Look for IP addresses in line
        if (line.indexOf('.') >= 0 || line.indexOf(':') >= 0) {
            TimelineEvent event;
            event.type = EVENT_NETWORK_CONNECTION;
            event.description = "Network connection";
            event.source_artifact = source;
            event.timestamp = millis();
            event.datetime = formatTimestamp(event.timestamp);
            event.details = line;
            event.significance = 6;
            timeline_events.push_back(event);
        }
    }

    return timeline_events;
}

std::vector<TimelineEvent> TimelineGenerator::parseAuthLogs(const String& content, const String& source) {
    std::vector<TimelineEvent> timeline_events;

    // Parse Linux auth logs
    // Format: timestamp username action

    std::vector<String> lines;
    int start = 0;
    int end = content.indexOf('\n');

    while (end >= 0) {
        String line = content.substring(start, end);
        if (line.length() > 0) lines.push_back(line);
        start = end + 1;
        end = content.indexOf('\n', start);
    }

    for (const auto& line : lines) {
        TimelineEvent event;
        event.source_artifact = source;
        event.timestamp = millis();
        event.datetime = formatTimestamp(event.timestamp);

        String lower_line = line;
        lower_line.toLowerCase();

        if (lower_line.indexOf("success") >= 0 || lower_line.indexOf("accepted") >= 0) {
            event.type = EVENT_LOGIN_SUCCESS;
            event.description = "Successful authentication";
            event.significance = 6;
        } else if (lower_line.indexOf("fail") >= 0 || lower_line.indexOf("denied") >= 0) {
            event.type = EVENT_LOGIN_FAILURE;
            event.description = "Failed authentication";
            event.significance = 8;
        } else {
            continue; // Skip non-auth entries
        }

        event.details = line;
        timeline_events.push_back(event);
    }

    return timeline_events;
}

std::vector<TimelineEvent> TimelineGenerator::parseProcessList(const String& content, const String& source) {
    std::vector<TimelineEvent> timeline_events;

    // Parse process list CSV
    // Columns: ProcessName, PID, StartTime

    std::vector<String> lines;
    int start = 0;
    int end = content.indexOf('\n');

    while (end >= 0) {
        String line = content.substring(start, end);
        if (line.length() > 0) lines.push_back(line);
        start = end + 1;
        end = content.indexOf('\n', start);
    }

    int start_line = lines.size() > 0 && lines[0].indexOf("Process") >= 0 ? 1 : 0;

    for (size_t i = start_line; i < lines.size(); i++) {
        std::vector<String> fields = parseCSVLine(lines[i]);

        if (fields.size() >= 3) {
            TimelineEvent event;
            event.timestamp = parseTimestamp(fields[2]);
            event.datetime = fields[2];
            event.type = EVENT_PROCESS_STARTED;
            event.description = "Process started";
            event.source_artifact = source;
            event.target = fields[0];
            event.details = "PID: " + fields[1];
            event.significance = 6;
            timeline_events.push_back(event);
        }
    }

    return timeline_events;
}

std::vector<TimelineEvent> TimelineGenerator::parseShimCache(const String& content, const String& source) {
    std::vector<TimelineEvent> timeline_events;

    // ShimCache shows executed programs
    std::vector<String> lines;
    int start = 0;
    int end = content.indexOf('\n');

    while (end >= 0) {
        String line = content.substring(start, end);
        if (line.length() > 0 && line.indexOf(".exe") >= 0) {
            TimelineEvent event;
            event.type = EVENT_PROCESS_STARTED;
            event.description = "Program executed (ShimCache)";
            event.source_artifact = source;
            event.target = line;
            event.timestamp = millis();
            event.datetime = formatTimestamp(event.timestamp);
            event.significance = 7;
            timeline_events.push_back(event);
        }
        start = end + 1;
        end = content.indexOf('\n', start);
    }

    return timeline_events;
}

std::vector<TimelineEvent> TimelineGenerator::parseAmCache(const String& content, const String& source) {
    std::vector<TimelineEvent> timeline_events;

    // AmCache - program execution
    std::vector<String> lines;
    int start = 0;
    int end = content.indexOf('\n');

    while (end >= 0) {
        String line = content.substring(start, end);
        if (line.length() > 0 && (line.indexOf(".exe") >= 0 || line.indexOf("Path") >= 0)) {
            TimelineEvent event;
            event.type = EVENT_PROCESS_STARTED;
            event.description = "Application executed (AmCache)";
            event.source_artifact = source;
            event.details = line;
            event.timestamp = millis();
            event.datetime = formatTimestamp(event.timestamp);
            event.significance = 7;
            timeline_events.push_back(event);
        }
        start = end + 1;
        end = content.indexOf('\n', start);
    }

    return timeline_events;
}

std::vector<TimelineEvent> TimelineGenerator::parseJumpLists(const String& content, const String& source) {
    std::vector<TimelineEvent> timeline_events;

    // Jump lists - recently used files
    std::vector<String> lines;
    int start = 0;
    int end = content.indexOf('\n');

    while (end >= 0) {
        String line = content.substring(start, end);
        if (line.length() > 0) {
            TimelineEvent event;
            event.type = EVENT_FILE_ACCESSED;
            event.description = "File accessed (Jump List)";
            event.source_artifact = source;
            event.target = line;
            event.timestamp = millis();
            event.datetime = formatTimestamp(event.timestamp);
            event.significance = 5;
            timeline_events.push_back(event);
        }
        start = end + 1;
        end = content.indexOf('\n', start);
    }

    return timeline_events;
}

void TimelineGenerator::addEvent(const TimelineEvent& event) {
    events.push_back(event);
}

void TimelineGenerator::clearTimeline() {
    events.clear();
}

std::vector<TimelineEvent> TimelineGenerator::getEventsByType(TimelineEventType type) const {
    std::vector<TimelineEvent> filtered;
    for (const auto& event : events) {
        if (event.type == type) {
            filtered.push_back(event);
        }
    }
    return filtered;
}

std::vector<TimelineEvent> TimelineGenerator::getEventsByTimeRange(unsigned long start, unsigned long end) const {
    std::vector<TimelineEvent> filtered;
    for (const auto& event : events) {
        if (event.timestamp >= start && event.timestamp <= end) {
            filtered.push_back(event);
        }
    }
    return filtered;
}

std::vector<TimelineEvent> TimelineGenerator::getEventsByActor(const String& actor) const {
    std::vector<TimelineEvent> filtered;
    for (const auto& event : events) {
        if (event.actor == actor) {
            filtered.push_back(event);
        }
    }
    return filtered;
}

std::vector<TimelineEvent> TimelineGenerator::getEventsByTarget(const String& target) const {
    std::vector<TimelineEvent> filtered;
    for (const auto& event : events) {
        if (event.target == target) {
            filtered.push_back(event);
        }
    }
    return filtered;
}

void TimelineGenerator::sortByTimestamp() {
    // Simple bubble sort for small datasets
    for (size_t i = 0; i < events.size(); i++) {
        for (size_t j = i + 1; j < events.size(); j++) {
            if (events[j].timestamp < events[i].timestamp) {
                TimelineEvent temp = events[i];
                events[i] = events[j];
                events[j] = temp;
            }
        }
    }
}

void TimelineGenerator::sortBySignificance() {
    for (size_t i = 0; i < events.size(); i++) {
        for (size_t j = i + 1; j < events.size(); j++) {
            if (events[j].significance > events[i].significance) {
                TimelineEvent temp = events[i];
                events[i] = events[j];
                events[j] = temp;
            }
        }
    }
}

String TimelineGenerator::exportToJSON() {
    String json = "{";
    json += "\"timeline\":{";
    json += "\"event_count\":" + String(events.size()) + ",";
    json += "\"generated_at\":" + String(millis()) + ",";
    json += "\"events\":[";

    for (size_t i = 0; i < events.size(); i++) {
        if (i > 0) json += ",";

        json += "{";
        json += "\"timestamp\":" + String(events[i].timestamp) + ",";
        json += "\"datetime\":\"" + events[i].datetime + "\",";
        json += "\"type\":\"" + getEventTypeName(events[i].type) + "\",";
        json += "\"description\":\"" + events[i].description + "\",";
        json += "\"source\":\"" + events[i].source_artifact + "\",";
        json += "\"target\":\"" + events[i].target + "\",";
        json += "\"significance\":" + String(events[i].significance);
        json += "}";
    }

    json += "]}}";
    return json;
}

String TimelineGenerator::exportToCSV() {
    String csv = "Timestamp,DateTime,Type,Description,Source,Actor,Target,Significance\n";

    for (const auto& event : events) {
        csv += String(event.timestamp) + ",";
        csv += "\"" + event.datetime + "\",";
        csv += getEventTypeName(event.type) + ",";
        csv += "\"" + event.description + "\",";
        csv += "\"" + event.source_artifact + "\",";
        csv += "\"" + event.actor + "\",";
        csv += "\"" + event.target + "\",";
        csv += String(event.significance) + "\n";
    }

    return csv;
}

String TimelineGenerator::exportToHTML() {
    String html = "<!DOCTYPE html><html><head><title>Forensic Timeline</title>";
    html += "<style>body{font-family:Arial;margin:20px;}";
    html += "table{border-collapse:collapse;width:100%;}";
    html += "th,td{border:1px solid #ddd;padding:8px;text-align:left;}";
    html += "th{background:#4CAF50;color:white;}";
    html += "tr:hover{background:#f5f5f5;}";
    html += ".high{color:red;font-weight:bold;}";
    html += ".medium{color:orange;}";
    html += ".low{color:green;}";
    html += "</style></head><body>";
    html += "<h1>Forensic Timeline Report</h1>";
    html += "<p>Total Events: " + String(events.size()) + "</p>";
    html += "<table><tr><th>Time</th><th>Type</th><th>Description</th><th>Target</th><th>Significance</th></tr>";

    for (const auto& event : events) {
        html += "<tr>";
        html += "<td>" + event.datetime + "</td>";
        html += "<td>" + getEventTypeName(event.type) + "</td>";
        html += "<td>" + event.description + "</td>";
        html += "<td>" + event.target + "</td>";

        String sig_class = event.significance >= 8 ? "high" : (event.significance >= 6 ? "medium" : "low");
        html += "<td class=\"" + sig_class + "\">" + String(event.significance) + "</td>";
        html += "</tr>";
    }

    html += "</table></body></html>";
    return html;
}

String TimelineGenerator::exportToBodyFile() {
    // Sleuth Kit body file format
    String body = "";

    for (const auto& event : events) {
        body += "0|";  // MD5
        body += event.target + "|";  // Name
        body += "0|";  // Inode
        body += "0|";  // Mode
        body += "0|";  // UID
        body += "0|";  // GID
        body += "0|";  // Size
        body += String(event.timestamp) + "|";  // Atime
        body += String(event.timestamp) + "|";  // Mtime
        body += String(event.timestamp) + "|";  // Ctime
        body += String(event.timestamp);  // Crtime
        body += "\n";
    }

    return body;
}

bool TimelineGenerator::saveToFile(const String& filename, const String& format) {
    if (!storage) return false;

    String content;
    if (format == "json") {
        content = exportToJSON();
    } else if (format == "csv") {
        content = exportToCSV();
    } else if (format == "html") {
        content = exportToHTML();
    } else if (format == "bodyfile") {
        content = exportToBodyFile();
    } else {
        return false;
    }

    return storage->writeFile(filename, content);
}

TimelineStatistics TimelineGenerator::getStatistics() const {
    TimelineStatistics stats;
    stats.total_events = events.size();
    stats.unique_actors = 0;
    stats.unique_targets = 0;
    stats.earliest_timestamp = events.size() > 0 ? events[0].timestamp : 0;
    stats.latest_timestamp = events.size() > 0 ? events[0].timestamp : 0;

    for (const auto& event : events) {
        // Update timestamp range
        if (event.timestamp < stats.earliest_timestamp) {
            stats.earliest_timestamp = event.timestamp;
        }
        if (event.timestamp > stats.latest_timestamp) {
            stats.latest_timestamp = event.timestamp;
        }

        // Count by type
        stats.events_by_type[event.type]++;

        // Count by source
        stats.events_by_source[event.source_artifact]++;
    }

    return stats;
}

String TimelineGenerator::getEventTypeName(TimelineEventType type) const {
    switch (type) {
        case EVENT_FILE_CREATED: return "file_created";
        case EVENT_FILE_MODIFIED: return "file_modified";
        case EVENT_FILE_ACCESSED: return "file_accessed";
        case EVENT_FILE_DELETED: return "file_deleted";
        case EVENT_REGISTRY_CREATED: return "registry_created";
        case EVENT_REGISTRY_MODIFIED: return "registry_modified";
        case EVENT_REGISTRY_DELETED: return "registry_deleted";
        case EVENT_PROCESS_STARTED: return "process_started";
        case EVENT_PROCESS_TERMINATED: return "process_terminated";
        case EVENT_NETWORK_CONNECTION: return "network_connection";
        case EVENT_LOGIN_SUCCESS: return "login_success";
        case EVENT_LOGIN_FAILURE: return "login_failure";
        case EVENT_SERVICE_STARTED: return "service_started";
        case EVENT_SERVICE_STOPPED: return "service_stopped";
        case EVENT_SCHEDULED_TASK: return "scheduled_task";
        case EVENT_BROWSER_NAVIGATION: return "browser_navigation";
        case EVENT_EMAIL_SENT: return "email_sent";
        case EVENT_EMAIL_RECEIVED: return "email_received";
        case EVENT_USB_CONNECTED: return "usb_connected";
        case EVENT_USB_DISCONNECTED: return "usb_disconnected";
        case EVENT_SYSTEM_BOOT: return "system_boot";
        case EVENT_SYSTEM_SHUTDOWN: return "system_shutdown";
        default: return "unknown";
    }
}

unsigned long TimelineGenerator::parseTimestamp(const String& timestamp_str) {
    // Simplified timestamp parsing
    // In production, would use proper date parsing library
    return millis(); // Placeholder
}

String TimelineGenerator::formatTimestamp(unsigned long timestamp) {
    return String(timestamp);
}

bool TimelineGenerator::isCSVFile(const String& filename) {
    return filename.endsWith(".csv");
}

bool TimelineGenerator::isTXTFile(const String& filename) {
    return filename.endsWith(".txt");
}

bool TimelineGenerator::isJSONFile(const String& filename) {
    return filename.endsWith(".json");
}

std::vector<String> TimelineGenerator::parseCSVLine(const String& line) {
    std::vector<String> fields;
    String current_field = "";
    bool in_quotes = false;

    for (size_t i = 0; i < line.length(); i++) {
        char c = line[i];

        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == ',' && !in_quotes) {
            fields.push_back(current_field);
            current_field = "";
        } else {
            current_field += c;
        }
    }

    fields.push_back(current_field);
    return fields;
}

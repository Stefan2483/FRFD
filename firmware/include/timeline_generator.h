#ifndef TIMELINE_GENERATOR_H
#define TIMELINE_GENERATOR_H

#include <Arduino.h>
#include <vector>
#include <map>
#include "storage.h"

/**
 * @brief Timeline Event Types
 */
enum TimelineEventType {
    EVENT_FILE_CREATED,
    EVENT_FILE_MODIFIED,
    EVENT_FILE_ACCESSED,
    EVENT_FILE_DELETED,
    EVENT_REGISTRY_CREATED,
    EVENT_REGISTRY_MODIFIED,
    EVENT_REGISTRY_DELETED,
    EVENT_PROCESS_STARTED,
    EVENT_PROCESS_TERMINATED,
    EVENT_NETWORK_CONNECTION,
    EVENT_LOGIN_SUCCESS,
    EVENT_LOGIN_FAILURE,
    EVENT_SERVICE_STARTED,
    EVENT_SERVICE_STOPPED,
    EVENT_SCHEDULED_TASK,
    EVENT_BROWSER_NAVIGATION,
    EVENT_EMAIL_SENT,
    EVENT_EMAIL_RECEIVED,
    EVENT_USB_CONNECTED,
    EVENT_USB_DISCONNECTED,
    EVENT_SYSTEM_BOOT,
    EVENT_SYSTEM_SHUTDOWN,
    EVENT_UNKNOWN
};

/**
 * @brief Timeline Event
 */
struct TimelineEvent {
    unsigned long timestamp;      // Unix timestamp or millis since boot
    String datetime;              // Human-readable datetime (ISO 8601)
    TimelineEventType type;
    String description;           // Event description
    String source_artifact;       // Which artifact this came from
    String actor;                 // User/process that caused event
    String target;                // Target file/registry key/etc.
    String details;               // Additional details (JSON or text)
    uint8_t significance;         // 1-10 (1=low, 10=critical)
};

/**
 * @brief Timeline Statistics
 */
struct TimelineStatistics {
    uint32_t total_events;
    uint32_t unique_actors;
    uint32_t unique_targets;
    unsigned long earliest_timestamp;
    unsigned long latest_timestamp;
    std::map<TimelineEventType, uint16_t> events_by_type;
    std::map<String, uint16_t> events_by_source;
};

/**
 * @brief Timeline Generator
 *
 * Parses forensic artifacts and creates unified forensic timeline
 * Supports multiple output formats and filtering
 */
class TimelineGenerator {
public:
    TimelineGenerator();
    ~TimelineGenerator();

    // Initialization
    void begin(FRFDStorage* storage_ptr);

    // Timeline Building
    bool buildTimelineFromFile(const String& file_path);
    bool buildTimelineFromDirectory(const String& dir_path);
    bool buildTimelineFromAllArtifacts();

    // Artifact Parsers
    std::vector<TimelineEvent> parseMFTTimeline(const String& content, const String& source);
    std::vector<TimelineEvent> parseUSNJournal(const String& content, const String& source);
    std::vector<TimelineEvent> parsePrefetchTimeline(const String& content, const String& source);
    std::vector<TimelineEvent> parseEventLogTimeline(const String& content, const String& source);
    std::vector<TimelineEvent> parseRegistryTimeline(const String& content, const String& source);
    std::vector<TimelineEvent> parseBrowserHistory(const String& content, const String& source);
    std::vector<TimelineEvent> parseNetworkConnections(const String& content, const String& source);
    std::vector<TimelineEvent> parseAuthLogs(const String& content, const String& source);
    std::vector<TimelineEvent> parseProcessList(const String& content, const String& source);
    std::vector<TimelineEvent> parseShimCache(const String& content, const String& source);
    std::vector<TimelineEvent> parseAmCache(const String& content, const String& source);
    std::vector<TimelineEvent> parseJumpLists(const String& content, const String& source);

    // Event Management
    void addEvent(const TimelineEvent& event);
    void clearTimeline();
    std::vector<TimelineEvent> getEvents() const { return events; }
    std::vector<TimelineEvent> getEventsByType(TimelineEventType type) const;
    std::vector<TimelineEvent> getEventsByTimeRange(unsigned long start, unsigned long end) const;
    std::vector<TimelineEvent> getEventsByActor(const String& actor) const;
    std::vector<TimelineEvent> getEventsByTarget(const String& target) const;

    // Sorting
    void sortByTimestamp();
    void sortBySignificance();
    void sortByType();

    // Filtering
    void filterByEventType(TimelineEventType type);
    void filterBySignificance(uint8_t min_significance);
    void filterByTimeRange(unsigned long start, unsigned long end);

    // Export Methods
    String exportToJSON();
    String exportToCSV();
    String exportToHTML();
    String exportToBodyFile(); // Sleuth Kit body file format
    bool saveToFile(const String& filename, const String& format);

    // Statistics
    TimelineStatistics getStatistics() const;
    uint32_t getEventCount() const { return events.size(); }

    // Utility
    String getEventTypeName(TimelineEventType type) const;
    TimelineEventType detectEventType(const String& description);
    unsigned long parseTimestamp(const String& timestamp_str);
    String formatTimestamp(unsigned long timestamp);

private:
    FRFDStorage* storage;
    std::vector<TimelineEvent> events;

    // Helper methods
    bool isCSVFile(const String& filename);
    bool isTXTFile(const String& filename);
    bool isJSONFile(const String& filename);

    // Timestamp parsing helpers
    unsigned long parseISO8601(const String& timestamp);
    unsigned long parseWindowsFileTime(const String& timestamp);
    unsigned long parseUnixTimestamp(const String& timestamp);

    // CSV parsing helpers
    std::vector<String> parseCSVLine(const String& line);
    String extractCSVField(const String& line, int field_index);

    // Significance calculation
    uint8_t calculateSignificance(const TimelineEvent& event);

    // Deduplication
    void deduplicateEvents();
};

#endif // TIMELINE_GENERATOR_H

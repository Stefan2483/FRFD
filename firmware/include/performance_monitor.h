#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <Arduino.h>
#include <vector>
#include <map>

/**
 * @brief Performance Metric Types
 */
enum MetricType {
    METRIC_MODULE_EXECUTION,
    METRIC_FILE_OPERATION,
    METRIC_NETWORK_TRANSFER,
    METRIC_ANALYSIS_OPERATION,
    METRIC_MEMORY_USAGE,
    METRIC_DISK_OPERATION,
    METRIC_CPU_USAGE,
    METRIC_CUSTOM
};

/**
 * @brief Performance Metric
 */
struct PerformanceMetric {
    MetricType type;
    String name;
    unsigned long start_time;
    unsigned long end_time;
    unsigned long duration_ms;
    uint32_t memory_used_bytes;
    uint32_t memory_peak_bytes;
    String details;
    bool completed;
};

/**
 * @brief Module Performance Statistics
 */
struct ModuleStats {
    String module_name;
    uint32_t execution_count;
    uint32_t success_count;
    uint32_t failure_count;
    unsigned long total_duration_ms;
    unsigned long min_duration_ms;
    unsigned long max_duration_ms;
    unsigned long avg_duration_ms;
    uint32_t avg_memory_used;
    float success_rate;
};

/**
 * @brief System Resource Statistics
 */
struct SystemStats {
    // Memory
    uint32_t total_heap_size;
    uint32_t free_heap;
    uint32_t used_heap;
    uint32_t min_free_heap;
    uint32_t max_alloc_heap;
    float heap_usage_percent;

    // PSRAM
    uint32_t total_psram_size;
    uint32_t free_psram;
    uint32_t used_psram;
    float psram_usage_percent;

    // Timing
    unsigned long uptime_ms;
    unsigned long total_idle_time_ms;
    unsigned long total_active_time_ms;
    float cpu_usage_percent;

    // Storage
    uint32_t sd_total_bytes;
    uint32_t sd_used_bytes;
    uint32_t sd_free_bytes;
    float sd_usage_percent;

    // Network
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t packets_sent;
    uint32_t packets_received;
};

/**
 * @brief Performance Profile
 */
struct PerformanceProfile {
    String profile_name;
    unsigned long start_time;
    unsigned long end_time;
    std::vector<PerformanceMetric> metrics;
    SystemStats start_stats;
    SystemStats end_stats;
    String summary;
};

/**
 * @brief Performance Alert
 */
struct PerformanceAlert {
    String alert_type;
    String message;
    String severity;
    unsigned long timestamp;
    String recommendation;
};

/**
 * @brief Performance Monitor & Profiler
 *
 * Tracks execution times, memory usage, and system resources
 * Provides profiling data for performance optimization
 */
class PerformanceMonitor {
public:
    PerformanceMonitor();
    ~PerformanceMonitor();

    // Initialization
    void begin();
    void reset();

    // Metric Tracking
    uint32_t startMetric(MetricType type, const String& name);
    void endMetric(uint32_t metric_id);
    void cancelMetric(uint32_t metric_id);
    PerformanceMetric getMetric(uint32_t metric_id);

    // Module Performance Tracking
    void startModule(const String& module_name);
    void endModule(const String& module_name, bool success);
    ModuleStats getModuleStats(const String& module_name);
    std::vector<ModuleStats> getAllModuleStats();
    std::vector<String> getSlowestModules(uint8_t count = 5);
    std::vector<String> getFastestModules(uint8_t count = 5);

    // System Resource Monitoring
    SystemStats getSystemStats();
    void updateSystemStats();
    uint32_t getFreeHeap();
    uint32_t getUsedHeap();
    float getHeapUsagePercent();
    uint32_t getFreePSRAM();
    float getCPUUsage();

    // Memory Profiling
    void trackMemoryAllocation(uint32_t size, const String& source);
    void trackMemoryFree(uint32_t size, const String& source);
    uint32_t getTotalAllocated();
    uint32_t getPeakMemoryUsage();
    std::map<String, uint32_t> getMemoryBySource();

    // Performance Profiling
    void startProfile(const String& profile_name);
    void endProfile(const String& profile_name);
    PerformanceProfile getProfile(const String& profile_name);
    void clearProfiles();

    // Performance Alerts
    void checkThresholds();
    std::vector<PerformanceAlert> getAlerts();
    void clearAlerts();
    void setMemoryThreshold(uint32_t threshold_bytes);
    void setDurationThreshold(unsigned long threshold_ms);
    void setCPUThreshold(float threshold_percent);

    // Statistics
    uint32_t getTotalMetrics() const { return metrics.size(); }
    uint32_t getActiveMetrics() const;
    unsigned long getTotalProfiledTime() const;
    float getAverageModuleDuration() const;

    // Export
    String exportToJSON();
    String exportToCSV();
    String exportModuleStatsJSON();
    String exportSystemStatsJSON();
    bool saveToFile(const String& filename);

    // Configuration
    void enableMetricTracking(bool enabled) { metric_tracking_enabled = enabled; }
    void enableMemoryTracking(bool enabled) { memory_tracking_enabled = enabled; }
    void enableCPUTracking(bool enabled) { cpu_tracking_enabled = enabled; }
    void setMaxMetrics(uint16_t max) { max_metrics = max; }

private:
    std::vector<PerformanceMetric> metrics;
    std::map<String, ModuleStats> module_stats;
    std::map<String, PerformanceProfile> profiles;
    std::map<String, uint32_t> memory_by_source;
    std::vector<PerformanceAlert> alerts;

    SystemStats current_system_stats;
    uint32_t next_metric_id;

    // Configuration
    bool metric_tracking_enabled;
    bool memory_tracking_enabled;
    bool cpu_tracking_enabled;
    uint16_t max_metrics;

    // Thresholds
    uint32_t memory_alert_threshold;
    unsigned long duration_alert_threshold;
    float cpu_alert_threshold;

    // Tracking
    uint32_t total_allocated;
    uint32_t peak_memory_usage;
    unsigned long system_start_time;
    unsigned long last_cpu_check;
    unsigned long last_active_time;

    // Helper methods
    void createAlert(const String& type, const String& message,
                    const String& severity, const String& recommendation);
    void updateModuleStats(const String& module_name, unsigned long duration, bool success);
    void cleanupOldMetrics();
    String getMetricTypeName(MetricType type);
    uint32_t calculateMemoryUsed(uint32_t metric_id);
    float calculateCPUUsage();
};

#endif // PERFORMANCE_MONITOR_H

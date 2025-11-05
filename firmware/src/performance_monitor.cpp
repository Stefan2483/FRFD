#include "performance_monitor.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

PerformanceMonitor::PerformanceMonitor()
    : next_metric_id(1),
      metric_tracking_enabled(true),
      memory_tracking_enabled(true),
      cpu_tracking_enabled(true),
      max_metrics(100),
      memory_alert_threshold(100000),  // 100KB
      duration_alert_threshold(30000), // 30 seconds
      cpu_alert_threshold(80.0),       // 80%
      total_allocated(0),
      peak_memory_usage(0),
      system_start_time(0),
      last_cpu_check(0),
      last_active_time(0) {
}

PerformanceMonitor::~PerformanceMonitor() {
}

void PerformanceMonitor::begin() {
    system_start_time = millis();
    updateSystemStats();

    Serial.println("[PerformanceMonitor] Initialized");
    Serial.println("[PerformanceMonitor] Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("[PerformanceMonitor] Free PSRAM: " + String(ESP.getFreePsram()) + " bytes");
}

void PerformanceMonitor::reset() {
    metrics.clear();
    module_stats.clear();
    profiles.clear();
    alerts.clear();
    memory_by_source.clear();

    next_metric_id = 1;
    total_allocated = 0;
    peak_memory_usage = 0;
    system_start_time = millis();

    Serial.println("[PerformanceMonitor] Reset");
}

// ===========================
// Metric Tracking
// ===========================

uint32_t PerformanceMonitor::startMetric(MetricType type, const String& name) {
    if (!metric_tracking_enabled) return 0;

    // Cleanup old metrics if we've reached the limit
    if (metrics.size() >= max_metrics) {
        cleanupOldMetrics();
    }

    PerformanceMetric metric;
    metric.type = type;
    metric.name = name;
    metric.start_time = millis();
    metric.end_time = 0;
    metric.duration_ms = 0;
    metric.memory_used_bytes = ESP.getFreeHeap();
    metric.memory_peak_bytes = 0;
    metric.completed = false;

    uint32_t metric_id = next_metric_id++;
    metrics.push_back(metric);

    return metric_id;
}

void PerformanceMonitor::endMetric(uint32_t metric_id) {
    if (!metric_tracking_enabled || metric_id == 0) return;

    // Find metric by ID (metrics are stored sequentially, so ID-1 should be the index)
    if (metric_id <= metrics.size()) {
        size_t index = metric_id - 1;
        if (index < metrics.size() && !metrics[index].completed) {
            metrics[index].end_time = millis();
            metrics[index].duration_ms = metrics[index].end_time - metrics[index].start_time;
            metrics[index].memory_peak_bytes = metrics[index].memory_used_bytes - ESP.getFreeHeap();
            metrics[index].completed = true;

            // Check for performance alerts
            if (metrics[index].duration_ms > duration_alert_threshold) {
                createAlert("SLOW_OPERATION",
                           metrics[index].name + " took " + String(metrics[index].duration_ms) + "ms",
                           "WARNING",
                           "Consider optimizing this operation");
            }

            if (metrics[index].memory_peak_bytes > memory_alert_threshold) {
                createAlert("HIGH_MEMORY_USAGE",
                           metrics[index].name + " used " + String(metrics[index].memory_peak_bytes) + " bytes",
                           "WARNING",
                           "Consider reducing memory usage");
            }
        }
    }
}

void PerformanceMonitor::cancelMetric(uint32_t metric_id) {
    if (metric_id == 0) return;

    if (metric_id <= metrics.size()) {
        size_t index = metric_id - 1;
        if (index < metrics.size()) {
            metrics[index].completed = true;
            metrics[index].details = "Cancelled";
        }
    }
}

PerformanceMetric PerformanceMonitor::getMetric(uint32_t metric_id) {
    if (metric_id > 0 && metric_id <= metrics.size()) {
        return metrics[metric_id - 1];
    }
    return PerformanceMetric();
}

// ===========================
// Module Performance Tracking
// ===========================

void PerformanceMonitor::startModule(const String& module_name) {
    uint32_t metric_id = startMetric(METRIC_MODULE_EXECUTION, module_name);

    // Initialize module stats if first time
    if (module_stats.find(module_name) == module_stats.end()) {
        ModuleStats stats;
        stats.module_name = module_name;
        stats.execution_count = 0;
        stats.success_count = 0;
        stats.failure_count = 0;
        stats.total_duration_ms = 0;
        stats.min_duration_ms = 0xFFFFFFFF;
        stats.max_duration_ms = 0;
        stats.avg_duration_ms = 0;
        stats.avg_memory_used = 0;
        stats.success_rate = 0.0;

        module_stats[module_name] = stats;
    }
}

void PerformanceMonitor::endModule(const String& module_name, bool success) {
    // Find the most recent metric for this module
    for (int i = metrics.size() - 1; i >= 0; i--) {
        if (metrics[i].name == module_name && !metrics[i].completed) {
            endMetric(i + 1);
            updateModuleStats(module_name, metrics[i].duration_ms, success);
            break;
        }
    }
}

ModuleStats PerformanceMonitor::getModuleStats(const String& module_name) {
    if (module_stats.find(module_name) != module_stats.end()) {
        return module_stats[module_name];
    }
    return ModuleStats();
}

std::vector<ModuleStats> PerformanceMonitor::getAllModuleStats() {
    std::vector<ModuleStats> all_stats;
    for (const auto& pair : module_stats) {
        all_stats.push_back(pair.second);
    }
    return all_stats;
}

std::vector<String> PerformanceMonitor::getSlowestModules(uint8_t count) {
    std::vector<std::pair<String, unsigned long>> sorted_modules;

    for (const auto& pair : module_stats) {
        sorted_modules.push_back({pair.first, pair.second.avg_duration_ms});
    }

    // Sort by duration (descending)
    std::sort(sorted_modules.begin(), sorted_modules.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<String> slowest;
    for (size_t i = 0; i < std::min((size_t)count, sorted_modules.size()); i++) {
        slowest.push_back(sorted_modules[i].first);
    }

    return slowest;
}

std::vector<String> PerformanceMonitor::getFastestModules(uint8_t count) {
    std::vector<std::pair<String, unsigned long>> sorted_modules;

    for (const auto& pair : module_stats) {
        sorted_modules.push_back({pair.first, pair.second.avg_duration_ms});
    }

    // Sort by duration (ascending)
    std::sort(sorted_modules.begin(), sorted_modules.end(),
             [](const auto& a, const auto& b) { return a.second < b.second; });

    std::vector<String> fastest;
    for (size_t i = 0; i < std::min((size_t)count, sorted_modules.size()); i++) {
        fastest.push_back(sorted_modules[i].first);
    }

    return fastest;
}

// ===========================
// System Resource Monitoring
// ===========================

SystemStats PerformanceMonitor::getSystemStats() {
    updateSystemStats();
    return current_system_stats;
}

void PerformanceMonitor::updateSystemStats() {
    // Memory stats
    current_system_stats.total_heap_size = ESP.getHeapSize();
    current_system_stats.free_heap = ESP.getFreeHeap();
    current_system_stats.used_heap = current_system_stats.total_heap_size - current_system_stats.free_heap;
    current_system_stats.min_free_heap = ESP.getMinFreeHeap();
    current_system_stats.max_alloc_heap = ESP.getMaxAllocHeap();
    current_system_stats.heap_usage_percent =
        (float)current_system_stats.used_heap / current_system_stats.total_heap_size * 100.0;

    // PSRAM stats
    current_system_stats.total_psram_size = ESP.getPsramSize();
    current_system_stats.free_psram = ESP.getFreePsram();
    current_system_stats.used_psram = current_system_stats.total_psram_size - current_system_stats.free_psram;
    current_system_stats.psram_usage_percent =
        current_system_stats.total_psram_size > 0 ?
        (float)current_system_stats.used_psram / current_system_stats.total_psram_size * 100.0 : 0.0;

    // Timing
    current_system_stats.uptime_ms = millis() - system_start_time;

    // CPU usage (simplified estimate)
    if (cpu_tracking_enabled) {
        current_system_stats.cpu_usage_percent = calculateCPUUsage();
    }
}

uint32_t PerformanceMonitor::getFreeHeap() {
    return ESP.getFreeHeap();
}

uint32_t PerformanceMonitor::getUsedHeap() {
    return ESP.getHeapSize() - ESP.getFreeHeap();
}

float PerformanceMonitor::getHeapUsagePercent() {
    return (float)getUsedHeap() / ESP.getHeapSize() * 100.0;
}

uint32_t PerformanceMonitor::getFreePSRAM() {
    return ESP.getFreePsram();
}

float PerformanceMonitor::getCPUUsage() {
    return calculateCPUUsage();
}

// ===========================
// Memory Profiling
// ===========================

void PerformanceMonitor::trackMemoryAllocation(uint32_t size, const String& source) {
    if (!memory_tracking_enabled) return;

    total_allocated += size;
    memory_by_source[source] += size;

    if (total_allocated > peak_memory_usage) {
        peak_memory_usage = total_allocated;
    }
}

void PerformanceMonitor::trackMemoryFree(uint32_t size, const String& source) {
    if (!memory_tracking_enabled) return;

    if (total_allocated >= size) {
        total_allocated -= size;
    }

    if (memory_by_source.find(source) != memory_by_source.end()) {
        if (memory_by_source[source] >= size) {
            memory_by_source[source] -= size;
        }
    }
}

uint32_t PerformanceMonitor::getTotalAllocated() {
    return total_allocated;
}

uint32_t PerformanceMonitor::getPeakMemoryUsage() {
    return peak_memory_usage;
}

std::map<String, uint32_t> PerformanceMonitor::getMemoryBySource() {
    return memory_by_source;
}

// ===========================
// Performance Profiling
// ===========================

void PerformanceMonitor::startProfile(const String& profile_name) {
    PerformanceProfile profile;
    profile.profile_name = profile_name;
    profile.start_time = millis();
    profile.start_stats = getSystemStats();

    profiles[profile_name] = profile;

    Serial.println("[PerformanceMonitor] Started profile: " + profile_name);
}

void PerformanceMonitor::endProfile(const String& profile_name) {
    if (profiles.find(profile_name) == profiles.end()) {
        return;
    }

    profiles[profile_name].end_time = millis();
    profiles[profile_name].end_stats = getSystemStats();

    unsigned long duration = profiles[profile_name].end_time - profiles[profile_name].start_time;
    profiles[profile_name].summary = "Duration: " + String(duration) + "ms";

    Serial.println("[PerformanceMonitor] Ended profile: " + profile_name + " (" + String(duration) + "ms)");
}

PerformanceProfile PerformanceMonitor::getProfile(const String& profile_name) {
    if (profiles.find(profile_name) != profiles.end()) {
        return profiles[profile_name];
    }
    return PerformanceProfile();
}

void PerformanceMonitor::clearProfiles() {
    profiles.clear();
}

// ===========================
// Performance Alerts
// ===========================

void PerformanceMonitor::checkThresholds() {
    updateSystemStats();

    // Check memory threshold
    if (current_system_stats.heap_usage_percent > 90.0) {
        createAlert("CRITICAL_MEMORY",
                   "Heap usage at " + String(current_system_stats.heap_usage_percent, 1) + "%",
                   "CRITICAL",
                   "Free memory immediately or system may crash");
    } else if (current_system_stats.heap_usage_percent > 80.0) {
        createAlert("HIGH_MEMORY",
                   "Heap usage at " + String(current_system_stats.heap_usage_percent, 1) + "%",
                   "WARNING",
                   "Consider freeing memory");
    }

    // Check CPU threshold
    if (cpu_tracking_enabled && current_system_stats.cpu_usage_percent > cpu_alert_threshold) {
        createAlert("HIGH_CPU",
                   "CPU usage at " + String(current_system_stats.cpu_usage_percent, 1) + "%",
                   "WARNING",
                   "System may be under heavy load");
    }
}

std::vector<PerformanceAlert> PerformanceMonitor::getAlerts() {
    return alerts;
}

void PerformanceMonitor::clearAlerts() {
    alerts.clear();
}

void PerformanceMonitor::setMemoryThreshold(uint32_t threshold_bytes) {
    memory_alert_threshold = threshold_bytes;
}

void PerformanceMonitor::setDurationThreshold(unsigned long threshold_ms) {
    duration_alert_threshold = threshold_ms;
}

void PerformanceMonitor::setCPUThreshold(float threshold_percent) {
    cpu_alert_threshold = threshold_percent;
}

// ===========================
// Statistics
// ===========================

uint32_t PerformanceMonitor::getActiveMetrics() const {
    uint32_t active = 0;
    for (const auto& metric : metrics) {
        if (!metric.completed) {
            active++;
        }
    }
    return active;
}

unsigned long PerformanceMonitor::getTotalProfiledTime() const {
    unsigned long total = 0;
    for (const auto& metric : metrics) {
        if (metric.completed) {
            total += metric.duration_ms;
        }
    }
    return total;
}

float PerformanceMonitor::getAverageModuleDuration() const {
    if (module_stats.empty()) return 0.0;

    unsigned long total = 0;
    for (const auto& pair : module_stats) {
        total += pair.second.avg_duration_ms;
    }

    return (float)total / module_stats.size();
}

// ===========================
// Export Methods
// ===========================

String PerformanceMonitor::exportToJSON() {
    String json = "{\n";
    json += "  \"system_stats\": " + exportSystemStatsJSON() + ",\n";
    json += "  \"module_stats\": " + exportModuleStatsJSON() + ",\n";
    json += "  \"metrics_count\": " + String(metrics.size()) + ",\n";
    json += "  \"active_metrics\": " + String(getActiveMetrics()) + ",\n";
    json += "  \"total_profiled_time_ms\": " + String(getTotalProfiledTime()) + ",\n";
    json += "  \"peak_memory_usage\": " + String(peak_memory_usage) + "\n";
    json += "}\n";

    return json;
}

String PerformanceMonitor::exportModuleStatsJSON() {
    String json = "[\n";

    size_t count = 0;
    for (const auto& pair : module_stats) {
        const ModuleStats& stats = pair.second;

        json += "    {\n";
        json += "      \"module\": \"" + stats.module_name + "\",\n";
        json += "      \"executions\": " + String(stats.execution_count) + ",\n";
        json += "      \"success_rate\": " + String(stats.success_rate, 2) + ",\n";
        json += "      \"avg_duration_ms\": " + String(stats.avg_duration_ms) + ",\n";
        json += "      \"min_duration_ms\": " + String(stats.min_duration_ms) + ",\n";
        json += "      \"max_duration_ms\": " + String(stats.max_duration_ms) + "\n";
        json += "    }";

        if (count < module_stats.size() - 1) json += ",";
        json += "\n";
        count++;
    }

    json += "  ]";
    return json;
}

String PerformanceMonitor::exportSystemStatsJSON() {
    updateSystemStats();

    String json = "{\n";
    json += "    \"heap_total\": " + String(current_system_stats.total_heap_size) + ",\n";
    json += "    \"heap_used\": " + String(current_system_stats.used_heap) + ",\n";
    json += "    \"heap_free\": " + String(current_system_stats.free_heap) + ",\n";
    json += "    \"heap_usage_percent\": " + String(current_system_stats.heap_usage_percent, 2) + ",\n";
    json += "    \"psram_total\": " + String(current_system_stats.total_psram_size) + ",\n";
    json += "    \"psram_used\": " + String(current_system_stats.used_psram) + ",\n";
    json += "    \"psram_free\": " + String(current_system_stats.free_psram) + ",\n";
    json += "    \"uptime_ms\": " + String(current_system_stats.uptime_ms) + ",\n";
    json += "    \"cpu_usage_percent\": " + String(current_system_stats.cpu_usage_percent, 2) + "\n";
    json += "  }";

    return json;
}

String PerformanceMonitor::exportToCSV() {
    String csv = "module,executions,success_rate,avg_duration_ms,min_duration_ms,max_duration_ms\n";

    for (const auto& pair : module_stats) {
        const ModuleStats& stats = pair.second;

        csv += stats.module_name + ",";
        csv += String(stats.execution_count) + ",";
        csv += String(stats.success_rate, 2) + ",";
        csv += String(stats.avg_duration_ms) + ",";
        csv += String(stats.min_duration_ms) + ",";
        csv += String(stats.max_duration_ms) + "\n";
    }

    return csv;
}

// ===========================
// Helper Methods
// ===========================

void PerformanceMonitor::createAlert(const String& type, const String& message,
                                     const String& severity, const String& recommendation) {
    PerformanceAlert alert;
    alert.alert_type = type;
    alert.message = message;
    alert.severity = severity;
    alert.timestamp = millis();
    alert.recommendation = recommendation;

    alerts.push_back(alert);

    Serial.println("[PerformanceMonitor] ALERT: " + type + " - " + message);
}

void PerformanceMonitor::updateModuleStats(const String& module_name, unsigned long duration, bool success) {
    if (module_stats.find(module_name) == module_stats.end()) {
        return;
    }

    ModuleStats& stats = module_stats[module_name];

    stats.execution_count++;
    if (success) {
        stats.success_count++;
    } else {
        stats.failure_count++;
    }

    stats.total_duration_ms += duration;

    if (duration < stats.min_duration_ms) {
        stats.min_duration_ms = duration;
    }

    if (duration > stats.max_duration_ms) {
        stats.max_duration_ms = duration;
    }

    stats.avg_duration_ms = stats.total_duration_ms / stats.execution_count;
    stats.success_rate = (float)stats.success_count / stats.execution_count * 100.0;
}

void PerformanceMonitor::cleanupOldMetrics() {
    // Remove oldest completed metrics
    size_t remove_count = max_metrics / 4; // Remove 25% of old metrics
    size_t removed = 0;

    for (size_t i = 0; i < metrics.size() && removed < remove_count; i++) {
        if (metrics[i].completed) {
            metrics.erase(metrics.begin() + i);
            removed++;
            i--;
        }
    }
}

String PerformanceMonitor::getMetricTypeName(MetricType type) {
    switch (type) {
        case METRIC_MODULE_EXECUTION: return "Module Execution";
        case METRIC_FILE_OPERATION: return "File Operation";
        case METRIC_NETWORK_TRANSFER: return "Network Transfer";
        case METRIC_ANALYSIS_OPERATION: return "Analysis Operation";
        case METRIC_MEMORY_USAGE: return "Memory Usage";
        case METRIC_DISK_OPERATION: return "Disk Operation";
        case METRIC_CPU_USAGE: return "CPU Usage";
        case METRIC_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

float PerformanceMonitor::calculateCPUUsage() {
    // Simplified CPU usage estimation based on active time
    unsigned long current_time = millis();
    unsigned long elapsed = current_time - last_cpu_check;

    if (elapsed > 1000) { // Update every second
        last_cpu_check = current_time;
        // Estimate based on delay vs active time (simplified)
        return 50.0; // Placeholder - actual implementation would require task monitoring
    }

    return current_system_stats.cpu_usage_percent;
}

#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <set>

/**
 * @brief Module Execution Status
 */
enum ModuleStatus {
    MODULE_PENDING,
    MODULE_READY,
    MODULE_RUNNING,
    MODULE_COMPLETED,
    MODULE_FAILED,
    MODULE_SKIPPED,
    MODULE_DISABLED
};

/**
 * @brief Module Priority
 */
enum ModulePriority {
    PRIORITY_CRITICAL = 1,   // Must execute first (system info, memory dump)
    PRIORITY_HIGH = 2,       // Important artifacts (registry, event logs)
    PRIORITY_NORMAL = 3,     // Standard artifacts (files, network)
    PRIORITY_LOW = 4,        // Optional artifacts (browser history, recent files)
    PRIORITY_ANALYSIS = 5    // Post-collection analysis (IOC extraction, correlation)
};

/**
 * @brief Module Definition
 */
struct ForensicModule {
    String module_id;
    String module_name;
    String description;
    ModulePriority priority;
    ModuleStatus status;
    std::vector<String> dependencies;     // Module IDs that must execute first
    std::vector<String> conflicts;        // Module IDs that conflict
    std::vector<String> required_files;   // Files that must exist
    std::vector<String> produces_files;   // Files this module creates
    bool requires_admin;                  // Requires elevated privileges
    bool requires_network;                // Requires network access
    uint32_t estimated_time_ms;           // Estimated execution time
    uint32_t timeout_ms;                  // Maximum execution time
    unsigned long start_time;
    unsigned long end_time;
    String error_message;
};

/**
 * @brief Execution Plan
 */
struct ExecutionPlan {
    std::vector<std::vector<String>> execution_batches;  // Batches that can run in parallel
    std::vector<String> execution_order;                 // Sequential order
    uint32_t total_modules;
    uint32_t estimated_total_time_ms;
    String plan_summary;
};

/**
 * @brief Execution Statistics
 */
struct ExecutionStats {
    uint32_t total_modules;
    uint32_t completed_modules;
    uint32_t failed_modules;
    uint32_t skipped_modules;
    float completion_percent;
    unsigned long total_time_ms;
    String current_module;
    ModulePriority current_priority;
};

/**
 * @brief Module Manager
 *
 * Manages forensic module dependencies, execution order, and lifecycle
 * Ensures modules execute in correct order based on dependencies
 */
class ModuleManager {
public:
    ModuleManager();
    ~ModuleManager();

    // Module Registration
    bool registerModule(const ForensicModule& module);
    bool unregisterModule(const String& module_id);
    ForensicModule* getModule(const String& module_id);
    std::vector<ForensicModule> getAllModules();
    uint32_t getModuleCount() const { return modules.size(); }

    // Dependency Management
    bool addDependency(const String& module_id, const String& depends_on);
    bool removeDependency(const String& module_id, const String& depends_on);
    bool addConflict(const String& module_id, const String& conflicts_with);
    std::vector<String> getDependencies(const String& module_id);
    std::vector<String> getDependents(const String& module_id);  // Modules that depend on this
    bool hasCyclicDependencies();

    // Execution Planning
    ExecutionPlan createExecutionPlan();
    ExecutionPlan createExecutionPlan(const std::vector<String>& selected_modules);
    bool validateExecutionPlan(const ExecutionPlan& plan);
    std::vector<String> getReadyModules();  // Modules ready to execute

    // Module Execution
    bool startModule(const String& module_id);
    bool completeModule(const String& module_id, bool success);
    bool failModule(const String& module_id, const String& error);
    bool skipModule(const String& module_id, const String& reason);
    void resetModuleStatus(const String& module_id);
    void resetAllModules();

    // Module Status
    ModuleStatus getModuleStatus(const String& module_id);
    bool isModuleReady(const String& module_id);
    bool areAllDependenciesCompleted(const String& module_id);
    bool hasConflictingModules(const String& module_id);

    // Module Filtering
    std::vector<String> getModulesByPriority(ModulePriority priority);
    std::vector<String> getModulesByStatus(ModuleStatus status);
    std::vector<String> getCompletedModules();
    std::vector<String> getFailedModules();
    std::vector<String> getPendingModules();

    // Execution Statistics
    ExecutionStats getExecutionStats();
    float getCompletionPercent();
    String getCurrentModule();
    uint32_t getRemainingModules();

    // Module Groups
    void createModuleGroup(const String& group_name, const std::vector<String>& module_ids);
    std::vector<String> getModuleGroup(const String& group_name);
    void enableModuleGroup(const String& group_name);
    void disableModuleGroup(const String& group_name);

    // Configuration
    void enableModule(const String& module_id);
    void disableModule(const String& module_id);
    void setModulePriority(const String& module_id, ModulePriority priority);
    void setModuleTimeout(const String& module_id, uint32_t timeout_ms);

    // Validation
    bool validateModule(const ForensicModule& module);
    std::vector<String> getModuleValidationErrors(const String& module_id);

private:
    std::map<String, ForensicModule> modules;
    std::map<String, std::vector<String>> module_groups;

    // Dependency resolution helpers
    bool resolveDependencies(const String& module_id, std::vector<String>& resolved,
                            std::set<String>& visiting);
    bool detectCycle(const String& module_id, std::set<String>& visited,
                    std::set<String>& rec_stack);
    std::vector<String> topologicalSort();
    std::vector<String> topologicalSort(const std::vector<String>& selected_modules);

    // Execution helpers
    bool canModuleExecute(const String& module_id);
    uint32_t calculateEstimatedTime(const std::vector<String>& modules);
    std::vector<std::vector<String>> createParallelBatches(const std::vector<String>& sorted_modules);

    // Validation helpers
    bool checkDependencyExists(const String& module_id, const String& dependency);
    bool checkCircularDependency(const String& module_id);
};

#endif // MODULE_MANAGER_H

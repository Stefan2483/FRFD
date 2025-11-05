#include "module_manager.h"

ModuleManager::ModuleManager() {
}

ModuleManager::~ModuleManager() {
}

// ===========================
// Module Registration
// ===========================

bool ModuleManager::registerModule(const ForensicModule& module) {
    if (modules.find(module.module_id) != modules.end()) {
        Serial.println("[ModuleManager] Module already registered: " + module.module_id);
        return false;
    }

    if (!validateModule(module)) {
        Serial.println("[ModuleManager] Module validation failed: " + module.module_id);
        return false;
    }

    modules[module.module_id] = module;
    Serial.println("[ModuleManager] Registered module: " + module.module_name);

    return true;
}

bool ModuleManager::unregisterModule(const String& module_id) {
    if (modules.find(module_id) == modules.end()) {
        return false;
    }

    modules.erase(module_id);
    return true;
}

ForensicModule* ModuleManager::getModule(const String& module_id) {
    if (modules.find(module_id) != modules.end()) {
        return &modules[module_id];
    }
    return nullptr;
}

std::vector<ForensicModule> ModuleManager::getAllModules() {
    std::vector<ForensicModule> all_modules;
    for (const auto& pair : modules) {
        all_modules.push_back(pair.second);
    }
    return all_modules;
}

// ===========================
// Dependency Management
// ===========================

bool ModuleManager::addDependency(const String& module_id, const String& depends_on) {
    if (modules.find(module_id) == modules.end()) {
        return false;
    }

    if (modules.find(depends_on) == modules.end()) {
        Serial.println("[ModuleManager] Dependency module not found: " + depends_on);
        return false;
    }

    modules[module_id].dependencies.push_back(depends_on);

    // Check for circular dependencies
    if (checkCircularDependency(module_id)) {
        Serial.println("[ModuleManager] Circular dependency detected!");
        // Remove the dependency we just added
        auto& deps = modules[module_id].dependencies;
        deps.erase(std::remove(deps.begin(), deps.end(), depends_on), deps.end());
        return false;
    }

    return true;
}

bool ModuleManager::removeDependency(const String& module_id, const String& depends_on) {
    if (modules.find(module_id) == modules.end()) {
        return false;
    }

    auto& deps = modules[module_id].dependencies;
    deps.erase(std::remove(deps.begin(), deps.end(), depends_on), deps.end());
    return true;
}

bool ModuleManager::addConflict(const String& module_id, const String& conflicts_with) {
    if (modules.find(module_id) == modules.end()) {
        return false;
    }

    modules[module_id].conflicts.push_back(conflicts_with);
    return true;
}

std::vector<String> ModuleManager::getDependencies(const String& module_id) {
    if (modules.find(module_id) != modules.end()) {
        return modules[module_id].dependencies;
    }
    return std::vector<String>();
}

std::vector<String> ModuleManager::getDependents(const String& module_id) {
    std::vector<String> dependents;

    for (const auto& pair : modules) {
        const ForensicModule& module = pair.second;
        for (const auto& dep : module.dependencies) {
            if (dep == module_id) {
                dependents.push_back(module.module_id);
                break;
            }
        }
    }

    return dependents;
}

bool ModuleManager::hasCyclicDependencies() {
    std::set<String> visited;
    std::set<String> rec_stack;

    for (const auto& pair : modules) {
        if (detectCycle(pair.first, visited, rec_stack)) {
            return true;
        }
    }

    return false;
}

// ===========================
// Execution Planning
// ===========================

ExecutionPlan ModuleManager::createExecutionPlan() {
    std::vector<String> all_module_ids;
    for (const auto& pair : modules) {
        if (pair.second.status != MODULE_DISABLED) {
            all_module_ids.push_back(pair.first);
        }
    }
    return createExecutionPlan(all_module_ids);
}

ExecutionPlan ModuleManager::createExecutionPlan(const std::vector<String>& selected_modules) {
    ExecutionPlan plan;

    Serial.println("[ModuleManager] Creating execution plan for " +
                   String(selected_modules.size()) + " modules...");

    // Get topologically sorted order
    std::vector<String> sorted = topologicalSort(selected_modules);

    if (sorted.empty()) {
        Serial.println("[ModuleManager] Failed to create execution plan - circular dependencies?");
        return plan;
    }

    plan.execution_order = sorted;
    plan.total_modules = sorted.size();

    // Create parallel execution batches based on dependencies
    plan.execution_batches = createParallelBatches(sorted);

    // Calculate estimated time
    plan.estimated_total_time_ms = calculateEstimatedTime(sorted);

    // Generate summary
    plan.plan_summary = "Execution plan: " + String(plan.total_modules) + " modules in " +
                       String(plan.execution_batches.size()) + " batches. ";
    plan.plan_summary += "Estimated time: " + String(plan.estimated_total_time_ms / 1000) + "s";

    Serial.println("[ModuleManager] " + plan.plan_summary);

    return plan;
}

bool ModuleManager::validateExecutionPlan(const ExecutionPlan& plan) {
    // Check that all dependencies are satisfied in execution order
    std::set<String> completed;

    for (const auto& module_id : plan.execution_order) {
        if (modules.find(module_id) == modules.end()) {
            return false;
        }

        const ForensicModule& module = modules[module_id];

        // Check all dependencies are already in completed set
        for (const auto& dep : module.dependencies) {
            if (completed.find(dep) == completed.end()) {
                Serial.println("[ModuleManager] Dependency not satisfied: " + module_id +
                              " depends on " + dep);
                return false;
            }
        }

        completed.insert(module_id);
    }

    return true;
}

std::vector<String> ModuleManager::getReadyModules() {
    std::vector<String> ready;

    for (const auto& pair : modules) {
        if (isModuleReady(pair.first)) {
            ready.push_back(pair.first);
        }
    }

    return ready;
}

// ===========================
// Module Execution
// ===========================

bool ModuleManager::startModule(const String& module_id) {
    ForensicModule* module = getModule(module_id);
    if (!module) return false;

    if (!isModuleReady(module_id)) {
        Serial.println("[ModuleManager] Module not ready: " + module_id);
        return false;
    }

    module->status = MODULE_RUNNING;
    module->start_time = millis();

    Serial.println("[ModuleManager] Started module: " + module->module_name);

    return true;
}

bool ModuleManager::completeModule(const String& module_id, bool success) {
    ForensicModule* module = getModule(module_id);
    if (!module) return false;

    module->end_time = millis();

    if (success) {
        module->status = MODULE_COMPLETED;
        Serial.println("[ModuleManager] Completed module: " + module->module_name +
                      " (" + String(module->end_time - module->start_time) + "ms)");
    } else {
        module->status = MODULE_FAILED;
        Serial.println("[ModuleManager] Module failed: " + module->module_name);
    }

    return true;
}

bool ModuleManager::failModule(const String& module_id, const String& error) {
    ForensicModule* module = getModule(module_id);
    if (!module) return false;

    module->status = MODULE_FAILED;
    module->error_message = error;
    module->end_time = millis();

    Serial.println("[ModuleManager] Module failed: " + module->module_name + " - " + error);

    return true;
}

bool ModuleManager::skipModule(const String& module_id, const String& reason) {
    ForensicModule* module = getModule(module_id);
    if (!module) return false;

    module->status = MODULE_SKIPPED;
    module->error_message = reason;

    Serial.println("[ModuleManager] Skipped module: " + module->module_name + " - " + reason);

    return true;
}

void ModuleManager::resetModuleStatus(const String& module_id) {
    ForensicModule* module = getModule(module_id);
    if (module) {
        module->status = MODULE_PENDING;
        module->start_time = 0;
        module->end_time = 0;
        module->error_message = "";
    }
}

void ModuleManager::resetAllModules() {
    for (auto& pair : modules) {
        pair.second.status = MODULE_PENDING;
        pair.second.start_time = 0;
        pair.second.end_time = 0;
        pair.second.error_message = "";
    }
    Serial.println("[ModuleManager] Reset all modules");
}

// ===========================
// Module Status
// ===========================

ModuleStatus ModuleManager::getModuleStatus(const String& module_id) {
    ForensicModule* module = getModule(module_id);
    if (module) {
        return module->status;
    }
    return MODULE_DISABLED;
}

bool ModuleManager::isModuleReady(const String& module_id) {
    ForensicModule* module = getModule(module_id);
    if (!module) return false;

    if (module->status != MODULE_PENDING && module->status != MODULE_READY) {
        return false;
    }

    if (!areAllDependenciesCompleted(module_id)) {
        return false;
    }

    if (hasConflictingModules(module_id)) {
        return false;
    }

    return true;
}

bool ModuleManager::areAllDependenciesCompleted(const String& module_id) {
    ForensicModule* module = getModule(module_id);
    if (!module) return false;

    for (const auto& dep : module->dependencies) {
        ForensicModule* dep_module = getModule(dep);
        if (!dep_module || dep_module->status != MODULE_COMPLETED) {
            return false;
        }
    }

    return true;
}

bool ModuleManager::hasConflictingModules(const String& module_id) {
    ForensicModule* module = getModule(module_id);
    if (!module) return false;

    for (const auto& conflict : module->conflicts) {
        ForensicModule* conflict_module = getModule(conflict);
        if (conflict_module &&
            (conflict_module->status == MODULE_RUNNING ||
             conflict_module->status == MODULE_COMPLETED)) {
            return true;
        }
    }

    return false;
}

// ===========================
// Module Filtering
// ===========================

std::vector<String> ModuleManager::getModulesByPriority(ModulePriority priority) {
    std::vector<String> filtered;

    for (const auto& pair : modules) {
        if (pair.second.priority == priority) {
            filtered.push_back(pair.first);
        }
    }

    return filtered;
}

std::vector<String> ModuleManager::getModulesByStatus(ModuleStatus status) {
    std::vector<String> filtered;

    for (const auto& pair : modules) {
        if (pair.second.status == status) {
            filtered.push_back(pair.first);
        }
    }

    return filtered;
}

std::vector<String> ModuleManager::getCompletedModules() {
    return getModulesByStatus(MODULE_COMPLETED);
}

std::vector<String> ModuleManager::getFailedModules() {
    return getModulesByStatus(MODULE_FAILED);
}

std::vector<String> ModuleManager::getPendingModules() {
    return getModulesByStatus(MODULE_PENDING);
}

// ===========================
// Execution Statistics
// ===========================

ExecutionStats ModuleManager::getExecutionStats() {
    ExecutionStats stats;
    stats.total_modules = 0;
    stats.completed_modules = 0;
    stats.failed_modules = 0;
    stats.skipped_modules = 0;
    stats.total_time_ms = 0;

    for (const auto& pair : modules) {
        const ForensicModule& module = pair.second;

        if (module.status == MODULE_DISABLED) continue;

        stats.total_modules++;

        switch (module.status) {
            case MODULE_COMPLETED:
                stats.completed_modules++;
                stats.total_time_ms += (module.end_time - module.start_time);
                break;
            case MODULE_FAILED:
                stats.failed_modules++;
                break;
            case MODULE_SKIPPED:
                stats.skipped_modules++;
                break;
            case MODULE_RUNNING:
                stats.current_module = module.module_name;
                stats.current_priority = module.priority;
                break;
            default:
                break;
        }
    }

    stats.completion_percent = stats.total_modules > 0 ?
        (float)stats.completed_modules / stats.total_modules * 100.0 : 0.0;

    return stats;
}

float ModuleManager::getCompletionPercent() {
    return getExecutionStats().completion_percent;
}

String ModuleManager::getCurrentModule() {
    for (const auto& pair : modules) {
        if (pair.second.status == MODULE_RUNNING) {
            return pair.second.module_name;
        }
    }
    return "";
}

uint32_t ModuleManager::getRemainingModules() {
    uint32_t remaining = 0;
    for (const auto& pair : modules) {
        if (pair.second.status == MODULE_PENDING || pair.second.status == MODULE_READY) {
            remaining++;
        }
    }
    return remaining;
}

// ===========================
// Module Groups
// ===========================

void ModuleManager::createModuleGroup(const String& group_name, const std::vector<String>& module_ids) {
    module_groups[group_name] = module_ids;
    Serial.println("[ModuleManager] Created module group: " + group_name +
                   " (" + String(module_ids.size()) + " modules)");
}

std::vector<String> ModuleManager::getModuleGroup(const String& group_name) {
    if (module_groups.find(group_name) != module_groups.end()) {
        return module_groups[group_name];
    }
    return std::vector<String>();
}

void ModuleManager::enableModuleGroup(const String& group_name) {
    if (module_groups.find(group_name) == module_groups.end()) return;

    for (const auto& module_id : module_groups[group_name]) {
        enableModule(module_id);
    }
}

void ModuleManager::disableModuleGroup(const String& group_name) {
    if (module_groups.find(group_name) == module_groups.end()) return;

    for (const auto& module_id : module_groups[group_name]) {
        disableModule(module_id);
    }
}

// ===========================
// Configuration
// ===========================

void ModuleManager::enableModule(const String& module_id) {
    ForensicModule* module = getModule(module_id);
    if (module && module->status == MODULE_DISABLED) {
        module->status = MODULE_PENDING;
    }
}

void ModuleManager::disableModule(const String& module_id) {
    ForensicModule* module = getModule(module_id);
    if (module) {
        module->status = MODULE_DISABLED;
    }
}

void ModuleManager::setModulePriority(const String& module_id, ModulePriority priority) {
    ForensicModule* module = getModule(module_id);
    if (module) {
        module->priority = priority;
    }
}

void ModuleManager::setModuleTimeout(const String& module_id, uint32_t timeout_ms) {
    ForensicModule* module = getModule(module_id);
    if (module) {
        module->timeout_ms = timeout_ms;
    }
}

// ===========================
// Validation
// ===========================

bool ModuleManager::validateModule(const ForensicModule& module) {
    if (module.module_id.isEmpty()) {
        Serial.println("[ModuleManager] Validation error: Empty module ID");
        return false;
    }

    if (module.module_name.isEmpty()) {
        Serial.println("[ModuleManager] Validation error: Empty module name");
        return false;
    }

    return true;
}

// ===========================
// Dependency Resolution Helpers
// ===========================

bool ModuleManager::detectCycle(const String& module_id, std::set<String>& visited,
                                std::set<String>& rec_stack) {
    visited.insert(module_id);
    rec_stack.insert(module_id);

    ForensicModule* module = getModule(module_id);
    if (!module) return false;

    for (const auto& dep : module->dependencies) {
        if (visited.find(dep) == visited.end()) {
            if (detectCycle(dep, visited, rec_stack)) {
                return true;
            }
        } else if (rec_stack.find(dep) != rec_stack.end()) {
            return true; // Cycle detected
        }
    }

    rec_stack.erase(module_id);
    return false;
}

std::vector<String> ModuleManager::topologicalSort() {
    std::vector<String> all_modules;
    for (const auto& pair : modules) {
        if (pair.second.status != MODULE_DISABLED) {
            all_modules.push_back(pair.first);
        }
    }
    return topologicalSort(all_modules);
}

std::vector<String> ModuleManager::topologicalSort(const std::vector<String>& selected_modules) {
    std::vector<String> sorted;
    std::set<String> visiting;

    for (const auto& module_id : selected_modules) {
        if (!resolveDependencies(module_id, sorted, visiting)) {
            Serial.println("[ModuleManager] Failed to resolve dependencies for: " + module_id);
            return std::vector<String>(); // Return empty on failure
        }
    }

    return sorted;
}

bool ModuleManager::resolveDependencies(const String& module_id, std::vector<String>& resolved,
                                       std::set<String>& visiting) {
    if (std::find(resolved.begin(), resolved.end(), module_id) != resolved.end()) {
        return true; // Already resolved
    }

    if (visiting.find(module_id) != visiting.end()) {
        Serial.println("[ModuleManager] Circular dependency detected at: " + module_id);
        return false; // Circular dependency
    }

    visiting.insert(module_id);

    ForensicModule* module = getModule(module_id);
    if (!module) return false;

    // Resolve dependencies first
    for (const auto& dep : module->dependencies) {
        if (!resolveDependencies(dep, resolved, visiting)) {
            return false;
        }
    }

    visiting.erase(module_id);
    resolved.push_back(module_id);

    return true;
}

std::vector<std::vector<String>> ModuleManager::createParallelBatches(const std::vector<String>& sorted_modules) {
    std::vector<std::vector<String>> batches;
    std::set<String> completed;

    for (const auto& module_id : sorted_modules) {
        ForensicModule* module = getModule(module_id);
        if (!module) continue;

        // Check if all dependencies are in completed set
        bool can_start = true;
        for (const auto& dep : module->dependencies) {
            if (completed.find(dep) == completed.end()) {
                can_start = false;
                break;
            }
        }

        if (can_start) {
            // Can add to current batch
            if (batches.empty()) {
                batches.push_back(std::vector<String>());
            }
            batches.back().push_back(module_id);
        } else {
            // Need new batch
            batches.push_back(std::vector<String>{module_id});
        }

        completed.insert(module_id);
    }

    return batches;
}

uint32_t ModuleManager::calculateEstimatedTime(const std::vector<String>& module_ids) {
    uint32_t total = 0;
    for (const auto& module_id : module_ids) {
        ForensicModule* module = getModule(module_id);
        if (module) {
            total += module->estimated_time_ms;
        }
    }
    return total;
}

bool ModuleManager::checkCircularDependency(const String& module_id) {
    std::set<String> visited;
    std::set<String> rec_stack;
    return detectCycle(module_id, visited, rec_stack);
}

#include "module_profiles.h"

ModuleProfileManager::ModuleProfileManager() {
    initializeBuiltInProfiles();

    // Set standard profile as default
    current_profile = getStandardProfile(OS_UNKNOWN);
}

ModuleConfig ModuleProfileManager::createModuleConfig(
    const String& name,
    bool enabled,
    uint8_t priority,
    uint16_t timeout,
    const String& description
) {
    ModuleConfig config;
    config.name = name;
    config.enabled = enabled;
    config.priority = priority;
    config.timeout_seconds = timeout;
    config.description = description;
    return config;
}

// ============================================================================
// BUILT-IN PROFILES
// ============================================================================

CollectionProfile ModuleProfileManager::getQuickProfile(OperatingSystem os) {
    CollectionProfile profile;
    profile.name = "Quick";
    profile.description = "Fast triage collection (2-3 minutes)";
    profile.target_os = os;
    profile.estimated_duration_ms = 180000;  // 3 minutes

    if (os == OS_WINDOWS || os == OS_UNKNOWN) {
        // Windows quick modules - only essential, fast artifacts
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::NETWORK, true, 1, 30,
            "Network connections and DNS cache"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::MEMORY, true, 2, 60,
            "Process information"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::PREFETCH, true, 3, 60,
            "Prefetch files (execution history)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::SERVICES, true, 4, 30,
            "Running services"
        ));
    }

    if (os == OS_LINUX || os == OS_UNKNOWN) {
        // Linux quick modules
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::SYSINFO, true, 1, 30,
            "System information"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::NETSTAT, true, 2, 30,
            "Network connections"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::PERSISTENCE, true, 3, 60,
            "Persistence mechanisms"
        ));
    }

    if (os == OS_MACOS || os == OS_UNKNOWN) {
        // macOS quick modules
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::SYSINFO, true, 1, 30,
            "System information"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::PERSISTENCE, true, 2, 60,
            "LaunchAgents and LaunchDaemons"
        ));
    }

    return profile;
}

CollectionProfile ModuleProfileManager::getStandardProfile(OperatingSystem os) {
    CollectionProfile profile;
    profile.name = "Standard";
    profile.description = "Balanced collection (5-8 minutes)";
    profile.target_os = os;
    profile.estimated_duration_ms = 420000;  // 7 minutes

    if (os == OS_WINDOWS || os == OS_UNKNOWN) {
        // Windows standard modules - balanced forensics
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::BROWSER_HISTORY, true, 1, 180,
            "Browser history (Chrome, Firefox, Edge)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::MEMORY, true, 2, 60,
            "Process information and memory artifacts"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::AUTORUNS, true, 3, 90,
            "Autorun entries (persistence)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::NETWORK, true, 4, 90,
            "Network state (TCP, DNS, ARP)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::EVENTLOGS, true, 5, 180,
            "Event logs (Security, System, Application)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::PREFETCH, true, 6, 60,
            "Prefetch files"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::SCHTASKS, true, 7, 60,
            "Scheduled tasks"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::SERVICES, true, 8, 60,
            "Services information"
        ));
    }

    if (os == OS_LINUX || os == OS_UNKNOWN) {
        // Linux standard modules - balanced forensics
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::SHELL_HISTORY, true, 1, 60,
            "Shell history (bash, zsh)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::BROWSER_HISTORY, true, 2, 120,
            "Browser history (Firefox, Chrome)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::SYSINFO, true, 3, 60,
            "System information"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::AUTHLOGS, true, 4, 90,
            "Authentication logs"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::NETSTAT, true, 5, 60,
            "Network connections"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::KERNEL_MODULES, true, 6, 30,
            "Loaded kernel modules"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::PERSISTENCE, true, 7, 120,
            "Persistence mechanisms (cron, systemd)"
        ));
    }

    if (os == OS_MACOS || os == OS_UNKNOWN) {
        // macOS standard modules - balanced forensics
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::BROWSER_HISTORY, true, 1, 120,
            "Browser history (Safari, Chrome, Firefox)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::SYSINFO, true, 2, 60,
            "System information"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::PERSISTENCE, true, 3, 120,
            "Launch agents and daemons"
        ));
    }

    return profile;
}

CollectionProfile ModuleProfileManager::getDeepProfile(OperatingSystem os) {
    CollectionProfile profile;
    profile.name = "Deep";
    profile.description = "Comprehensive collection (15-30 minutes)";
    profile.target_os = os;
    profile.estimated_duration_ms = 1500000;  // 25 minutes

    if (os == OS_WINDOWS || os == OS_UNKNOWN) {
        // Windows deep - all modules including new ones
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::REGISTRY, true, 1, 300,
            "Registry hives (SAM, SYSTEM, SOFTWARE, SECURITY, NTUSER)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::MFT, true, 2, 600,
            "MFT and timeline artifacts (USN Journal)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::BROWSER_HISTORY, true, 3, 180,
            "Browser history (Chrome, Firefox, Edge)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::USER_FILES, true, 4, 240,
            "User file metadata (Downloads, Desktop, Documents, Recent)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::EVENTLOGS, true, 5, 300,
            "All event logs"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::MEMORY, true, 6, 120,
            "Process and memory artifacts"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::NETWORK, true, 7, 90,
            "Network state"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::PREFETCH, true, 8, 60,
            "Prefetch files"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::AUTORUNS, true, 9, 120,
            "All autorun locations"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::SCHTASKS, true, 10, 90,
            "Scheduled tasks with XML"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::SERVICES, true, 11, 90,
            "Services with details"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::RECYCLE_BIN, true, 12, 120,
            "Recycle Bin contents"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::SHIMCACHE, true, 13, 60,
            "ShimCache (AppCompatCache) execution artifacts"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::AMCACHE, true, 14, 60,
            "AmCache and BAM execution history"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::JUMPLISTS, true, 15, 90,
            "Jump Lists (recent file access)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::WMI, true, 16, 90,
            "WMI persistence mechanisms"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::USB_HISTORY, true, 17, 60,
            "USB device connection history"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Windows::PS_HISTORY, true, 18, 90,
            "PowerShell command history and logs"
        ));
    }

    if (os == OS_LINUX || os == OS_UNKNOWN) {
        // Linux deep - comprehensive with new modules
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::SHELL_HISTORY, true, 1, 90,
            "Shell history for all users (bash, zsh)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::SSH_CONFIG, true, 2, 90,
            "SSH configuration, authorized keys, known hosts"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::USER_ACCOUNTS, true, 3, 60,
            "User account database (passwd, shadow, group)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::BROWSER_HISTORY, true, 4, 180,
            "Browser history (Firefox, Chrome, Chromium)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::AUTHLOGS, true, 5, 120,
            "Authentication logs"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::DOCKER, true, 6, 240,
            "Docker containers and images"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::PERSISTENCE, true, 7, 180,
            "All persistence mechanisms (cron, systemd)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::NETSTAT, true, 8, 90,
            "Network state"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::SYSINFO, true, 9, 90,
            "System information"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::KERNEL_MODULES, true, 10, 60,
            "Kernel modules"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::SYSTEMD_JOURNAL, true, 11, 180,
            "systemd journal logs (comprehensive)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::FIREWALL, true, 12, 60,
            "Firewall rules (iptables, ufw, firewalld)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::Linux::CRON, true, 13, 90,
            "Cron jobs and scheduled tasks"
        ));
    }

    if (os == OS_MACOS || os == OS_UNKNOWN) {
        // macOS deep - comprehensive with new modules
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::UNIFIED_LOGS, true, 1, 300,
            "Unified logs (errors, security, auth, network)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::FSEVENTS, true, 2, 180,
            "Filesystem events database (/.fseventsd)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::BROWSER_HISTORY, true, 3, 180,
            "Browser history (Safari, Chrome, Firefox)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::SPOTLIGHT, true, 4, 240,
            "Spotlight index"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::USER_ACCOUNTS, true, 5, 90,
            "User account database"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::PERSISTENCE, true, 6, 180,
            "Launch agents, daemons, login items"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::SYSINFO, true, 7, 120,
            "System information"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::QUARANTINE, true, 8, 90,
            "Quarantine database (download tracking)"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::INSTALL_HISTORY, true, 9, 90,
            "Installation history and package managers"
        ));
        profile.modules.push_back(createModuleConfig(
            Modules::macOS::KEYCHAIN, true, 10, 90,
            "Keychain metadata (certificates, identities)"
        ));
    }

    return profile;
}

CollectionProfile ModuleProfileManager::getCustomProfile() {
    CollectionProfile profile;
    profile.name = "Custom";
    profile.description = "User-defined module selection";
    profile.target_os = OS_UNKNOWN;
    profile.estimated_duration_ms = 0;  // Calculate based on enabled modules

    // Start with all modules disabled, user can enable what they want
    // This is a placeholder - would be loaded from saved configuration

    return profile;
}

// ============================================================================
// PROFILE MANAGEMENT
// ============================================================================

void ModuleProfileManager::setCurrentProfile(const CollectionProfile& profile) {
    current_profile = profile;
}

bool ModuleProfileManager::isModuleEnabled(const String& module_name) const {
    for (const auto& module : current_profile.modules) {
        if (module.name == module_name) {
            return module.enabled;
        }
    }
    return false;
}

ModuleConfig ModuleProfileManager::getModuleConfig(const String& module_name) const {
    for (const auto& module : current_profile.modules) {
        if (module.name == module_name) {
            return module;
        }
    }

    // Return default config if not found
    ModuleConfig default_config;
    default_config.name = module_name;
    default_config.enabled = false;
    default_config.priority = 99;
    default_config.timeout_seconds = 60;
    default_config.description = "Unknown module";
    return default_config;
}

std::vector<ModuleConfig> ModuleProfileManager::getEnabledModules() const {
    std::vector<ModuleConfig> enabled;

    for (const auto& module : current_profile.modules) {
        if (module.enabled) {
            enabled.push_back(module);
        }
    }

    return enabled;
}

std::vector<ModuleConfig> ModuleProfileManager::getModulesByPriority() const {
    std::vector<ModuleConfig> modules = getEnabledModules();

    // Sort by priority (lower number = higher priority)
    std::sort(modules.begin(), modules.end(),
             [](const ModuleConfig& a, const ModuleConfig& b) {
                 return a.priority < b.priority;
             });

    return modules;
}

uint16_t ModuleProfileManager::getEnabledModuleCount() const {
    return getEnabledModules().size();
}

unsigned long ModuleProfileManager::getEstimatedDuration() const {
    if (current_profile.estimated_duration_ms > 0) {
        return current_profile.estimated_duration_ms;
    }

    // Calculate based on module timeouts
    unsigned long total = 0;
    for (const auto& module : current_profile.modules) {
        if (module.enabled) {
            total += module.timeout_seconds * 1000;
        }
    }

    return total;
}

void ModuleProfileManager::initializeBuiltInProfiles() {
    // Built-in profiles are created on-demand via get*Profile() methods
    // No persistent storage needed for built-in profiles
}

// ============================================================================
// PROFILE PERSISTENCE (File-based)
// ============================================================================

bool ModuleProfileManager::saveProfile(const CollectionProfile& profile) {
    // TODO: Implement JSON serialization and SD card storage
    // For now, return true (profiles work in-memory)
    return true;
}

bool ModuleProfileManager::loadProfile(const String& profile_name) {
    // TODO: Implement JSON deserialization from SD card
    // For now, load built-in profiles
    if (profile_name == "Quick") {
        current_profile = getQuickProfile(OS_UNKNOWN);
        return true;
    } else if (profile_name == "Standard") {
        current_profile = getStandardProfile(OS_UNKNOWN);
        return true;
    } else if (profile_name == "Deep") {
        current_profile = getDeepProfile(OS_UNKNOWN);
        return true;
    }

    return false;
}

CollectionProfile ModuleProfileManager::getProfile(const String& profile_name) {
    if (profile_name == "Quick") {
        return getQuickProfile(OS_UNKNOWN);
    } else if (profile_name == "Standard") {
        return getStandardProfile(OS_UNKNOWN);
    } else if (profile_name == "Deep") {
        return getDeepProfile(OS_UNKNOWN);
    } else if (profile_name == "Custom") {
        return getCustomProfile();
    }

    // Return standard as default
    return getStandardProfile(OS_UNKNOWN);
}

std::vector<String> ModuleProfileManager::listProfiles() {
    std::vector<String> profiles;
    profiles.push_back("Quick");
    profiles.push_back("Standard");
    profiles.push_back("Deep");
    profiles.push_back("Custom");
    return profiles;
}

bool ModuleProfileManager::deleteProfile(const String& profile_name) {
    // Cannot delete built-in profiles
    if (profile_name == "Quick" || profile_name == "Standard" ||
        profile_name == "Deep" || profile_name == "Custom") {
        return false;
    }

    // TODO: Implement deletion of user-saved profiles
    return false;
}

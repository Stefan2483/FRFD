#ifndef MODULE_PROFILES_H
#define MODULE_PROFILES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "config.h"

/**
 * @brief Module Configuration
 * Defines whether a module is enabled and its settings
 */
struct ModuleConfig {
    String name;                  // Module identifier
    bool enabled;                 // Whether module is enabled
    uint8_t priority;             // Execution priority (1=highest)
    uint16_t timeout_seconds;     // Maximum execution time
    String description;           // Human-readable description
};

/**
 * @brief Collection Profile
 * Pre-defined or custom module selection profiles
 */
struct CollectionProfile {
    String name;                  // Profile name
    String description;           // Profile description
    OperatingSystem target_os;    // Target OS (or OS_UNKNOWN for all)
    std::vector<ModuleConfig> modules;  // Module configurations
    unsigned long estimated_duration_ms;  // Estimated collection time
};

/**
 * @brief Profile Manager
 * Manages module selection profiles for forensics collection
 */
class ModuleProfileManager {
public:
    ModuleProfileManager();

    // Profile Management
    bool loadProfile(const String& profile_name);
    bool saveProfile(const CollectionProfile& profile);
    CollectionProfile getProfile(const String& profile_name);
    std::vector<String> listProfiles();
    bool deleteProfile(const String& profile_name);

    // Built-in Profiles
    CollectionProfile getQuickProfile(OperatingSystem os);
    CollectionProfile getStandardProfile(OperatingSystem os);
    CollectionProfile getDeepProfile(OperatingSystem os);
    CollectionProfile getCustomProfile();

    // Current Profile
    void setCurrentProfile(const CollectionProfile& profile);
    CollectionProfile getCurrentProfile() const { return current_profile; }

    // Module Queries
    bool isModuleEnabled(const String& module_name) const;
    ModuleConfig getModuleConfig(const String& module_name) const;
    std::vector<ModuleConfig> getEnabledModules() const;
    std::vector<ModuleConfig> getModulesByPriority() const;

    // Statistics
    uint16_t getEnabledModuleCount() const;
    unsigned long getEstimatedDuration() const;

private:
    CollectionProfile current_profile;

    // Helper methods
    void initializeBuiltInProfiles();
    ModuleConfig createModuleConfig(const String& name, bool enabled,
                                   uint8_t priority, uint16_t timeout,
                                   const String& description);
};

// Pre-defined module names
namespace Modules {
    // Windows modules
    namespace Windows {
        const String MEMORY = "win_memory";
        const String REGISTRY = "win_registry";
        const String AUTORUNS = "win_autoruns";
        const String NETWORK = "win_network";
        const String EVENTLOGS = "win_eventlogs";
        const String PREFETCH = "win_prefetch";
        const String SCHTASKS = "win_schtasks";
        const String SERVICES = "win_services";
        const String BROWSER_HISTORY = "win_browser";
        const String MFT = "win_mft";
        const String USER_FILES = "win_user_files";
        const String RECYCLE_BIN = "win_recycle";
        const String SHIMCACHE = "win_shimcache";
        const String AMCACHE = "win_amcache";
        const String JUMPLISTS = "win_jumplists";
        const String WMI = "win_wmi";
        const String USB_HISTORY = "win_usb";
        const String PS_HISTORY = "win_powershell";
    }

    // Linux modules
    namespace Linux {
        const String SYSINFO = "lnx_sysinfo";
        const String AUTHLOGS = "lnx_authlogs";
        const String NETSTAT = "lnx_netstat";
        const String KERNEL_MODULES = "lnx_kernel";
        const String PERSISTENCE = "lnx_persistence";
        const String SHELL_HISTORY = "lnx_shell_history";
        const String SSH_CONFIG = "lnx_ssh";
        const String USER_ACCOUNTS = "lnx_users";
        const String DOCKER = "lnx_docker";
        const String BROWSER_HISTORY = "lnx_browser";
        const String SYSTEMD_JOURNAL = "lnx_journal";
        const String FIREWALL = "lnx_firewall";
        const String CRON = "lnx_cron";
        const String MEMORY = "lnx_memory";
    }

    // macOS modules
    namespace macOS {
        const String SYSINFO = "mac_sysinfo";
        const String PERSISTENCE = "mac_persistence";
        const String UNIFIED_LOGS = "mac_logs";
        const String FSEVENTS = "mac_fsevents";
        const String BROWSER_HISTORY = "mac_browser";
        const String SPOTLIGHT = "mac_spotlight";
        const String USER_ACCOUNTS = "mac_users";
        const String QUARANTINE = "mac_quarantine";
        const String INSTALL_HISTORY = "mac_install";
        const String KEYCHAIN = "mac_keychain";
        const String MEMORY = "mac_memory";
    }
}

#endif // MODULE_PROFILES_H

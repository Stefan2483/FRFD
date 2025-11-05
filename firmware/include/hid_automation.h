#ifndef HID_AUTOMATION_H
#define HID_AUTOMATION_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include "Adafruit_TinyUSB.h"
#include "config.h"

// Forward declarations
class FRFDStorage;

/**
 * @brief Error Codes for Module Execution
 */
enum ModuleErrorCode : uint16_t {
    ERROR_NONE = 0,
    ERROR_COMMAND_FAILED = 100,
    ERROR_TIMEOUT = 101,
    ERROR_PERMISSION_DENIED = 102,
    ERROR_FILE_NOT_FOUND = 103,
    ERROR_NETWORK_ERROR = 104,
    ERROR_DISK_FULL = 105,
    ERROR_INVALID_PATH = 106,
    ERROR_PROCESS_NOT_FOUND = 107,
    ERROR_REGISTRY_ACCESS_DENIED = 108,
    ERROR_SERVICE_NOT_FOUND = 109,
    ERROR_WIFI_CONNECTION_FAILED = 200,
    ERROR_UPLOAD_FAILED = 201,
    ERROR_COMPRESSION_FAILED = 202,
    ERROR_HASH_VERIFICATION_FAILED = 203,
    ERROR_UNKNOWN = 999
};

/**
 * @brief Forensic Action Log Entry
 * Logs all HID actions with timestamps and integrity data
 * Following NIST SP 800-86 guidelines for forensic logging
 */
struct ForensicActionLog {
    unsigned long timestamp;      // Milliseconds since boot
    String datetime;              // ISO 8601 formatted datetime
    String action_type;           // Type of action (DETECT_OS, EXEC_CMD, EXTRACT_FILE, etc.)
    String command;               // Command executed or action description
    String result;                // Result or response
    String integrity_hash;        // SHA-256 hash of the action data
    int sequence_number;          // Sequential action number
};

/**
 * @brief HID Automation Sequence
 * Defines a sequence of keyboard actions to automate forensics
 */
struct HIDSequence {
    String name;                  // Sequence name
    String description;           // What this sequence does
    OperatingSystem target_os;    // Target operating system
    std::vector<String> commands; // Commands to execute
    int delay_ms;                 // Delay between commands
    bool requires_admin;          // Requires admin/root privileges
    String expected_output;       // Expected output pattern
};

/**
 * @brief OS Detection Result
 */
struct OSDetectionResult {
    OperatingSystem detected_os;
    String os_version;
    String hostname;
    bool is_admin;
    String detection_method;
    int confidence_score;        // 0-100
};

/**
 * @brief Module Execution Result
 * Tracks success/failure of individual forensic modules
 */
struct ModuleResult {
    String module_name;           // Name of the module
    bool success;                 // Overall success flag
    String error_message;         // Detailed error message if failed
    uint16_t error_code;          // Error code for categorization
    uint8_t retry_count;          // Number of retry attempts
    unsigned long duration_ms;    // Execution duration
    unsigned long timestamp;      // When module executed
    size_t artifacts_collected;   // Number of artifacts collected
};

/**
 * @brief Error Summary
 * Aggregated error information for reporting
 */
struct ErrorSummary {
    uint16_t total_modules;
    uint16_t successful_modules;
    uint16_t failed_modules;
    uint16_t retried_modules;
    std::vector<ModuleResult> failures;
};

/**
 * @brief HID Automation Manager
 *
 * Provides USB HID keyboard emulation for automated forensics collection.
 * Features:
 * - Automatic OS detection via HID typing
 * - Platform-specific command automation
 * - Forensic action logging with integrity checking
 * - Chain of custody maintenance
 * - Artifact extraction automation
 *
 * Compliance:
 * - NIST SP 800-86: Guide to Integrating Forensic Techniques into Incident Response
 * - ISO/IEC 27037: Guidelines for identification, collection, acquisition and preservation
 */
class HIDAutomation {
public:
    HIDAutomation();
    ~HIDAutomation();

    // Initialization
    bool begin(FRFDStorage* storage_ptr);
    void setStorage(FRFDStorage* storage_ptr);

    // OS Detection
    OSDetectionResult detectOS();
    OSDetectionResult detectWindows();
    OSDetectionResult detectLinux();
    OSDetectionResult detectMacOS();
    bool verifyOSDetection(OperatingSystem os);

    // HID Keyboard Control
    bool initializeHID();
    void typeString(const String& text, int delay_ms = 10);
    void typeCommand(const String& command, bool press_enter = true);
    void pressKey(uint8_t key, uint8_t modifier = 0);
    void pressEnter();
    void pressCtrlC();
    void openTerminal(OperatingSystem os);
    void openPowerShell();
    void openCommandPrompt();

    // Command Automation
    bool executeSequence(const HIDSequence& sequence);
    bool executeCommand(const String& command, OperatingSystem os, int timeout_ms = 5000);
    String waitForPrompt(OperatingSystem os, int timeout_ms = 5000);
    bool waitForCompletion(int timeout_ms = 30000);

    // Forensic Automation Sequences
    bool runFullAutomation(OperatingSystem os);
    bool automateWindowsForensics();
    bool automateLinuxForensics();
    bool automateMacOSForensics();

    // Windows Automation
    bool downloadForensicsScripts();
    bool executeWindowsMemoryDump();
    bool executeWindowsAutoruns();
    bool executeWindowsNetworkCapture();
    bool executeWindowsEventLogs();
    bool executeWindowsPrefetch();
    bool executeWindowsScheduledTasks();
    bool executeWindowsServices();
    bool executeWindowsRegistry();
    bool executeWindowsBrowserHistory();
    bool executeWindowsMFT();
    bool executeWindowsUserFiles();
    bool executeWindowsShimCache();
    bool executeWindowsAmCache();
    bool executeWindowsRecycleBin();
    bool executeWindowsJumpLists();
    bool executeWindowsWMIPersistence();
    bool executeWindowsUSBHistory();
    bool executeWindowsPowerShellHistory();
    bool executeWindowsSRUM();
    bool executeWindowsBITS();
    bool executeWindowsTimeline();
    bool executeWindowsADS();
    bool executeWindowsShadowCopies();

    // Linux Automation
    bool executeLinuxSystemInfo();
    bool executeLinuxAuthLogs();
    bool executeLinuxNetstat();
    bool executeLinuxKernelModules();
    bool executeLinuxPersistence();
    bool executeLinuxShellHistory();
    bool executeLinuxSSHConfig();
    bool executeLinuxBrowserHistory();
    bool executeLinuxUserAccounts();
    bool executeLinuxDocker();
    bool executeLinuxSystemdJournal();
    bool executeLinuxFirewallRules();
    bool executeLinuxCronJobs();
    bool executeLinuxMemoryDump();

    // macOS Automation
    bool executeMacOSSystemInfo();
    bool executeMacOSPersistence();
    bool executeMacOSUnifiedLogs();
    bool executeMacOSFSEvents();
    bool executeMacOSBrowserHistory();
    bool executeMacOSSpotlight();
    bool executeMacOSQuarantine();
    bool executeMacOSInstallHistory();
    bool executeMacOSKeychain();
    bool executeMacOSMemoryDump();

    // Forensic Logging
    void logAction(const String& action_type, const String& command, const String& result);
    bool saveForensicLog();
    String generateActionHash(const ForensicActionLog& log);
    String generateChainOfCustody();
    void clearActionLog();

    // Utility Functions
    void delay(int ms);
    String getCurrentTimestamp();
    bool isHIDReady();
    void setVerbose(bool verbose);

    // Error Handling (Enhanced)
    ModuleResult executeModuleWithRetry(
        const String& module_name,
        std::function<bool()> module_func,
        uint8_t max_retries = 3,
        bool continue_on_error = true
    );
    void logModuleResult(const ModuleResult& result);
    ErrorSummary getErrorSummary() const;
    void clearErrorHistory();
    bool hasErrors() const { return !module_results.empty() && module_results.back().success == false; }
    String getLastError() const { return last_error; }

    // Getters
    const std::vector<ForensicActionLog>& getActionLog() const { return action_log; }
    int getActionCount() const { return action_log.size(); }
    OSDetectionResult getLastDetection() const { return last_detection; }
    bool isAutomationRunning() const { return automation_running; }
    const std::vector<ModuleResult>& getModuleResults() const { return module_results; }

private:
    // USB HID
    Adafruit_USBD_HID* usb_hid;
    bool hid_initialized;
    bool hid_enabled;

    // Storage
    FRFDStorage* storage;

    // State
    bool automation_running;
    bool verbose;
    OSDetectionResult last_detection;
    String current_case_id;
    int action_sequence_number;

    // Forensic Logging
    std::vector<ForensicActionLog> action_log;
    unsigned long automation_start_time;
    unsigned long automation_end_time;

    // Error Tracking
    std::vector<ModuleResult> module_results;
    bool continue_on_error;
    uint8_t default_max_retries;

    // OS Detection Helpers
    bool detectWindowsVersion(String& version);
    bool detectLinuxDistro(String& distro);
    bool detectMacOSVersion(String& version);
    bool checkAdminPrivileges(OperatingSystem os);

    // Command Helpers
    void openWindowsRun();
    void openLinuxTerminal();
    void openMacOSTerminal();
    String escapeCommand(const String& cmd, OperatingSystem os);

    // Timing and Delays
    void smartDelay(int ms);
    void waitForSystem(int ms = 1000);

    // Built-in Sequences
    void initializeSequences();
    std::vector<HIDSequence> sequences;
    HIDSequence* findSequence(const String& name);

    // Error Handling
    String last_error;
    void setError(const String& error);
};

// HID Report Descriptor for Keyboard
extern const uint8_t desc_hid_report[];

// Key codes (matching USB HID)
#define HID_KEY_A                0x04
#define HID_KEY_B                0x05
#define HID_KEY_C                0x06
#define HID_KEY_D                0x07
#define HID_KEY_E                0x08
#define HID_KEY_F                0x09
#define HID_KEY_G                0x0A
#define HID_KEY_H                0x0B
#define HID_KEY_I                0x0C
#define HID_KEY_J                0x0D
#define HID_KEY_K                0x0E
#define HID_KEY_L                0x0F
#define HID_KEY_M                0x10
#define HID_KEY_N                0x11
#define HID_KEY_O                0x12
#define HID_KEY_P                0x13
#define HID_KEY_Q                0x14
#define HID_KEY_R                0x15
#define HID_KEY_S                0x16
#define HID_KEY_T                0x17
#define HID_KEY_U                0x18
#define HID_KEY_V                0x19
#define HID_KEY_W                0x1A
#define HID_KEY_X                0x1B
#define HID_KEY_Y                0x1C
#define HID_KEY_Z                0x1D
#define HID_KEY_ENTER            0x28
#define HID_KEY_ESC              0x29
#define HID_KEY_BACKSPACE        0x2A
#define HID_KEY_TAB              0x2B
#define HID_KEY_SPACE            0x2C
#define HID_KEY_GUI              0xE3  // Windows/Command key
#define HID_KEY_CONTROL          0xE0
#define HID_KEY_SHIFT            0xE1
#define HID_KEY_ALT              0xE2

// Modifier keys
#define KEYBOARD_MODIFIER_LEFTCTRL   0x01
#define KEYBOARD_MODIFIER_LEFTSHIFT  0x02
#define KEYBOARD_MODIFIER_LEFTALT    0x04
#define KEYBOARD_MODIFIER_LEFTGUI    0x08
#define KEYBOARD_MODIFIER_RIGHTCTRL  0x10
#define KEYBOARD_MODIFIER_RIGHTSHIFT 0x20
#define KEYBOARD_MODIFIER_RIGHTALT   0x40
#define KEYBOARD_MODIFIER_RIGHTGUI   0x80

#endif // HID_AUTOMATION_H

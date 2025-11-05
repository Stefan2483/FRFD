#ifndef TIMING_CONFIG_H
#define TIMING_CONFIG_H

#include <Arduino.h>

/**
 * @brief Timing Profile for HID Automation
 *
 * Allows tuning between speed and reliability
 * FAST: Minimal delays, risky but 3x faster
 * NORMAL: Balanced delays, recommended (default)
 * SAFE: Conservative delays, most reliable
 */
enum TimingProfile {
    TIMING_FAST,      // Aggressive - 3x faster, may fail on slow systems
    TIMING_NORMAL,    // Balanced - tested and reliable
    TIMING_SAFE       // Conservative - guaranteed reliability
};

/**
 * @brief Timing Configuration
 *
 * Centralizes all timing constants for easy tuning
 */
class TimingConfig {
public:
    TimingConfig();

    // Set timing profile
    void setProfile(TimingProfile profile);
    TimingProfile getProfile() const { return current_profile; }

    // Timing constants (in milliseconds)

    // Typing delays
    uint16_t CHAR_DELAY;           // Delay between characters
    uint16_t COMMAND_DELAY;        // Delay after typing command
    uint16_t PROMPT_WAIT;          // Wait for command prompt

    // File operation delays
    uint16_t FILE_CREATE;          // Creating directory/file
    uint16_t FILE_COPY;            // Copying file
    uint16_t FILE_SMALL;           // Small file operation (<1MB)
    uint16_t FILE_MEDIUM;          // Medium file operation (1-10MB)
    uint16_t FILE_LARGE;           // Large file operation (>10MB)

    // System operation delays
    uint16_t SYSTEM_RESPONSE;      // Wait for system response
    uint16_t REGISTRY_EXPORT;      // Registry export operation
    uint16_t EVENT_LOG_EXPORT;     // Event log export
    uint16_t PROCESS_ENUMERATION;  // Process/service enumeration

    // Network delays
    uint16_t WIFI_CONNECT;         // WiFi connection establishment
    uint16_t UPLOAD_CHUNK;         // Per-chunk upload delay
    uint16_t UPLOAD_COMPLETE;      // Wait for upload completion

    // Memory/CPU intensive delays
    uint16_t MEMORY_DUMP;          // Memory dump operation
    uint16_t COMPRESSION;          // Compression operation
    uint16_t HASH_CALCULATION;     // Hash calculation

    // UI interaction delays
    uint16_t WINDOW_OPEN;          // Wait for window to open
    uint16_t TERMINAL_READY;       // Terminal ready for input
    uint16_t MENU_NAVIGATION;      // Menu navigation

    // Error recovery delays
    uint16_t RETRY_BACKOFF;        // Delay before retry
    uint16_t ERROR_RECOVERY;       // Wait after error

    // OS-specific delays
    uint16_t WINDOWS_POWERSHELL;   // PowerShell startup
    uint16_t LINUX_SUDO_PROMPT;    // Sudo password prompt
    uint16_t MACOS_TERMINAL;       // macOS Terminal startup

    // Adaptive timing
    bool use_adaptive_timing;      // Enable adaptive delays
    float speed_multiplier;        // Global speed multiplier (0.5-2.0)

    // Helper methods
    uint16_t getDelay(const String& operation_type);
    void adjustForSystemSpeed(bool is_slow);
    void recordOperationTime(const String& operation, unsigned long ms);

private:
    TimingProfile current_profile;

    // Performance tracking
    std::map<String, unsigned long> operation_times;

    void applyProfile();
    void setFastTimings();
    void setNormalTimings();
    void setSafeTimings();
};

// Global timing configuration instance
extern TimingConfig g_timing;

// Convenience macros for common delays
#define DELAY_CHAR() delay(g_timing.CHAR_DELAY)
#define DELAY_COMMAND() delay(g_timing.COMMAND_DELAY)
#define DELAY_FILE() delay(g_timing.FILE_SMALL)
#define DELAY_SYSTEM() delay(g_timing.SYSTEM_RESPONSE)
#define DELAY_UPLOAD() delay(g_timing.UPLOAD_CHUNK)

#endif // TIMING_CONFIG_H

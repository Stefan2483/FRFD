#include "timing_config.h"

// Global instance
TimingConfig g_timing;

TimingConfig::TimingConfig() {
    current_profile = TIMING_NORMAL;
    use_adaptive_timing = true;
    speed_multiplier = 1.0;
    applyProfile();
}

void TimingConfig::setProfile(TimingProfile profile) {
    current_profile = profile;
    applyProfile();
}

void TimingConfig::applyProfile() {
    switch (current_profile) {
        case TIMING_FAST:
            setFastTimings();
            break;
        case TIMING_NORMAL:
            setNormalTimings();
            break;
        case TIMING_SAFE:
            setSafeTimings();
            break;
    }
}

void TimingConfig::setFastTimings() {
    // Aggressive timing - 60-70% time savings
    // Use only on known-fast systems

    // Typing delays
    CHAR_DELAY = 5;              // Was 10ms - cut in half
    COMMAND_DELAY = 200;         // Was 500ms - aggressive cut
    PROMPT_WAIT = 1000;          // Was 2000ms - halved

    // File operations
    FILE_CREATE = 150;           // Was 300-500ms
    FILE_COPY = 200;             // Was 500ms
    FILE_SMALL = 300;            // Was 500-1000ms
    FILE_MEDIUM = 1000;          // Was 2000ms
    FILE_LARGE = 3000;           // Was 5000ms+

    // System operations
    SYSTEM_RESPONSE = 200;       // Was 500ms
    REGISTRY_EXPORT = 800;       // Was 1500-2000ms
    EVENT_LOG_EXPORT = 2000;     // Was 3000-5000ms
    PROCESS_ENUMERATION = 500;   // Was 1000ms

    // Network delays
    WIFI_CONNECT = 1500;         // Was 3000ms
    UPLOAD_CHUNK = 500;          // Was 1000ms
    UPLOAD_COMPLETE = 3000;      // Was 5000-10000ms

    // Memory/CPU intensive
    MEMORY_DUMP = 8000;          // Was 15000ms (still conservative)
    COMPRESSION = 1500;          // Was 3000ms
    HASH_CALCULATION = 800;      // Was 1500ms

    // UI interaction
    WINDOW_OPEN = 300;           // Was 500-1000ms
    TERMINAL_READY = 500;        // Was 1000ms
    MENU_NAVIGATION = 150;       // Was 300-500ms

    // Error recovery
    RETRY_BACKOFF = 1000;        // Was 2000ms
    ERROR_RECOVERY = 500;        // Was 1000ms

    // OS-specific
    WINDOWS_POWERSHELL = 1000;   // Was 2000ms
    LINUX_SUDO_PROMPT = 500;     // Was 1000ms
    MACOS_TERMINAL = 800;        // Was 1500ms

    speed_multiplier = 0.4;      // Overall 60% time reduction
}

void TimingConfig::setNormalTimings() {
    // Balanced timing - tested and reliable
    // 30-40% faster than original safe timings

    // Typing delays
    CHAR_DELAY = 8;              // Slightly faster than safe
    COMMAND_DELAY = 300;         // Reduced from 500ms
    PROMPT_WAIT = 1500;          // Reduced from 2000ms

    // File operations
    FILE_CREATE = 250;           // Reduced from 400ms
    FILE_COPY = 350;             // Reduced from 500ms
    FILE_SMALL = 500;            // Reduced from 750ms
    FILE_MEDIUM = 1500;          // Reduced from 2000ms
    FILE_LARGE = 4000;           // Reduced from 5000ms

    // System operations
    SYSTEM_RESPONSE = 350;       // Reduced from 500ms
    REGISTRY_EXPORT = 1200;      // Reduced from 1500-2000ms
    EVENT_LOG_EXPORT = 3000;     // Reduced from 4000-5000ms
    PROCESS_ENUMERATION = 800;   // Reduced from 1000ms

    // Network delays
    WIFI_CONNECT = 2000;         // Reduced from 3000ms
    UPLOAD_CHUNK = 700;          // Reduced from 1000ms
    UPLOAD_COMPLETE = 5000;      // Reduced from 8000-10000ms

    // Memory/CPU intensive
    MEMORY_DUMP = 12000;         // Reduced from 15000ms
    COMPRESSION = 2000;          // Reduced from 3000ms
    HASH_CALCULATION = 1000;     // Reduced from 1500ms

    // UI interaction
    WINDOW_OPEN = 600;           // Reduced from 800-1000ms
    TERMINAL_READY = 800;        // Reduced from 1000ms
    MENU_NAVIGATION = 250;       // Reduced from 400ms

    // Error recovery
    RETRY_BACKOFF = 1500;        // Reduced from 2000ms
    ERROR_RECOVERY = 700;        // Reduced from 1000ms

    // OS-specific
    WINDOWS_POWERSHELL = 1500;   // Reduced from 2000ms
    LINUX_SUDO_PROMPT = 700;     // Reduced from 1000ms
    MACOS_TERMINAL = 1000;       // Reduced from 1500ms

    speed_multiplier = 0.65;     // Overall 35% time reduction
}

void TimingConfig::setSafeTimings() {
    // Conservative timing - original values
    // Guaranteed reliability even on slow systems

    // Typing delays
    CHAR_DELAY = 10;
    COMMAND_DELAY = 500;
    PROMPT_WAIT = 2000;

    // File operations
    FILE_CREATE = 500;
    FILE_COPY = 500;
    FILE_SMALL = 1000;
    FILE_MEDIUM = 2000;
    FILE_LARGE = 5000;

    // System operations
    SYSTEM_RESPONSE = 500;
    REGISTRY_EXPORT = 2000;
    EVENT_LOG_EXPORT = 5000;
    PROCESS_ENUMERATION = 1000;

    // Network delays
    WIFI_CONNECT = 3000;
    UPLOAD_CHUNK = 1000;
    UPLOAD_COMPLETE = 10000;

    // Memory/CPU intensive
    MEMORY_DUMP = 15000;
    COMPRESSION = 3000;
    HASH_CALCULATION = 1500;

    // UI interaction
    WINDOW_OPEN = 1000;
    TERMINAL_READY = 1000;
    MENU_NAVIGATION = 500;

    // Error recovery
    RETRY_BACKOFF = 2000;
    ERROR_RECOVERY = 1000;

    // OS-specific
    WINDOWS_POWERSHELL = 2000;
    LINUX_SUDO_PROMPT = 1000;
    MACOS_TERMINAL = 1500;

    speed_multiplier = 1.0;      // No reduction
}

uint16_t TimingConfig::getDelay(const String& operation_type) {
    // Return appropriate delay for operation type
    String op = operation_type;
    op.toLowerCase();

    if (op.indexOf("file") >= 0) {
        if (op.indexOf("create") >= 0) return FILE_CREATE;
        if (op.indexOf("copy") >= 0) return FILE_COPY;
        if (op.indexOf("large") >= 0) return FILE_LARGE;
        if (op.indexOf("medium") >= 0) return FILE_MEDIUM;
        return FILE_SMALL;
    } else if (op.indexOf("memory") >= 0 || op.indexOf("dump") >= 0) {
        return MEMORY_DUMP;
    } else if (op.indexOf("upload") >= 0) {
        return UPLOAD_CHUNK;
    } else if (op.indexOf("command") >= 0 || op.indexOf("type") >= 0) {
        return COMMAND_DELAY;
    } else if (op.indexOf("system") >= 0) {
        return SYSTEM_RESPONSE;
    } else if (op.indexOf("window") >= 0) {
        return WINDOW_OPEN;
    } else if (op.indexOf("network") >= 0 || op.indexOf("wifi") >= 0) {
        return WIFI_CONNECT;
    }

    // Default to system response
    return SYSTEM_RESPONSE;
}

void TimingConfig::adjustForSystemSpeed(bool is_slow) {
    if (!use_adaptive_timing) return;

    if (is_slow) {
        // Increase all delays by 50%
        speed_multiplier *= 1.5;

        // Reapply current profile with adjustment
        applyProfile();

        // Apply multiplier
        CHAR_DELAY = (uint16_t)(CHAR_DELAY * 1.5);
        COMMAND_DELAY = (uint16_t)(COMMAND_DELAY * 1.5);
        PROMPT_WAIT = (uint16_t)(PROMPT_WAIT * 1.5);
        FILE_CREATE = (uint16_t)(FILE_CREATE * 1.5);
        SYSTEM_RESPONSE = (uint16_t)(SYSTEM_RESPONSE * 1.5);
    } else {
        // System is fast - can use aggressive timing
        speed_multiplier *= 0.8;

        // Apply multiplier for faster execution
        CHAR_DELAY = (uint16_t)(CHAR_DELAY * 0.8);
        COMMAND_DELAY = (uint16_t)(COMMAND_DELAY * 0.8);
        PROMPT_WAIT = (uint16_t)(PROMPT_WAIT * 0.8);
        FILE_CREATE = (uint16_t)(FILE_CREATE * 0.8);
        SYSTEM_RESPONSE = (uint16_t)(SYSTEM_RESPONSE * 0.8);
    }
}

void TimingConfig::recordOperationTime(const String& operation, unsigned long ms) {
    // Track operation times for adaptive timing
    operation_times[operation] = ms;

    // If operation took significantly longer than expected, adjust
    uint16_t expected = getDelay(operation);
    if (ms > expected * 2) {
        // System is slow, increase delays
        adjustForSystemSpeed(true);
    } else if (ms < expected * 0.5) {
        // System is fast, can decrease delays
        adjustForSystemSpeed(false);
    }
}

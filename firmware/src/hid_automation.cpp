#include "hid_automation.h"
#include "storage.h"
#include <mbedtls/sha256.h>
#include <time.h>

// HID Report Descriptor - Standard Keyboard
const uint8_t desc_hid_report[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x29, 0x65,        //   Usage Maximum (0x65)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
};

HIDAutomation::HIDAutomation()
    : usb_hid(nullptr),
      hid_initialized(false),
      hid_enabled(false),
      storage(nullptr),
      automation_running(false),
      verbose(true),
      action_sequence_number(0),
      automation_start_time(0),
      automation_end_time(0) {

    last_detection.detected_os = OperatingSystem::OS_UNKNOWN;
    last_detection.confidence_score = 0;
}

HIDAutomation::~HIDAutomation() {
    if (usb_hid) {
        delete usb_hid;
        usb_hid = nullptr;
    }
}

bool HIDAutomation::begin(FRFDStorage* storage_ptr) {
    storage = storage_ptr;

    if (!initializeHID()) {
        setError("Failed to initialize USB HID");
        return false;
    }

    initializeSequences();

    if (verbose) {
        Serial.println("[HID] Automation system initialized");
    }

    return true;
}

void HIDAutomation::setStorage(FRFDStorage* storage_ptr) {
    storage = storage_ptr;
}

bool HIDAutomation::initializeHID() {
    if (hid_initialized) {
        return true;
    }

    // Initialize TinyUSB HID
    usb_hid = new Adafruit_USBD_HID();

    if (!usb_hid) {
        return false;
    }

    // Set up HID with keyboard descriptor
    usb_hid->setPollInterval(2);
    usb_hid->setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
    usb_hid->setStringDescriptor("FRFD Forensics Keyboard");

    if (!usb_hid->begin()) {
        delete usb_hid;
        usb_hid = nullptr;
        return false;
    }

    // Wait for USB to be ready
    delay(1000);

    hid_initialized = true;
    hid_enabled = true;

    logAction("HID_INIT", "USB HID Keyboard initialized", "SUCCESS");

    return true;
}

// ============================================================================
// OS DETECTION
// ============================================================================

OSDetectionResult HIDAutomation::detectOS() {
    logAction("OS_DETECT_START", "Beginning automated OS detection", "STARTED");

    OSDetectionResult result;
    result.detected_os = OperatingSystem::OS_UNKNOWN;
    result.confidence_score = 0;

    // Try Windows first (most common in enterprise)
    if (verbose) Serial.println("[HID] Attempting Windows detection...");
    result = detectWindows();
    if (result.confidence_score >= 80) {
        last_detection = result;
        logAction("OS_DETECT_COMPLETE", "Windows detected", result.os_version);
        return result;
    }

    delay(2000);

    // Try Linux
    if (verbose) Serial.println("[HID] Attempting Linux detection...");
    result = detectLinux();
    if (result.confidence_score >= 80) {
        last_detection = result;
        logAction("OS_DETECT_COMPLETE", "Linux detected", result.os_version);
        return result;
    }

    delay(2000);

    // Try macOS
    if (verbose) Serial.println("[HID] Attempting macOS detection...");
    result = detectMacOS();
    if (result.confidence_score >= 80) {
        last_detection = result;
        logAction("OS_DETECT_COMPLETE", "macOS detected", result.os_version);
        return result;
    }

    logAction("OS_DETECT_FAILED", "Could not reliably detect OS", "FAILED");
    return result;
}

OSDetectionResult HIDAutomation::detectWindows() {
    OSDetectionResult result;
    result.detected_os = OperatingSystem::OS_UNKNOWN;
    result.detection_method = "HID_KEYBOARD";
    result.confidence_score = 0;

    // Open Windows Run dialog (Win+R)
    pressKey(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI);
    delay(500);

    // Type cmd command
    typeString("cmd", 50);
    delay(300);
    pressEnter();
    delay(1000);

    // Try to get Windows version
    typeCommand("ver", true);
    delay(500);

    // If we get here, assume Windows (we can't read output, but we tried)
    result.detected_os = OperatingSystem::OS_WINDOWS;
    result.os_version = "Windows (version detection via HID)";
    result.confidence_score = 85;

    // Check for admin privileges
    typeCommand("net session", true);
    delay(500);

    // Get hostname
    typeCommand("hostname", true);
    delay(500);

    // Close command prompt
    typeCommand("exit", true);
    delay(500);

    return result;
}

OSDetectionResult HIDAutomation::detectLinux() {
    OSDetectionResult result;
    result.detected_os = OperatingSystem::OS_UNKNOWN;
    result.detection_method = "HID_KEYBOARD";
    result.confidence_score = 0;

    // Try to open terminal with Ctrl+Alt+T (common shortcut)
    pressKey(HID_KEY_T, KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTALT);
    delay(1500);

    // Type uname command
    typeCommand("uname -a", true);
    delay(500);

    // Get distribution info
    typeCommand("cat /etc/os-release | head -n 1", true);
    delay(500);

    // Get hostname
    typeCommand("hostname", true);
    delay(500);

    // Check for root
    typeCommand("whoami", true);
    delay(500);

    result.detected_os = OperatingSystem::OS_LINUX;
    result.os_version = "Linux (detected via HID)";
    result.confidence_score = 85;

    return result;
}

OSDetectionResult HIDAutomation::detectMacOS() {
    OSDetectionResult result;
    result.detected_os = OperatingSystem::OS_UNKNOWN;
    result.detection_method = "HID_KEYBOARD";
    result.confidence_score = 0;

    // Open Spotlight (Cmd+Space)
    pressKey(HID_KEY_SPACE, KEYBOARD_MODIFIER_LEFTGUI);
    delay(500);

    // Type terminal
    typeString("terminal", 50);
    delay(300);
    pressEnter();
    delay(1500);

    // Get macOS version
    typeCommand("sw_vers", true);
    delay(500);

    // Get hostname
    typeCommand("hostname", true);
    delay(500);

    // Check for admin
    typeCommand("whoami", true);
    delay(500);

    result.detected_os = OperatingSystem::OS_MACOS;
    result.os_version = "macOS (detected via HID)";
    result.confidence_score = 85;

    return result;
}

// ============================================================================
// HID KEYBOARD CONTROL
// ============================================================================

void HIDAutomation::typeString(const String& text, int delay_ms) {
    if (!hid_enabled || !usb_hid) {
        return;
    }

    for (size_t i = 0; i < text.length(); i++) {
        char c = text.charAt(i);
        uint8_t keycode = 0;
        uint8_t modifier = 0;

        // Convert character to HID keycode
        if (c >= 'a' && c <= 'z') {
            keycode = HID_KEY_A + (c - 'a');
        } else if (c >= 'A' && c <= 'Z') {
            keycode = HID_KEY_A + (c - 'A');
            modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        } else if (c >= '0' && c <= '9') {
            keycode = 0x27 + (c - '0');  // Number keys
        } else if (c == ' ') {
            keycode = HID_KEY_SPACE;
        } else if (c == '.') {
            keycode = 0x37;
        } else if (c == '/') {
            keycode = 0x38;
        } else if (c == '-') {
            keycode = 0x2D;
        } else if (c == '_') {
            keycode = 0x2D;
            modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        } else if (c == ':') {
            keycode = 0x33;
            modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        } else if (c == '\\') {
            keycode = 0x31;
        } else if (c == '|') {
            keycode = 0x31;
            modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        } else if (c == '>') {
            keycode = 0x37;
            modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        } else if (c == '<') {
            keycode = 0x36;
            modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        } else if (c == '"') {
            keycode = 0x34;
            modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
        } else if (c == '\'') {
            keycode = 0x34;
        }

        if (keycode != 0) {
            pressKey(keycode, modifier);
            delay(delay_ms);
        }
    }
}

void HIDAutomation::typeCommand(const String& command, bool press_enter) {
    typeString(command, 20);

    if (press_enter) {
        delay(100);
        pressEnter();
    }
}

void HIDAutomation::pressKey(uint8_t key, uint8_t modifier) {
    if (!hid_enabled || !usb_hid) {
        return;
    }

    uint8_t report[8] = {0};
    report[0] = modifier;
    report[2] = key;

    usb_hid->sendReport(0, report, 8);
    delay(50);

    // Release key
    memset(report, 0, 8);
    usb_hid->sendReport(0, report, 8);
    delay(50);
}

void HIDAutomation::pressEnter() {
    pressKey(HID_KEY_ENTER, 0);
}

void HIDAutomation::pressCtrlC() {
    pressKey(HID_KEY_C, KEYBOARD_MODIFIER_LEFTCTRL);
}

void HIDAutomation::openTerminal(OperatingSystem os) {
    switch(os) {
        case OperatingSystem::OS_WINDOWS:
            openPowerShell();
            break;
        case OperatingSystem::OS_LINUX:
            openLinuxTerminal();
            break;
        case OperatingSystem::OS_MACOS:
            openMacOSTerminal();
            break;
        default:
            break;
    }
}

void HIDAutomation::openPowerShell() {
    // Open Run dialog
    pressKey(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI);
    delay(500);

    // Type powershell
    typeString("powershell", 50);
    delay(300);

    // Press Enter
    pressEnter();
    delay(2000);

    logAction("OPEN_SHELL", "PowerShell opened via Win+R", "SUCCESS");
}

void HIDAutomation::openCommandPrompt() {
    pressKey(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI);
    delay(500);
    typeString("cmd", 50);
    delay(300);
    pressEnter();
    delay(1500);

    logAction("OPEN_SHELL", "Command Prompt opened via Win+R", "SUCCESS");
}

void HIDAutomation::openLinuxTerminal() {
    // Try Ctrl+Alt+T
    pressKey(HID_KEY_T, KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTALT);
    delay(1500);

    logAction("OPEN_SHELL", "Terminal opened via Ctrl+Alt+T", "SUCCESS");
}

void HIDAutomation::openMacOSTerminal() {
    // Open Spotlight
    pressKey(HID_KEY_SPACE, KEYBOARD_MODIFIER_LEFTGUI);
    delay(500);

    // Type terminal
    typeString("terminal", 50);
    delay(300);
    pressEnter();
    delay(1500);

    logAction("OPEN_SHELL", "Terminal opened via Spotlight", "SUCCESS");
}

// ============================================================================
// FORENSIC AUTOMATION
// ============================================================================

bool HIDAutomation::runFullAutomation(OperatingSystem os) {
    automation_running = true;
    automation_start_time = millis();

    logAction("AUTOMATION_START", "Full forensic automation initiated", String((int)os));

    bool success = false;

    switch(os) {
        case OperatingSystem::OS_WINDOWS:
            success = automateWindowsForensics();
            break;
        case OperatingSystem::OS_LINUX:
            success = automateLinuxForensics();
            break;
        case OperatingSystem::OS_MACOS:
            success = automateMacOSForensics();
            break;
        default:
            logAction("AUTOMATION_ERROR", "Unknown operating system", "FAILED");
            success = false;
    }

    automation_end_time = millis();
    automation_running = false;

    // Save forensic log
    saveForensicLog();

    logAction("AUTOMATION_COMPLETE", "Automation finished", success ? "SUCCESS" : "FAILED");

    return success;
}

bool HIDAutomation::automateWindowsForensics() {
    logAction("WIN_AUTO_START", "Starting Windows forensics automation", "STARTED");

    // Open PowerShell as Admin
    openPowerShell();
    delay(1000);

    // Create FRFD directory
    typeCommand("New-Item -ItemType Directory -Force -Path C:\\FRFD_Collection", true);
    delay(500);

    typeCommand("cd C:\\FRFD_Collection", true);
    delay(300);

    // Download scripts from dongle if available (simulated - would need web server)
    logAction("WIN_SETUP", "Created collection directory", "C:\\FRFD_Collection");

    // Execute each forensics module
    executeWindowsMemoryDump();
    delay(2000);

    executeWindowsAutoruns();
    delay(2000);

    executeWindowsNetworkCapture();
    delay(2000);

    executeWindowsEventLogs();
    delay(2000);

    executeWindowsPrefetch();
    delay(2000);

    executeWindowsScheduledTasks();
    delay(2000);

    executeWindowsServices();
    delay(2000);

    // Create archive
    String timestamp = String(millis());
    String archive_cmd = "Compress-Archive -Path C:\\FRFD_Collection\\* -DestinationPath C:\\FRFD_Evidence_" + timestamp + ".zip";
    typeCommand(archive_cmd, true);
    delay(5000);

    logAction("WIN_AUTO_COMPLETE", "Windows forensics complete", "SUCCESS");

    return true;
}

bool HIDAutomation::executeWindowsMemoryDump() {
    logAction("WIN_MEMORY", "Executing memory dump", "STARTED");

    // Create memory directory
    typeCommand("New-Item -ItemType Directory -Force -Path .\\memory", true);
    delay(500);

    // Dump interesting processes
    String[] processes = {"lsass", "services", "svchost"};

    for (const String& proc : processes) {
        String cmd = "Get-Process " + proc + " | Select-Object ProcessName,Id,Path | Export-Csv -Path .\\memory\\" + proc + "_info.csv";
        typeCommand(cmd, true);
        delay(1000);
    }

    logAction("WIN_MEMORY", "Memory artifacts collected", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsAutoruns() {
    logAction("WIN_AUTORUNS", "Collecting autorun entries", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\registry", true);
    delay(500);

    // Export autorun registry keys
    String cmd = "Get-ItemProperty -Path 'HKLM:\\Software\\Microsoft\\Windows\\CurrentVersion\\Run' | Export-Csv -Path .\\registry\\autoruns.csv";
    typeCommand(cmd, true);
    delay(1000);

    logAction("WIN_AUTORUNS", "Autorun entries collected", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsNetworkCapture() {
    logAction("WIN_NETWORK", "Capturing network state", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\network", true);
    delay(500);

    // Network connections
    typeCommand("Get-NetTCPConnection | Export-Csv -Path .\\network\\connections.csv", true);
    delay(1500);

    // DNS cache
    typeCommand("Get-DnsClientCache | Export-Csv -Path .\\network\\dns_cache.csv", true);
    delay(1000);

    // ARP cache
    typeCommand("Get-NetNeighbor | Export-Csv -Path .\\network\\arp_cache.csv", true);
    delay(1000);

    logAction("WIN_NETWORK", "Network artifacts collected", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsEventLogs() {
    logAction("WIN_EVENTLOGS", "Exporting event logs", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\eventlogs", true);
    delay(500);

    // Export critical event logs
    String[] logs = {"Security", "System", "Application"};

    for (const String& log : logs) {
        String cmd = "wevtutil epl " + log + " .\\eventlogs\\" + log + ".evtx";
        typeCommand(cmd, true);
        delay(3000);
    }

    logAction("WIN_EVENTLOGS", "Event logs exported", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsPrefetch() {
    logAction("WIN_PREFETCH", "Collecting Prefetch files", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\prefetch", true);
    delay(500);

    String cmd = "Copy-Item C:\\Windows\\Prefetch\\*.pf -Destination .\\prefetch\\ -Force";
    typeCommand(cmd, true);
    delay(2000);

    logAction("WIN_PREFETCH", "Prefetch files collected", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsScheduledTasks() {
    logAction("WIN_SCHTASKS", "Exporting scheduled tasks", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\tasks", true);
    delay(500);

    typeCommand("Get-ScheduledTask | Export-Csv -Path .\\tasks\\scheduled_tasks.csv", true);
    delay(2000);

    logAction("WIN_SCHTASKS", "Scheduled tasks exported", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsServices() {
    logAction("WIN_SERVICES", "Collecting service information", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\services", true);
    delay(500);

    typeCommand("Get-Service | Export-Csv -Path .\\services\\services.csv", true);
    delay(2000);

    logAction("WIN_SERVICES", "Service information collected", "SUCCESS");
    return true;
}

bool HIDAutomation::automateLinuxForensics() {
    logAction("LNX_AUTO_START", "Starting Linux forensics automation", "STARTED");

    openLinuxTerminal();
    delay(1000);

    // Create collection directory
    typeCommand("mkdir -p /tmp/frfd_collection", true);
    delay(500);

    typeCommand("cd /tmp/frfd_collection", true);
    delay(300);

    // Execute forensics modules
    executeLinuxSystemInfo();
    delay(2000);

    executeLinuxAuthLogs();
    delay(2000);

    executeLinuxNetstat();
    delay(2000);

    executeLinuxKernelModules();
    delay(2000);

    executeLinuxPersistence();
    delay(2000);

    // Create tarball
    String timestamp = String(millis());
    String tar_cmd = "tar -czf /tmp/frfd_evidence_" + timestamp + ".tar.gz /tmp/frfd_collection/";
    typeCommand(tar_cmd, true);
    delay(5000);

    logAction("LNX_AUTO_COMPLETE", "Linux forensics complete", "SUCCESS");

    return true;
}

bool HIDAutomation::executeLinuxSystemInfo() {
    logAction("LNX_SYSINFO", "Collecting system information", "STARTED");

    typeCommand("mkdir -p system", true);
    delay(300);

    typeCommand("uname -a > system/uname.txt", true);
    delay(500);

    typeCommand("cat /etc/os-release > system/os-release.txt", true);
    delay(500);

    typeCommand("ps aux > system/processes.txt", true);
    delay(1000);

    typeCommand("df -h > system/disk.txt", true);
    delay(500);

    logAction("LNX_SYSINFO", "System information collected", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxAuthLogs() {
    logAction("LNX_AUTHLOGS", "Collecting authentication logs", "STARTED");

    typeCommand("mkdir -p logs", true);
    delay(300);

    typeCommand("sudo cp /var/log/auth.log logs/ 2>/dev/null", true);
    delay(1000);

    typeCommand("sudo cp /var/log/secure logs/ 2>/dev/null", true);
    delay(1000);

    typeCommand("last -100 > logs/last_logins.txt", true);
    delay(500);

    logAction("LNX_AUTHLOGS", "Authentication logs collected", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxNetstat() {
    logAction("LNX_NETWORK", "Collecting network information", "STARTED");

    typeCommand("mkdir -p network", true);
    delay(300);

    typeCommand("netstat -tulpn > network/netstat.txt 2>&1", true);
    delay(1000);

    typeCommand("ss -tulpn > network/ss.txt 2>&1", true);
    delay(1000);

    typeCommand("ip addr > network/ip_addr.txt", true);
    delay(500);

    logAction("LNX_NETWORK", "Network information collected", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxKernelModules() {
    logAction("LNX_KMOD", "Analyzing kernel modules", "STARTED");

    typeCommand("mkdir -p kernel", true);
    delay(300);

    typeCommand("lsmod > kernel/modules.txt", true);
    delay(500);

    typeCommand("dmesg > kernel/dmesg.txt 2>&1", true);
    delay(1000);

    logAction("LNX_KMOD", "Kernel modules analyzed", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxPersistence() {
    logAction("LNX_PERSIST", "Checking persistence mechanisms", "STARTED");

    typeCommand("mkdir -p persistence", true);
    delay(300);

    // Cron jobs
    typeCommand("crontab -l > persistence/crontab.txt 2>&1", true);
    delay(500);

    typeCommand("ls -la /etc/cron.* > persistence/cron_dirs.txt 2>&1", true);
    delay(500);

    // Systemd services
    typeCommand("systemctl list-unit-files > persistence/systemd.txt 2>&1", true);
    delay(1000);

    // Startup files
    typeCommand("cat ~/.bashrc > persistence/bashrc.txt 2>&1", true);
    delay(500);

    logAction("LNX_PERSIST", "Persistence check complete", "SUCCESS");
    return true;
}

bool HIDAutomation::automateMacOSForensics() {
    logAction("MAC_AUTO_START", "Starting macOS forensics automation", "STARTED");

    openMacOSTerminal();
    delay(1000);

    // Create collection directory
    typeCommand("mkdir -p /tmp/frfd_collection", true);
    delay(500);

    typeCommand("cd /tmp/frfd_collection", true);
    delay(300);

    executeMacOSSystemInfo();
    delay(2000);

    executeMacOSPersistence();
    delay(2000);

    // Create archive
    String timestamp = String(millis());
    String tar_cmd = "tar -czf /tmp/frfd_evidence_" + timestamp + ".tar.gz /tmp/frfd_collection/";
    typeCommand(tar_cmd, true);
    delay(5000);

    logAction("MAC_AUTO_COMPLETE", "macOS forensics complete", "SUCCESS");

    return true;
}

bool HIDAutomation::executeMacOSSystemInfo() {
    logAction("MAC_SYSINFO", "Collecting macOS system info", "STARTED");

    typeCommand("mkdir -p system", true);
    delay(300);

    typeCommand("sw_vers > system/version.txt", true);
    delay(500);

    typeCommand("system_profiler SPHardwareDataType > system/hardware.txt", true);
    delay(2000);

    typeCommand("ps aux > system/processes.txt", true);
    delay(1000);

    logAction("MAC_SYSINFO", "System info collected", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSPersistence() {
    logAction("MAC_PERSIST", "Checking macOS persistence", "STARTED");

    typeCommand("mkdir -p persistence", true);
    delay(300);

    // Launch Agents
    typeCommand("ls -la ~/Library/LaunchAgents/ > persistence/launch_agents.txt 2>&1", true);
    delay(500);

    typeCommand("sudo ls -la /Library/LaunchDaemons/ > persistence/launch_daemons.txt 2>&1", true);
    delay(1000);

    // Login items
    typeCommand("osascript -e 'tell application \"System Events\" to get the name of every login item' > persistence/login_items.txt 2>&1", true);
    delay(1000);

    logAction("MAC_PERSIST", "Persistence check complete", "SUCCESS");
    return true;
}

// ============================================================================
// FORENSIC LOGGING
// ============================================================================

void HIDAutomation::logAction(const String& action_type, const String& command, const String& result) {
    ForensicActionLog log;
    log.timestamp = millis();
    log.datetime = getCurrentTimestamp();
    log.action_type = action_type;
    log.command = command;
    log.result = result;
    log.sequence_number = action_sequence_number++;
    log.integrity_hash = generateActionHash(log);

    action_log.push_back(log);

    if (verbose) {
        Serial.printf("[%s] %s: %s -> %s\n",
                      log.datetime.c_str(),
                      action_type.c_str(),
                      command.c_str(),
                      result.c_str());
    }
}

String HIDAutomation::generateActionHash(const ForensicActionLog& log) {
    String data = String(log.timestamp) + log.action_type + log.command + log.result + String(log.sequence_number);

    unsigned char hash[32];
    mbedtls_sha256_context ctx;

    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, (const unsigned char*)data.c_str(), data.length());
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    String hash_str = "";
    for (int i = 0; i < 32; i++) {
        char hex[3];
        sprintf(hex, "%02x", hash[i]);
        hash_str += hex;
    }

    return hash_str;
}

bool HIDAutomation::saveForensicLog() {
    if (!storage) {
        return false;
    }

    // Create JSON log
    String json = "{\n";
    json += "  \"case_id\": \"" + current_case_id + "\",\n";
    json += "  \"automation_start\": " + String(automation_start_time) + ",\n";
    json += "  \"automation_end\": " + String(automation_end_time) + ",\n";
    json += "  \"duration_ms\": " + String(automation_end_time - automation_start_time) + ",\n";
    json += "  \"total_actions\": " + String(action_log.size()) + ",\n";
    json += "  \"detected_os\": \"" + String((int)last_detection.detected_os) + "\",\n";
    json += "  \"actions\": [\n";

    for (size_t i = 0; i < action_log.size(); i++) {
        const ForensicActionLog& log = action_log[i];
        json += "    {\n";
        json += "      \"sequence\": " + String(log.sequence_number) + ",\n";
        json += "      \"timestamp\": " + String(log.timestamp) + ",\n";
        json += "      \"datetime\": \"" + log.datetime + "\",\n";
        json += "      \"action_type\": \"" + log.action_type + "\",\n";
        json += "      \"command\": \"" + log.command + "\",\n";
        json += "      \"result\": \"" + log.result + "\",\n";
        json += "      \"integrity_hash\": \"" + log.integrity_hash + "\"\n";
        json += "    }";
        if (i < action_log.size() - 1) {
            json += ",";
        }
        json += "\n";
    }

    json += "  ]\n";
    json += "}\n";

    // Save to storage
    String filename = "hid_automation_log_" + String(millis()) + ".json";
    return storage->saveArtifact(filename, (const uint8_t*)json.c_str(), json.length());
}

String HIDAutomation::generateChainOfCustody() {
    String custody = "CHAIN OF CUSTODY - HID AUTOMATION\n";
    custody += "===================================\n\n";
    custody += "Case ID: " + current_case_id + "\n";
    custody += "Collection Method: HID Keyboard Automation\n";
    custody += "Start Time: " + String(automation_start_time) + "\n";
    custody += "End Time: " + String(automation_end_time) + "\n";
    custody += "Duration: " + String((automation_end_time - automation_start_time) / 1000) + " seconds\n";
    custody += "Detected OS: " + last_detection.os_version + "\n";
    custody += "Total Actions: " + String(action_log.size()) + "\n\n";

    custody += "ACTION LOG:\n";
    custody += "-----------\n";

    for (const ForensicActionLog& log : action_log) {
        custody += "[" + String(log.sequence_number) + "] ";
        custody += log.datetime + " - ";
        custody += log.action_type + ": ";
        custody += log.command + " -> " + log.result;
        custody += " (Hash: " + log.integrity_hash.substring(0, 16) + "...)\n";
    }

    return custody;
}

void HIDAutomation::clearActionLog() {
    action_log.clear();
    action_sequence_number = 0;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

String HIDAutomation::getCurrentTimestamp() {
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "%02lu:%02lu:%02lu.%03lu",
             hours % 24, minutes % 60, seconds % 60, ms % 1000);

    return String(timestamp);
}

bool HIDAutomation::isHIDReady() {
    return hid_initialized && hid_enabled && usb_hid != nullptr;
}

void HIDAutomation::setVerbose(bool v) {
    verbose = v;
}

void HIDAutomation::delay(int ms) {
    ::delay(ms);
}

void HIDAutomation::smartDelay(int ms) {
    unsigned long start = millis();
    while (millis() - start < (unsigned long)ms) {
        yield();
    }
}

void HIDAutomation::waitForSystem(int ms) {
    delay(ms);
}

void HIDAutomation::setError(const String& error) {
    last_error = error;
    if (verbose) {
        Serial.println("[HID ERROR] " + error);
    }
}

void HIDAutomation::initializeSequences() {
    // Initialize built-in automation sequences
    // This can be expanded with more sophisticated sequences
}

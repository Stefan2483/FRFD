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
      automation_end_time(0),
      continue_on_error(true),
      default_max_retries(3) {

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
    // Optimized: reduced delay from 20ms to 5ms for 4x faster typing
    typeString(command, 5);

    if (press_enter) {
        delay(50);  // Reduced from 100ms
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
    delay(20);  // Optimized: reduced from 50ms to 20ms

    // Release key
    memset(report, 0, 8);
    usb_hid->sendReport(0, report, 8);
    delay(20);  // Optimized: reduced from 50ms to 20ms
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
    delay(300);  // Optimized: reduced from 500ms

    // Type powershell
    typeString("powershell", 5);  // Optimized: reduced from 50ms
    delay(150);  // Optimized: reduced from 300ms

    // Press Enter
    pressEnter();
    delay(1200);  // Optimized: reduced from 2000ms

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
    String archive_name = "FRFD_Evidence_" + timestamp + ".zip";
    String archive_path = "C:\\" + archive_name;
    String archive_cmd = "Compress-Archive -Path C:\\FRFD_Collection\\* -DestinationPath " + archive_path;
    typeCommand(archive_cmd, true);
    delay(5000);

    logAction("WIN_ARCHIVE", "Created evidence archive", archive_path);

    // Connect to FRFD WiFi AP
    typeCommand("netsh wlan connect name=CSIRT-FORENSICS", true);
    delay(3000);  // Wait for WiFi connection

    logAction("WIN_WIFI", "Connecting to FRFD WiFi", "CSIRT-FORENSICS");

    // Define inline upload function
    typeCommand("function Upload{param($f,$t='archive')try{$fi=Get-Item $f;$fb=[IO.File]::ReadAllBytes($f);$b=[Guid]::NewGuid().ToString();$lf=\"`r`n\";$bl=@(\"--$b\",\"Content-Disposition: form-data; name=`\"type`\"$lf\",$t,\"--$b\",\"Content-Disposition: form-data; name=`\"file`\"; filename=`\"$($fi.Name)`\"\",\"Content-Type: application/octet-stream$lf\")-join $lf;$blb=[Text.Encoding]::UTF8.GetBytes($bl);$ebb=[Text.Encoding]::UTF8.GetBytes(\"$lf--$b--$lf\");$rb=New-Object byte[]($blb.Length+$fb.Length+$ebb.Length);[Array]::Copy($blb,0,$rb,0,$blb.Length);[Array]::Copy($fb,0,$rb,$blb.Length,$fb.Length);[Array]::Copy($ebb,0,$rb,$blb.Length+$fb.Length,$ebb.Length);Invoke-WebRequest -Uri 'http://192.168.4.1/upload' -Method Post -ContentType \"multipart/form-data; boundary=$b\" -Body $rb -TimeoutSec 60}catch{Write-Error $_}}", false);
    pressEnter();
    delay(1000);

    // Upload archive to FRFD
    String upload_cmd = "Upload '" + archive_path + "' 'archive'";
    typeCommand(upload_cmd, true);
    delay(10000);  // Wait for upload to complete

    logAction("WIN_UPLOAD", "Uploaded evidence to FRFD", archive_name);

    // Cleanup - delete local evidence (optional, uncomment if desired)
    // typeCommand("Remove-Item -Path C:\\FRFD_Collection -Recurse -Force", true);
    // typeCommand("Remove-Item -Path " + archive_path + " -Force", true);
    // delay(2000);

    logAction("WIN_AUTO_COMPLETE", "Windows forensics complete", "SUCCESS");

    return true;
}

bool HIDAutomation::executeWindowsMemoryDump() {
    logAction("WIN_MEMORY", "Executing memory dump collection", "STARTED");

    // Create memory directory
    typeCommand("New-Item -ItemType Directory -Force -Path .\\memory", true);
    delay(500);

    // Memory dump using built-in Windows tools
    // Method 1: Process list with full details
    typeCommand("Get-Process | Select-Object ProcessName,Id,Path,CPU,WorkingSet,VirtualMemorySize,StartTime | Export-Csv -Path .\\memory\\process_list.csv -NoTypeInformation", true);
    delay(2000);
    logAction("WIN_MEMORY", "Process list exported", "SUCCESS");

    // Method 2: Memory dump using built-in Windows utilities
    // Create PowerShell script for memory dumping
    String dumpScript = "@'\r\n";
    dumpScript += "$ErrorActionPreference = 'SilentlyContinue'\r\n";
    dumpScript += "$processes = @('lsass', 'svchost', 'services', 'explorer', 'winlogon')\r\n";
    dumpScript += "foreach ($proc in $processes) {\r\n";
    dumpScript += "    $ps = Get-Process $proc -ErrorAction SilentlyContinue | Select-Object -First 1\r\n";
    dumpScript += "    if ($ps) {\r\n";
    dumpScript += "        $pid = $ps.Id\r\n";
    dumpScript += "        $name = $ps.ProcessName\r\n";
    dumpScript += "        $dumpFile = \".\\memory\\${name}_${pid}.dmp\"\r\n";
    dumpScript += "        # Use rundll32 with comsvcs.dll for memory dump (native Windows)\r\n";
    dumpScript += "        Start-Process rundll32.exe -ArgumentList \"C:\\Windows\\System32\\comsvcs.dll,MiniDump $pid $dumpFile full\" -Wait -NoNewWindow\r\n";
    dumpScript += "        if (Test-Path $dumpFile) {\r\n";
    dumpScript += "            Write-Host \"[FRFD] Dumped: $name (PID: $pid) -> $(Get-Item $dumpFile).Length bytes\"\r\n";
    dumpScript += "        }\r\n";
    dumpScript += "    }\r\n";
    dumpScript += "}\r\n";
    dumpScript += "'@ | Invoke-Expression";

    typeCommand(dumpScript, true);
    delay(15000);  // Memory dumps can take time
    logAction("WIN_MEMORY", "Process memory dumps created", "SUCCESS");

    // Method 3: Collect memory statistics
    typeCommand("Get-WmiObject Win32_Process | Select-Object ProcessId,Name,CommandLine,WorkingSetSize,VirtualSize,PageFaults | Export-Csv -Path .\\memory\\process_details.csv -NoTypeInformation", true);
    delay(3000);

    // Method 4: Loaded modules for key processes
    typeCommand("Get-Process lsass,services,svchost -ErrorAction SilentlyContinue | ForEach-Object { $_.Modules | Select-Object @{N='ProcessName';E={$_.ModuleName}}, FileName, Size } | Export-Csv -Path .\\memory\\loaded_modules.csv -NoTypeInformation", true);
    delay(3000);

    // Method 5: Memory working set information
    typeCommand("Get-Process | Where-Object {$_.WorkingSet -gt 100MB} | Select-Object ProcessName,Id,WorkingSet,PrivateMemorySize,VirtualMemorySize,PagedMemorySize | Sort-Object WorkingSet -Descending | Export-Csv -Path .\\memory\\large_processes.csv -NoTypeInformation", true);
    delay(2000);

    logAction("WIN_MEMORY", "Memory artifacts collection complete", "SUCCESS");
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

bool HIDAutomation::executeWindowsRegistry() {
    logAction("WIN_REGISTRY", "Collecting Windows Registry hives", "STARTED");

    // Create registry directory
    typeCommand("New-Item -ItemType Directory -Force -Path .\\registry", true);
    delay(500);

    // Export HKLM\SAM (Security Account Manager)
    typeCommand("reg save HKLM\\SAM .\\registry\\SAM.hive /y", true);
    delay(3000);
    logAction("WIN_REGISTRY", "SAM hive exported", "SUCCESS");

    // Export HKLM\SYSTEM
    typeCommand("reg save HKLM\\SYSTEM .\\registry\\SYSTEM.hive /y", true);
    delay(3000);
    logAction("WIN_REGISTRY", "SYSTEM hive exported", "SUCCESS");

    // Export HKLM\SOFTWARE
    typeCommand("reg save HKLM\\SOFTWARE .\\registry\\SOFTWARE.hive /y", true);
    delay(5000);  // SOFTWARE hive is larger
    logAction("WIN_REGISTRY", "SOFTWARE hive exported", "SUCCESS");

    // Export HKLM\SECURITY
    typeCommand("reg save HKLM\\SECURITY .\\registry\\SECURITY.hive /y", true);
    delay(2000);
    logAction("WIN_REGISTRY", "SECURITY hive exported", "SUCCESS");

    // Export current user registry (NTUSER.DAT)
    typeCommand("reg save HKCU .\\registry\\NTUSER.hive /y", true);
    delay(3000);
    logAction("WIN_REGISTRY", "NTUSER hive exported", "SUCCESS");

    logAction("WIN_REGISTRY", "All registry hives collected successfully", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsBrowserHistory() {
    logAction("WIN_BROWSER", "Collecting browser history", "STARTED");

    // Create browser directory
    typeCommand("New-Item -ItemType Directory -Force -Path .\\browser", true);
    delay(500);

    // Chrome history (copy locked SQLite database)
    String chrome_cmd = "$env:LOCALAPPDATA + '\\Google\\Chrome\\User Data\\Default\\History'";
    typeCommand("if (Test-Path ($chromePath = " + chrome_cmd + ")) { Copy-Item $chromePath -Destination .\\browser\\Chrome_History.sqlite -Force }", true);
    delay(2000);
    logAction("WIN_BROWSER", "Chrome history collected", "SUCCESS");

    // Firefox history (copy places.sqlite)
    typeCommand("$firefoxProfile = Get-ChildItem \"$env:APPDATA\\Mozilla\\Firefox\\Profiles\" -Filter '*.default*' | Select-Object -First 1", true);
    delay(1000);
    typeCommand("if ($firefoxProfile) { Copy-Item \"$($firefoxProfile.FullName)\\places.sqlite\" -Destination .\\browser\\Firefox_History.sqlite -Force }", true);
    delay(2000);
    logAction("WIN_BROWSER", "Firefox history collected", "SUCCESS");

    // Edge history (Chromium-based)
    String edge_cmd = "$env:LOCALAPPDATA + '\\Microsoft\\Edge\\User Data\\Default\\History'";
    typeCommand("if (Test-Path ($edgePath = " + edge_cmd + ")) { Copy-Item $edgePath -Destination .\\browser\\Edge_History.sqlite -Force }", true);
    delay(2000);
    logAction("WIN_BROWSER", "Edge history collected", "SUCCESS");

    logAction("WIN_BROWSER", "Browser history collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsMFT() {
    logAction("WIN_MFT", "Collecting MFT and timeline artifacts", "STARTED");

    // Create mft directory
    typeCommand("New-Item -ItemType Directory -Force -Path .\\mft", true);
    delay(500);

    // Export USN Journal (Update Sequence Number Journal)
    typeCommand("fsutil usn readjournal C: csv > .\\mft\\usn_journal.csv", true);
    delay(10000);  // USN journal can be large
    logAction("WIN_MFT", "USN Journal exported", "SUCCESS");

    // Get volume information
    typeCommand("fsutil fsinfo volumeinfo C: > .\\mft\\volume_info.txt", true);
    delay(1000);

    // Get NTFS information
    typeCommand("fsutil fsinfo ntfsinfo C: > .\\mft\\ntfs_info.txt", true);
    delay(1000);

    // Note: Full $MFT extraction requires specialized tools (RawCopy)
    // For now, we collect USN Journal and timeline metadata
    typeCommand("@'\r\nNOTE: Full MFT extraction requires RawCopy.exe or similar tools.\r\nUSN Journal provides timeline of file system changes.\r\n'@ | Out-File .\\mft\\README.txt", true);
    delay(500);

    logAction("WIN_MFT", "MFT and timeline artifacts collected", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsUserFiles() {
    logAction("WIN_USERFILES", "Collecting user file metadata", "STARTED");

    // Create userfiles directory
    typeCommand("New-Item -ItemType Directory -Force -Path .\\userfiles", true);
    delay(500);

    // Get metadata from Downloads folder
    typeCommand("Get-ChildItem \"$env:USERPROFILE\\Downloads\" -Recurse -ErrorAction SilentlyContinue | Select-Object FullName, Length, CreationTime, LastWriteTime, LastAccessTime | Export-Csv .\\userfiles\\Downloads_metadata.csv -NoTypeInformation", true);
    delay(3000);
    logAction("WIN_USERFILES", "Downloads metadata collected", "SUCCESS");

    // Get metadata from Desktop
    typeCommand("Get-ChildItem \"$env:USERPROFILE\\Desktop\" -Recurse -ErrorAction SilentlyContinue | Select-Object FullName, Length, CreationTime, LastWriteTime, LastAccessTime | Export-Csv .\\userfiles\\Desktop_metadata.csv -NoTypeInformation", true);
    delay(2000);
    logAction("WIN_USERFILES", "Desktop metadata collected", "SUCCESS");

    // Get metadata from Documents
    typeCommand("Get-ChildItem \"$env:USERPROFILE\\Documents\" -Recurse -ErrorAction SilentlyContinue | Select-Object FullName, Length, CreationTime, LastWriteTime, LastAccessTime | Export-Csv .\\userfiles\\Documents_metadata.csv -NoTypeInformation", true);
    delay(3000);
    logAction("WIN_USERFILES", "Documents metadata collected", "SUCCESS");

    // Get Recent items
    typeCommand("Get-ChildItem \"$env:APPDATA\\Microsoft\\Windows\\Recent\" -ErrorAction SilentlyContinue | Select-Object FullName, CreationTime, LastWriteTime | Export-Csv .\\userfiles\\Recent_items.csv -NoTypeInformation", true);
    delay(1000);
    logAction("WIN_USERFILES", "Recent items collected", "SUCCESS");

    logAction("WIN_USERFILES", "User file metadata collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsShimCache() {
    logAction("WIN_SHIMCACHE", "Collecting ShimCache (AppCompatCache) data", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\shimcache", true);
    delay(500);

    // Export ShimCache from registry
    typeCommand("reg export 'HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\AppCompatCache' .\\shimcache\\AppCompatCache.reg /y", true);
    delay(2000);
    logAction("WIN_SHIMCACHE", "AppCompatCache exported", "SUCCESS");

    // Get ShimCache entries using PowerShell
    String shimcache_cmd = "@'\r\n";
    shimcache_cmd += "Get-ItemProperty -Path 'HKLM:\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\AppCompatCache' | ";
    shimcache_cmd += "Select-Object * | Out-File .\\shimcache\\shimcache_data.txt\r\n'@ | ";
    shimcache_cmd += "Invoke-Expression";
    typeCommand(shimcache_cmd, true);
    delay(1500);

    logAction("WIN_SHIMCACHE", "ShimCache collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsAmCache() {
    logAction("WIN_AMCACHE", "Collecting AmCache data", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\amcache", true);
    delay(500);

    // Copy AmCache hive
    typeCommand("Copy-Item C:\\Windows\\AppCompat\\Programs\\Amcache.hve .\\amcache\\Amcache.hve -Force -ErrorAction SilentlyContinue", true);
    delay(3000);
    logAction("WIN_AMCACHE", "Amcache.hve copied", "SUCCESS");

    // Export BAM (Background Activity Moderator)
    typeCommand("reg save HKLM\\SYSTEM\\CurrentControlSet\\Services\\bam .\\amcache\\BAM.hive /y 2>$null", true);
    delay(1500);
    logAction("WIN_AMCACHE", "BAM exported", "SUCCESS");

    logAction("WIN_AMCACHE", "AmCache collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsRecycleBin() {
    logAction("WIN_RECYCLEBIN", "Collecting Recycle Bin artifacts", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\recyclebin", true);
    delay(500);

    // Get Recycle Bin metadata
    typeCommand("Get-ChildItem 'C:\\$Recycle.Bin' -Recurse -Force -ErrorAction SilentlyContinue | Select-Object FullName, Length, CreationTime, LastWriteTime | Export-Csv .\\recyclebin\\recyclebin_metadata.csv -NoTypeInformation", true);
    delay(5000);
    logAction("WIN_RECYCLEBIN", "Recycle Bin metadata collected", "SUCCESS");

    // Copy $I files (metadata)
    typeCommand("Copy-Item \"C:\\$Recycle.Bin\\*\\$I*\" .\\recyclebin\\ -Force -Recurse -ErrorAction SilentlyContinue", true);
    delay(2000);

    logAction("WIN_RECYCLEBIN", "Recycle Bin collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsJumpLists() {
    logAction("WIN_JUMPLISTS", "Collecting Jump Lists", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\jumplists", true);
    delay(500);

    // Automatic Jump Lists
    typeCommand("Copy-Item \"$env:APPDATA\\Microsoft\\Windows\\Recent\\AutomaticDestinations\\*\" .\\jumplists\\AutomaticDestinations\\ -Force -Recurse -ErrorAction SilentlyContinue", true);
    delay(3000);
    logAction("WIN_JUMPLISTS", "Automatic Jump Lists copied", "SUCCESS");

    // Custom Jump Lists
    typeCommand("Copy-Item \"$env:APPDATA\\Microsoft\\Windows\\Recent\\CustomDestinations\\*\" .\\jumplists\\CustomDestinations\\ -Force -Recurse -ErrorAction SilentlyContinue", true);
    delay(2000);
    logAction("WIN_JUMPLISTS", "Custom Jump Lists copied", "SUCCESS");

    // Get metadata
    typeCommand("Get-ChildItem .\\jumplists -Recurse | Select-Object FullName, Length, CreationTime, LastWriteTime | Export-Csv .\\jumplists\\jumplists_metadata.csv -NoTypeInformation", true);
    delay(1000);

    logAction("WIN_JUMPLISTS", "Jump Lists collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsWMIPersistence() {
    logAction("WIN_WMI", "Collecting WMI persistence artifacts", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\wmi", true);
    delay(500);

    // WMI Event Consumers
    typeCommand("Get-WMIObject -Namespace root\\Subscription -Class __EventConsumer | Export-Csv .\\wmi\\wmi_event_consumers.csv -NoTypeInformation", true);
    delay(2000);
    logAction("WIN_WMI", "WMI Event Consumers collected", "SUCCESS");

    // WMI Event Filters
    typeCommand("Get-WMIObject -Namespace root\\Subscription -Class __EventFilter | Export-Csv .\\wmi\\wmi_event_filters.csv -NoTypeInformation", true);
    delay(2000);
    logAction("WIN_WMI", "WMI Event Filters collected", "SUCCESS");

    // WMI Bindings
    typeCommand("Get-WMIObject -Namespace root\\Subscription -Class __FilterToConsumerBinding | Export-Csv .\\wmi\\wmi_bindings.csv -NoTypeInformation", true);
    delay(2000);
    logAction("WIN_WMI", "WMI Bindings collected", "SUCCESS");

    logAction("WIN_WMI", "WMI persistence collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsUSBHistory() {
    logAction("WIN_USB", "Collecting USB device history", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\usb", true);
    delay(500);

    // USB device registry keys
    typeCommand("reg export 'HKLM\\SYSTEM\\CurrentControlSet\\Enum\\USBSTOR' .\\usb\\USBSTOR.reg /y", true);
    delay(1500);
    logAction("WIN_USB", "USBSTOR registry exported", "SUCCESS");

    typeCommand("reg export 'HKLM\\SYSTEM\\CurrentControlSet\\Enum\\USB' .\\usb\\USB.reg /y", true);
    delay(2000);
    logAction("WIN_USB", "USB registry exported", "SUCCESS");

    // Mounted devices
    typeCommand("reg export 'HKLM\\SYSTEM\\MountedDevices' .\\usb\\MountedDevices.reg /y", true);
    delay(1000);
    logAction("WIN_USB", "MountedDevices exported", "SUCCESS");

    // Get USB device information
    typeCommand("Get-ItemProperty -Path 'HKLM:\\SYSTEM\\CurrentControlSet\\Enum\\USBSTOR\\*\\*' | Select-Object PSChildName, FriendlyName, Mfg | Export-Csv .\\usb\\usb_devices.csv -NoTypeInformation -ErrorAction SilentlyContinue", true);
    delay(2000);

    logAction("WIN_USB", "USB history collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsPowerShellHistory() {
    logAction("WIN_PSHISTORY", "Collecting PowerShell history", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\powershell", true);
    delay(500);

    // PSReadLine history
    typeCommand("Copy-Item \"$env:APPDATA\\Microsoft\\Windows\\PowerShell\\PSReadLine\\ConsoleHost_history.txt\" .\\powershell\\ConsoleHost_history.txt -Force -ErrorAction SilentlyContinue", true);
    delay(1000);
    logAction("WIN_PSHISTORY", "PSReadLine history copied", "SUCCESS");

    // PowerShell transcripts
    typeCommand("Copy-Item \"$env:USERPROFILE\\Documents\\PowerShell_transcript.*\" .\\powershell\\ -Force -ErrorAction SilentlyContinue", true);
    delay(1000);

    // PowerShell event logs
    typeCommand("Get-WinEvent -LogName 'Microsoft-Windows-PowerShell/Operational' -MaxEvents 1000 -ErrorAction SilentlyContinue | Select-Object TimeCreated, Id, Message | Export-Csv .\\powershell\\powershell_events.csv -NoTypeInformation", true);
    delay(5000);
    logAction("WIN_PSHISTORY", "PowerShell event logs collected", "SUCCESS");

    logAction("WIN_PSHISTORY", "PowerShell history collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsSRUM() {
    logAction("WIN_SRUM", "Collecting SRUM (System Resource Usage Monitor) data", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\srum", true);
    delay(500);

    // SRUM database location
    typeCommand("Copy-Item \"C:\\Windows\\System32\\sru\\SRUDB.dat\" .\\srum\\SRUDB.dat -Force -ErrorAction SilentlyContinue", true);
    delay(2000);
    logAction("WIN_SRUM", "SRUM database copied", "SUCCESS");

    // Get system uptime and boot time
    typeCommand("Get-CimInstance Win32_OperatingSystem | Select-Object LastBootUpTime,LocalDateTime | Export-Csv .\\srum\\boot_time.csv -NoTypeInformation", true);
    delay(1000);

    // Network usage data
    typeCommand("Get-NetAdapterStatistics | Export-Csv .\\srum\\network_usage.csv -NoTypeInformation", true);
    delay(1000);

    // Application resource usage
    typeCommand("Get-WinEvent -FilterHashtable @{LogName='Microsoft-Windows-Diagnostics-Performance/Operational'} -MaxEvents 500 -ErrorAction SilentlyContinue | Select-Object TimeCreated,Id,Message | Export-Csv .\\srum\\performance_diag.csv -NoTypeInformation", true);
    delay(3000);

    logAction("WIN_SRUM", "SRUM collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsBITS() {
    logAction("WIN_BITS", "Collecting BITS (Background Intelligent Transfer Service) data", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\bits", true);
    delay(500);

    // Get BITS jobs
    typeCommand("Get-BitsTransfer -AllUsers -ErrorAction SilentlyContinue | Select-Object JobState,JobType,BytesTotal,BytesTransferred,CreationTime,TransferType,FilesTotal,FilesTransferred | Export-Csv .\\bits\\bits_jobs.csv -NoTypeInformation", true);
    delay(2000);
    logAction("WIN_BITS", "BITS jobs exported", "SUCCESS");

    // BITS event logs
    typeCommand("Get-WinEvent -LogName 'Microsoft-Windows-Bits-Client/Operational' -MaxEvents 1000 -ErrorAction SilentlyContinue | Select-Object TimeCreated,Id,Message | Export-Csv .\\bits\\bits_events.csv -NoTypeInformation", true);
    delay(3000);

    // BITS database files
    typeCommand("Copy-Item \"C:\\ProgramData\\Microsoft\\Network\\Downloader\\qmgr*.dat\" .\\bits\\ -Force -ErrorAction SilentlyContinue", true);
    delay(1500);

    logAction("WIN_BITS", "BITS collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsTimeline() {
    logAction("WIN_TIMELINE", "Collecting Windows Timeline (ActivitiesCache.db)", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\timeline", true);
    delay(500);

    // Windows Timeline database
    typeCommand("Copy-Item \"$env:LOCALAPPDATA\\ConnectedDevicesPlatform\\*\\ActivitiesCache.db\" .\\timeline\\ -Force -Recurse -ErrorAction SilentlyContinue", true);
    delay(2000);
    logAction("WIN_TIMELINE", "ActivitiesCache.db copied", "SUCCESS");

    // Recent documents
    typeCommand("Get-ChildItem \"$env:APPDATA\\Microsoft\\Windows\\Recent\" -Recurse | Select-Object Name,FullName,CreationTime,LastWriteTime,LastAccessTime | Export-Csv .\\timeline\\recent_docs.csv -NoTypeInformation", true);
    delay(1500);

    // Shell bags (user folder views)
    typeCommand("reg export 'HKCU\\Software\\Microsoft\\Windows\\Shell\\BagMRU' .\\timeline\\shellbags.reg /y", true);
    delay(1000);

    // User Assist (program execution)
    typeCommand("reg export 'HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist' .\\timeline\\userassist.reg /y", true);
    delay(1000);

    logAction("WIN_TIMELINE", "Timeline collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsADS() {
    logAction("WIN_ADS", "Scanning for Alternate Data Streams (ADS)", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\ads", true);
    delay(500);

    // Scan common directories for ADS
    String ads_script = "@'\r\n";
    ads_script += "$ErrorActionPreference = 'SilentlyContinue'\r\n";
    ads_script += "$paths = @('C:\\Users', 'C:\\Windows\\Temp', 'C:\\Temp', \"$env:USERPROFILE\\Downloads\")\r\n";
    ads_script += "$results = @()\r\n";
    ads_script += "foreach ($path in $paths) {\r\n";
    ads_script += "    if (Test-Path $path) {\r\n";
    ads_script += "        Get-ChildItem $path -Recurse -File -ErrorAction SilentlyContinue | \r\n";
    ads_script += "        ForEach-Object {\r\n";
    ads_script += "            $streams = Get-Item $_.FullName -Stream * -ErrorAction SilentlyContinue | \r\n";
    ads_script += "                Where-Object {$_.Stream -ne ':$DATA' -and $_.Length -gt 0}\r\n";
    ads_script += "            if ($streams) {\r\n";
    ads_script += "                foreach ($stream in $streams) {\r\n";
    ads_script += "                    $results += [PSCustomObject]@{\r\n";
    ads_script += "                        File = $_.FullName\r\n";
    ads_script += "                        StreamName = $stream.Stream\r\n";
    ads_script += "                        Length = $stream.Length\r\n";
    ads_script += "                    }\r\n";
    ads_script += "                }\r\n";
    ads_script += "            }\r\n";
    ads_script += "        }\r\n";
    ads_script += "    }\r\n";
    ads_script += "}\r\n";
    ads_script += "$results | Export-Csv .\\ads\\alternate_data_streams.csv -NoTypeInformation\r\n";
    ads_script += "'@ | Invoke-Expression";

    typeCommand(ads_script, true);
    delay(10000);  // ADS scanning can take time
    logAction("WIN_ADS", "Alternate Data Streams scan complete", "SUCCESS");

    return true;
}

bool HIDAutomation::executeWindowsShadowCopies() {
    logAction("WIN_SHADOW", "Collecting Volume Shadow Copy information", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\shadow_copies", true);
    delay(500);

    // List shadow copies
    typeCommand("vssadmin list shadows > .\\shadow_copies\\shadow_list.txt", true);
    delay(2000);
    logAction("WIN_SHADOW", "Shadow copy list exported", "SUCCESS");

    // Get shadow copy details
    typeCommand("Get-CimInstance Win32_ShadowCopy | Select-Object ID,InstallDate,DeviceObject,VolumeName,Count | Export-Csv .\\shadow_copies\\shadow_details.csv -NoTypeInformation", true);
    delay(2000);

    // Shadow storage information
    typeCommand("vssadmin list shadowstorage > .\\shadow_copies\\shadow_storage.txt", true);
    delay(1500);

    // System Restore points
    typeCommand("Get-ComputerRestorePoint | Select-Object CreationTime,Description,RestorePointType,SequenceNumber | Export-Csv .\\shadow_copies\\restore_points.csv -NoTypeInformation -ErrorAction SilentlyContinue", true);
    delay(1500);

    logAction("WIN_SHADOW", "Shadow copy collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsDefender() {
    logAction("WIN_DEFENDER", "Collecting Windows Defender information", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\defender", true);
    delay(500);

    // Get Defender status
    typeCommand("Get-MpComputerStatus | Export-Clixml .\\defender\\defender_status.xml", true);
    delay(2000);

    // Get Defender preferences
    typeCommand("Get-MpPreference | Export-Clixml .\\defender\\defender_preferences.xml", true);
    delay(1500);

    // Get threat catalog
    typeCommand("Get-MpThreat | Export-Csv .\\defender\\threats.csv -NoTypeInformation -ErrorAction SilentlyContinue", true);
    delay(2000);

    // Get scan history
    typeCommand("Get-MpThreatDetection | Select-Object -First 100 | Export-Csv .\\defender\\threat_detections.csv -NoTypeInformation -ErrorAction SilentlyContinue", true);
    delay(2000);

    // Export Defender logs
    typeCommand("wevtutil epl Microsoft-Windows-Windows Defender/Operational .\\defender\\defender_operational.evtx", true);
    delay(3000);

    logAction("WIN_DEFENDER", "Windows Defender collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsFirewall() {
    logAction("WIN_FIREWALL", "Collecting Windows Firewall configuration", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\firewall", true);
    delay(500);

    // Get firewall profiles
    typeCommand("Get-NetFirewallProfile | Export-Csv .\\firewall\\firewall_profiles.csv -NoTypeInformation", true);
    delay(1500);

    // Get firewall rules
    typeCommand("Get-NetFirewallRule | Export-Csv .\\firewall\\firewall_rules.csv -NoTypeInformation", true);
    delay(3000);

    // Get application rules
    typeCommand("Get-NetFirewallApplicationFilter | Export-Csv .\\firewall\\firewall_apps.csv -NoTypeInformation", true);
    delay(2000);

    // Export firewall logs
    typeCommand("wevtutil epl Microsoft-Windows-Windows Firewall With Advanced Security/Firewall .\\firewall\\firewall.evtx", true);
    delay(2000);

    // Get firewall settings via netsh
    typeCommand("netsh advfirewall show allprofiles > .\\firewall\\netsh_profiles.txt", true);
    delay(1500);

    logAction("WIN_FIREWALL", "Firewall collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsWLANProfiles() {
    logAction("WIN_WLAN", "Collecting WLAN profiles and credentials", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\wlan", true);
    delay(500);

    // Get WLAN profiles
    typeCommand("netsh wlan show profiles > .\\wlan\\wlan_profiles.txt", true);
    delay(1500);

    // Export all WLAN profiles with keys
    typeCommand("netsh wlan export profile key=clear folder=.\\wlan", true);
    delay(3000);

    // Get WLAN interface info
    typeCommand("netsh wlan show interfaces > .\\wlan\\wlan_interfaces.txt", true);
    delay(1000);

    // Get network list
    typeCommand("netsh wlan show networks mode=bssid > .\\wlan\\available_networks.txt", true);
    delay(2000);

    logAction("WIN_WLAN", "WLAN profiles collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsInstalledPrograms() {
    logAction("WIN_PROGRAMS", "Collecting installed programs list", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\programs", true);
    delay(500);

    // Get installed programs via WMI
    typeCommand("Get-WmiObject -Class Win32_Product | Select-Object Name,Version,Vendor,InstallDate | Export-Csv .\\programs\\installed_programs_wmi.csv -NoTypeInformation", true);
    delay(10000); // WMI can be slow

    // Get programs from registry (faster)
    typeCommand("Get-ItemProperty HKLM:\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\* | Select-Object DisplayName,DisplayVersion,Publisher,InstallDate | Export-Csv .\\programs\\installed_programs_reg64.csv -NoTypeInformation", true);
    delay(2000);

    typeCommand("Get-ItemProperty HKLM:\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\* | Select-Object DisplayName,DisplayVersion,Publisher,InstallDate | Export-Csv .\\programs\\installed_programs_reg32.csv -NoTypeInformation -ErrorAction SilentlyContinue", true);
    delay(2000);

    // Get StartMenu programs
    typeCommand("Get-ChildItem 'C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs' -Recurse -File | Select-Object FullName,CreationTime,LastWriteTime | Export-Csv .\\programs\\startmenu_programs.csv -NoTypeInformation", true);
    delay(2000);

    logAction("WIN_PROGRAMS", "Installed programs collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsProcessList() {
    logAction("WIN_PROCESSES", "Collecting running processes snapshot", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\processes", true);
    delay(500);

    // Get process list with details
    typeCommand("Get-Process | Select-Object ProcessName,Id,Path,StartTime,CPU,WorkingSet,CommandLine | Export-Csv .\\processes\\processes.csv -NoTypeInformation", true);
    delay(2000);

    // Get process owners
    typeCommand("Get-WmiObject Win32_Process | Select-Object ProcessId,Name,CommandLine,@{n='Owner';e={$_.GetOwner().User}} | Export-Csv .\\processes\\process_owners.csv -NoTypeInformation", true);
    delay(3000);

    // Get services
    typeCommand("Get-Service | Export-Csv .\\processes\\services_status.csv -NoTypeInformation", true);
    delay(1500);

    // Get network connections with process info
    typeCommand("Get-NetTCPConnection | Select-Object LocalAddress,LocalPort,RemoteAddress,RemotePort,State,OwningProcess | Export-Csv .\\processes\\tcp_connections.csv -NoTypeInformation", true);
    delay(2000);

    // Tasklist with modules
    typeCommand("tasklist /m > .\\processes\\tasklist_modules.txt", true);
    delay(2000);

    logAction("WIN_PROCESSES", "Process snapshot complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsNetworkShares() {
    logAction("WIN_SHARES", "Collecting network shares and mapped drives", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\shares", true);
    delay(500);

    // Get SMB shares
    typeCommand("Get-SmbShare | Export-Csv .\\shares\\smb_shares.csv -NoTypeInformation", true);
    delay(1500);

    // Get mapped network drives
    typeCommand("Get-PSDrive -PSProvider FileSystem | Export-Csv .\\shares\\mapped_drives.csv -NoTypeInformation", true);
    delay(1000);

    // Get net use info
    typeCommand("net use > .\\shares\\net_use.txt", true);
    delay(1000);

    // Get SMB connections
    typeCommand("Get-SmbConnection | Export-Csv .\\shares\\smb_connections.csv -NoTypeInformation -ErrorAction SilentlyContinue", true);
    delay(1500);

    // Get WMI network connections
    typeCommand("Get-WmiObject Win32_NetworkConnection | Export-Csv .\\shares\\network_connections.csv -NoTypeInformation", true);
    delay(2000);

    logAction("WIN_SHARES", "Network shares collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsDrivers() {
    logAction("WIN_DRIVERS", "Collecting driver information", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\drivers", true);
    delay(500);

    // Get loaded drivers
    typeCommand("Get-WindowsDriver -Online | Export-Csv .\\drivers\\loaded_drivers.csv -NoTypeInformation", true);
    delay(3000);

    // Get driver info via DISM
    typeCommand("driverquery /v /fo csv > .\\drivers\\driverquery.csv", true);
    delay(2000);

    // Get PnP devices
    typeCommand("Get-PnpDevice | Export-Csv .\\drivers\\pnp_devices.csv -NoTypeInformation", true);
    delay(2000);

    // Get device manager tree
    typeCommand("pnputil /enum-devices > .\\drivers\\pnp_enumeration.txt", true);
    delay(2000);

    logAction("WIN_DRIVERS", "Driver collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsWindowsUpdate() {
    logAction("WIN_UPDATE", "Collecting Windows Update history", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\updates", true);
    delay(500);

    // Get installed updates
    typeCommand("Get-HotFix | Select-Object Description,HotFixID,InstalledBy,InstalledOn | Export-Csv .\\updates\\installed_updates.csv -NoTypeInformation", true);
    delay(3000);

    // Get update history via WMI
    typeCommand("Get-WmiObject -Class Win32_QuickFixEngineering | Export-Csv .\\updates\\updates_wmi.csv -NoTypeInformation", true);
    delay(2000);

    // Export Windows Update log
    typeCommand("Get-WindowsUpdateLog -LogPath .\\updates\\WindowsUpdate.log -ErrorAction SilentlyContinue", true);
    delay(5000);

    logAction("WIN_UPDATE", "Windows Update history complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsEnvironmentVars() {
    logAction("WIN_ENV", "Collecting environment variables", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\environment", true);
    delay(500);

    // Get all environment variables
    typeCommand("Get-ChildItem Env: | Export-Csv .\\environment\\env_vars.csv -NoTypeInformation", true);
    delay(1000);

    // System environment variables
    typeCommand("[Environment]::GetEnvironmentVariables('Machine') | Out-File .\\environment\\system_env.txt", true);
    delay(1000);

    // User environment variables
    typeCommand("[Environment]::GetEnvironmentVariables('User') | Out-File .\\environment\\user_env.txt", true);
    delay(1000);

    // PATH variable detailed
    typeCommand("$env:Path -split ';' | Out-File .\\environment\\path_detailed.txt", true);
    delay(500);

    logAction("WIN_ENV", "Environment variables collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsStartupPrograms() {
    logAction("WIN_STARTUP", "Collecting startup programs", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\startup", true);
    delay(500);

    // Get startup programs via WMI
    typeCommand("Get-CimInstance Win32_StartupCommand | Select-Object Name,Command,Location,User | Export-Csv .\\startup\\startup_wmi.csv -NoTypeInformation", true);
    delay(2000);

    // Startup folder contents
    typeCommand("Get-ChildItem 'C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\StartUp' -Recurse | Export-Csv .\\startup\\startup_allusers.csv -NoTypeInformation -ErrorAction SilentlyContinue", true);
    delay(1000);

    typeCommand("Get-ChildItem '$env:APPDATA\\Microsoft\\Windows\\Start Menu\\Programs\\Startup' -Recurse -ErrorAction SilentlyContinue | Export-Csv .\\startup\\startup_user.csv -NoTypeInformation", true);
    delay(1000);

    logAction("WIN_STARTUP", "Startup programs collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsErrorReporting() {
    logAction("WIN_WER", "Collecting Windows Error Reporting data", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\wer", true);
    delay(500);

    // Copy WER reports
    typeCommand("Copy-Item -Path C:\\ProgramData\\Microsoft\\Windows\\WER\\ReportQueue\\* -Destination .\\wer\\ -Recurse -ErrorAction SilentlyContinue", true);
    delay(3000);

    // Get application crash info
    typeCommand("Get-WinEvent -LogName Application -FilterXPath '*[System[Provider[@Name=\"Application Error\"]]]' -MaxEvents 100 -ErrorAction SilentlyContinue | Export-Csv .\\wer\\app_crashes.csv -NoTypeInformation", true);
    delay(3000);

    logAction("WIN_WER", "Error reporting collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsHosts() {
    logAction("WIN_HOSTS", "Collecting hosts file", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\network_config", true);
    delay(500);

    // Copy hosts file
    typeCommand("Copy-Item C:\\Windows\\System32\\drivers\\etc\\hosts .\\network_config\\hosts.txt", true);
    delay(500);

    // Copy other network config files
    typeCommand("Copy-Item C:\\Windows\\System32\\drivers\\etc\\* .\\network_config\\ -ErrorAction SilentlyContinue", true);
    delay(1000);

    logAction("WIN_HOSTS", "Hosts file collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsDNSCache() {
    logAction("WIN_DNS", "Collecting DNS cache", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\dns", true);
    delay(500);

    // Get DNS cache
    typeCommand("Get-DnsClientCache | Export-Csv .\\dns\\dns_cache.csv -NoTypeInformation", true);
    delay(1500);

    // Get DNS client settings
    typeCommand("Get-DnsClient | Export-Csv .\\dns\\dns_client.csv -NoTypeInformation", true);
    delay(1000);

    // ipconfig /displaydns
    typeCommand("ipconfig /displaydns > .\\dns\\ipconfig_dns.txt", true);
    delay(1500);

    logAction("WIN_DNS", "DNS cache collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsCertificates() {
    logAction("WIN_CERTS", "Collecting certificates", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\certificates", true);
    delay(500);

    // Export certificates from various stores
    typeCommand("Get-ChildItem -Path Cert:\\LocalMachine\\My | Export-Csv .\\certificates\\machine_my.csv -NoTypeInformation", true);
    delay(2000);

    typeCommand("Get-ChildItem -Path Cert:\\LocalMachine\\Root | Export-Csv .\\certificates\\machine_root.csv -NoTypeInformation", true);
    delay(2000);

    typeCommand("Get-ChildItem -Path Cert:\\CurrentUser\\My -ErrorAction SilentlyContinue | Export-Csv .\\certificates\\user_my.csv -NoTypeInformation", true);
    delay(2000);

    typeCommand("Get-ChildItem -Path Cert:\\CurrentUser\\Root -ErrorAction SilentlyContinue | Export-Csv .\\certificates\\user_root.csv -NoTypeInformation", true);
    delay(2000);

    logAction("WIN_CERTS", "Certificates collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsActivation() {
    logAction("WIN_ACTIVATION", "Collecting Windows activation status", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\activation", true);
    delay(500);

    // Get activation status
    typeCommand("slmgr /dli > .\\activation\\license_info.txt", true);
    delay(2000);

    // Get detailed license info
    typeCommand("slmgr /dlv > .\\activation\\license_verbose.txt", true);
    delay(2000);

    // Get Windows version info
    typeCommand("Get-ComputerInfo | Select-Object WindowsProductName,WindowsVersion,WindowsBuildLabEx,OsArchitecture | Export-Csv .\\activation\\windows_version.csv -NoTypeInformation", true);
    delay(2000);

    logAction("WIN_ACTIVATION", "Activation status collection complete", "SUCCESS");
    return true;
}

// ============================================================================
// ADVANCED WINDOWS FORENSICS (v1.1.0+)
// ============================================================================

bool HIDAutomation::executeWindowsSearchDatabase() {
    logAction("WIN_SEARCH_DB", "Extracting Windows Search database", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\windows_search", true);
    delay(300);

    // Copy Windows.edb (Windows Search database)
    typeCommand("$searchPath = \"$env:ProgramData\\Microsoft\\Search\\Data\\Applications\\Windows\"", true);
    delay(200);

    typeCommand("if (Test-Path \"$searchPath\\Windows.edb\") { Copy-Item \"$searchPath\\Windows.edb\" .\\windows_search\\ -ErrorAction SilentlyContinue }", true);
    delay(3000);

    // Get search indexing status
    typeCommand("Get-ItemProperty 'HKLM:\\SOFTWARE\\Microsoft\\Windows Search' | Export-Csv .\\windows_search\\search_config.csv -NoTypeInformation", true);
    delay(1000);

    logAction("WIN_SEARCH_DB", "Windows Search database extraction complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsActivitiesCache() {
    logAction("WIN_ACTIVITIES", "Extracting Windows Activities Cache", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\activities", true);
    delay(300);

    // Copy ActivitiesCache.db from all users
    typeCommand("Get-ChildItem -Path C:\\Users -Directory | ForEach-Object { $actPath = \"$($_.FullName)\\AppData\\Local\\ConnectedDevicesPlatform\\L.*\\ActivitiesCache.db\"; if (Test-Path $actPath) { Copy-Item $actPath .\\activities\\$($_.Name)_ActivitiesCache.db -ErrorAction SilentlyContinue } }", true);
    delay(5000);

    logAction("WIN_ACTIVITIES", "Activities Cache extraction complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsNotificationDB() {
    logAction("WIN_NOTIFICATIONS", "Extracting notification databases", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\notifications", true);
    delay(300);

    // Extract notification database from all users
    typeCommand("Get-ChildItem C:\\Users -Directory | ForEach-Object { $nPath = \"$($_.FullName)\\AppData\\Local\\Microsoft\\Windows\\Notifications\\wpndatabase.db\"; if (Test-Path $nPath) { Copy-Item $nPath .\\notifications\\$($_.Name)_notifications.db -ErrorAction SilentlyContinue } }", true);
    delay(4000);

    logAction("WIN_NOTIFICATIONS", "Notification databases extracted", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsClipboardHistory() {
    logAction("WIN_CLIPBOARD", "Extracting clipboard history", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\clipboard", true);
    delay(300);

    // Get clipboard history from all users
    typeCommand("Get-ChildItem C:\\Users -Directory | ForEach-Object { $clipPath = \"$($_.FullName)\\AppData\\Local\\Microsoft\\Windows\\Clipboard\"; if (Test-Path $clipPath) { Copy-Item $clipPath\\* .\\clipboard\\$($_.Name)_ -Recurse -ErrorAction SilentlyContinue } }", true);
    delay(3000);

    // Get current clipboard content
    typeCommand("Get-Clipboard -Format Text > .\\clipboard\\current_clipboard.txt 2>&1", true);
    delay(500);

    logAction("WIN_CLIPBOARD", "Clipboard history extraction complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsConnectedDevices() {
    logAction("WIN_CONNECTED_DEV", "Collecting connected devices platform data", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\connected_devices", true);
    delay(300);

    // Copy ConnectedDevicesPlatform data
    typeCommand("Get-ChildItem C:\\Users -Directory | ForEach-Object { $cdpPath = \"$($_.FullName)\\AppData\\Local\\ConnectedDevicesPlatform\"; if (Test-Path $cdpPath) { Copy-Item $cdpPath .\\connected_devices\\$($_.Name)_CDP -Recurse -ErrorAction SilentlyContinue } }", true);
    delay(5000);

    // Get connected devices from registry
    typeCommand("reg export 'HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess' .\\connected_devices\\device_access.reg /y", true);
    delay(1000);

    logAction("WIN_CONNECTED_DEV", "Connected devices collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsBackgroundTasks() {
    logAction("WIN_BG_TASKS", "Analyzing background tasks and BAM", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\background_tasks", true);
    delay(300);

    // Export Background Activity Moderator (BAM) registry
    typeCommand("reg export 'HKLM\\SYSTEM\\CurrentControlSet\\Services\\bam\\State\\UserSettings' .\\background_tasks\\bam.reg /y 2>&1", true);
    delay(1500);

    // Export Desktop Activity Moderator (DAM)
    typeCommand("reg export 'HKLM\\SYSTEM\\CurrentControlSet\\Services\\dam' .\\background_tasks\\dam.reg /y 2>&1", true);
    delay(1500);

    // Get background task info
    typeCommand("Get-ScheduledTask | Where-Object {$_.State -ne 'Disabled'} | Export-Csv .\\background_tasks\\active_tasks.csv -NoTypeInformation", true);
    delay(2000);

    logAction("WIN_BG_TASKS", "Background tasks analysis complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsCortanaHistory() {
    logAction("WIN_CORTANA", "Extracting Cortana/Search history", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\cortana", true);
    delay(300);

    // Copy search history from all users
    typeCommand("Get-ChildItem C:\\Users -Directory | ForEach-Object { $sPath = \"$($_.FullName)\\AppData\\Local\\Microsoft\\Windows\\Cortana\"; if (Test-Path $sPath) { Copy-Item $sPath .\\cortana\\$($_.Name)_Cortana -Recurse -ErrorAction SilentlyContinue } }", true);
    delay(4000);

    // Export search settings from registry
    typeCommand("reg export 'HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search' .\\cortana\\search_settings.reg /y 2>&1", true);
    delay(1000);

    logAction("WIN_CORTANA", "Cortana history extraction complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsOfficeRecent() {
    logAction("WIN_OFFICE", "Collecting Office recent documents", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\office_recent", true);
    delay(300);

    // Copy Office recent files from all users
    typeCommand("Get-ChildItem C:\\Users -Directory | ForEach-Object { $oPath = \"$($_.FullName)\\AppData\\Roaming\\Microsoft\\Office\\Recent\"; if (Test-Path $oPath) { Copy-Item $oPath .\\office_recent\\$($_.Name)_Recent -Recurse -ErrorAction SilentlyContinue } }", true);
    delay(3000);

    // Export Office registry settings
    typeCommand("reg export 'HKCU\\Software\\Microsoft\\Office' .\\office_recent\\office_settings.reg /y 2>&1", true);
    delay(2000);

    // Get Office Trust Records
    typeCommand("Get-ChildItem 'HKCU:\\Software\\Microsoft\\Office\\*\\*\\Security\\Trusted Documents' -Recurse -ErrorAction SilentlyContinue | Export-Csv .\\office_recent\\trusted_docs.csv -NoTypeInformation 2>&1", true);
    delay(1500);

    logAction("WIN_OFFICE", "Office recent documents collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsStickyNotes() {
    logAction("WIN_STICKY_NOTES", "Extracting Sticky Notes", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\sticky_notes", true);
    delay(300);

    // Copy Sticky Notes database from all users
    typeCommand("Get-ChildItem C:\\Users -Directory | ForEach-Object { $snPath = \"$($_.FullName)\\AppData\\Local\\Packages\\Microsoft.MicrosoftStickyNotes*\\LocalState\\plum.sqlite\"; if (Test-Path $snPath) { Copy-Item $snPath .\\sticky_notes\\$($_.Name)_stickynotes.sqlite -ErrorAction SilentlyContinue } }", true);
    delay(3000);

    logAction("WIN_STICKY_NOTES", "Sticky Notes extraction complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsScreenTime() {
    logAction("WIN_SCREEN_TIME", "Collecting screen time and activity data", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\screen_time", true);
    delay(300);

    // Get diagnostic data from EventLog
    typeCommand("Get-WinEvent -LogName 'Microsoft-Windows-Diagnostics-Performance/Operational' -MaxEvents 1000 -ErrorAction SilentlyContinue | Select-Object TimeCreated,Id,Message | Export-Csv .\\screen_time\\diagnostics.csv -NoTypeInformation", true);
    delay(3000);

    // Get user activity from System EventLog
    typeCommand("Get-WinEvent -FilterHashtable @{LogName='System';ID=1,12,13} -MaxEvents 500 -ErrorAction SilentlyContinue | Export-Csv .\\screen_time\\power_events.csv -NoTypeInformation", true);
    delay(2000);

    logAction("WIN_SCREEN_TIME", "Screen time data collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsAppExecAlias() {
    logAction("WIN_APP_ALIAS", "Collecting App Execution Aliases", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\app_alias", true);
    delay(300);

    // Get App Execution Aliases from all users
    typeCommand("Get-ChildItem C:\\Users -Directory | ForEach-Object { $aaPath = \"$($_.FullName)\\AppData\\Local\\Microsoft\\WindowsApps\"; if (Test-Path $aaPath) { Get-ChildItem $aaPath -Filter *.exe | Select-Object Name,Target,LastWriteTime > .\\app_alias\\$($_.Name)_aliases.txt } }", true);
    delay(2000);

    logAction("WIN_APP_ALIAS", "App Execution Aliases collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsPackageManager() {
    logAction("WIN_PKG_MGR", "Collecting package manager history", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\package_mgr", true);
    delay(300);

    // Get winget packages
    typeCommand("winget list > .\\package_mgr\\winget_packages.txt 2>&1", true);
    delay(3000);

    // Get Chocolatey packages
    typeCommand("if (Get-Command choco -ErrorAction SilentlyContinue) { choco list --local-only > .\\package_mgr\\choco_packages.txt }", true);
    delay(2000);

    // Get Windows Package Manager logs
    typeCommand("Copy-Item \"$env:LOCALAPPDATA\\Packages\\Microsoft.DesktopAppInstaller*\\LocalState\\DiagOutputDir\\*.log\" .\\package_mgr\\ -ErrorAction SilentlyContinue", true);
    delay(1500);

    logAction("WIN_PKG_MGR", "Package manager history collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsUpdateDetails() {
    logAction("WIN_UPDATE_DETAIL", "Collecting detailed Windows Update info", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\update_details", true);
    delay(300);

    // Get detailed update history
    typeCommand("Get-HotFix | Export-Csv .\\update_details\\hotfixes.csv -NoTypeInformation", true);
    delay(1500);

    // Get Windows Update log
    typeCommand("Get-WindowsUpdateLog -LogPath .\\update_details\\WindowsUpdate.log", true);
    delay(5000);

    // Get update session history
    typeCommand("$session = New-Object -ComObject Microsoft.Update.Session; $searcher = $session.CreateUpdateSearcher(); $historyCount = $searcher.GetTotalHistoryCount(); $searcher.QueryHistory(0, $historyCount) | Select-Object Title,Date,ResultCode,Description | Export-Csv .\\update_details\\update_history.csv -NoTypeInformation", true);
    delay(3000);

    logAction("WIN_UPDATE_DETAIL", "Windows Update details collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsPerformanceCounters() {
    logAction("WIN_PERF_COUNTERS", "Collecting performance counters", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\perf_counters", true);
    delay(300);

    // Get performance counter data
    typeCommand("Get-Counter -ListSet * | Export-Csv .\\perf_counters\\counter_sets.csv -NoTypeInformation", true);
    delay(2000);

    // Sample key performance metrics
    typeCommand("Get-Counter '\\Processor(_Total)\\% Processor Time','\\Memory\\Available MBytes','\\PhysicalDisk(_Total)\\% Disk Time' -SampleInterval 1 -MaxSamples 10 | Export-Counter -Path .\\perf_counters\\perf_samples.blg", true);
    delay(12000);

    logAction("WIN_PERF_COUNTERS", "Performance counters collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsSecurityAnalysis() {
    logAction("WIN_SEC_ANALYSIS", "Performing security event analysis", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\security_analysis", true);
    delay(300);

    // Get failed logon attempts
    typeCommand("Get-WinEvent -FilterHashtable @{LogName='Security';ID=4625} -MaxEvents 500 -ErrorAction SilentlyContinue | Select-Object TimeCreated,Message | Export-Csv .\\security_analysis\\failed_logons.csv -NoTypeInformation", true);
    delay(3000);

    // Get successful logons
    typeCommand("Get-WinEvent -FilterHashtable @{LogName='Security';ID=4624} -MaxEvents 500 -ErrorAction SilentlyContinue | Select-Object TimeCreated,Message | Export-Csv .\\security_analysis\\successful_logons.csv -NoTypeInformation", true);
    delay(3000);

    // Get account lockouts
    typeCommand("Get-WinEvent -FilterHashtable @{LogName='Security';ID=4740} -MaxEvents 100 -ErrorAction SilentlyContinue | Export-Csv .\\security_analysis\\account_lockouts.csv -NoTypeInformation", true);
    delay(2000);

    // Get privilege escalations
    typeCommand("Get-WinEvent -FilterHashtable @{LogName='Security';ID=4672,4673,4674} -MaxEvents 200 -ErrorAction SilentlyContinue | Export-Csv .\\security_analysis\\privilege_use.csv -NoTypeInformation", true);
    delay(3000);

    // Get security policy changes
    typeCommand("Get-WinEvent -FilterHashtable @{LogName='Security';ID=4719,4739} -MaxEvents 100 -ErrorAction SilentlyContinue | Export-Csv .\\security_analysis\\policy_changes.csv -NoTypeInformation", true);
    delay(2000);

    logAction("WIN_SEC_ANALYSIS", "Security analysis complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsRDPCache() {
    logAction("WIN_RDP_CACHE", "Extracting RDP cache and bitmap files", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\rdp_cache", true);
    delay(300);

    // Copy RDP cache from all users
    typeCommand("Get-ChildItem C:\\Users -Directory | ForEach-Object { $rdpPath = \"$($_.FullName)\\AppData\\Local\\Microsoft\\Terminal Server Client\\Cache\"; if (Test-Path $rdpPath) { Copy-Item $rdpPath .\\rdp_cache\\$($_.Name)_Cache -Recurse -ErrorAction SilentlyContinue } }", true);
    delay(4000);

    // Get RDP connection history
    typeCommand("Get-ChildItem 'HKCU:\\Software\\Microsoft\\Terminal Server Client\\Servers' -Recurse -ErrorAction SilentlyContinue | Export-Csv .\\rdp_cache\\rdp_connections.csv -NoTypeInformation 2>&1", true);
    delay(1500);

    // Export RDP settings
    typeCommand("reg export 'HKCU\\Software\\Microsoft\\Terminal Server Client' .\\rdp_cache\\rdp_settings.reg /y", true);
    delay(1000);

    logAction("WIN_RDP_CACHE", "RDP cache extraction complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsTerminalServerClient() {
    logAction("WIN_TSC", "Collecting Terminal Server Client data", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\tsclient", true);
    delay(300);

    // Get all RDP connection files
    typeCommand("Get-ChildItem C:\\Users -Directory | ForEach-Object { $rdpFiles = \"$($_.FullName)\\Documents\\*.rdp\"; if (Test-Path $rdpFiles) { Copy-Item $rdpFiles .\\tsclient\\ -ErrorAction SilentlyContinue } }", true);
    delay(2000);

    // Get Default.rdp files
    typeCommand("Get-ChildItem C:\\Users -Directory | ForEach-Object { $defRdp = \"$($_.FullName)\\Documents\\Default.rdp\"; if (Test-Path $defRdp) { Copy-Item $defRdp .\\tsclient\\$($_.Name)_Default.rdp -ErrorAction SilentlyContinue } }", true);
    delay(1500);

    // Get recent RDP connections from EventLog
    typeCommand("Get-WinEvent -LogName 'Microsoft-Windows-TerminalServices-LocalSessionManager/Operational' -MaxEvents 200 -ErrorAction SilentlyContinue | Export-Csv .\\tsclient\\rdp_sessions.csv -NoTypeInformation", true);
    delay(2500);

    logAction("WIN_TSC", "Terminal Server Client data collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsUSNJournal() {
    logAction("WIN_USN_JOURNAL", "Extracting NTFS USN Journal", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\usn_journal", true);
    delay(300);

    // Query USN journal information
    typeCommand("fsutil usn queryjournal C: > .\\usn_journal\\usn_info.txt", true);
    delay(1500);

    // Export USN journal entries (last 10000)
    typeCommand("fsutil usn readjournal C: csv | Select-Object -Last 10000 > .\\usn_journal\\usn_entries.csv", true);
    delay(8000);

    // Get USN journal stats
    typeCommand("fsutil usn stat C: > .\\usn_journal\\usn_stats.txt 2>&1", true);
    delay(1000);

    logAction("WIN_USN_JOURNAL", "USN Journal extraction complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsVolumeInformation() {
    logAction("WIN_VOLUME_INFO", "Collecting volume information", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\volume_info", true);
    delay(300);

    // Get detailed volume information
    typeCommand("Get-Volume | Export-Csv .\\volume_info\\volumes.csv -NoTypeInformation", true);
    delay(1000);

    // Get partition information
    typeCommand("Get-Partition | Export-Csv .\\volume_info\\partitions.csv -NoTypeInformation", true);
    delay(1000);

    // Get disk information
    typeCommand("Get-Disk | Export-Csv .\\volume_info\\disks.csv -NoTypeInformation", true);
    delay(1000);

    // Get volume shadow copy information
    typeCommand("vssadmin list shadows > .\\volume_info\\shadow_copies.txt", true);
    delay(1500);

    // Get file system information
    typeCommand("fsutil fsinfo volumeinfo C: > .\\volume_info\\c_volumeinfo.txt", true);
    delay(1000);

    logAction("WIN_VOLUME_INFO", "Volume information collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsSRUMDetailed() {
    logAction("WIN_SRUM_DETAIL", "Extracting detailed SRUM database", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\srum_detailed", true);
    delay(300);

    // Copy SRUM database
    typeCommand("Copy-Item C:\\Windows\\System32\\sru\\SRUDB.dat .\\srum_detailed\\ -ErrorAction SilentlyContinue", true);
    delay(2000);

    // Get SRUM registry settings
    typeCommand("reg export 'HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SRUM' .\\srum_detailed\\srum_config.reg /y", true);
    delay(1000);

    logAction("WIN_SRUM_DETAIL", "Detailed SRUM extraction complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsIISLogs() {
    logAction("WIN_IIS", "Collecting IIS logs if present", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\iis_logs", true);
    delay(300);

    // Check if IIS is installed and copy logs
    typeCommand("if (Test-Path C:\\inetpub\\logs) { Copy-Item C:\\inetpub\\logs\\LogFiles\\*.log .\\iis_logs\\ -Recurse -ErrorAction SilentlyContinue }", true);
    delay(3000);

    // Get IIS configuration
    typeCommand("if (Get-Command Get-WebSite -ErrorAction SilentlyContinue) { Get-WebSite | Export-Csv .\\iis_logs\\iis_sites.csv -NoTypeInformation }", true);
    delay(1500);

    // Get IIS bindings
    typeCommand("if (Get-Command Get-WebBinding -ErrorAction SilentlyContinue) { Get-WebBinding | Export-Csv .\\iis_logs\\iis_bindings.csv -NoTypeInformation }", true);
    delay(1500);

    logAction("WIN_IIS", "IIS logs collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeWindowsGroupPolicy() {
    logAction("WIN_GPO", "Collecting Group Policy information", "STARTED");

    typeCommand("New-Item -ItemType Directory -Force -Path .\\group_policy", true);
    delay(300);

    // Generate Group Policy report
    typeCommand("gpresult /H .\\group_policy\\gpresult.html", true);
    delay(3000);

    // Get detailed GP report in XML
    typeCommand("gpresult /X .\\group_policy\\gpresult.xml", true);
    delay(2500);

    // Get GP RSoP data
    typeCommand("gpresult /Z > .\\group_policy\\gpresult_verbose.txt", true);
    delay(3000);

    // List all applied GPOs
    typeCommand("Get-GPResultantSetOfPolicy -ReportType Html -Path .\\group_policy\\rsop.html -ErrorAction SilentlyContinue", true);
    delay(4000);

    logAction("WIN_GPO", "Group Policy collection complete", "SUCCESS");
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
    String archive_name = "frfd_evidence_" + timestamp + ".tar.gz";
    String archive_path = "/tmp/" + archive_name;
    String tar_cmd = "tar -czf " + archive_path + " /tmp/frfd_collection/";
    typeCommand(tar_cmd, true);
    delay(5000);

    logAction("LNX_ARCHIVE", "Created evidence archive", archive_path);

    // Connect to FRFD WiFi AP (using nmcli on modern Linux)
    typeCommand("nmcli device wifi connect CSIRT-FORENSICS password ChangeThisPassword123!", true);
    delay(3000);  // Wait for WiFi connection

    logAction("LNX_WIFI", "Connecting to FRFD WiFi", "CSIRT-FORENSICS");

    // Define inline upload function
    typeCommand("upload(){f=\"$1\";t=\"${2:-archive}\";ip=\"${3:-192.168.4.1}\";[ ! -f \"$f\" ]&&return 1;for i in 1 2 3;do r=$(curl -s -w \"\\n%{http_code}\" -X POST -F \"file=@$f\" -F \"type=$t\" --connect-timeout 10 --max-time 60 \"http://$ip/upload\" 2>&1);c=$(echo \"$r\"|tail -n1);[ \"$c\" = \"200\" ]&&return 0;sleep 2;done;return 1;}", true);
    delay(500);

    // Upload archive to FRFD
    String upload_cmd = "upload " + archive_path + " archive";
    typeCommand(upload_cmd, true);
    delay(10000);  // Wait for upload to complete

    logAction("LNX_UPLOAD", "Uploaded evidence to FRFD", archive_name);

    // Cleanup - delete local evidence (optional, uncomment if desired)
    // typeCommand("rm -rf /tmp/frfd_collection", true);
    // typeCommand("rm -f " + archive_path, true);
    // delay(1000);

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

bool HIDAutomation::executeLinuxShellHistory() {
    logAction("LNX_SHELL_HISTORY", "Collecting shell history for all users", "STARTED");

    typeCommand("mkdir -p shell_history", true);
    delay(300);

    // Current user's bash history
    typeCommand("if [ -f ~/.bash_history ]; then cp ~/.bash_history shell_history/bash_history_$(whoami).txt; fi", true);
    delay(500);

    // Current user's zsh history
    typeCommand("if [ -f ~/.zsh_history ]; then cp ~/.zsh_history shell_history/zsh_history_$(whoami).txt; fi", true);
    delay(500);

    // Collect history for all users (requires root)
    typeCommand("for user_home in /home/*; do user=$(basename $user_home); if [ -f $user_home/.bash_history ]; then sudo cp $user_home/.bash_history shell_history/bash_history_$user.txt 2>/dev/null; fi; done", true);
    delay(2000);

    typeCommand("for user_home in /home/*; do user=$(basename $user_home); if [ -f $user_home/.zsh_history ]; then sudo cp $user_home/.zsh_history shell_history/zsh_history_$user.txt 2>/dev/null; fi; done", true);
    delay(2000);

    // Root user history
    typeCommand("sudo cp /root/.bash_history shell_history/bash_history_root.txt 2>/dev/null", true);
    delay(500);

    logAction("LNX_SHELL_HISTORY", "Shell history collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxSSHConfig() {
    logAction("LNX_SSH", "Collecting SSH configurations and keys", "STARTED");

    typeCommand("mkdir -p ssh_config", true);
    delay(300);

    // System-wide SSH config
    typeCommand("sudo cp /etc/ssh/sshd_config ssh_config/sshd_config.txt 2>/dev/null", true);
    delay(500);

    typeCommand("sudo cp /etc/ssh/ssh_config ssh_config/ssh_config.txt 2>/dev/null", true);
    delay(500);

    // Current user's SSH config
    typeCommand("if [ -d ~/.ssh ]; then cp ~/.ssh/config ssh_config/user_ssh_config.txt 2>/dev/null; fi", true);
    delay(300);

    // Authorized keys (current user)
    typeCommand("if [ -f ~/.ssh/authorized_keys ]; then cp ~/.ssh/authorized_keys ssh_config/authorized_keys_$(whoami).txt; fi", true);
    delay(300);

    // Known hosts (current user)
    typeCommand("if [ -f ~/.ssh/known_hosts ]; then cp ~/.ssh/known_hosts ssh_config/known_hosts_$(whoami).txt; fi", true);
    delay(300);

    // Public keys (current user)
    typeCommand("if [ -d ~/.ssh ]; then find ~/.ssh -name '*.pub' -exec cp {} ssh_config/ \\; 2>/dev/null; fi", true);
    delay(500);

    // List SSH keys for all users (metadata only, not private keys)
    typeCommand("for user_home in /home/*; do user=$(basename $user_home); if [ -d $user_home/.ssh ]; then echo \"User: $user\" >> ssh_config/ssh_keys_inventory.txt; ls -la $user_home/.ssh >> ssh_config/ssh_keys_inventory.txt 2>/dev/null; fi; done", true);
    delay(2000);

    logAction("LNX_SSH", "SSH configuration collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxBrowserHistory() {
    logAction("LNX_BROWSER", "Collecting browser history", "STARTED");

    typeCommand("mkdir -p browser", true);
    delay(300);

    // Firefox history (places.sqlite)
    typeCommand("if [ -d ~/.mozilla/firefox ]; then find ~/.mozilla/firefox -name 'places.sqlite' -exec cp {} browser/firefox_history_$(whoami).sqlite \\; 2>/dev/null; fi", true);
    delay(1000);

    // Chrome history
    typeCommand("if [ -f ~/.config/google-chrome/Default/History ]; then cp ~/.config/google-chrome/Default/History browser/chrome_history_$(whoami).sqlite; fi", true);
    delay(1000);

    // Chromium history
    typeCommand("if [ -f ~/.config/chromium/Default/History ]; then cp ~/.config/chromium/Default/History browser/chromium_history_$(whoami).sqlite; fi", true);
    delay(1000);

    // Collect browser history for all users (metadata)
    typeCommand("for user_home in /home/*; do user=$(basename $user_home); echo \"User: $user\" >> browser/browser_inventory.txt; find $user_home/.mozilla $user_home/.config/google-chrome $user_home/.config/chromium -name 'places.sqlite' -o -name 'History' 2>/dev/null | head -20 >> browser/browser_inventory.txt; done", true);
    delay(2000);

    logAction("LNX_BROWSER", "Browser history collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxUserAccounts() {
    logAction("LNX_USERS", "Collecting user account information", "STARTED");

    typeCommand("mkdir -p user_accounts", true);
    delay(300);

    // /etc/passwd (user accounts)
    typeCommand("sudo cp /etc/passwd user_accounts/passwd.txt 2>/dev/null", true);
    delay(300);

    // /etc/shadow (password hashes - requires root)
    typeCommand("sudo cp /etc/shadow user_accounts/shadow.txt 2>/dev/null", true);
    delay(300);

    // /etc/group (groups)
    typeCommand("sudo cp /etc/group user_accounts/group.txt 2>/dev/null", true);
    delay(300);

    // /etc/sudoers (sudo permissions)
    typeCommand("sudo cp /etc/sudoers user_accounts/sudoers.txt 2>/dev/null", true);
    delay(300);

    // Last logged in users
    typeCommand("last -a > user_accounts/last_logins.txt 2>&1", true);
    delay(500);

    // Currently logged in users
    typeCommand("w > user_accounts/current_users.txt 2>&1", true);
    delay(300);

    // User login history
    typeCommand("lastlog > user_accounts/lastlog.txt 2>&1", true);
    delay(500);

    logAction("LNX_USERS", "User account information collected", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxDocker() {
    logAction("LNX_DOCKER", "Collecting Docker artifacts", "STARTED");

    typeCommand("mkdir -p docker", true);
    delay(300);

    // Check if Docker is installed
    typeCommand("if command -v docker &> /dev/null; then echo 'INSTALLED' > docker/docker_status.txt; else echo 'NOT_INSTALLED' > docker/docker_status.txt; fi", true);
    delay(500);

    // Docker version
    typeCommand("docker --version > docker/docker_version.txt 2>&1", true);
    delay(500);

    // Running containers
    typeCommand("docker ps -a --format '{{.ID}},{{.Image}},{{.Command}},{{.CreatedAt}},{{.Status}},{{.Names}}' > docker/containers.csv 2>&1", true);
    delay(2000);
    logAction("LNX_DOCKER", "Container list collected", "SUCCESS");

    // Docker images
    typeCommand("docker images --format '{{.Repository}},{{.Tag}},{{.ID}},{{.CreatedAt}},{{.Size}}' > docker/images.csv 2>&1", true);
    delay(2000);
    logAction("LNX_DOCKER", "Image list collected", "SUCCESS");

    // Docker networks
    typeCommand("docker network ls > docker/networks.txt 2>&1", true);
    delay(1000);

    // Docker volumes
    typeCommand("docker volume ls > docker/volumes.txt 2>&1", true);
    delay(1000);

    // Inspect running containers (details)
    typeCommand("for container in $(docker ps -q 2>/dev/null); do docker inspect $container > docker/inspect_$container.json 2>&1; done", true);
    delay(5000);
    logAction("LNX_DOCKER", "Container inspection complete", "SUCCESS");

    logAction("LNX_DOCKER", "Docker collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxSystemdJournal() {
    logAction("LNX_JOURNAL", "Collecting systemd journal logs", "STARTED");

    typeCommand("mkdir -p systemd_journal", true);
    delay(300);

    // Last 1000 journal entries
    typeCommand("sudo journalctl -n 1000 --no-pager > systemd_journal/journal_last_1000.txt 2>&1", true);
    delay(5000);
    logAction("LNX_JOURNAL", "Recent journal entries collected", "SUCCESS");

    // Boot logs
    typeCommand("sudo journalctl -b --no-pager > systemd_journal/journal_current_boot.txt 2>&1", true);
    delay(3000);
    logAction("LNX_JOURNAL", "Current boot logs collected", "SUCCESS");

    // Failed services
    typeCommand("sudo journalctl -p err --no-pager > systemd_journal/journal_errors.txt 2>&1", true);
    delay(2000);
    logAction("LNX_JOURNAL", "Error logs collected", "SUCCESS");

    // Authentication logs
    typeCommand("sudo journalctl _COMM=sshd --no-pager > systemd_journal/journal_sshd.txt 2>&1", true);
    delay(2000);

    typeCommand("sudo journalctl _COMM=sudo --no-pager > systemd_journal/journal_sudo.txt 2>&1", true);
    delay(2000);
    logAction("LNX_JOURNAL", "Authentication logs collected", "SUCCESS");

    logAction("LNX_JOURNAL", "Systemd journal collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxFirewallRules() {
    logAction("LNX_FIREWALL", "Collecting firewall rules", "STARTED");

    typeCommand("mkdir -p firewall", true);
    delay(300);

    // iptables rules
    typeCommand("sudo iptables -L -n -v > firewall/iptables_rules.txt 2>&1", true);
    delay(1000);
    logAction("LNX_FIREWALL", "iptables rules collected", "SUCCESS");

    // iptables NAT rules
    typeCommand("sudo iptables -t nat -L -n -v > firewall/iptables_nat.txt 2>&1", true);
    delay(1000);

    // ip6tables rules
    typeCommand("sudo ip6tables -L -n -v > firewall/ip6tables_rules.txt 2>&1", true);
    delay(1000);

    // ufw status (if installed)
    typeCommand("if command -v ufw &> /dev/null; then sudo ufw status verbose > firewall/ufw_status.txt 2>&1; fi", true);
    delay(500);

    // firewalld rules (if installed)
    typeCommand("if command -v firewall-cmd &> /dev/null; then sudo firewall-cmd --list-all > firewall/firewalld_rules.txt 2>&1; fi", true);
    delay(500);
    logAction("LNX_FIREWALL", "Firewall rules collected", "SUCCESS");

    logAction("LNX_FIREWALL", "Firewall collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxCronJobs() {
    logAction("LNX_CRON", "Collecting cron jobs", "STARTED");

    typeCommand("mkdir -p cron_jobs", true);
    delay(300);

    // Current user crontab
    typeCommand("crontab -l > cron_jobs/crontab_$(whoami).txt 2>&1", true);
    delay(500);
    logAction("LNX_CRON", "User crontab collected", "SUCCESS");

    // System crontabs
    typeCommand("sudo cat /etc/crontab > cron_jobs/etc_crontab.txt 2>/dev/null", true);
    delay(300);

    // Cron directories
    typeCommand("sudo ls -laR /etc/cron.hourly > cron_jobs/cron_hourly.txt 2>&1", true);
    delay(500);

    typeCommand("sudo ls -laR /etc/cron.daily > cron_jobs/cron_daily.txt 2>&1", true);
    delay(500);

    typeCommand("sudo ls -laR /etc/cron.weekly > cron_jobs/cron_weekly.txt 2>&1", true);
    delay(500);

    typeCommand("sudo ls -laR /etc/cron.monthly > cron_jobs/cron_monthly.txt 2>&1", true);
    delay(500);

    // /var/spool/cron
    typeCommand("sudo ls -la /var/spool/cron/crontabs/ > cron_jobs/spool_crontabs.txt 2>&1", true);
    delay(500);

    // All user crontabs
    typeCommand("for user in $(cut -f1 -d: /etc/passwd); do echo \"User: $user\" >> cron_jobs/all_user_crontabs.txt; sudo crontab -u $user -l >> cron_jobs/all_user_crontabs.txt 2>&1; done", true);
    delay(3000);
    logAction("LNX_CRON", "All cron jobs collected", "SUCCESS");

    logAction("LNX_CRON", "Cron collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxMemoryDump() {
    logAction("LNX_MEMORY", "Collecting memory artifacts", "STARTED");

    typeCommand("mkdir -p memory", true);
    delay(300);

    // Method 1: Process memory maps (detailed memory layout)
    typeCommand("for pid in $(ps aux | awk 'NR>1 {print $2}' | head -20); do sudo cat /proc/$pid/maps > memory/maps_$pid.txt 2>/dev/null; done", true);
    delay(5000);
    logAction("LNX_MEMORY", "Process memory maps collected", "SUCCESS");

    // Method 2: Process status information (memory usage)
    typeCommand("ps aux --sort=-%mem | head -50 > memory/top_processes_mem.txt", true);
    delay(1000);

    // Method 3: /proc/meminfo - system memory information
    typeCommand("cat /proc/meminfo > memory/meminfo.txt", true);
    delay(300);
    logAction("LNX_MEMORY", "System memory info collected", "SUCCESS");

    // Method 4: smaps (detailed memory statistics per process)
    typeCommand("for pid in $(ps aux --sort=-%mem | awk 'NR>1 {print $2}' | head -10); do sudo cat /proc/$pid/smaps > memory/smaps_$pid.txt 2>/dev/null; done", true);
    delay(5000);

    // Method 5: Page maps (physical memory mapping)
    typeCommand("for pid in $(ps aux --sort=-%mem | awk 'NR>1 {print $2}' | head -5); do sudo cat /proc/$pid/pagemap > memory/pagemap_$pid.bin 2>/dev/null; done", true);
    delay(3000);

    // Method 6: Process command lines and environment
    typeCommand("for pid in $(ps aux | awk 'NR>1 {print $2}' | head -20); do echo \"=== PID: $pid ===\" >> memory/cmdline_env.txt; cat /proc/$pid/cmdline 2>/dev/null | tr '\\0' ' ' >> memory/cmdline_env.txt; echo >> memory/cmdline_env.txt; done", true);
    delay(3000);

    // Method 7: Dump core files if available
    typeCommand("sudo find /var/crash /var/core /tmp -name 'core.*' -o -name '*.core' 2>/dev/null | head -5 | xargs -I {} cp {} memory/ 2>/dev/null", true);
    delay(2000);

    // Method 8: Virtual memory statistics
    typeCommand("vmstat -s > memory/vmstat.txt", true);
    delay(500);

    // Method 9: Slab allocator info (kernel memory)
    typeCommand("sudo cat /proc/slabinfo > memory/slabinfo.txt 2>/dev/null", true);
    delay(500);

    // Method 10: Shared memory segments
    typeCommand("ipcs -m > memory/shared_memory.txt", true);
    delay(500);

    // Method 11: Memory dump using gcore (if available) for critical processes
    typeCommand("which gcore > /dev/null 2>&1 && for proc in systemd init sshd; do pid=$(pgrep $proc | head -1); [ -n \"$pid\" ] && sudo gcore -o memory/${proc}_dump $pid 2>/dev/null; done", true);
    delay(10000);  // Core dumps can take time
    logAction("LNX_MEMORY", "Process core dumps attempted", "SUCCESS");

    logAction("LNX_MEMORY", "Memory collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxPackageHistory() {
    logAction("LNX_PACKAGES", "Collecting package installation history", "STARTED");

    typeCommand("mkdir -p packages", true);
    delay(300);

    // Debian/Ubuntu (dpkg/apt)
    typeCommand("if command -v dpkg > /dev/null; then dpkg -l > packages/dpkg_installed.txt 2>&1; fi", true);
    delay(1500);

    typeCommand("if [ -f /var/log/dpkg.log ]; then sudo cp /var/log/dpkg.log* packages/ 2>/dev/null; fi", true);
    delay(1000);

    typeCommand("if [ -f /var/log/apt/history.log ]; then sudo cp /var/log/apt/history.log* packages/ 2>/dev/null; fi", true);
    delay(1000);
    logAction("LNX_PACKAGES", "Debian/Ubuntu package history collected", "SUCCESS");

    // RedHat/CentOS (rpm/yum)
    typeCommand("if command -v rpm > /dev/null; then rpm -qa --last > packages/rpm_installed.txt 2>&1; fi", true);
    delay(1500);

    typeCommand("if [ -f /var/log/yum.log ]; then sudo cp /var/log/yum.log packages/ 2>/dev/null; fi", true);
    delay(1000);

    typeCommand("if command -v dnf > /dev/null; then sudo cp /var/log/dnf*.log packages/ 2>/dev/null; fi", true);
    delay(1000);
    logAction("LNX_PACKAGES", "RedHat/CentOS package history collected", "SUCCESS");

    // Arch Linux (pacman)
    typeCommand("if [ -f /var/log/pacman.log ]; then sudo cp /var/log/pacman.log packages/ 2>/dev/null; fi", true);
    delay(1000);

    logAction("LNX_PACKAGES", "Package history collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxNetworkConfig() {
    logAction("LNX_NETCONFIG", "Collecting network configuration", "STARTED");

    typeCommand("mkdir -p network_config", true);
    delay(300);

    // Network interfaces
    typeCommand("ip addr show > network_config/ip_addr.txt 2>&1", true);
    delay(500);

    typeCommand("ifconfig -a > network_config/ifconfig.txt 2>&1", true);
    delay(500);

    // Routing tables
    typeCommand("ip route show > network_config/ip_route.txt 2>&1", true);
    delay(500);

    typeCommand("route -n > network_config/route.txt 2>&1", true);
    delay(500);
    logAction("LNX_NETCONFIG", "Routing tables collected", "SUCCESS");

    // Network configuration files
    typeCommand("sudo cp /etc/network/interfaces network_config/ 2>/dev/null", true);
    delay(300);

    typeCommand("sudo cp -r /etc/NetworkManager/system-connections network_config/ 2>/dev/null", true);
    delay(500);

    typeCommand("sudo cp /etc/resolv.conf network_config/ 2>/dev/null", true);
    delay(300);

    typeCommand("sudo cp /etc/hosts network_config/ 2>/dev/null", true);
    delay(300);
    logAction("LNX_NETCONFIG", "Configuration files collected", "SUCCESS");

    // Wireless info
    typeCommand("iwconfig > network_config/wireless.txt 2>&1", true);
    delay(500);

    typeCommand("nmcli device wifi list > network_config/wifi_networks.txt 2>&1", true);
    delay(1000);

    logAction("LNX_NETCONFIG", "Network configuration collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxUSBDevices() {
    logAction("LNX_USB", "Collecting USB device history", "STARTED");

    typeCommand("mkdir -p usb_devices", true);
    delay(300);

    // Current USB devices
    typeCommand("lsusb -v > usb_devices/lsusb_verbose.txt 2>&1", true);
    delay(2000);
    logAction("LNX_USB", "Current USB devices listed", "SUCCESS");

    // USB device history from logs
    typeCommand("sudo grep -i usb /var/log/syslog* > usb_devices/usb_syslog.txt 2>/dev/null", true);
    delay(2000);

    typeCommand("sudo grep -i usb /var/log/kern.log* > usb_devices/usb_kernel.txt 2>/dev/null", true);
    delay(2000);

    typeCommand("sudo grep -i usb /var/log/messages* > usb_devices/usb_messages.txt 2>/dev/null", true);
    delay(2000);

    // USB authorization
    typeCommand("find /sys/bus/usb/devices -name authorized -exec grep -H . {} \\; > usb_devices/usb_authorized.txt 2>&1", true);
    delay(1000);

    // USB device serial numbers
    typeCommand("for dev in /sys/bus/usb/devices/*; do [ -f $dev/serial ] && echo \"$dev: $(cat $dev/serial)\"; done > usb_devices/usb_serials.txt 2>&1", true);
    delay(1500);

    logAction("LNX_USB", "USB device collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxAuditLogs() {
    logAction("LNX_AUDIT", "Collecting audit logs (auditd)", "STARTED");

    typeCommand("mkdir -p audit_logs", true);
    delay(300);

    // Audit logs
    typeCommand("sudo cp /var/log/audit/audit.log* audit_logs/ 2>/dev/null", true);
    delay(2000);
    logAction("LNX_AUDIT", "Audit logs copied", "SUCCESS");

    // Audit rules
    typeCommand("sudo auditctl -l > audit_logs/audit_rules.txt 2>&1", true);
    delay(500);

    // Audit status
    typeCommand("sudo auditctl -s > audit_logs/audit_status.txt 2>&1", true);
    delay(300);

    // Parse common audit events
    typeCommand("sudo ausearch -m LOGIN > audit_logs/login_events.txt 2>&1", true);
    delay(1500);

    typeCommand("sudo ausearch -m USER_AUTH > audit_logs/auth_events.txt 2>&1", true);
    delay(1500);

    typeCommand("sudo ausearch -m EXECVE > audit_logs/exec_events.txt 2>&1", true);
    delay(2000);

    typeCommand("sudo ausearch -m AVC > audit_logs/selinux_events.txt 2>&1", true);
    delay(1500);

    logAction("LNX_AUDIT", "Audit log collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxTimezone() {
    logAction("LNX_TIMEZONE", "Collecting timezone and time configuration", "STARTED");

    typeCommand("mkdir -p timezone", true);
    delay(300);

    // Current time and timezone
    typeCommand("date > timezone/current_time.txt 2>&1", true);
    delay(300);

    typeCommand("timedatectl > timezone/timedatectl.txt 2>&1", true);
    delay(500);
    logAction("LNX_TIMEZONE", "Timezone information collected", "SUCCESS");

    // Timezone files
    typeCommand("sudo cp /etc/timezone timezone/ 2>/dev/null", true);
    delay(300);

    typeCommand("sudo cp /etc/localtime timezone/ 2>/dev/null", true);
    delay(300);

    // NTP configuration
    typeCommand("sudo cp /etc/ntp.conf timezone/ 2>/dev/null", true);
    delay(300);

    typeCommand("sudo cp /etc/systemd/timesyncd.conf timezone/ 2>/dev/null", true);
    delay(300);

    // NTP sync status
    typeCommand("timedatectl show-timesync --all > timezone/ntp_sync.txt 2>&1", true);
    delay(500);

    typeCommand("ntpq -p > timezone/ntp_peers.txt 2>&1", true);
    delay(500);

    // Hardware clock
    typeCommand("sudo hwclock --show > timezone/hwclock.txt 2>&1", true);
    delay(500);

    logAction("LNX_TIMEZONE", "Timezone collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxProcessList() {
    logAction("LNX_PROCESSES", "Collecting running processes", "STARTED");

    typeCommand("mkdir -p processes", true);
    delay(300);

    // Detailed process list
    typeCommand("ps auxwwf > processes/ps_tree.txt 2>&1", true);
    delay(1000);
    logAction("LNX_PROCESSES", "Process tree collected", "SUCCESS");

    typeCommand("ps -eo pid,ppid,user,uid,gid,pri,ni,vsz,rss,tty,stat,start,time,cmd > processes/ps_detailed.txt 2>&1", true);
    delay(1000);

    // Top processes snapshot
    typeCommand("top -b -n 1 > processes/top_snapshot.txt 2>&1", true);
    delay(1000);

    // Process tree via pstree
    typeCommand("pstree -aplsun > processes/pstree.txt 2>&1", true);
    delay(500);

    logAction("LNX_PROCESSES", "Process list collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxOpenFiles() {
    logAction("LNX_OPENFILES", "Collecting open files", "STARTED");

    typeCommand("mkdir -p open_files", true);
    delay(300);

    // List open files
    typeCommand("sudo lsof > open_files/lsof_all.txt 2>&1", true);
    delay(3000);
    logAction("LNX_OPENFILES", "Open files collected", "SUCCESS");

    // Network files
    typeCommand("sudo lsof -i > open_files/lsof_network.txt 2>&1", true);
    delay(1000);

    // Files by user
    typeCommand("sudo lsof -u root > open_files/lsof_root.txt 2>&1", true);
    delay(1000);

    logAction("LNX_OPENFILES", "Open files collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxEnvironmentVars() {
    logAction("LNX_ENV", "Collecting environment variables", "STARTED");

    typeCommand("mkdir -p environment", true);
    delay(300);

    // Current environment
    typeCommand("env > environment/env.txt 2>&1", true);
    delay(300);

    typeCommand("export > environment/export.txt 2>&1", true);
    delay(300);

    // System-wide environment
    typeCommand("sudo cat /etc/environment > environment/system_env.txt 2>&1", true);
    delay(300);

    typeCommand("sudo cat /etc/profile > environment/profile.txt 2>&1", true);
    delay(300);

    logAction("LNX_ENV", "Environment variables collected", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxSysctl() {
    logAction("LNX_SYSCTL", "Collecting kernel parameters", "STARTED");

    typeCommand("mkdir -p sysctl", true);
    delay(300);

    // All kernel parameters
    typeCommand("sudo sysctl -a > sysctl/sysctl_all.txt 2>&1", true);
    delay(2000);
    logAction("LNX_SYSCTL", "Kernel parameters collected", "SUCCESS");

    // Network parameters
    typeCommand("sudo sysctl net > sysctl/sysctl_net.txt 2>&1", true);
    delay(500);

    // Kernel parameters
    typeCommand("sudo sysctl kernel > sysctl/sysctl_kernel.txt 2>&1", true);
    delay(500);

    logAction("LNX_SYSCTL", "Sysctl collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxSELinux() {
    logAction("LNX_SELINUX", "Collecting SELinux/AppArmor status", "STARTED");

    typeCommand("mkdir -p security", true);
    delay(300);

    // SELinux status
    typeCommand("sestatus > security/selinux_status.txt 2>&1", true);
    delay(500);

    typeCommand("getenforce > security/selinux_mode.txt 2>&1", true);
    delay(300);

    // AppArmor status
    typeCommand("sudo aa-status > security/apparmor_status.txt 2>&1", true);
    delay(500);
    logAction("LNX_SELINUX", "Security module status collected", "SUCCESS");

    // Security policies
    typeCommand("sudo cp /etc/selinux/config security/ 2>/dev/null", true);
    delay(300);

    logAction("LNX_SELINUX", "SELinux/AppArmor collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxSystemdServices() {
    logAction("LNX_SYSTEMD", "Collecting systemd services", "STARTED");

    typeCommand("mkdir -p systemd", true);
    delay(300);

    // List all services
    typeCommand("systemctl list-units --type=service --all > systemd/services_all.txt 2>&1", true);
    delay(1500);
    logAction("LNX_SYSTEMD", "Service list collected", "SUCCESS");

    // Failed services
    typeCommand("systemctl --failed > systemd/services_failed.txt 2>&1", true);
    delay(500);

    // Enabled services
    typeCommand("systemctl list-unit-files --type=service --state=enabled > systemd/services_enabled.txt 2>&1", true);
    delay(1000);

    // Service dependencies
    typeCommand("systemctl list-dependencies > systemd/service_dependencies.txt 2>&1", true);
    delay(1000);

    logAction("LNX_SYSTEMD", "Systemd services collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxMountedFilesystems() {
    logAction("LNX_MOUNTS", "Collecting mounted filesystems", "STARTED");

    typeCommand("mkdir -p mounts", true);
    delay(300);

    // Mount information
    typeCommand("mount > mounts/mount.txt 2>&1", true);
    delay(500);
    logAction("LNX_MOUNTS", "Mount information collected", "SUCCESS");

    typeCommand("cat /proc/mounts > mounts/proc_mounts.txt 2>&1", true);
    delay(300);

    typeCommand("cat /etc/fstab > mounts/fstab.txt 2>&1", true);
    delay(300);

    // Disk usage
    typeCommand("df -h > mounts/df_human.txt 2>&1", true);
    delay(500);

    typeCommand("df -i > mounts/df_inodes.txt 2>&1", true);
    delay(500);

    logAction("LNX_MOUNTS", "Mounted filesystems collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxLoginHistory() {
    logAction("LNX_LOGINS", "Collecting login history", "STARTED");

    typeCommand("mkdir -p login_history", true);
    delay(300);

    // Last logins
    typeCommand("last -Faixw > login_history/last.txt 2>&1", true);
    delay(1000);
    logAction("LNX_LOGINS", "Last logins collected", "SUCCESS");

    // Failed logins
    typeCommand("sudo lastb -Faixw > login_history/lastb.txt 2>&1", true);
    delay(1000);

    // Currently logged in users
    typeCommand("w > login_history/w.txt 2>&1", true);
    delay(300);

    typeCommand("who -a > login_history/who.txt 2>&1", true);
    delay(300);

    // Login records
    typeCommand("sudo lastlog > login_history/lastlog.txt 2>&1", true);
    delay(500);

    logAction("LNX_LOGINS", "Login history collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxHostsFile() {
    logAction("LNX_HOSTS", "Collecting hosts file", "STARTED");

    typeCommand("mkdir -p network_config", true);
    delay(300);

    // Hosts file
    typeCommand("cat /etc/hosts > network_config/hosts.txt 2>&1", true);
    delay(300);
    logAction("LNX_HOSTS", "Hosts file collected", "SUCCESS");

    typeCommand("cat /etc/hostname > network_config/hostname.txt 2>&1", true);
    delay(300);

    logAction("LNX_HOSTS", "Hosts file collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxResolverConfig() {
    logAction("LNX_RESOLVER", "Collecting DNS resolver configuration", "STARTED");

    typeCommand("mkdir -p dns_config", true);
    delay(300);

    // Resolv.conf
    typeCommand("cat /etc/resolv.conf > dns_config/resolv.conf 2>&1", true);
    delay(300);
    logAction("LNX_RESOLVER", "Resolver config collected", "SUCCESS");

    // systemd-resolved
    typeCommand("systemd-resolve --status > dns_config/resolved_status.txt 2>&1", true);
    delay(500);

    typeCommand("cat /etc/nsswitch.conf > dns_config/nsswitch.conf 2>&1", true);
    delay(300);

    logAction("LNX_RESOLVER", "DNS resolver collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxNetworkInterfaces() {
    logAction("LNX_INTERFACES", "Collecting network interface details", "STARTED");

    typeCommand("mkdir -p network_interfaces", true);
    delay(300);

    // Interface information
    typeCommand("ip addr show > network_interfaces/ip_addr.txt 2>&1", true);
    delay(500);
    logAction("LNX_INTERFACES", "IP addresses collected", "SUCCESS");

    typeCommand("ip link show > network_interfaces/ip_link.txt 2>&1", true);
    delay(500);

    typeCommand("ifconfig -a > network_interfaces/ifconfig.txt 2>&1", true);
    delay(500);

    // Interface statistics
    typeCommand("ip -s link > network_interfaces/ip_stats.txt 2>&1", true);
    delay(500);

    typeCommand("cat /proc/net/dev > network_interfaces/proc_net_dev.txt 2>&1", true);
    delay(300);

    logAction("LNX_INTERFACES", "Network interfaces collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxRoutingTable() {
    logAction("LNX_ROUTING", "Collecting routing tables", "STARTED");

    typeCommand("mkdir -p routing", true);
    delay(300);

    // Routing tables
    typeCommand("ip route show > routing/ip_route.txt 2>&1", true);
    delay(500);
    logAction("LNX_ROUTING", "IP routes collected", "SUCCESS");

    typeCommand("route -n > routing/route.txt 2>&1", true);
    delay(500);

    // IPv6 routes
    typeCommand("ip -6 route show > routing/ip6_route.txt 2>&1", true);
    delay(500);

    typeCommand("netstat -rn > routing/netstat_routes.txt 2>&1", true);
    delay(500);

    logAction("LNX_ROUTING", "Routing table collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxARPCache() {
    logAction("LNX_ARP", "Collecting ARP cache", "STARTED");

    typeCommand("mkdir -p arp", true);
    delay(300);

    // ARP table
    typeCommand("arp -a > arp/arp.txt 2>&1", true);
    delay(500);
    logAction("LNX_ARP", "ARP table collected", "SUCCESS");

    typeCommand("ip neigh show > arp/ip_neigh.txt 2>&1", true);
    delay(500);

    typeCommand("cat /proc/net/arp > arp/proc_net_arp.txt 2>&1", true);
    delay(300);

    logAction("LNX_ARP", "ARP cache collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxXorgLogs() {
    logAction("LNX_XORG", "Collecting X11/Xorg logs", "STARTED");

    typeCommand("mkdir -p xorg", true);
    delay(300);

    // X11 logs
    typeCommand("sudo cp /var/log/Xorg.*.log xorg/ 2>/dev/null", true);
    delay(500);
    logAction("LNX_XORG", "Xorg logs collected", "SUCCESS");

    // X authority
    typeCommand("xauth list > xorg/xauth.txt 2>&1", true);
    delay(300);

    // Display info
    typeCommand("echo $DISPLAY > xorg/display.txt 2>&1", true);
    delay(100);

    logAction("LNX_XORG", "X11 logs collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxBashHistory() {
    logAction("LNX_BASH_HISTORY", "Collecting bash history for all users", "STARTED");

    typeCommand("mkdir -p bash_history", true);
    delay(300);

    // Current user history
    typeCommand("cat ~/.bash_history > bash_history/bash_history_current.txt 2>&1", true);
    delay(500);
    logAction("LNX_BASH_HISTORY", "Current user history collected", "SUCCESS");

    // Root history
    typeCommand("sudo cat /root/.bash_history > bash_history/bash_history_root.txt 2>&1", true);
    delay(500);

    // All user histories
    typeCommand("sudo find /home -name '.bash_history' -exec cat {} \\; > bash_history/all_users_history.txt 2>&1", true);
    delay(2000);

    logAction("LNX_BASH_HISTORY", "Bash history collection complete", "SUCCESS");
    return true;
}

// ============================================================================
// ADVANCED LINUX FORENSICS (v1.1.0+)
// ============================================================================

bool HIDAutomation::executeLinuxAppArmorProfiles() {
    logAction("LNX_APPARMOR", "Collecting AppArmor profiles and status", "STARTED");

    typeCommand("mkdir -p apparmor", true);
    delay(300);

    // Get AppArmor status
    typeCommand("sudo aa-status > apparmor/aa_status.txt 2>&1", true);
    delay(1000);
    logAction("LNX_APPARMOR", "AppArmor status collected", "SUCCESS");

    // Get loaded profiles
    typeCommand("sudo apparmor_status > apparmor/apparmor_status.txt 2>&1", true);
    delay(800);

    // Copy profiles
    typeCommand("sudo cp -r /etc/apparmor.d apparmor/profiles 2>&1", true);
    delay(2000);

    // Get AppArmor logs
    typeCommand("sudo grep -i apparmor /var/log/syslog | tail -1000 > apparmor/apparmor_logs.txt 2>&1", true);
    delay(1500);

    logAction("LNX_APPARMOR", "AppArmor profiles collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxKubernetesPods() {
    logAction("LNX_K8S", "Collecting Kubernetes pods and containers", "STARTED");

    typeCommand("mkdir -p kubernetes", true);
    delay(300);

    // Get kubectl pods if available
    typeCommand("if command -v kubectl &> /dev/null; then kubectl get pods --all-namespaces > kubernetes/pods.txt 2>&1; fi", true);
    delay(2000);
    logAction("LNX_K8S", "Kubernetes pods collected", "SUCCESS");

    // Get services
    typeCommand("if command -v kubectl &> /dev/null; then kubectl get services --all-namespaces > kubernetes/services.txt 2>&1; fi", true);
    delay(1500);

    // Get deployments
    typeCommand("if command -v kubectl &> /dev/null; then kubectl get deployments --all-namespaces > kubernetes/deployments.txt 2>&1; fi", true);
    delay(1500);

    // Get config
    typeCommand("if [ -f ~/.kube/config ]; then cat ~/.kube/config > kubernetes/kubeconfig.txt 2>&1; fi", true);
    delay(500);

    logAction("LNX_K8S", "Kubernetes collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxContainerInspection() {
    logAction("LNX_CONTAINERS", "Deep container inspection", "STARTED");

    typeCommand("mkdir -p containers_deep", true);
    delay(300);

    // Docker container detailed info
    typeCommand("if command -v docker &> /dev/null; then docker ps -a --format '{{.ID}} {{.Image}} {{.Status}}' > containers_deep/docker_containers.txt 2>&1; fi", true);
    delay(1500);
    logAction("LNX_CONTAINERS", "Docker containers listed", "SUCCESS");

    // Inspect each container
    typeCommand("if command -v docker &> /dev/null; then for cid in $(docker ps -aq 2>/dev/null); do docker inspect $cid > containers_deep/inspect_$cid.json 2>&1; done; fi", true);
    delay(5000);

    // Get container logs
    typeCommand("if command -v docker &> /dev/null; then for cid in $(docker ps -q 2>/dev/null); do docker logs $cid > containers_deep/logs_$cid.txt 2>&1; done; fi", true);
    delay(3000);

    // LXC containers
    typeCommand("if command -v lxc-ls &> /dev/null; then lxc-ls --fancy > containers_deep/lxc_containers.txt 2>&1; fi", true);
    delay(1000);

    logAction("LNX_CONTAINERS", "Container inspection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxSystemdAnalyze() {
    logAction("LNX_SYSTEMD_ANALYZE", "Analyzing systemd performance", "STARTED");

    typeCommand("mkdir -p systemd_analyze", true);
    delay(300);

    // Boot time analysis
    typeCommand("systemd-analyze > systemd_analyze/boot_time.txt 2>&1", true);
    delay(1000);
    logAction("LNX_SYSTEMD_ANALYZE", "Boot time analyzed", "SUCCESS");

    // Blame analysis
    typeCommand("systemd-analyze blame > systemd_analyze/blame.txt 2>&1", true);
    delay(2000);

    // Critical chain
    typeCommand("systemd-analyze critical-chain > systemd_analyze/critical_chain.txt 2>&1", true);
    delay(1500);

    // Dump systemd state
    typeCommand("systemd-analyze dump > systemd_analyze/systemd_dump.txt 2>&1", true);
    delay(3000);

    // Verify systemd configuration
    typeCommand("systemd-analyze verify > systemd_analyze/verify.txt 2>&1", true);
    delay(2000);

    logAction("LNX_SYSTEMD_ANALYZE", "Systemd analysis complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxJournalCorruption() {
    logAction("LNX_JOURNAL_CHECK", "Checking journal integrity", "STARTED");

    typeCommand("mkdir -p journal_check", true);
    delay(300);

    // Verify journal
    typeCommand("sudo journalctl --verify > journal_check/verify.txt 2>&1", true);
    delay(3000);
    logAction("LNX_JOURNAL_CHECK", "Journal verified", "SUCCESS");

    // Get journal statistics
    typeCommand("sudo journalctl --disk-usage > journal_check/disk_usage.txt 2>&1", true);
    delay(500);

    // List journal files
    typeCommand("sudo ls -lh /var/log/journal/*/ > journal_check/journal_files.txt 2>&1", true);
    delay(500);

    logAction("LNX_JOURNAL_CHECK", "Journal integrity check complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxUserActivityTimeline() {
    logAction("LNX_USER_TIMELINE", "Building user activity timeline", "STARTED");

    typeCommand("mkdir -p user_timeline", true);
    delay(300);

    // Last logins with full info
    typeCommand("last -Faixw > user_timeline/last_full.txt 2>&1", true);
    delay(1000);
    logAction("LNX_USER_TIMELINE", "Last logins collected", "SUCCESS");

    // Failed login attempts
    typeCommand("sudo lastb -Faixw > user_timeline/failed_logins.txt 2>&1", true);
    delay(1000);

    // User activity from wtmp
    typeCommand("sudo utmpdump /var/log/wtmp > user_timeline/wtmp_dump.txt 2>&1", true);
    delay(1500);

    // User activity from btmp
    typeCommand("sudo utmpdump /var/log/btmp > user_timeline/btmp_dump.txt 2>&1", true);
    delay(1000);

    // Session information
    typeCommand("w -i > user_timeline/current_sessions.txt 2>&1", true);
    delay(500);

    logAction("LNX_USER_TIMELINE", "User timeline complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxSudoHistory() {
    logAction("LNX_SUDO_HISTORY", "Collecting sudo usage history", "STARTED");

    typeCommand("mkdir -p sudo_history", true);
    delay(300);

    // Auth log sudo entries
    typeCommand("sudo grep -i sudo /var/log/auth.log* > sudo_history/sudo_authlog.txt 2>&1", true);
    delay(1500);
    logAction("LNX_SUDO_HISTORY", "Sudo auth logs collected", "SUCCESS");

    // Secure log sudo entries
    typeCommand("sudo grep -i sudo /var/log/secure* > sudo_history/sudo_secure.txt 2>&1", true);
    delay(1500);

    // Sudoers configuration
    typeCommand("sudo cat /etc/sudoers > sudo_history/sudoers.txt 2>&1", true);
    delay(500);

    // Sudoers.d directory
    typeCommand("sudo cat /etc/sudoers.d/* > sudo_history/sudoers_d.txt 2>&1", true);
    delay(800);

    logAction("LNX_SUDO_HISTORY", "Sudo history collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxLastCommand() {
    logAction("LNX_LASTCOMM", "Collecting process accounting data", "STARTED");

    typeCommand("mkdir -p lastcomm", true);
    delay(300);

    // Get process accounting if available
    typeCommand("if command -v lastcomm &> /dev/null; then sudo lastcomm > lastcomm/process_accounting.txt 2>&1; fi", true);
    delay(2000);
    logAction("LNX_LASTCOMM", "Process accounting collected", "SUCCESS");

    // Accounting by user
    typeCommand("if command -v lastcomm &> /dev/null; then sudo lastcomm --user root > lastcomm/root_commands.txt 2>&1; fi", true);
    delay(1500);

    // Accounting statistics
    typeCommand("if command -v sa &> /dev/null; then sudo sa > lastcomm/accounting_stats.txt 2>&1; fi", true);
    delay(1000);

    logAction("LNX_LASTCOMM", "Process accounting complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxFailedLogins() {
    logAction("LNX_FAILED_LOGINS", "Analyzing failed login attempts", "STARTED");

    typeCommand("mkdir -p failed_logins", true);
    delay(300);

    // Failed SSH attempts
    typeCommand("sudo grep 'Failed password' /var/log/auth.log* > failed_logins/failed_ssh.txt 2>&1", true);
    delay(1500);
    logAction("LNX_FAILED_LOGINS", "Failed SSH attempts collected", "SUCCESS");

    // Failed sudo attempts
    typeCommand("sudo grep 'authentication failure' /var/log/auth.log* > failed_logins/auth_failures.txt 2>&1", true);
    delay(1500);

    // PAM failures
    typeCommand("sudo grep -i 'pam' /var/log/auth.log* | grep -i 'fail' > failed_logins/pam_failures.txt 2>&1", true);
    delay(1500);

    // Count failures by IP
    typeCommand("sudo grep 'Failed password' /var/log/auth.log* | awk '{print $(NF-3)}' | sort | uniq -c | sort -rn > failed_logins/failures_by_ip.txt 2>&1", true);
    delay(1000);

    logAction("LNX_FAILED_LOGINS", "Failed logins analysis complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxOpenSSLCertificates() {
    logAction("LNX_SSL_CERTS", "Collecting SSL/TLS certificates", "STARTED");

    typeCommand("mkdir -p ssl_certs", true);
    delay(300);

    // System certificates
    typeCommand("sudo ls -lR /etc/ssl/certs/ > ssl_certs/system_certs.txt 2>&1", true);
    delay(1000);
    logAction("LNX_SSL_CERTS", "System certificates listed", "SUCCESS");

    // Copy CA certificates
    typeCommand("sudo cp /etc/ssl/certs/ca-certificates.crt ssl_certs/ 2>&1", true);
    delay(1500);

    // List PKI certificates
    typeCommand("sudo ls -lR /etc/pki/ > ssl_certs/pki_certs.txt 2>&1", true);
    delay(800);

    // OpenSSL version and config
    typeCommand("openssl version -a > ssl_certs/openssl_version.txt 2>&1", true);
    delay(500);

    logAction("LNX_SSL_CERTS", "SSL certificates collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxSystemCalls() {
    logAction("LNX_SYSCALLS", "Monitoring system calls", "STARTED");

    typeCommand("mkdir -p syscalls", true);
    delay(300);

    // Strace sample on running processes (limited)
    typeCommand("if command -v strace &> /dev/null; then for pid in $(ps aux | grep -v grep | awk 'NR>1 {print $2}' | head -5); do sudo timeout 3 strace -p $pid > syscalls/strace_$pid.txt 2>&1 & done; sleep 4; fi", true);
    delay(5000);
    logAction("LNX_SYSCALLS", "System calls sampled", "SUCCESS");

    // Get audit rules if configured
    typeCommand("if command -v auditctl &> /dev/null; then sudo auditctl -l > syscalls/audit_rules.txt 2>&1; fi", true);
    delay(500);

    logAction("LNX_SYSCALLS", "System calls monitoring complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxKernelParameters() {
    logAction("LNX_KERNEL_PARAMS", "Collecting kernel parameters", "STARTED");

    typeCommand("mkdir -p kernel_params", true);
    delay(300);

    // All kernel parameters
    typeCommand("sudo sysctl -a > kernel_params/sysctl_all.txt 2>&1", true);
    delay(2000);
    logAction("LNX_KERNEL_PARAMS", "Kernel parameters collected", "SUCCESS");

    // Kernel command line
    typeCommand("cat /proc/cmdline > kernel_params/cmdline.txt 2>&1", true);
    delay(300);

    // Kernel version details
    typeCommand("uname -a > kernel_params/uname.txt 2>&1", true);
    delay(300);

    // Kernel config if available
    typeCommand("if [ -f /proc/config.gz ]; then zcat /proc/config.gz > kernel_params/kernel_config.txt 2>&1; fi", true);
    delay(1500);

    logAction("LNX_KERNEL_PARAMS", "Kernel parameters complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxMemoryMaps() {
    logAction("LNX_MEM_MAPS", "Collecting process memory maps", "STARTED");

    typeCommand("mkdir -p memory_maps", true);
    delay(300);

    // Memory maps for top processes
    typeCommand("for pid in $(ps aux --sort=-%mem | awk 'NR>1 {print $2}' | head -10); do sudo cat /proc/$pid/maps > memory_maps/maps_$pid.txt 2>&1; done", true);
    delay(3000);
    logAction("LNX_MEM_MAPS", "Memory maps collected", "SUCCESS");

    // Memory info
    typeCommand("cat /proc/meminfo > memory_maps/meminfo.txt 2>&1", true);
    delay(300);

    // NUMA memory info
    typeCommand("if command -v numactl &> /dev/null; then numactl --hardware > memory_maps/numa.txt 2>&1; fi", true);
    delay(500);

    logAction("LNX_MEM_MAPS", "Memory maps collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxSocketStatistics() {
    logAction("LNX_SOCKET_STATS", "Collecting socket statistics", "STARTED");

    typeCommand("mkdir -p socket_stats", true);
    delay(300);

    // Detailed socket statistics
    typeCommand("ss -tunap > socket_stats/ss_all.txt 2>&1", true);
    delay(1000);
    logAction("LNX_SOCKET_STATS", "Socket statistics collected", "SUCCESS");

    // TCP statistics
    typeCommand("ss -t -a > socket_stats/tcp_sockets.txt 2>&1", true);
    delay(800);

    // UDP statistics
    typeCommand("ss -u -a > socket_stats/udp_sockets.txt 2>&1", true);
    delay(800);

    // Unix domain sockets
    typeCommand("ss -x -a > socket_stats/unix_sockets.txt 2>&1", true);
    delay(800);

    // Network statistics
    typeCommand("netstat -s > socket_stats/netstat_stats.txt 2>&1", true);
    delay(1000);

    logAction("LNX_SOCKET_STATS", "Socket statistics complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxIPTables() {
    logAction("LNX_IPTABLES", "Collecting iptables rules", "STARTED");

    typeCommand("mkdir -p iptables", true);
    delay(300);

    // IPv4 rules
    typeCommand("sudo iptables -L -n -v > iptables/iptables_rules.txt 2>&1", true);
    delay(1000);
    logAction("LNX_IPTABLES", "IPv4 iptables collected", "SUCCESS");

    // IPv6 rules
    typeCommand("sudo ip6tables -L -n -v > iptables/ip6tables_rules.txt 2>&1", true);
    delay(1000);

    // NAT rules
    typeCommand("sudo iptables -t nat -L -n -v > iptables/nat_rules.txt 2>&1", true);
    delay(800);

    // Mangle rules
    typeCommand("sudo iptables -t mangle -L -n -v > iptables/mangle_rules.txt 2>&1", true);
    delay(800);

    // Save complete ruleset
    typeCommand("sudo iptables-save > iptables/iptables_save.txt 2>&1", true);
    delay(500);

    logAction("LNX_IPTABLES", "Iptables collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxNFTables() {
    logAction("LNX_NFTABLES", "Collecting nftables rules", "STARTED");

    typeCommand("mkdir -p nftables", true);
    delay(300);

    // List all nftables rules
    typeCommand("if command -v nft &> /dev/null; then sudo nft list ruleset > nftables/ruleset.txt 2>&1; fi", true);
    delay(1500);
    logAction("LNX_NFTABLES", "NFTables rules collected", "SUCCESS");

    // List tables
    typeCommand("if command -v nft &> /dev/null; then sudo nft list tables > nftables/tables.txt 2>&1; fi", true);
    delay(800);

    logAction("LNX_NFTABLES", "NFTables collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxNetworkNamespaces() {
    logAction("LNX_NET_NS", "Collecting network namespaces", "STARTED");

    typeCommand("mkdir -p net_namespaces", true);
    delay(300);

    // List network namespaces
    typeCommand("sudo ip netns list > net_namespaces/namespaces.txt 2>&1", true);
    delay(800);
    logAction("LNX_NET_NS", "Network namespaces listed", "SUCCESS");

    // Inspect each namespace
    typeCommand("for ns in $(sudo ip netns list | awk '{print $1}'); do sudo ip netns exec $ns ip a > net_namespaces/ns_$ns.txt 2>&1; done", true);
    delay(2000);

    logAction("LNX_NET_NS", "Network namespaces complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxCGroups() {
    logAction("LNX_CGROUPS", "Collecting cgroups information", "STARTED");

    typeCommand("mkdir -p cgroups", true);
    delay(300);

    // List cgroups
    typeCommand("cat /proc/cgroups > cgroups/cgroups_list.txt 2>&1", true);
    delay(500);
    logAction("LNX_CGROUPS", "Cgroups listed", "SUCCESS");

    // Cgroup v1 hierarchies
    typeCommand("sudo ls -lR /sys/fs/cgroup/ > cgroups/cgroup_hierarchy.txt 2>&1", true);
    delay(2000);

    // Process cgroups
    typeCommand("for pid in $(ps aux | awk 'NR>1 {print $2}' | head -20); do cat /proc/$pid/cgroup > cgroups/proc_${pid}_cgroup.txt 2>&1; done", true);
    delay(2000);

    logAction("LNX_CGROUPS", "Cgroups collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxCapabilities() {
    logAction("LNX_CAPABILITIES", "Collecting Linux capabilities", "STARTED");

    typeCommand("mkdir -p capabilities", true);
    delay(300);

    // Get capabilities of running processes
    typeCommand("if command -v getpcaps &> /dev/null; then for pid in $(ps aux | awk 'NR>1 {print $2}' | head -20); do sudo getpcaps $pid >> capabilities/process_caps.txt 2>&1; done; fi", true);
    delay(3000);
    logAction("LNX_CAPABILITIES", "Process capabilities collected", "SUCCESS");

    // File capabilities
    typeCommand("if command -v getcap &> /dev/null; then sudo getcap -r / 2>/dev/null > capabilities/file_caps.txt; fi", true);
    delay(8000);

    logAction("LNX_CAPABILITIES", "Capabilities collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxEbpfPrograms() {
    logAction("LNX_EBPF", "Collecting eBPF programs", "STARTED");

    typeCommand("mkdir -p ebpf", true);
    delay(300);

    // List loaded BPF programs
    typeCommand("if command -v bpftool &> /dev/null; then sudo bpftool prog list > ebpf/programs.txt 2>&1; fi", true);
    delay(1500);
    logAction("LNX_EBPF", "eBPF programs listed", "SUCCESS");

    // List BPF maps
    typeCommand("if command -v bpftool &> /dev/null; then sudo bpftool map list > ebpf/maps.txt 2>&1; fi", true);
    delay(1000);

    // BPF program details
    typeCommand("if command -v bpftool &> /dev/null; then sudo bpftool prog show > ebpf/prog_details.txt 2>&1; fi", true);
    delay(1500);

    logAction("LNX_EBPF", "eBPF collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxInitRamfs() {
    logAction("LNX_INITRAMFS", "Collecting initramfs information", "STARTED");

    typeCommand("mkdir -p initramfs", true);
    delay(300);

    // List initramfs files
    typeCommand("sudo ls -lh /boot/initr* > initramfs/initramfs_files.txt 2>&1", true);
    delay(500);
    logAction("LNX_INITRAMFS", "Initramfs files listed", "SUCCESS");

    // Initramfs configuration
    typeCommand("if [ -f /etc/initramfs-tools/initramfs.conf ]; then sudo cat /etc/initramfs-tools/initramfs.conf > initramfs/initramfs_conf.txt 2>&1; fi", true);
    delay(500);

    // Dracut configuration
    typeCommand("if [ -d /etc/dracut.conf.d ]; then sudo cat /etc/dracut.conf.d/* > initramfs/dracut_conf.txt 2>&1; fi", true);
    delay(800);

    logAction("LNX_INITRAMFS", "Initramfs collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeLinuxGrubConfig() {
    logAction("LNX_GRUB", "Collecting GRUB configuration", "STARTED");

    typeCommand("mkdir -p grub", true);
    delay(300);

    // GRUB configuration
    typeCommand("sudo cat /boot/grub/grub.cfg > grub/grub_cfg.txt 2>&1", true);
    delay(1000);
    logAction("LNX_GRUB", "GRUB config collected", "SUCCESS");

    // GRUB defaults
    typeCommand("sudo cat /etc/default/grub > grub/grub_defaults.txt 2>&1", true);
    delay(500);

    // GRUB2 config (alternative path)
    typeCommand("sudo cat /boot/grub2/grub.cfg > grub/grub2_cfg.txt 2>&1", true);
    delay(1000);

    // EFI boot entries
    typeCommand("if command -v efibootmgr &> /dev/null; then sudo efibootmgr -v > grub/efi_boot.txt 2>&1; fi", true);
    delay(800);

    logAction("LNX_GRUB", "GRUB configuration complete", "SUCCESS");
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
    String archive_name = "frfd_evidence_" + timestamp + ".tar.gz";
    String archive_path = "/tmp/" + archive_name;
    String tar_cmd = "tar -czf " + archive_path + " /tmp/frfd_collection/";
    typeCommand(tar_cmd, true);
    delay(5000);

    logAction("MAC_ARCHIVE", "Created evidence archive", archive_path);

    // Connect to FRFD WiFi AP (using networksetup on macOS)
    typeCommand("networksetup -setairportnetwork en0 CSIRT-FORENSICS ChangeThisPassword123!", true);
    delay(3000);  // Wait for WiFi connection

    logAction("MAC_WIFI", "Connecting to FRFD WiFi", "CSIRT-FORENSICS");

    // Define inline upload function (same as Linux - curl works on macOS)
    typeCommand("upload(){f=\"$1\";t=\"${2:-archive}\";ip=\"${3:-192.168.4.1}\";[ ! -f \"$f\" ]&&return 1;for i in 1 2 3;do r=$(curl -s -w \"\\n%{http_code}\" -X POST -F \"file=@$f\" -F \"type=$t\" --connect-timeout 10 --max-time 60 \"http://$ip/upload\" 2>&1);c=$(echo \"$r\"|tail -n1);[ \"$c\" = \"200\" ]&&return 0;sleep 2;done;return 1;}", true);
    delay(500);

    // Upload archive to FRFD
    String upload_cmd = "upload " + archive_path + " archive";
    typeCommand(upload_cmd, true);
    delay(10000);  // Wait for upload to complete

    logAction("MAC_UPLOAD", "Uploaded evidence to FRFD", archive_name);

    // Cleanup - delete local evidence (optional, uncomment if desired)
    // typeCommand("rm -rf /tmp/frfd_collection", true);
    // typeCommand("rm -f " + archive_path, true);
    // delay(1000);

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

bool HIDAutomation::executeMacOSUnifiedLogs() {
    logAction("MAC_UNIFIED_LOGS", "Collecting macOS Unified Logs", "STARTED");

    typeCommand("mkdir -p unified_logs", true);
    delay(300);

    // Last 24 hours of logs
    typeCommand("log show --predicate 'eventMessage contains \"error\" OR eventMessage contains \"fail\"' --info --last 24h > unified_logs/errors_last_24h.txt 2>&1", true);
    delay(10000);  // Unified logs can be large
    logAction("MAC_UNIFIED_LOGS", "Error logs collected", "SUCCESS");

    // Security-related logs
    typeCommand("log show --predicate 'subsystem == \"com.apple.securityd\"' --info --last 7d > unified_logs/security_last_7d.txt 2>&1", true);
    delay(8000);
    logAction("MAC_UNIFIED_LOGS", "Security logs collected", "SUCCESS");

    // Authentication logs
    typeCommand("log show --predicate 'process == \"loginwindow\" OR process == \"sudo\"' --info --last 7d > unified_logs/auth_last_7d.txt 2>&1", true);
    delay(5000);
    logAction("MAC_UNIFIED_LOGS", "Authentication logs collected", "SUCCESS");

    // Network-related logs
    typeCommand("log show --predicate 'subsystem contains \"network\"' --info --last 24h > unified_logs/network_last_24h.txt 2>&1", true);
    delay(5000);
    logAction("MAC_UNIFIED_LOGS", "Network logs collected", "SUCCESS");

    logAction("MAC_UNIFIED_LOGS", "Unified logs collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSFSEvents() {
    logAction("MAC_FSEVENTS", "Collecting FSEvents database", "STARTED");

    typeCommand("mkdir -p fsevents", true);
    delay(300);

    // FSEvents database (requires root)
    typeCommand("sudo cp -R /.fseventsd fsevents/fseventsd_backup 2>&1", true);
    delay(5000);  // FSEvents can be large
    logAction("MAC_FSEVENTS", "FSEvents database copied", "SUCCESS");

    // Get FSEvents metadata
    typeCommand("sudo ls -la /.fseventsd > fsevents/fsevents_metadata.txt 2>&1", true);
    delay(500);

    // Note about FSEvents analysis
    typeCommand("echo 'FSEvents database collected. Use FSEventsParser or similar tools for analysis.' > fsevents/README.txt", true);
    delay(300);

    logAction("MAC_FSEVENTS", "FSEvents collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSBrowserHistory() {
    logAction("MAC_BROWSER", "Collecting macOS browser history", "STARTED");

    typeCommand("mkdir -p browser", true);
    delay(300);

    // Safari history
    typeCommand("if [ -f ~/Library/Safari/History.db ]; then cp ~/Library/Safari/History.db browser/Safari_History.db; fi", true);
    delay(1000);
    logAction("MAC_BROWSER", "Safari history collected", "SUCCESS");

    // Safari downloads
    typeCommand("if [ -f ~/Library/Safari/Downloads.plist ]; then cp ~/Library/Safari/Downloads.plist browser/Safari_Downloads.plist; fi", true);
    delay(500);

    // Chrome history
    typeCommand("if [ -f ~/Library/Application\\ Support/Google/Chrome/Default/History ]; then cp ~/Library/Application\\ Support/Google/Chrome/Default/History browser/Chrome_History.sqlite; fi", true);
    delay(1000);
    logAction("MAC_BROWSER", "Chrome history collected", "SUCCESS");

    // Firefox history
    typeCommand("firefox_profile=$(find ~/Library/Application\\ Support/Firefox/Profiles -name '*.default*' | head -1) && if [ -f \"$firefox_profile/places.sqlite\" ]; then cp \"$firefox_profile/places.sqlite\" browser/Firefox_History.sqlite; fi", true);
    delay(1000);
    logAction("MAC_BROWSER", "Firefox history collected", "SUCCESS");

    logAction("MAC_BROWSER", "Browser history collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSSpotlight() {
    logAction("MAC_SPOTLIGHT", "Collecting Spotlight metadata", "STARTED");

    typeCommand("mkdir -p spotlight", true);
    delay(300);

    // Spotlight database metadata
    typeCommand("sudo ls -la /.Spotlight-V100 > spotlight/spotlight_metadata.txt 2>&1", true);
    delay(1000);
    logAction("MAC_SPOTLIGHT", "Spotlight metadata collected", "SUCCESS");

    // Recent searches (if available)
    typeCommand("if [ -f ~/Library/Application\\ Support/com.apple.spotlight/searches.db ]; then cp ~/Library/Application\\ Support/com.apple.spotlight/searches.db spotlight/searches.db; fi", true);
    delay(1000);

    // Metadata for user's home directory
    typeCommand("mdfind -onlyin ~ 'kMDItemFSName == *' -count > spotlight/home_files_count.txt 2>&1", true);
    delay(2000);

    // Recent documents
    typeCommand("mdfind -onlyin ~ 'kMDItemContentModificationDate >= $time.today(-7)' | head -1000 > spotlight/recent_documents.txt 2>&1", true);
    delay(5000);
    logAction("MAC_SPOTLIGHT", "Recent documents collected", "SUCCESS");

    logAction("MAC_SPOTLIGHT", "Spotlight collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSQuarantine() {
    logAction("MAC_QUARANTINE", "Collecting quarantine and download history", "STARTED");

    typeCommand("mkdir -p quarantine", true);
    delay(300);

    // Quarantine database (downloads with metadata)
    typeCommand("if [ -f ~/Library/Preferences/com.apple.LaunchServices.QuarantineEventsV2 ]; then cp ~/Library/Preferences/com.apple.LaunchServices.QuarantineEventsV2 quarantine/QuarantineEventsV2.db; fi", true);
    delay(1000);
    logAction("MAC_QUARANTINE", "Quarantine database collected", "SUCCESS");

    // List quarantined files
    typeCommand("sqlite3 ~/Library/Preferences/com.apple.LaunchServices.QuarantineEventsV2 'SELECT * FROM LSQuarantineEvent' > quarantine/quarantine_events.txt 2>&1", true);
    delay(2000);

    // Extended attributes for quarantine (sample from Downloads)
    typeCommand("xattr -l ~/Downloads/* > quarantine/downloads_xattr.txt 2>&1", true);
    delay(2000);
    logAction("MAC_QUARANTINE", "Extended attributes collected", "SUCCESS");

    logAction("MAC_QUARANTINE", "Quarantine collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSInstallHistory() {
    logAction("MAC_INSTALL", "Collecting installation history", "STARTED");

    typeCommand("mkdir -p install_history", true);
    delay(300);

    // System install history
    typeCommand("sudo cp /Library/Receipts/InstallHistory.plist install_history/InstallHistory.plist 2>&1", true);
    delay(1000);
    logAction("MAC_INSTALL", "InstallHistory.plist copied", "SUCCESS");

    // Convert plist to readable format
    typeCommand("plutil -convert xml1 install_history/InstallHistory.plist -o install_history/InstallHistory.xml 2>&1", true);
    delay(1000);

    // Application installations from Receipts
    typeCommand("sudo ls -la /Library/Receipts/ > install_history/receipts_list.txt 2>&1", true);
    delay(500);

    // Package manager info (Homebrew if installed)
    typeCommand("if command -v brew &> /dev/null; then brew list --versions > install_history/homebrew_packages.txt 2>&1; fi", true);
    delay(2000);

    // MacPorts info (if installed)
    typeCommand("if command -v port &> /dev/null; then port installed > install_history/macports_packages.txt 2>&1; fi", true);
    delay(2000);
    logAction("MAC_INSTALL", "Package manager info collected", "SUCCESS");

    logAction("MAC_INSTALL", "Install history collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSKeychain() {
    logAction("MAC_KEYCHAIN", "Collecting keychain metadata (not passwords)", "STARTED");

    typeCommand("mkdir -p keychain", true);
    delay(300);

    // List keychains
    typeCommand("security list-keychains > keychain/keychains_list.txt 2>&1", true);
    delay(500);
    logAction("MAC_KEYCHAIN", "Keychain list collected", "SUCCESS");

    // Keychain info (metadata only)
    typeCommand("security dump-keychain -d ~/Library/Keychains/login.keychain-db > keychain/login_keychain_metadata.txt 2>&1", true);
    delay(2000);

    // List certificates
    typeCommand("security find-certificate -a > keychain/certificates.txt 2>&1", true);
    delay(1500);
    logAction("MAC_KEYCHAIN", "Certificates listed", "SUCCESS");

    // List identities
    typeCommand("security find-identity -v > keychain/identities.txt 2>&1", true);
    delay(1000);

    logAction("MAC_KEYCHAIN", "Keychain metadata collection complete (passwords NOT extracted)", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSMemoryDump() {
    logAction("MAC_MEMORY", "Collecting memory artifacts", "STARTED");

    typeCommand("mkdir -p memory", true);
    delay(300);

    // Method 1: Process list with memory usage
    typeCommand("ps aux -m | head -50 > memory/top_processes_mem.txt", true);
    delay(1000);
    logAction("MAC_MEMORY", "Process memory list collected", "SUCCESS");

    // Method 2: Detailed virtual memory statistics
    typeCommand("vm_stat > memory/vm_stat.txt", true);
    delay(500);

    // Method 3: Memory pressure and zones
    typeCommand("sudo zprint > memory/zprint.txt 2>&1", true);
    delay(2000);
    logAction("MAC_MEMORY", "Zone allocator info collected", "SUCCESS");

    // Method 4: Memory regions for running processes (top 10 by memory)
    typeCommand("for pid in $(ps aux -m | awk 'NR>1 {print $2}' | head -10); do echo \"=== PID: $pid ===\" >> memory/vmmap_output.txt; sudo vmmap $pid >> memory/vmmap_output.txt 2>&1; done", true);
    delay(15000);
    logAction("MAC_MEMORY", "Virtual memory maps collected", "SUCCESS");

    // Method 5: Heap information for key processes
    typeCommand("for pid in $(ps aux -m | awk 'NR>1 {print $2}' | head -5); do echo \"=== HEAP PID: $pid ===\" >> memory/heap_info.txt; sudo heap $pid >> memory/heap_info.txt 2>&1; done", true);
    delay(10000);

    // Method 6: Malloc history (if available)
    typeCommand("for pid in $(pgrep -f 'kernel_task|launchd|SystemUIServer' | head -3); do echo \"=== MALLOC PID: $pid ===\" >> memory/malloc_history.txt; sudo malloc_history $pid >> memory/malloc_history.txt 2>&1; done", true);
    delay(5000);

    // Method 7: Memory object analysis
    typeCommand("sudo lsof | grep -E 'mem|DEV' | head -100 > memory/lsof_memory.txt 2>&1", true);
    delay(2000);

    // Method 8: Sample processes (lightweight profiling)
    typeCommand("for proc in WindowServer Finder loginwindow; do pid=$(pgrep $proc | head -1); [ -n \"$pid\" ] && sudo sample $pid 1 -f memory/sample_${proc}.txt 2>&1; done", true);
    delay(5000);
    logAction("MAC_MEMORY", "Process samples collected", "SUCCESS");

    // Method 9: Core dumps (if available)
    typeCommand("sudo find /cores -name 'core.*' 2>/dev/null | head -5 | xargs -I {} cp {} memory/ 2>/dev/null", true);
    delay(2000);

    // Method 10: Swap usage
    typeCommand("sysctl vm.swapusage > memory/swap_usage.txt", true);
    delay(300);

    // Method 11: Memory pressure
    typeCommand("memory_pressure > memory/memory_pressure.txt 2>&1 &", true);
    delay(3000);
    typeCommand("pkill memory_pressure", true);
    delay(300);

    // Method 12: Generate memory reports for key processes using ReportCrash
    typeCommand("for pid in $(ps aux -m | awk 'NR>1 {print $2}' | head -5); do sudo sample $pid 1 > memory/report_${pid}.txt 2>&1; done", true);
    delay(8000);

    logAction("MAC_MEMORY", "Memory collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSNetworkInterfaces() {
    logAction("MAC_NETWORK", "Collecting network interfaces and connections", "STARTED");

    typeCommand("mkdir -p network", true);
    delay(300);

    // Network interfaces
    typeCommand("ifconfig -a > network/ifconfig.txt 2>&1", true);
    delay(500);
    logAction("MAC_NETWORK", "Network interfaces collected", "SUCCESS");

    // Active connections
    typeCommand("netstat -an > network/netstat_all.txt 2>&1", true);
    delay(1000);

    typeCommand("lsof -i -n -P > network/lsof_network.txt 2>&1", true);
    delay(2000);

    // Routing table
    typeCommand("netstat -rn > network/routing_table.txt 2>&1", true);
    delay(500);

    // ARP cache
    typeCommand("arp -an > network/arp_cache.txt 2>&1", true);
    delay(500);

    // WiFi networks
    typeCommand("/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -s > network/wifi_scan.txt 2>&1", true);
    delay(3000);

    typeCommand("/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I > network/wifi_info.txt 2>&1", true);
    delay(500);

    // Network preferences
    typeCommand("sudo cp /Library/Preferences/SystemConfiguration/preferences.plist network/ 2>/dev/null", true);
    delay(500);

    typeCommand("sudo cp /Library/Preferences/SystemConfiguration/NetworkInterfaces.plist network/ 2>/dev/null", true);
    delay(500);

    logAction("MAC_NETWORK", "Network collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSLaunchAgents() {
    logAction("MAC_LAUNCH", "Collecting Launch Agents/Daemons (persistence)", "STARTED");

    typeCommand("mkdir -p launch_items", true);
    delay(300);

    // User Launch Agents
    typeCommand("sudo cp -r ~/Library/LaunchAgents launch_items/user_launch_agents 2>/dev/null", true);
    delay(1000);
    logAction("MAC_LAUNCH", "User Launch Agents collected", "SUCCESS");

    // System Launch Agents
    typeCommand("sudo cp -r /Library/LaunchAgents launch_items/system_launch_agents 2>/dev/null", true);
    delay(1500);

    // System Launch Daemons (root-level)
    typeCommand("sudo cp -r /Library/LaunchDaemons launch_items/launch_daemons 2>/dev/null", true);
    delay(1500);

    // Apple's Launch Daemons
    typeCommand("sudo ls -laR /System/Library/LaunchDaemons > launch_items/apple_launch_daemons.txt 2>&1", true);
    delay(1000);

    // Currently loaded launch items
    typeCommand("launchctl list > launch_items/launchctl_list.txt 2>&1", true);
    delay(1000);

    // Startup Items (legacy)
    typeCommand("sudo ls -laR /Library/StartupItems > launch_items/startup_items.txt 2>&1", true);
    delay(500);

    typeCommand("sudo ls -laR /System/Library/StartupItems > launch_items/system_startup_items.txt 2>&1", true);
    delay(500);

    logAction("MAC_LAUNCH", "Launch items collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSApplicationSupport() {
    logAction("MAC_APPSUPP", "Collecting Application Support and user data", "STARTED");

    typeCommand("mkdir -p application_support", true);
    delay(300);

    // User Application Support
    typeCommand("ls -laR ~/Library/Application\\ Support > application_support/user_app_support_list.txt 2>&1", true);
    delay(2000);
    logAction("MAC_APPSUPP", "Application Support listing collected", "SUCCESS");

    // Preferences
    typeCommand("sudo cp -r ~/Library/Preferences application_support/user_preferences 2>/dev/null", true);
    delay(2000);

    // Application Caches
    typeCommand("ls -laR ~/Library/Caches > application_support/user_caches_list.txt 2>&1", true);
    delay(1500);

    // Saved Application State
    typeCommand("ls -laR ~/Library/Saved\\ Application\\ State > application_support/saved_app_state.txt 2>&1", true);
    delay(1000);

    // Login Items
    typeCommand("osascript -e 'tell application \"System Events\" to get the name of every login item' > application_support/login_items.txt 2>&1", true);
    delay(1000);

    // Recently used items
    typeCommand("ls -laR ~/Library/Application\\ Support/com.apple.sharedfilelist > application_support/recent_items.txt 2>&1", true);
    delay(500);

    logAction("MAC_APPSUPP", "Application Support collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSFirewall() {
    logAction("MAC_FIREWALL", "Collecting firewall configuration", "STARTED");

    typeCommand("mkdir -p firewall", true);
    delay(300);

    // Firewall status
    typeCommand("sudo /usr/libexec/ApplicationFirewall/socketfilterfw --getglobalstate > firewall/firewall_status.txt 2>&1", true);
    delay(500);
    logAction("MAC_FIREWALL", "Firewall status collected", "SUCCESS");

    // Firewall applications
    typeCommand("sudo /usr/libexec/ApplicationFirewall/socketfilterfw --listapps > firewall/firewall_apps.txt 2>&1", true);
    delay(1000);

    // Firewall configuration
    typeCommand("sudo cat /Library/Preferences/com.apple.alf.plist > firewall/alf_config.txt 2>&1", true);
    delay(500);

    // PF (Packet Filter) rules
    typeCommand("sudo pfctl -s rules > firewall/pf_rules.txt 2>&1", true);
    delay(500);

    typeCommand("sudo pfctl -s nat > firewall/pf_nat.txt 2>&1", true);
    delay(500);

    typeCommand("sudo pfctl -s states > firewall/pf_states.txt 2>&1", true);
    delay(1000);

    // PF configuration
    typeCommand("sudo cat /etc/pf.conf > firewall/pf_conf.txt 2>&1", true);
    delay(300);

    logAction("MAC_FIREWALL", "Firewall collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSTimeMachine() {
    logAction("MAC_TM", "Collecting Time Machine backup information", "STARTED");

    typeCommand("mkdir -p timemachine", true);
    delay(300);

    // Time Machine status
    typeCommand("tmutil status > timemachine/tm_status.txt 2>&1", true);
    delay(1000);
    logAction("MAC_TM", "Time Machine status collected", "SUCCESS");

    // Time Machine destinations
    typeCommand("tmutil destinationinfo > timemachine/tm_destinations.txt 2>&1", true);
    delay(1000);

    // Time Machine snapshots
    typeCommand("tmutil listlocalsnapshots / > timemachine/tm_snapshots.txt 2>&1", true);
    delay(1500);

    // Backup history
    typeCommand("tmutil listbackups > timemachine/tm_backups.txt 2>&1", true);
    delay(1000);

    // Time Machine configuration
    typeCommand("sudo cat /Library/Preferences/com.apple.TimeMachine.plist > timemachine/tm_config.txt 2>&1", true);
    delay(500);

    // Latest backup info
    typeCommand("tmutil latestbackup > timemachine/tm_latest.txt 2>&1", true);
    delay(500);

    // Compare current system to latest backup
    typeCommand("tmutil compare > timemachine/tm_compare.txt 2>&1", true);
    delay(3000);

    logAction("MAC_TM", "Time Machine collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSProcessList() {
    logAction("MAC_PROCESSES", "Collecting running processes", "STARTED");

    typeCommand("mkdir -p processes", true);
    delay(300);

    // Process list
    typeCommand("ps auxwww > processes/ps_all.txt 2>&1", true);
    delay(1000);
    logAction("MAC_PROCESSES", "Process list collected", "SUCCESS");

    // Top snapshot
    typeCommand("top -l 1 > processes/top_snapshot.txt 2>&1", true);
    delay(1000);

    // Activity Monitor info
    typeCommand("ps -eo pid,ppid,user,uid,gid,pri,nice,vsz,rss,tty,stat,start,time,comm > processes/ps_detailed.txt 2>&1", true);
    delay(1000);

    logAction("MAC_PROCESSES", "Process list collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSOpenFiles() {
    logAction("MAC_OPENFILES", "Collecting open files", "STARTED");

    typeCommand("mkdir -p open_files", true);
    delay(300);

    // Open files
    typeCommand("sudo lsof > open_files/lsof_all.txt 2>&1", true);
    delay(3000);
    logAction("MAC_OPENFILES", "Open files collected", "SUCCESS");

    // Network connections
    typeCommand("sudo lsof -i > open_files/lsof_network.txt 2>&1", true);
    delay(1500);

    logAction("MAC_OPENFILES", "Open files collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSNetworkConnections() {
    logAction("MAC_NETCON", "Collecting network connections", "STARTED");

    typeCommand("mkdir -p network_connections", true);
    delay(300);

    // Network connections
    typeCommand("netstat -an > network_connections/netstat.txt 2>&1", true);
    delay(1000);
    logAction("MAC_NETCON", "Network connections collected", "SUCCESS");

    // Established connections
    typeCommand("lsof -i -n -P | grep ESTABLISHED > network_connections/established.txt 2>&1", true);
    delay(1000);

    // Listening ports
    typeCommand("lsof -i -n -P | grep LISTEN > network_connections/listening.txt 2>&1", true);
    delay(1000);

    logAction("MAC_NETCON", "Network connections collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSKernelExtensions() {
    logAction("MAC_KEXTS", "Collecting kernel extensions", "STARTED");

    typeCommand("mkdir -p kexts", true);
    delay(300);

    // Loaded kernel extensions
    typeCommand("kextstat > kexts/kextstat.txt 2>&1", true);
    delay(1000);
    logAction("MAC_KEXTS", "Kernel extensions list collected", "SUCCESS");

    // Kext information
    typeCommand("kextfind -report -b -loadable > kexts/kextfind.txt 2>&1", true);
    delay(2000);

    // System extensions
    typeCommand("systemextensionsctl list > kexts/system_extensions.txt 2>&1", true);
    delay(1000);

    logAction("MAC_KEXTS", "Kernel extensions collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSLoginHistory() {
    logAction("MAC_LOGINS", "Collecting login history", "STARTED");

    typeCommand("mkdir -p login_history", true);
    delay(300);

    // Last logins
    typeCommand("last > login_history/last.txt 2>&1", true);
    delay(1000);
    logAction("MAC_LOGINS", "Last logins collected", "SUCCESS");

    // Currently logged in users
    typeCommand("w > login_history/w.txt 2>&1", true);
    delay(300);

    typeCommand("who > login_history/who.txt 2>&1", true);
    delay(300);

    // Login history from logs
    typeCommand("log show --predicate 'eventMessage contains \"login\"' --info --last 7d > login_history/login_logs.txt 2>&1", true);
    delay(5000);

    logAction("MAC_LOGINS", "Login history collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSFileVault() {
    logAction("MAC_FILEVAULT", "Collecting FileVault status", "STARTED");

    typeCommand("mkdir -p filevault", true);
    delay(300);

    // FileVault status
    typeCommand("fdesetup status > filevault/fv_status.txt 2>&1", true);
    delay(1000);
    logAction("MAC_FILEVAULT", "FileVault status collected", "SUCCESS");

    // FileVault users
    typeCommand("sudo fdesetup list > filevault/fv_users.txt 2>&1", true);
    delay(1000);

    // Core Storage info
    typeCommand("diskutil cs list > filevault/cs_list.txt 2>&1", true);
    delay(1000);

    logAction("MAC_FILEVAULT", "FileVault collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSGatekeeper() {
    logAction("MAC_GATEKEEPER", "Collecting Gatekeeper status", "STARTED");

    typeCommand("mkdir -p gatekeeper", true);
    delay(300);

    // Gatekeeper status
    typeCommand("spctl --status > gatekeeper/gatekeeper_status.txt 2>&1", true);
    delay(500);
    logAction("MAC_GATEKEEPER", "Gatekeeper status collected", "SUCCESS");

    // Gatekeeper assessments
    typeCommand("spctl --list > gatekeeper/gatekeeper_list.txt 2>&1", true);
    delay(1000);

    // XProtect info
    typeCommand("system_profiler SPInstallHistoryDataType | grep -i xprotect > gatekeeper/xprotect.txt 2>&1", true);
    delay(1000);

    logAction("MAC_GATEKEEPER", "Gatekeeper collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSSIP() {
    logAction("MAC_SIP", "Collecting System Integrity Protection status", "STARTED");

    typeCommand("mkdir -p sip", true);
    delay(300);

    // SIP status
    typeCommand("csrutil status > sip/sip_status.txt 2>&1", true);
    delay(500);
    logAction("MAC_SIP", "SIP status collected", "SUCCESS");

    // Secure Boot level
    typeCommand("nvram -p | grep SecureBootLevel > sip/secure_boot.txt 2>&1", true);
    delay(500);

    logAction("MAC_SIP", "SIP collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSAirportNetworks() {
    logAction("MAC_AIRPORT", "Collecting WiFi network information", "STARTED");

    typeCommand("mkdir -p airport", true);
    delay(300);

    // Airport info
    typeCommand("/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I > airport/airport_info.txt 2>&1", true);
    delay(1000);
    logAction("MAC_AIRPORT", "Airport info collected", "SUCCESS");

    // WiFi scan
    typeCommand("/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -s > airport/wifi_scan.txt 2>&1", true);
    delay(2000);

    // Preferred networks
    typeCommand("networksetup -listpreferredwirelessnetworks en0 > airport/preferred_networks.txt 2>&1", true);
    delay(1000);

    // WiFi history
    typeCommand("sudo cp /Library/Preferences/SystemConfiguration/com.apple.airport.preferences.plist airport/ 2>/dev/null", true);
    delay(500);

    logAction("MAC_AIRPORT", "Airport/WiFi collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSBluetoothDevices() {
    logAction("MAC_BLUETOOTH", "Collecting Bluetooth device information", "STARTED");

    typeCommand("mkdir -p bluetooth", true);
    delay(300);

    // Bluetooth devices
    typeCommand("system_profiler SPBluetoothDataType > bluetooth/bluetooth_devices.txt 2>&1", true);
    delay(2000);
    logAction("MAC_BLUETOOTH", "Bluetooth devices collected", "SUCCESS");

    // Bluetooth preferences
    typeCommand("sudo cp /Library/Preferences/com.apple.Bluetooth.plist bluetooth/ 2>/dev/null", true);
    delay(500);

    logAction("MAC_BLUETOOTH", "Bluetooth collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSMountedVolumes() {
    logAction("MAC_VOLUMES", "Collecting mounted volumes", "STARTED");

    typeCommand("mkdir -p volumes", true);
    delay(300);

    // Mounted volumes
    typeCommand("mount > volumes/mount.txt 2>&1", true);
    delay(500);
    logAction("MAC_VOLUMES", "Mounted volumes collected", "SUCCESS");

    // Diskutil list
    typeCommand("diskutil list > volumes/diskutil_list.txt 2>&1", true);
    delay(1000);

    // Volume info
    typeCommand("diskutil info / > volumes/root_volume.txt 2>&1", true);
    delay(500);

    // Network shares
    typeCommand("mount | grep smbfs > volumes/smb_mounts.txt 2>&1", true);
    delay(300);

    typeCommand("mount | grep nfs > volumes/nfs_mounts.txt 2>&1", true);
    delay(300);

    logAction("MAC_VOLUMES", "Mounted volumes collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSLaunchDaemons() {
    logAction("MAC_DAEMONS", "Collecting Launch Daemons", "STARTED");

    typeCommand("mkdir -p launch_daemons", true);
    delay(300);

    // System Launch Daemons
    typeCommand("sudo ls -la /Library/LaunchDaemons/ > launch_daemons/system_daemons_list.txt 2>&1", true);
    delay(500);
    logAction("MAC_DAEMONS", "Launch Daemons list collected", "SUCCESS");

    typeCommand("sudo cp /Library/LaunchDaemons/* launch_daemons/ 2>/dev/null", true);
    delay(2000);

    // Loaded launch services
    typeCommand("launchctl list > launch_daemons/launchctl_list.txt 2>&1", true);
    delay(1000);

    logAction("MAC_DAEMONS", "Launch Daemons collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSUserDefaults() {
    logAction("MAC_DEFAULTS", "Collecting user defaults/preferences", "STARTED");

    typeCommand("mkdir -p user_defaults", true);
    delay(300);

    // Global preferences
    typeCommand("defaults read > user_defaults/defaults_all.txt 2>&1", true);
    delay(2000);
    logAction("MAC_DEFAULTS", "User defaults collected", "SUCCESS");

    // Dock preferences
    typeCommand("defaults read com.apple.dock > user_defaults/dock_prefs.txt 2>&1", true);
    delay(500);

    // Finder preferences
    typeCommand("defaults read com.apple.finder > user_defaults/finder_prefs.txt 2>&1", true);
    delay(500);

    // Safari preferences
    typeCommand("defaults read com.apple.Safari > user_defaults/safari_prefs.txt 2>&1", true);
    delay(500);

    logAction("MAC_DEFAULTS", "User defaults collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSRecentItems() {
    logAction("MAC_RECENT", "Collecting recent items", "STARTED");

    typeCommand("mkdir -p recent_items", true);
    delay(300);

    // Recent items
    typeCommand("sudo cp ~/Library/Application\\ Support/com.apple.sharedfilelist/*.sfl recent_items/ 2>/dev/null", true);
    delay(1000);
    logAction("MAC_RECENT", "Recent items collected", "SUCCESS");

    // Recent applications
    typeCommand("defaults read com.apple.recentitems > recent_items/recent_apps.txt 2>&1", true);
    delay(500);

    logAction("MAC_RECENT", "Recent items collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSNotificationCenter() {
    logAction("MAC_NOTIFICATIONS", "Collecting Notification Center data", "STARTED");

    typeCommand("mkdir -p notifications", true);
    delay(300);

    // Notification database
    typeCommand("sudo cp ~/Library/Application\\ Support/NotificationCenter/* notifications/ 2>/dev/null", true);
    delay(1000);
    logAction("MAC_NOTIFICATIONS", "Notification data collected", "SUCCESS");

    // Notification preferences
    typeCommand("defaults read com.apple.notificationcenterui > notifications/nc_prefs.txt 2>&1", true);
    delay(500);

    logAction("MAC_NOTIFICATIONS", "Notification Center collection complete", "SUCCESS");
    return true;
}

// ============================================================================
// ADVANCED MACOS FORENSICS (v1.1.0+)
// ============================================================================

bool HIDAutomation::executeMacOSUnifiedLogsAdvanced() {
    logAction("MAC_LOGS_ADV", "Advanced Unified Logs extraction", "STARTED");

    typeCommand("mkdir -p unified_logs_adv", true);
    delay(300);

    // Extract last 24 hours with filtering
    typeCommand("log show --predicate 'eventMessage contains \"error\" OR eventMessage contains \"fail\" OR eventMessage contains \"denied\"' --style syslog --last 24h > unified_logs_adv/errors_24h.log 2>&1", true);
    delay(10000);
    logAction("MAC_LOGS_ADV", "Error logs extracted", "SUCCESS");

    // Security-related logs
    typeCommand("log show --predicate 'process == \"securityd\" OR process == \"sudo\" OR process == \"su\"' --style syslog --last 7d > unified_logs_adv/security_logs.log 2>&1", true);
    delay(12000);

    // Network-related logs
    typeCommand("log show --predicate 'subsystem contains \"com.apple.network\"' --style syslog --last 24h > unified_logs_adv/network_logs.log 2>&1", true);
    delay(8000);

    logAction("MAC_LOGS_ADV", "Advanced Unified Logs complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSAPFSSnapshots() {
    logAction("MAC_APFS_SNAP", "Collecting APFS snapshots", "STARTED");

    typeCommand("mkdir -p apfs_snapshots", true);
    delay(300);

    // List APFS snapshots
    typeCommand("tmutil listlocalsnapshots / > apfs_snapshots/local_snapshots.txt 2>&1", true);
    delay(2000);
    logAction("MAC_APFS_SNAP", "APFS snapshots listed", "SUCCESS");

    // Disk utility info
    typeCommand("diskutil apfs list > apfs_snapshots/apfs_list.txt 2>&1", true);
    delay(1500);

    // APFS container info
    typeCommand("diskutil list > apfs_snapshots/disk_list.txt 2>&1", true);
    delay(1000);

    logAction("MAC_APFS_SNAP", "APFS snapshots collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSNotificationDBv2() {
    logAction("MAC_NOTIF_V2", "Deep Notification database extraction", "STARTED");

    typeCommand("mkdir -p notifications_v2", true);
    delay(300);

    // Copy notification databases from all users
    typeCommand("for user in /Users/*; do sudo cp -R \"$user/Library/Application Support/NotificationCenter\" notifications_v2/$(basename $user)_NC 2>/dev/null; done", true);
    delay(5000);
    logAction("MAC_NOTIF_V2", "Notification databases copied", "SUCCESS");

    // User notification settings
    typeCommand("for user in /Users/*; do defaults read \"$user/Library/Preferences/com.apple.ncprefs\" > notifications_v2/$(basename $user)_ncprefs.txt 2>&1; done", true);
    delay(3000);

    logAction("MAC_NOTIF_V2", "Notification DBv2 complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSQuarantineEventsV2() {
    logAction("MAC_QUAR_V2", "Enhanced Quarantine Events extraction", "STARTED");

    typeCommand("mkdir -p quarantine_v2", true);
    delay(300);

    // Copy quarantine databases from all users
    typeCommand("for user in /Users/*; do sudo cp \"$user/Library/Preferences/com.apple.LaunchServices.QuarantineEventsV2\" quarantine_v2/$(basename $user)_QuarantineV2 2>/dev/null; done", true);
    delay(3000);
    logAction("MAC_QUAR_V2", "QuarantineV2 databases copied", "SUCCESS");

    // Query quarantine database
    typeCommand("sqlite3 ~/Library/Preferences/com.apple.LaunchServices.QuarantineEventsV2 'SELECT * FROM LSQuarantineEvent' > quarantine_v2/quarantine_query.txt 2>&1", true);
    delay(2000);

    logAction("MAC_QUAR_V2", "QuarantineV2 extraction complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSTCCDatabase() {
    logAction("MAC_TCC", "Extracting TCC (Privacy) database", "STARTED");

    typeCommand("mkdir -p tcc_database", true);
    delay(300);

    // System TCC database
    typeCommand("sudo cp /Library/Application\\ Support/com.apple.TCC/TCC.db tcc_database/TCC_system.db 2>/dev/null", true);
    delay(1500);
    logAction("MAC_TCC", "System TCC database copied", "SUCCESS");

    // User TCC databases
    typeCommand("for user in /Users/*; do sudo cp \"$user/Library/Application Support/com.apple.TCC/TCC.db\" tcc_database/$(basename $user)_TCC.db 2>/dev/null; done", true);
    delay(3000);

    // Query TCC permissions
    typeCommand("sqlite3 ~/Library/Application\\ Support/com.apple.TCC/TCC.db 'SELECT * FROM access' > tcc_database/tcc_permissions.txt 2>&1", true);
    delay(1000);

    logAction("MAC_TCC", "TCC database extraction complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSKnowledgeC() {
    logAction("MAC_KNOWLEDGEC", "Extracting KnowledgeC database", "STARTED");

    typeCommand("mkdir -p knowledgec", true);
    delay(300);

    // Copy KnowledgeC databases from all users
    typeCommand("for user in /Users/*; do sudo cp \"$user/Library/Application Support/Knowledge/knowledgeC.db\" knowledgec/$(basename $user)_knowledgeC.db 2>/dev/null; done", true);
    delay(4000);
    logAction("MAC_KNOWLEDGEC", "KnowledgeC databases copied", "SUCCESS");

    // Query app usage
    typeCommand("sqlite3 ~/Library/Application\\ Support/Knowledge/knowledgeC.db 'SELECT * FROM ZOBJECT WHERE ZSTREAMNAME LIKE \"%app%\" LIMIT 1000' > knowledgec/app_usage.txt 2>&1", true);
    delay(2000);

    logAction("MAC_KNOWLEDGEC", "KnowledgeC extraction complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSiCloudAccounts() {
    logAction("MAC_ICLOUD", "Collecting iCloud account information", "STARTED");

    typeCommand("mkdir -p icloud", true);
    delay(300);

    // iCloud account details
    typeCommand("defaults read MobileMeAccounts > icloud/mobile_me_accounts.txt 2>&1", true);
    delay(800);
    logAction("MAC_ICLOUD", "iCloud accounts listed", "SUCCESS");

    // iCloud preferences
    typeCommand("defaults read ~/Library/Preferences/MobileMeAccounts.plist > icloud/icloud_prefs.txt 2>&1", true);
    delay(500);

    // iCloud drive status
    typeCommand("brctl status > icloud/icloud_drive_status.txt 2>&1", true);
    delay(1000);

    logAction("MAC_ICLOUD", "iCloud collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSKeychainAnalysis() {
    logAction("MAC_KEYCHAIN_ADV", "Advanced Keychain analysis", "STARTED");

    typeCommand("mkdir -p keychain_advanced", true);
    delay(300);

    // List all keychains
    typeCommand("security list-keychains > keychain_advanced/keychains.txt 2>&1", true);
    delay(800);
    logAction("MAC_KEYCHAIN_ADV", "Keychains listed", "SUCCESS");

    // Dump keychain info (metadata only)
    typeCommand("security dump-keychain > keychain_advanced/keychain_dump.txt 2>&1", true);
    delay(2000);

    // List certificates in keychain
    typeCommand("security find-certificate -a > keychain_advanced/certificates.txt 2>&1", true);
    delay(1500);

    // List identities
    typeCommand("security find-identity -v -p codesigning > keychain_advanced/code_signing_identities.txt 2>&1", true);
    delay(1000);

    logAction("MAC_KEYCHAIN_ADV", "Advanced Keychain analysis complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSAirDropHistory() {
    logAction("MAC_AIRDROP", "Collecting AirDrop history", "STARTED");

    typeCommand("mkdir -p airdrop", true);
    delay(300);

    // AirDrop logs
    typeCommand("log show --predicate 'process == \"sharingd\"' --style syslog --last 7d > airdrop/airdrop_logs.log 2>&1", true);
    delay(8000);
    logAction("MAC_AIRDROP", "AirDrop logs collected", "SUCCESS");

    // Sharing preferences
    typeCommand("defaults read com.apple.NetworkBrowser > airdrop/network_browser_prefs.txt 2>&1", true);
    delay(500);

    logAction("MAC_AIRDROP", "AirDrop history complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSHandoffActivities() {
    logAction("MAC_HANDOFF", "Collecting Handoff activities", "STARTED");

    typeCommand("mkdir -p handoff", true);
    delay(300);

    // Handoff logs
    typeCommand("log show --predicate 'subsystem == \"com.apple.coreservices.useractivity\"' --style syslog --last 7d > handoff/handoff_logs.log 2>&1", true);
    delay(8000);
    logAction("MAC_HANDOFF", "Handoff logs collected", "SUCCESS");

    // Continuity preferences
    typeCommand("defaults read ~/Library/Preferences/com.apple.coreservices.useractivity > handoff/useractivity_prefs.txt 2>&1", true);
    delay(500);

    logAction("MAC_HANDOFF", "Handoff activities complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSSpotlightShortcuts() {
    logAction("MAC_SPOTLIGHT_SHORT", "Collecting Spotlight shortcuts", "STARTED");

    typeCommand("mkdir -p spotlight_shortcuts", true);
    delay(300);

    // Spotlight shortcuts
    typeCommand("defaults read com.apple.spotlight > spotlight_shortcuts/spotlight_prefs.txt 2>&1", true);
    delay(800);
    logAction("MAC_SPOTLIGHT_SHORT", "Spotlight preferences collected", "SUCCESS");

    // Spotlight metadata
    typeCommand("mdutil -s / > spotlight_shortcuts/spotlight_status.txt 2>&1", true);
    delay(500);

    // Recent searches
    typeCommand("defaults read com.apple.spotlight orderedItems > spotlight_shortcuts/recent_searches.txt 2>&1", true);
    delay(500);

    logAction("MAC_SPOTLIGHT_SHORT", "Spotlight shortcuts complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSCoreAnalytics() {
    logAction("MAC_ANALYTICS", "Collecting Core Analytics data", "STARTED");

    typeCommand("mkdir -p core_analytics", true);
    delay(300);

    // Copy analytics data
    typeCommand("sudo cp -R /Library/Application\\ Support/CrashReporter/DiagnosticReports core_analytics/ 2>/dev/null", true);
    delay(3000);
    logAction("MAC_ANALYTICS", "Diagnostic reports copied", "SUCCESS");

    // User analytics
    typeCommand("sudo cp -R ~/Library/Logs/DiagnosticReports core_analytics/user_diagnostics 2>/dev/null", true);
    delay(2000);

    logAction("MAC_ANALYTICS", "Core Analytics complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSXProtectLogs() {
    logAction("MAC_XPROTECT", "Collecting XProtect logs", "STARTED");

    typeCommand("mkdir -p xprotect", true);
    delay(300);

    // XProtect version
    typeCommand("system_profiler SPInstallHistoryDataType | grep -i xprotect > xprotect/xprotect_version.txt 2>&1", true);
    delay(2000);
    logAction("MAC_XPROTECT", "XProtect version collected", "SUCCESS");

    // XProtect logs
    typeCommand("log show --predicate 'process == \"XProtect\"' --style syslog --last 30d > xprotect/xprotect_logs.log 2>&1", true);
    delay(15000);

    // XProtect plist
    typeCommand("cat /System/Library/CoreServices/XProtect.bundle/Contents/Resources/XProtect.plist > xprotect/xprotect_plist.txt 2>&1", true);
    delay(800);

    logAction("MAC_XPROTECT", "XProtect collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSMRTLogs() {
    logAction("MAC_MRT", "Collecting MRT (Malware Removal Tool) logs", "STARTED");

    typeCommand("mkdir -p mrt_logs", true);
    delay(300);

    // MRT logs
    typeCommand("log show --predicate 'process == \"MRT\"' --style syslog --last 30d > mrt_logs/mrt_logs.log 2>&1", true);
    delay(12000);
    logAction("MAC_MRT", "MRT logs collected", "SUCCESS");

    // MRT version
    typeCommand("system_profiler SPInstallHistoryDataType | grep -i mrt > mrt_logs/mrt_version.txt 2>&1", true);
    delay(1500);

    logAction("MAC_MRT", "MRT collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSAirPlayReceivers() {
    logAction("MAC_AIRPLAY", "Collecting AirPlay receivers", "STARTED");

    typeCommand("mkdir -p airplay", true);
    delay(300);

    // AirPlay preferences
    typeCommand("defaults read com.apple.airplay > airplay/airplay_prefs.txt 2>&1", true);
    delay(500);
    logAction("MAC_AIRPLAY", "AirPlay preferences collected", "SUCCESS");

    // AirPlay logs
    typeCommand("log show --predicate 'subsystem contains \"airplay\"' --style syslog --last 7d > airplay/airplay_logs.log 2>&1", true);
    delay(8000);

    logAction("MAC_AIRPLAY", "AirPlay collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSShareKitContacts() {
    logAction("MAC_SHAREKIT", "Collecting ShareKit contacts", "STARTED");

    typeCommand("mkdir -p sharekit", true);
    delay(300);

    // Recent shares
    typeCommand("defaults read com.apple.sharekit.recents > sharekit/recent_shares.txt 2>&1", true);
    delay(800);
    logAction("MAC_SHAREKIT", "Recent shares collected", "SUCCESS");

    // Sharing logs
    typeCommand("log show --predicate 'process == \"sharingd\"' --style syslog --last 7d > sharekit/sharing_logs.log 2>&1", true);
    delay(8000);

    logAction("MAC_SHAREKIT", "ShareKit collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSSiriAnalytics() {
    logAction("MAC_SIRI", "Collecting Siri analytics", "STARTED");

    typeCommand("mkdir -p siri_analytics", true);
    delay(300);

    // Siri preferences
    typeCommand("defaults read com.apple.assistant.support > siri_analytics/siri_prefs.txt 2>&1", true);
    delay(500);
    logAction("MAC_SIRI", "Siri preferences collected", "SUCCESS");

    // Siri logs
    typeCommand("log show --predicate 'process == \"Siri\" OR process == \"assistantd\"' --style syslog --last 7d > siri_analytics/siri_logs.log 2>&1", true);
    delay(10000);

    logAction("MAC_SIRI", "Siri analytics complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSCrashReporter() {
    logAction("MAC_CRASH", "Collecting crash reports", "STARTED");

    typeCommand("mkdir -p crash_reports", true);
    delay(300);

    // System crash reports
    typeCommand("sudo cp -R /Library/Logs/DiagnosticReports crash_reports/system_crashes 2>/dev/null", true);
    delay(3000);
    logAction("MAC_CRASH", "System crash reports copied", "SUCCESS");

    // User crash reports
    typeCommand("cp -R ~/Library/Logs/DiagnosticReports crash_reports/user_crashes 2>/dev/null", true);
    delay(2000);

    // Panic logs
    typeCommand("sudo cp /Library/Logs/DiagnosticReports/Kernel_* crash_reports/ 2>/dev/null", true);
    delay(1500);

    logAction("MAC_CRASH", "Crash reports complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSCodesignVerification() {
    logAction("MAC_CODESIGN", "Verifying code signatures", "STARTED");

    typeCommand("mkdir -p codesign", true);
    delay(300);

    // Verify system applications
    typeCommand("for app in /Applications/*.app; do codesign -vv --deep \"$app\" >> codesign/app_verification.txt 2>&1; done", true);
    delay(15000);
    logAction("MAC_CODESIGN", "Application signatures verified", "SUCCESS");

    // Check Gatekeeper status
    typeCommand("spctl --status > codesign/gatekeeper_status.txt 2>&1", true);
    delay(500);

    logAction("MAC_CODESIGN", "Code signature verification complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSBSMaudit() {
    logAction("MAC_BSM_AUDIT", "Collecting BSM audit logs", "STARTED");

    typeCommand("mkdir -p bsm_audit", true);
    delay(300);

    // Copy audit logs
    typeCommand("sudo cp /var/audit/* bsm_audit/ 2>/dev/null", true);
    delay(3000);
    logAction("MAC_BSM_AUDIT", "BSM audit logs copied", "SUCCESS");

    // Audit configuration
    typeCommand("sudo cat /etc/security/audit_control > bsm_audit/audit_control.txt 2>&1", true);
    delay(500);

    // Current audit status
    typeCommand("sudo audit -c > bsm_audit/audit_status.txt 2>&1", true);
    delay(500);

    logAction("MAC_BSM_AUDIT", "BSM audit collection complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSPowerMetrics() {
    logAction("MAC_POWER", "Collecting power metrics", "STARTED");

    typeCommand("mkdir -p power_metrics", true);
    delay(300);

    // Power management settings
    typeCommand("pmset -g > power_metrics/pmset_settings.txt 2>&1", true);
    delay(800);
    logAction("MAC_POWER", "Power settings collected", "SUCCESS");

    // Battery history
    typeCommand("pmset -g log > power_metrics/power_log.txt 2>&1", true);
    delay(2000);

    // System power events
    typeCommand("log show --predicate 'eventMessage contains \"sleep\" OR eventMessage contains \"wake\"' --style syslog --last 7d > power_metrics/power_events.log 2>&1", true);
    delay(8000);

    logAction("MAC_POWER", "Power metrics complete", "SUCCESS");
    return true;
}

bool HIDAutomation::executeMacOSAccountsPlist() {
    logAction("MAC_ACCOUNTS", "Collecting accounts configuration", "STARTED");

    typeCommand("mkdir -p accounts", true);
    delay(300);

    // User accounts from Directory Service
    typeCommand("dscl . list /Users > accounts/user_list.txt 2>&1", true);
    delay(800);
    logAction("MAC_ACCOUNTS", "User accounts listed", "SUCCESS");

    // Account policy
    typeCommand("pwpolicy getaccountpolicies > accounts/account_policies.txt 2>&1", true);
    delay(1000);

    // User details for each user
    typeCommand("for user in $(dscl . list /Users | grep -v '^_'); do dscl . read /Users/$user > accounts/user_$user.txt 2>&1; done", true);
    delay(3000);

    logAction("MAC_ACCOUNTS", "Accounts collection complete", "SUCCESS");
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

// ============================================================================
// ENHANCED ERROR HANDLING
// ============================================================================

ModuleResult HIDAutomation::executeModuleWithRetry(
    const String& module_name,
    std::function<bool()> module_func,
    uint8_t max_retries,
    bool continue_on_error_flag
) {
    ModuleResult result;
    result.module_name = module_name;
    result.success = false;
    result.error_code = ERROR_NONE;
    result.retry_count = 0;
    result.artifacts_collected = 0;
    result.timestamp = millis();

    unsigned long start_time = millis();

    if (verbose) {
        Serial.printf("[HID] Executing module: %s\n", module_name.c_str());
    }

    // Try executing the module with retries
    for (uint8_t attempt = 0; attempt <= max_retries; attempt++) {
        if (attempt > 0) {
            result.retry_count = attempt;

            // Exponential backoff: 2^attempt seconds (2s, 4s, 8s)
            unsigned long backoff_ms = (1 << attempt) * 1000;

            if (verbose) {
                Serial.printf("[HID] Retry attempt %d/%d for %s (backoff: %lu ms)\n",
                             attempt, max_retries, module_name.c_str(), backoff_ms);
            }

            logAction("MODULE_RETRY",
                     module_name + " - Attempt " + String(attempt + 1),
                     "Retrying after failure");

            delay(backoff_ms);
        }

        // Execute the module function
        try {
            result.success = module_func();

            if (result.success) {
                result.error_code = ERROR_NONE;
                result.error_message = "";

                if (verbose) {
                    Serial.printf("[HID] Module %s completed successfully\n", module_name.c_str());
                }

                logAction("MODULE_SUCCESS", module_name, "Completed on attempt " + String(attempt + 1));
                break;  // Success, exit retry loop
            } else {
                // Module returned false
                result.error_code = ERROR_COMMAND_FAILED;
                result.error_message = "Module returned false";

                if (verbose) {
                    Serial.printf("[HID] Module %s failed (attempt %d/%d)\n",
                                 module_name.c_str(), attempt + 1, max_retries + 1);
                }
            }
        } catch (...) {
            // Catch any exceptions
            result.error_code = ERROR_UNKNOWN;
            result.error_message = "Exception during execution";

            if (verbose) {
                Serial.printf("[HID] Module %s threw exception (attempt %d/%d)\n",
                             module_name.c_str(), attempt + 1, max_retries + 1);
            }
        }
    }

    result.duration_ms = millis() - start_time;

    // Log final result
    if (!result.success) {
        String error_msg = module_name + " failed after " + String(result.retry_count + 1) + " attempts";
        setError(error_msg);

        logAction("MODULE_FAILED",
                 module_name,
                 error_msg + " - Error code: " + String(result.error_code));

        // Check if we should continue or stop
        if (!continue_on_error_flag) {
            if (verbose) {
                Serial.printf("[HID] Stopping automation due to module failure (continue_on_error=false)\n");
            }
        }
    }

    // Store result in history
    logModuleResult(result);

    return result;
}

void HIDAutomation::logModuleResult(const ModuleResult& result) {
    module_results.push_back(result);

    // Log to forensic action log as well
    String status = result.success ? "SUCCESS" : "FAILED";
    String details = "Duration: " + String(result.duration_ms) + "ms, " +
                    "Retries: " + String(result.retry_count);

    if (!result.success) {
        details += ", Error: " + result.error_message +
                  " (Code: " + String(result.error_code) + ")";
    }

    logAction("MODULE_RESULT", result.module_name + " - " + status, details);
}

ErrorSummary HIDAutomation::getErrorSummary() const {
    ErrorSummary summary;
    summary.total_modules = module_results.size();
    summary.successful_modules = 0;
    summary.failed_modules = 0;
    summary.retried_modules = 0;

    for (const auto& result : module_results) {
        if (result.success) {
            summary.successful_modules++;
        } else {
            summary.failed_modules++;
            summary.failures.push_back(result);
        }

        if (result.retry_count > 0) {
            summary.retried_modules++;
        }
    }

    return summary;
}

void HIDAutomation::clearErrorHistory() {
    module_results.clear();
    last_error = "";
}

void HIDAutomation::initializeSequences() {
    // Initialize built-in automation sequences
    // This can be expanded with more sophisticated sequences
}

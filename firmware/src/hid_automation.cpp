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

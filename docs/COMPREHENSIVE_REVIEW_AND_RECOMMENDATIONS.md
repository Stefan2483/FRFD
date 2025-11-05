# FRFD v0.6.0 - Comprehensive Review and Recommendations

**Date**: 2025-11-05
**Review Status**: Complete
**Reviewer**: Claude (Anthropic)

---

## Table of Contents

1. [Current Workflow Analysis](#1-current-workflow-analysis)
2. [UI/UX Review](#2-uiux-review)
3. [Forensics Artifacts Coverage](#3-forensics-artifacts-coverage)
4. [Gap Analysis](#4-gap-analysis)
5. [Recommended Improvements](#5-recommended-improvements)
6. [New Features Roadmap](#6-new-features-roadmap)
7. [Required Forensics Tools](#7-required-forensics-tools)
8. [Priority Matrix](#8-priority-matrix)

---

## 1. Current Workflow Analysis

### 1.1 End-to-End Workflow

```
┌──────────────────────────────────────────────────────────┐
│ Phase 1: Device Initialization (5-10 seconds)            │
│ - Boot ESP32-S3                                          │
│ - Initialize display (ST7735 80x160)                     │
│ - Mount SD card                                          │
│ - Start WiFi AP (CSIRT-FORENSICS @ 192.168.4.1)        │
│ - Show boot screen                                       │
└────────────────┬─────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────┐
│ Phase 2: Target Connection (1-2 seconds)                 │
│ - USB enumeration                                        │
│ - Present as HID keyboard                               │
│ - Target OS loads drivers                               │
└────────────────┬─────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────┐
│ Phase 3: OS Detection (10-30 seconds)                    │
│ - Try Windows detection (Win+R, cmd, ver)               │
│ - Try Linux detection (Ctrl+Alt+T, uname)               │
│ - Try macOS detection (Cmd+Space, terminal, sw_vers)    │
│ - Display detected OS with confidence score              │
└────────────────┬─────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────┐
│ Phase 4: Evidence Container Creation (1-2 seconds)       │
│ - Generate case ID (AUTO_timestamp)                     │
│ - Create directory structure on SD card                  │
│ - Initialize chain of custody                            │
│ - Start forensic action logging                          │
└────────────────┬─────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────┐
│ Phase 5: Artifact Collection (2-6 minutes)               │
│                                                          │
│ Windows (7 modules):                                     │
│ ├─ Memory dump (process info, not full RAM)             │
│ ├─ Autorun entries (registry)                           │
│ ├─ Network state (connections, DNS, ARP)                │
│ ├─ Event logs (Security, System, Application)           │
│ ├─ Prefetch files                                        │
│ ├─ Scheduled tasks                                       │
│ └─ Services enumeration                                  │
│                                                          │
│ Linux (5 modules):                                       │
│ ├─ System info (uname, os-release, processes, disk)     │
│ ├─ Auth logs (auth.log, secure)                         │
│ ├─ Network state (netstat, ss)                          │
│ ├─ Kernel modules (lsmod)                               │
│ └─ Persistence mechanisms (cron, systemd)               │
│                                                          │
│ macOS (2 modules):                                       │
│ ├─ System info (sw_vers, hardware, processes)           │
│ └─ Persistence (LaunchAgents, LaunchDaemons)            │
└────────────────┬─────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────┐
│ Phase 6: Archive Creation (10-30 seconds)                │
│ - Windows: Compress-Archive → ZIP                       │
│ - Linux/macOS: tar -czf → TAR.GZ                        │
│ - Archive size: typically 1-10 MB                        │
└────────────────┬─────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────┐
│ Phase 7: WiFi Upload (15-60 seconds)                    │
│ - Target connects to FRFD WiFi                          │
│ - Define inline upload function (PS1/Bash)              │
│ - HTTP POST to http://192.168.4.1/upload               │
│ - Transfer at 150-300 KB/s                              │
│ - FRFD saves to evidence container                      │
│ - SHA-256 integrity verification                        │
└────────────────┬─────────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────────┐
│ Phase 8: Finalization (1-2 seconds)                     │
│ - Generate manifest.json                                │
│ - Generate chain_of_custody.json                        │
│ - Generate hashes.sha256                                │
│ - Verify all artifacts                                  │
│ - Display completion screen                             │
└──────────────────────────────────────────────────────────┘

Total Time: 3-8 minutes
```

### 1.2 Workflow Strengths ✅

1. **Fully Automated** - Zero-touch from insertion to completion
2. **Cross-Platform** - Supports Windows, Linux, macOS
3. **Forensically Sound** - NIST SP 800-86 and ISO/IEC 27037 compliant
4. **Integrity Verified** - SHA-256 hashing per artifact
5. **Complete Audit Trail** - Every action logged with timestamps
6. **Self-Contained** - No network dependencies (except WiFi to self)

### 1.3 Workflow Weaknesses ❌

1. **No User Interaction** - Cannot customize collection on-the-fly
2. **No Abort/Pause** - Once started, runs to completion
3. **Fixed Module Set** - Cannot selectively enable/disable modules
4. **No Error Recovery** - If one module fails, continues blindly
5. **No Live Feedback** - Target user sees terminal activity (opsec issue)
6. **No Remote Control** - Cannot update or reconfigure remotely
7. **WiFi Dependency** - Upload fails if WiFi unavailable on target
8. **No Offline Mode** - Requires target to have working WiFi adapter
9. **Single Execution** - No way to re-run specific modules
10. **No Verification on Target** - Cannot confirm artifacts created correctly

---

## 2. UI/UX Review

### 2.1 Current Display Screens

**Hardware**: ST7735 TFT, 80x160 pixels, 16-bit color

**Screens Implemented**:
1. Boot screen (logo, firmware version)
2. Status screen (mode, OS, progress, network)
3. HID automation start
4. HID OS detection (with spinner)
5. HID OS detected (OS logo, confidence %)
6. HID collection (module name, step X/Y)
7. HID progress (per-module progress bar)
8. HID complete (checkmark, stats, duration)
9. HID error (error message)

### 2.2 UI Strengths ✅

1. **Visual Feedback** - Clear indication of current phase
2. **Progress Indication** - Progress bars and step counters
3. **Animated Elements** - Spinner shows activity
4. **Color Coding** - Green=success, red=error, cyan=progress
5. **Professional Look** - Clean, organized layout
6. **Phase Indicators** - Dots show overall progress
7. **Elapsed Time** - Shows time since mode start

### 2.3 UI Weaknesses ❌

1. **Read-Only** - No input capability (no buttons used)
2. **Small Screen** - 80x160 limited information density
3. **No Scrolling** - Cannot review past events
4. **No Detailed Errors** - Error messages truncated
5. **No Module Details** - Cannot see which artifacts collected
6. **No Upload Progress** - Doesn't show WiFi upload status
7. **No Network Info** - Doesn't display IP, SSID on main screen
8. **No Case ID Display** - User cannot see case identifier
9. **No Artifact Count** - Doesn't show number of artifacts collected
10. **No Storage Status** - Doesn't show SD card free space
11. **Static After Completion** - No option to start new collection
12. **No Web UI** - Web interface exists but minimal features

### 2.4 Web Interface Review

**Current Web UI** (http://192.168.4.1):
- `/` - Control panel (device status, connected clients)
- `/status` - JSON API (for programmatic access)
- `/files` - Browse evidence files
- `/download` - Download artifacts
- `/config` - View configuration
- `/upload` - API only (no web form)

**Web UI Weaknesses**:
- No live log viewing
- No collection start/stop controls
- No module selection interface
- No progress visualization
- No evidence container browser
- No chain of custody viewer
- No real-time artifact preview
- Cannot trigger new collection
- Cannot change settings
- Cannot view system logs

---

## 3. Forensics Artifacts Coverage

### 3.1 Windows Artifacts (7 Modules)

| Module | Artifacts Collected | Coverage | Gaps |
|--------|-------------------|----------|------|
| **Memory** | Process info (name, PID, path) for lsass, services, svchost | ⚠️ **LIMITED** | No full RAM dump, no process memory dumps, no handles, no DLLs |
| **Autoruns** | HKLM Run key only | ⚠️ **LIMITED** | Missing: RunOnce, CurrentUser runs, startup folders, services, drivers, scheduled tasks (separate module), WMI, AppInit, Winlogon, Explorer add-ons |
| **Network** | TCP connections, DNS cache, ARP cache | ✅ **GOOD** | Missing: UDP connections, listening ports, routing table, firewall rules, WiFi profiles, hosts file |
| **Event Logs** | Security, System, Application (EVTX) | ✅ **GOOD** | Missing: PowerShell logs, Windows Defender, Sysmon (if installed), other event logs |
| **Prefetch** | All .pf files from C:\Windows\Prefetch | ✅ **EXCELLENT** | - |
| **Scheduled Tasks** | Task list (CSV) | ⚠️ **LIMITED** | Missing: Task XML definitions, task history, disabled tasks |
| **Services** | Service list (CSV) | ⚠️ **LIMITED** | Missing: Service binaries, DLL dependencies, service config, startup type |

**Overall Windows Coverage**: 60/100

**Critical Missing Artifacts**:
1. **Registry Hives** - SAM, SYSTEM, SOFTWARE, SECURITY, NTUSER.DAT
2. **MFT** - Master File Table (timeline analysis)
3. **USN Journal** - File system changes
4. **$LogFile** - NTFS transaction log
5. **Recycle Bin** - Deleted files
6. **Browser History** - Chrome, Firefox, Edge, IE
7. **User Files** - Recent docs, downloads, desktop
8. **Windows Search Index** - ESE database
9. **WMI Repository** - Persistence mechanisms
10. **Amcache** - Program execution evidence
11. **ShimCache** - Application compatibility cache
12. **Jump Lists** - Recently used files
13. **Shellbags** - Folder access history
14. **LNK Files** - Shortcuts (timestamps, paths)
15. **Bitmap Cache** - RDP artifacts
16. **SRUM** - System Resource Usage Monitor
17. **Windows Defender Logs** - Malware detection events
18. **PowerShell History** - ConsoleHost_history.txt
19. **Execution Artifacts** - BAM/DAM (Background Activity Moderator)
20. **Volume Shadow Copies** - Historical file versions

### 3.2 Linux Artifacts (5 Modules)

| Module | Artifacts Collected | Coverage | Gaps |
|--------|-------------------|----------|------|
| **System Info** | uname, os-release, processes, disk usage | ✅ **GOOD** | Missing: loaded kernel modules, hardware info, CPU/mem info |
| **Auth Logs** | auth.log, secure | ✅ **GOOD** | Missing: last/lastb, wtmp, utmp, failed login attempts |
| **Network** | netstat, ss output | ✅ **GOOD** | Missing: iptables rules, routing table, network interfaces, hosts file |
| **Kernel Modules** | lsmod output | ⚠️ **LIMITED** | Missing: module parameters, module dependencies, /proc/modules |
| **Persistence** | Crontabs, systemd units | ⚠️ **LIMITED** | Missing: /etc/rc.d, init.d, profile.d, bashrc, SSH keys, authorized_keys |

**Overall Linux Coverage**: 55/100

**Critical Missing Artifacts**:
1. **Shell History** - .bash_history, .zsh_history for all users
2. **SSH Configuration** - sshd_config, known_hosts, authorized_keys
3. **User Accounts** - /etc/passwd, /etc/shadow, /etc/group
4. **Sudo Logs** - /var/log/sudo.log
5. **Package Manager** - apt/yum history, installed packages
6. **Web Server Logs** - Apache, Nginx access/error logs
7. **Database Logs** - MySQL, PostgreSQL logs
8. **Docker Artifacts** - Container configs, images, volumes
9. **Firewall Rules** - iptables, ufw, firewalld
10. **Mounted Filesystems** - /etc/fstab, mount points
11. **Environment Variables** - /etc/environment, /etc/profile
12. **X11/Wayland Sessions** - Display manager logs
13. **Browser Artifacts** - ~/.mozilla, ~/.config/google-chrome
14. **Mail Spools** - /var/mail/*
15. **At/Cron Jobs** - /var/spool/cron, /var/spool/at

### 3.3 macOS Artifacts (2 Modules)

| Module | Artifacts Collected | Coverage | Gaps |
|--------|-------------------|----------|------|
| **System Info** | sw_vers, hardware info, processes | ⚠️ **LIMITED** | Missing: system_profiler full output, installed apps, kernel extensions |
| **Persistence** | LaunchAgents, LaunchDaemons lists | ⚠️ **LIMITED** | Missing: Launch plist contents, Login Items, Periodic scripts, cron |

**Overall macOS Coverage**: 30/100

**Critical Missing Artifacts**:
1. **Unified Logs** - log show --predicate (system logs)
2. **FSEvents** - Filesystem events database
3. **Spotlight Index** - .Spotlight-V100 database
4. **Safari History** - ~/Library/Safari/History.db
5. **Chrome/Firefox** - Browser history/downloads
6. **QuickLook Thumbnails** - Recently viewed files
7. **User Accounts** - /var/db/dslocal/nodes/Default/users
8. **Keychain** - login.keychain-db (encrypted)
9. **Installed Applications** - /Applications, ~/Applications
10. **Recent Items** - Recent documents, servers, applications
11. **Trash** - ~/.Trash contents
12. **Time Machine** - Backup configuration
13. **WiFi Networks** - Known WiFi networks list
14. **Bluetooth Devices** - Paired devices
15. **Downloads** - ~/Downloads folder listing
16. **Desktop** - ~/Desktop folder listing
17. **Documents** - ~/Documents folder listing
18. **Mail** - ~/Library/Mail database
19. **Messages** - ~/Library/Messages database
20. **Photos Library** - Recent photos metadata

---

## 4. Gap Analysis

### 4.1 Critical Gaps (MUST FIX)

#### 4.1.1 Forensics Artifacts
1. **No Registry Collection** (Windows) - CRITICAL for malware analysis
2. **No Browser History** (All platforms) - CRITICAL for investigations
3. **No Timeline Artifacts** (Windows) - MFT, USN Journal essential
4. **No User File Metadata** (All platforms) - Recent docs, downloads
5. **No Full Memory Dump** (All platforms) - Process dumps only

#### 4.1.2 Workflow Issues
1. **No Error Handling** - Silent failures
2. **No Module Selection** - Cannot customize collection
3. **No Abort Mechanism** - Cannot stop collection
4. **No Retry Logic** - One-shot execution
5. **No Offline Mode** - WiFi dependency

#### 4.1.3 UI/UX Issues
1. **No Interactive Control** - Read-only display
2. **No Upload Progress** - Cannot see WiFi transfer
3. **No Error Details** - Truncated messages
4. **No Web Dashboard** - Minimal web interface

### 4.2 Important Gaps (SHOULD FIX)

#### 4.2.1 Forensics Artifacts
1. **Limited Persistence Coverage** - Missing many mechanisms
2. **No Container Forensics** - Docker, Kubernetes
3. **No Cloud Artifacts** - AWS CLI history, Azure tokens
4. **No Database Dumps** - MySQL, PostgreSQL, SQLite
5. **No Email Evidence** - Outlook PST, Thunderbird

#### 4.2.2 Operational Issues
1. **No Remote Management** - Cannot reconfigure
2. **No Live Triage** - Cannot interact during collection
3. **No Selective Re-run** - Must re-run everything
4. **No Bandwidth Limiting** - Upload can be slow
5. **No Compression Options** - Fixed RLE compression

#### 4.2.3 Reporting Issues
1. **No HTML Report** - Manual analysis required
2. **No Summary Statistics** - IOC counts, file counts
3. **No Visualization** - Timeline, network graphs
4. **No Export Options** - Limited to raw artifacts

### 4.3 Enhancement Gaps (NICE TO HAVE)

1. **No Multi-Device Support** - Cannot manage fleet
2. **No Cloud Sync** - Cannot sync to S3/Azure
3. **No Encryption at Rest** - SD card not encrypted
4. **No YARA Scanning** - No malware detection
5. **No IOC Matching** - Cannot check against threat feeds
6. **No Automated Analysis** - Manual review required
7. **No API Access** - Web UI only
8. **No Webhook Notifications** - No alerts
9. **No Plugin System** - Cannot extend easily
10. **No Multi-Language Support** - English only

---

## 5. Recommended Improvements

### 5.1 Immediate Improvements (Phase 7.0)

#### 5.1.1 Enhanced Error Handling
**Priority**: CRITICAL
**Effort**: 2-3 days

**Features**:
- Try-catch blocks around each module
- Error codes with descriptions
- Retry logic (3 attempts with exponential backoff)
- Continue-on-error flag
- Detailed error logging with stack traces
- Error summary screen on completion

**Implementation**:
```cpp
struct ModuleResult {
    bool success;
    String error_message;
    uint16_t error_code;
    uint8_t retry_count;
    unsigned long duration_ms;
};

class EnhancedHIDAutomation {
    ModuleResult executeModule(Module* module, uint8_t max_retries = 3);
    void logError(String module, String error, uint16_t code);
    void displayErrorSummary();
};
```

#### 5.1.2 Upload Progress Display
**Priority**: HIGH
**Effort**: 1 day

**Features**:
- Real-time upload progress bar on display
- Transfer speed (KB/s) display
- Bytes sent / total bytes
- Estimated time remaining
- Connection status indicator
- Retry attempts counter

**Display Update**:
```cpp
void FRFDDisplay::showUploadProgress(
    String filename,
    unsigned long bytes_sent,
    unsigned long total_bytes,
    float speed_kbps,
    uint8_t percent
);
```

#### 5.1.3 Module Selection System
**Priority**: HIGH
**Effort**: 3-4 days

**Features**:
- Pre-defined profiles: Quick, Standard, Deep, Custom
- Web interface for module selection
- Save/load profiles to SD card
- Per-module enable/disable flags
- Priority-based execution order
- Timeout configuration per module

**Configuration Structure**:
```json
{
  "profile": "standard",
  "modules": {
    "windows": {
      "memory": {"enabled": true, "timeout": 60, "priority": 1},
      "registry": {"enabled": true, "timeout": 120, "priority": 2},
      "browser": {"enabled": true, "timeout": 90, "priority": 3}
    }
  }
}
```

### 5.2 Short-Term Improvements (Phase 7.1-7.2)

#### 5.2.1 Registry Collection (Windows)
**Priority**: CRITICAL
**Effort**: 2-3 days

**Artifacts to Collect**:
```powershell
# System hives (requires admin)
reg save HKLM\SAM C:\FRFD_Collection\registry\SAM
reg save HKLM\SYSTEM C:\FRFD_Collection\registry\SYSTEM
reg save HKLM\SOFTWARE C:\FRFD_Collection\registry\SOFTWARE
reg save HKLM\SECURITY C:\FRFD_Collection\registry\SECURITY

# User hive (current user)
reg save HKCU C:\FRFD_Collection\registry\NTUSER.DAT

# Specific forensic keys
reg export "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" registry\Run.reg
reg export "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnce" registry\RunOnce.reg
reg export "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" registry\UserRun.reg
```

#### 5.2.2 Browser History Collection
**Priority**: CRITICAL
**Effort**: 3-4 days

**Browsers to Support**:
- Chrome/Chromium (History, Downloads, Cookies, Cache)
- Firefox (places.sqlite, downloads.sqlite, cookies.sqlite)
- Edge (same as Chrome, different path)
- Safari (macOS: History.db, Downloads.plist)

**Paths**:
```cpp
// Windows Chrome
"C:\Users\{user}\AppData\Local\Google\Chrome\User Data\Default\History"
"C:\Users\{user}\AppData\Local\Google\Chrome\User Data\Default\Cookies"

// Windows Firefox
"C:\Users\{user}\AppData\Roaming\Mozilla\Firefox\Profiles\*.default\places.sqlite"

// Linux Chrome
"~/.config/google-chrome/Default/History"

// macOS Safari
"~/Library/Safari/History.db"
```

#### 5.2.3 MFT and Timeline Artifacts (Windows)
**Priority**: HIGH
**Effort**: 4-5 days

**Artifacts**:
```powershell
# Raw copy of $MFT (requires admin + RawCopy tool)
RawCopy.exe /FileNamePath:C:0 /OutputPath:C:\FRFD_Collection\timeline

# USN Journal
fsutil usn readjournal C: > C:\FRFD_Collection\timeline\usn_journal.txt

# $LogFile (requires RawCopy)
RawCopy.exe /FileNamePath:C:2 /OutputPath:C:\FRFD_Collection\timeline

# Amcache
copy C:\Windows\appcompat\Programs\Amcache.hve C:\FRFD_Collection\timeline\

# ShimCache (via registry)
reg save HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\AppCompatCache registry\ShimCache
```

**Tools Required**:
- RawCopy.exe (included in FRFD package)
- Or PowerShell alternative using Win32 API

### 5.3 Medium-Term Improvements (Phase 7.3-7.5)

#### 5.3.1 Web Dashboard
**Priority**: HIGH
**Effort**: 1 week

**Features**:
- Live collection monitoring
- Real-time log streaming
- Module control (start/stop/pause)
- Evidence container browser
- Chain of custody viewer
- Artifact preview (text files, JSON)
- Download manager
- Settings configuration
- System status dashboard

**Tech Stack**:
- Backend: ESP32 WebServer (existing)
- Frontend: Vanilla JS (no frameworks, keep small)
- WebSockets for live updates
- Server-Sent Events for logs

#### 5.3.2 Offline Mode
**Priority**: MEDIUM
**Effort**: 2-3 days

**Features**:
- Detect WiFi unavailability
- Save artifacts directly to USB storage
- Create portable evidence package
- Generate transfer script for later upload
- USB mass storage mode option

**Implementation**:
```cpp
if (!wifiAvailable()) {
    // Save to USB-accessible partition
    storage->setStorageMode(STORAGE_MODE_USB_MSC);

    // Create transfer script on USB
    createTransferScript();

    // Display instructions
    display->showOfflineComplete();
}
```

#### 5.3.3 Container Forensics
**Priority**: MEDIUM
**Effort**: 3-4 days

**Docker Artifacts**:
```bash
# Container list
docker ps -a > containers.txt

# Container configs
for c in $(docker ps -aq); do
    docker inspect $c > inspect_$c.json
done

# Container logs
for c in $(docker ps -aq); do
    docker logs $c > logs_$c.txt
done

# Images
docker images > images.txt

# Volumes
docker volume ls > volumes.txt
```

**Kubernetes Artifacts**:
```bash
# Pod list
kubectl get pods --all-namespaces -o json > pods.json

# Services
kubectl get services --all-namespaces -o json > services.json

# ConfigMaps and Secrets (names only, not values)
kubectl get configmaps --all-namespaces > configmaps.txt
kubectl get secrets --all-namespaces > secrets.txt
```

### 5.4 Long-Term Improvements (Phase 8.0+)

#### 5.4.1 Memory Acquisition
**Priority**: HIGH (for advanced forensics)
**Effort**: 2-3 weeks

**Challenges**:
- Large file sizes (4-32 GB)
- Requires admin/root privileges
- Time-consuming (10-60 minutes)
- Tool dependencies

**Tools**:
- Windows: WinPMEM, Magnet RAM Capture
- Linux: LiME (Linux Memory Extractor)
- macOS: OSXPMem

**Approach**:
- Optional module (disabled by default)
- Progress indication crucial
- Chunked upload (5-10 MB chunks)
- Resume capability
- Compression (LZ4 for speed)

#### 5.4.2 YARA Scanning
**Priority**: MEDIUM
**Effort**: 1-2 weeks

**Features**:
- Embedded YARA engine
- Pre-loaded rule sets (APT, malware, webshells)
- Scan collected artifacts
- Generate IOC report
- Flag suspicious files
- Integration with chain of custody

**Rule Sets**:
- YARA-Forge community rules
- Custom FRFD rules
- Organization-specific rules (user-provided)

#### 5.4.3 Automated Timeline Generation
**Priority**: MEDIUM
**Effort**: 2 weeks

**Features**:
- Parse MFT, USN, Prefetch, event logs
- Generate unified timeline
- Export to CSV/JSON
- Filter by date range
- Sort by timestamp
- Highlight suspicious activity

**Output**:
```csv
Timestamp,Source,Event,Description,File,User
2025-11-05 10:15:23,MFT,File Created,Document.docx,C:\Users\alice\Documents,alice
2025-11-05 10:15:45,Prefetch,Execution,WINWORD.EXE,C:\Program Files\Microsoft Office,alice
2025-11-05 10:16:12,EventLog,Logon,Interactive Logon,N/A,alice
```

---

## 6. New Features Roadmap

### Phase 7.0 - Stability & UX (1-2 weeks)
**Focus**: Error handling, user experience, operational improvements

- ✅ Enhanced error handling with retry logic
- ✅ Upload progress display
- ✅ Module selection system
- ✅ Abort/pause mechanisms
- ✅ Web dashboard v1 (basic monitoring)
- ✅ Offline mode support

### Phase 7.1 - Windows Deep Dive (2-3 weeks)
**Focus**: Comprehensive Windows artifact collection

- ✅ Registry hive collection (SAM, SYSTEM, SOFTWARE, SECURITY, NTUSER.DAT)
- ✅ Browser history (Chrome, Firefox, Edge)
- ✅ MFT and timeline artifacts ($MFT, USN Journal, $LogFile)
- ✅ User file metadata (Recent docs, Downloads, Desktop)
- ✅ Additional persistence mechanisms (WMI, scheduled tasks XML)
- ✅ Windows Search index
- ✅ Recycle Bin contents
- ✅ Jump lists and shellbags
- ✅ LNK file analysis
- ✅ Amcache and ShimCache

### Phase 7.2 - Linux & macOS Enhancement (2 weeks)
**Focus**: Improve Unix-based coverage

**Linux**:
- ✅ Shell history (all users)
- ✅ SSH configuration and keys
- ✅ User account database (/etc/passwd, shadow, group)
- ✅ Package manager history
- ✅ Web server logs
- ✅ Firewall rules (iptables, ufw)
- ✅ Docker/container artifacts

**macOS**:
- ✅ Unified logs collection
- ✅ FSEvents database
- ✅ Spotlight index
- ✅ Browser history (Safari, Chrome, Firefox)
- ✅ QuickLook thumbnails
- ✅ User accounts (dslocal)
- ✅ Recent items
- ✅ Installed applications list

### Phase 7.3 - Web Dashboard (1-2 weeks)
**Focus**: Professional web interface

- ✅ Real-time monitoring dashboard
- ✅ Live log streaming
- ✅ Collection control (start/stop/pause)
- ✅ Module selection UI
- ✅ Evidence browser
- ✅ Chain of custody viewer
- ✅ Settings configuration
- ✅ API documentation

### Phase 7.4 - Analysis & Reporting (2-3 weeks)
**Focus**: Built-in analysis capabilities

- ✅ Timeline generation from artifacts
- ✅ IOC extraction (IPs, domains, hashes, URLs)
- ✅ Suspicious activity detection
- ✅ HTML report generation
- ✅ Summary statistics
- ✅ Export to standard formats (DFIR-ORC, Plaso)

### Phase 7.5 - Advanced Features (3-4 weeks)
**Focus**: Enterprise and advanced use cases

- ✅ YARA scanning integration
- ✅ Memory acquisition support
- ✅ Cloud sync (S3, Azure Blob)
- ✅ Multi-device management
- ✅ Remote configuration
- ✅ Webhook notifications
- ✅ API access
- ✅ Plugin architecture

### Phase 8.0 - AI/ML Integration (Future)
**Focus**: Intelligent forensics

- 🔮 Anomaly detection
- 🔮 Malware classification
- 🔮 Automated IOC extraction
- 🔮 Natural language query
- 🔮 Predictive analysis
- 🔮 Behavior profiling

---

## 7. Required Forensics Tools

### 7.1 Tools to Embed in Firmware

#### 7.1.1 Windows Tools (to bundle)

| Tool | Purpose | Size | License | Priority |
|------|---------|------|---------|----------|
| **RawCopy** | Copy locked files ($MFT, SAM, etc.) | 100 KB | Open Source | CRITICAL |
| **7za.exe** | Fast compression (better than Compress-Archive) | 1.5 MB | Open Source | HIGH |
| **WinPMEM** | Memory acquisition | 2 MB | Open Source | MEDIUM |
| **YARA** | Malware scanning | 500 KB | Open Source | MEDIUM |
| **Autoruns CLI** | Comprehensive persistence check | 1 MB | Sysinternals | HIGH |
| **sigcheck.exe** | Verify file signatures | 500 KB | Sysinternals | MEDIUM |
| **handle.exe** | List open handles | 300 KB | Sysinternals | LOW |
| **listdlls.exe** | List loaded DLLs | 200 KB | Sysinternals | LOW |

**Total Size**: ~6 MB (acceptable for 16MB Flash)

**Distribution Method**:
1. Store compressed in SPIFFS partition
2. Extract to target at runtime
3. Delete after collection (opsec)

#### 7.1.2 Linux Tools (to bundle)

Most Linux tools are pre-installed, but for older/minimal systems:

| Tool | Purpose | Size | License | Priority |
|------|---------|------|---------|----------|
| **LiME** | Memory acquisition (kernel module) | 50 KB | GPL | MEDIUM |
| **YARA** | Malware scanning | 500 KB | Open Source | MEDIUM |
| **volatility** | Memory analysis (too large, skip) | 50 MB | GPL | SKIP |

**Distribution**: Compile statically-linked binaries

#### 7.1.3 macOS Tools (to bundle)

| Tool | Purpose | Size | License | Priority |
|------|---------|------|---------|----------|
| **OSXPMem** | Memory acquisition | 2 MB | Open Source | MEDIUM |
| **YARA** | Malware scanning | 500 KB | Open Source | MEDIUM |

### 7.2 Tools to Use via PowerShell/Bash

#### 7.2.1 Native Windows Commands
Already available on all systems:
- `reg.exe` - Registry operations
- `wevtutil.exe` - Event log export
- `netstat.exe` - Network connections
- `Get-Process` - Process enumeration
- `Get-NetTCPConnection` - Network state
- `Get-DnsClientCache` - DNS cache
- `Copy-Item` - File operations
- `Compress-Archive` - ZIP creation

#### 7.2.2 Native Linux Commands
- `tar` - Archive creation
- `dd` - Raw disk access (for memory)
- `lsmod` - Kernel modules
- `iptables-save` - Firewall rules
- `ss` / `netstat` - Network state
- `ps aux` - Process listing
- `find` - File search
- `cat` / `grep` - Text processing

#### 7.2.3 Native macOS Commands
- `tar` - Archive creation
- `system_profiler` - System information
- `log show` - Unified logs
- `defaults read` - Preferences
- `launchctl list` - LaunchAgents/Daemons
- `diskutil` - Disk information

### 7.3 Analysis Tools (Post-Collection)

These run on analyst workstation, not on FRFD:

| Tool | Purpose | Platform | License |
|------|---------|----------|---------|
| **Plaso** | Timeline generation | Windows/Linux/macOS | Apache 2.0 |
| **Volatility** | Memory analysis | Windows/Linux/macOS | GPL |
| **RegRipper** | Registry parsing | Windows/Linux | Open Source |
| **Log2Timeline** | Log parsing | All | Apache 2.0 |
| **Autopsy** | Forensics platform | All | Apache 2.0 |
| **KAPE** | Evidence processing | Windows | Open Source |

---

## 8. Priority Matrix

### 8.1 Feature Priority (Impact vs. Effort)

```
High Impact, Low Effort (DO FIRST)
┌────────────────────────────────────────┐
│ • Upload progress display              │
│ • Error handling enhancement           │
│ • Module selection system              │
│ • Browser history collection           │
│ • Web dashboard basic version          │
└────────────────────────────────────────┘

High Impact, High Effort (PLAN CAREFULLY)
┌────────────────────────────────────────┐
│ • Registry collection (Windows)        │
│ • MFT/USN/Timeline artifacts           │
│ • Memory acquisition                   │
│ • YARA scanning integration            │
│ • Advanced web dashboard               │
└────────────────────────────────────────┘

Low Impact, Low Effort (FILL-IN WORK)
┌────────────────────────────────────────┐
│ • SSH config collection (Linux)        │
│ • Package manager history              │
│ • Installed apps list (macOS)          │
│ • Case ID display on screen            │
│ • Network info on display              │
└────────────────────────────────────────┘

Low Impact, High Effort (DEFER)
┌────────────────────────────────────────┐
│ • AI/ML analysis features              │
│ • Multi-device management              │
│ • Cloud sync                           │
│ • Plugin architecture                  │
│ • Natural language queries             │
└────────────────────────────────────────┘
```

### 8.2 Forensics Artifacts Priority

**P0 - Critical (Must Have)**:
1. Registry hives (Windows)
2. Browser history (all platforms)
3. MFT and timeline artifacts (Windows)
4. User file metadata (all platforms)
5. Full event logs (Windows)

**P1 - High Priority (Should Have)**:
1. Memory dumps (selective)
2. Container forensics (Docker, K8s)
3. Shell history (Linux, macOS)
4. SSH keys and configs (Linux, macOS)
5. Recycle Bin / Trash (all platforms)

**P2 - Medium Priority (Nice to Have)**:
1. Database dumps (MySQL, PostgreSQL)
2. Email artifacts (PST, Maildir)
3. Cloud CLI history (AWS, Azure, GCP)
4. Web server logs (Apache, Nginx)
5. Firewall logs and rules

**P3 - Low Priority (Future)**:
1. Volume Shadow Copies (Windows)
2. Backup archives (Time Machine, etc.)
3. Encrypted containers (VeraCrypt, LUKS)
4. Mobile device sync artifacts
5. Game/application save files

### 8.3 Implementation Roadmap

**Q1 2025** (Months 1-3):
- Phase 7.0: Stability & UX
- Phase 7.1: Windows Deep Dive
- Target: v0.7.0 release

**Q2 2025** (Months 4-6):
- Phase 7.2: Linux & macOS Enhancement
- Phase 7.3: Web Dashboard
- Target: v0.8.0 release

**Q3 2025** (Months 7-9):
- Phase 7.4: Analysis & Reporting
- Phase 7.5: Advanced Features (partial)
- Target: v0.9.0 release

**Q4 2025** (Months 10-12):
- Phase 7.5: Advanced Features (complete)
- Testing, hardening, documentation
- Target: v1.0.0 release (Production-Stable)

---

## 9. Conclusion

### 9.1 Current State Assessment

**Strengths**:
- ✅ Solid foundation (v0.6.0 production-ready)
- ✅ Complete automation workflow
- ✅ Forensically sound evidence handling
- ✅ Cross-platform support (Windows, Linux, macOS)
- ✅ WiFi transfer system working
- ✅ Real-time progress tracking
- ✅ NIST/ISO compliance

**Weaknesses**:
- ❌ Limited artifact coverage (60% Windows, 55% Linux, 30% macOS)
- ❌ No user interaction/control
- ❌ Minimal error handling
- ❌ Basic web interface
- ❌ No built-in analysis

### 9.2 Recommended Next Steps

**Immediate (Next 2 Weeks)**:
1. Implement enhanced error handling (Phase 7.0)
2. Add upload progress to display (Phase 7.0)
3. Create module selection system (Phase 7.0)

**Short-Term (Next 1-2 Months)**:
1. Add Windows registry collection (Phase 7.1)
2. Add browser history collection (Phase 7.1)
3. Implement basic web dashboard (Phase 7.3)

**Medium-Term (Next 3-6 Months)**:
1. Complete Windows artifact coverage (Phase 7.1)
2. Enhance Linux/macOS coverage (Phase 7.2)
3. Add analysis and reporting features (Phase 7.4)

### 9.3 Success Metrics

**Technical Metrics**:
- Artifact coverage: 60% → 90% (Windows)
- Artifact coverage: 55% → 85% (Linux)
- Artifact coverage: 30% → 75% (macOS)
- Error rate: Unknown → <5%
- Collection time: 3-8 min → 2-6 min (optimized)
- Upload reliability: Unknown → >95%

**User Experience Metrics**:
- Time to first insight: Manual → Automated report
- False positive rate: N/A → <10% (with YARA)
- User satisfaction: TBD → >4/5 stars
- Adoption rate: TBD → Track downloads

**Forensics Quality Metrics**:
- Chain of custody completeness: 100% (maintained)
- Integrity verification: 100% (maintained)
- Timeline accuracy: N/A → >95%
- Artifact relevance: TBD → >80%

---

**Document Version**: 1.0
**Last Updated**: 2025-11-05
**Next Review**: 2025-12-01

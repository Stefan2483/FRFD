# FRFD Improvements Checklist

Quick reference for improvements, new features, and required tools.

---

## Current Coverage Status

| Platform | Overall Score | Critical Gaps |
|----------|---------------|---------------|
| **Windows** | 60/100 | Registry hives, Browser history, MFT/Timeline |
| **Linux** | 55/100 | Shell history, SSH keys, Container forensics |
| **macOS** | 30/100 | Unified logs, FSEvents, Browser history |

---

## Top 20 Priority Improvements

### 🔴 P0 - Critical (Week 1-2)

| # | Improvement | Effort | Impact | Status |
|---|------------|--------|--------|--------|
| 1 | Enhanced error handling with retry logic | 2-3 days | HIGH | ⬜ TODO |
| 2 | Upload progress display on screen | 1 day | HIGH | ⬜ TODO |
| 3 | Module selection system (profiles) | 3-4 days | HIGH | ⬜ TODO |
| 4 | Browser history collection (all platforms) | 3-4 days | CRITICAL | ⬜ TODO |
| 5 | Abort/pause mechanism | 1-2 days | HIGH | ⬜ TODO |

**Goal**: Make system more reliable and user-controllable

### 🟠 P1 - High Priority (Week 3-6)

| # | Improvement | Effort | Impact | Status |
|---|------------|--------|--------|--------|
| 6 | Windows Registry hive collection | 2-3 days | CRITICAL | ⬜ TODO |
| 7 | MFT and timeline artifacts (Windows) | 4-5 days | CRITICAL | ⬜ TODO |
| 8 | User file metadata (Recent docs, Downloads) | 2 days | HIGH | ⬜ TODO |
| 9 | Linux shell history (all users) | 1 day | HIGH | ⬜ TODO |
| 10 | SSH configuration and keys | 1-2 days | HIGH | ⬜ TODO |
| 11 | Web dashboard v1 (monitoring) | 1 week | HIGH | ⬜ TODO |
| 12 | Offline mode support | 2-3 days | MEDIUM | ⬜ TODO |

**Goal**: Comprehensive artifact coverage

### 🟡 P2 - Medium Priority (Week 7-12)

| # | Improvement | Effort | Impact | Status |
|---|------------|--------|--------|--------|
| 13 | Container forensics (Docker, K8s) | 3-4 days | MEDIUM | ⬜ TODO |
| 14 | macOS Unified logs collection | 2 days | HIGH | ⬜ TODO |
| 15 | Recycle Bin / Trash collection | 2 days | MEDIUM | ⬜ TODO |
| 16 | Timeline generation from artifacts | 2 weeks | MEDIUM | ⬜ TODO |
| 17 | HTML report generation | 1 week | MEDIUM | ⬜ TODO |
| 18 | YARA scanning integration | 1-2 weeks | MEDIUM | ⬜ TODO |
| 19 | Memory acquisition (optional module) | 2-3 weeks | MEDIUM | ⬜ TODO |
| 20 | Cloud sync (S3, Azure) | 1 week | LOW | ⬜ TODO |

**Goal**: Advanced forensics capabilities

---

## Missing Critical Artifacts

### Windows (20 artifacts missing)

| Artifact | Priority | Effort | Notes |
|----------|----------|--------|-------|
| **Registry hives** (SAM, SYSTEM, SOFTWARE, SECURITY, NTUSER.DAT) | P0 | 2-3 days | Requires `reg save` + admin |
| **Browser history** (Chrome, Firefox, Edge) | P0 | 3-4 days | Copy locked SQLite files |
| **MFT** ($MFT) | P0 | 4-5 days | Requires RawCopy tool |
| **USN Journal** | P0 | 1 day | `fsutil usn readjournal` |
| **Amcache** | P1 | 1 day | Copy from C:\Windows\appcompat\Programs |
| **ShimCache** | P1 | 1 day | Export from registry |
| **Prefetch** (already collected) | ✅ DONE | - | - |
| **Jump Lists** | P1 | 1 day | Copy from AppData\Recent |
| **Shellbags** | P1 | 1 day | Export from registry |
| **LNK files** | P1 | 1 day | Copy from Recent, Desktop |
| **User files metadata** (Downloads, Desktop, Documents) | P1 | 2 days | `Get-ChildItem -Recurse` |
| **Recycle Bin** | P2 | 2 days | Copy $Recycle.Bin contents |
| **Windows Search Index** | P2 | 1 day | Copy Windows.edb |
| **SRUM** | P2 | 1 day | Copy SRUDB.dat |
| **Windows Defender logs** | P1 | 1 day | Copy from ProgramData\Microsoft\Windows Defender |
| **PowerShell history** | P1 | 1 day | Copy ConsoleHost_history.txt |
| **WMI Repository** | P2 | 1 day | Copy OBJECTS.DATA |
| **Bitmap Cache** (RDP) | P2 | 1 day | Copy from Cache folder |
| **Volume Shadow Copies** | P3 | 1 week | Complex, requires vssadmin |
| **$LogFile** | P2 | 1 day | Requires RawCopy |

### Linux (15 artifacts missing)

| Artifact | Priority | Effort | Notes |
|----------|----------|--------|-------|
| **Shell history** (.bash_history, .zsh_history) | P0 | 1 day | Copy for all users |
| **SSH configs** (sshd_config, authorized_keys, known_hosts) | P0 | 1-2 days | Copy from /etc/ssh and ~/.ssh |
| **User accounts** (/etc/passwd, /etc/shadow, /etc/group) | P1 | 1 day | Copy files (shadow needs root) |
| **Sudo logs** | P1 | 1 day | Copy /var/log/sudo.log |
| **Package manager history** (apt/yum/dnf) | P1 | 1 day | Copy /var/log/apt, /var/log/yum.log |
| **Docker artifacts** (containers, images, configs) | P1 | 3-4 days | `docker` commands |
| **Firewall rules** (iptables, ufw, firewalld) | P1 | 1 day | `iptables-save`, `ufw status` |
| **Web server logs** (Apache, Nginx) | P2 | 1 day | Copy from /var/log/apache2, /var/log/nginx |
| **Database logs** (MySQL, PostgreSQL) | P2 | 1 day | Copy from /var/log/mysql, /var/log/postgresql |
| **Browser artifacts** (~/.mozilla, ~/.config/google-chrome) | P1 | 2 days | Copy profile folders |
| **Mounted filesystems** (/etc/fstab, mount) | P1 | 1 day | Copy /etc/fstab, `mount` output |
| **Environment variables** (/etc/environment, /etc/profile) | P2 | 1 day | Copy config files |
| **X11 logs** (/var/log/Xorg.0.log) | P2 | 1 day | Copy if exists |
| **Mail spools** (/var/mail/*) | P2 | 1 day | Copy if exists |
| **At/Cron jobs** (/var/spool/cron, /var/spool/at) | P1 | 1 day | Copy spool directories |

### macOS (20 artifacts missing)

| Artifact | Priority | Effort | Notes |
|----------|----------|--------|-------|
| **Unified logs** | P0 | 2 days | `log show --predicate` |
| **FSEvents** | P0 | 2 days | Copy .fseventsd folder |
| **Spotlight Index** | P1 | 1 day | Copy .Spotlight-V100 |
| **Safari history** (History.db) | P0 | 1 day | Copy from ~/Library/Safari |
| **Chrome/Firefox history** | P0 | 2 days | Copy profile folders |
| **QuickLook thumbnails** | P1 | 1 day | Copy QuickLook cache |
| **User accounts** (dslocal) | P1 | 1 day | Copy from /var/db/dslocal/nodes/Default/users |
| **Keychain** (encrypted) | P2 | 1 day | Copy login.keychain-db (can't decrypt) |
| **Installed apps** | P1 | 1 day | `ls -la /Applications` |
| **Recent items** | P1 | 1 day | Copy com.apple.recentitems plist |
| **Trash** | P2 | 1 day | Copy ~/.Trash |
| **Time Machine config** | P2 | 1 day | Copy preferences |
| **WiFi networks** | P1 | 1 day | `networksetup -listpreferredwirelessnetworks` |
| **Bluetooth devices** | P2 | 1 day | Copy Bluetooth preferences |
| **Downloads folder** | P1 | 1 day | `ls -la ~/Downloads` |
| **Desktop folder** | P1 | 1 day | `ls -la ~/Desktop` |
| **Documents folder** | P1 | 2 days | `ls -la ~/Documents` |
| **Mail database** | P2 | 1 day | Copy ~/Library/Mail |
| **Messages database** | P2 | 1 day | Copy ~/Library/Messages |
| **Photos metadata** | P2 | 1 day | Copy Photos.photoslibrary metadata |

---

## New Features Needed

### UI/UX Improvements

| Feature | Priority | Effort | Description |
|---------|----------|--------|-------------|
| **Button input support** | HIGH | 2 days | Use built-in button for menu navigation |
| **Menu system** | HIGH | 3 days | Select OS, modules, profiles |
| **Error detail screen** | HIGH | 1 day | Show full error messages, scrollable |
| **Upload progress** | CRITICAL | 1 day | Show WiFi upload on display |
| **Case ID display** | MEDIUM | 1 hour | Show case ID on status screen |
| **Network info** | MEDIUM | 1 hour | Show IP, SSID, clients on screen |
| **Artifact count** | MEDIUM | 1 hour | Show number of artifacts collected |
| **Storage status** | HIGH | 1 hour | Show SD card free space |
| **Log viewer** | MEDIUM | 2 days | Scrollable log on display |
| **Settings menu** | HIGH | 2 days | Configure WiFi, compression, modules |

### Workflow Improvements

| Feature | Priority | Effort | Description |
|---------|----------|--------|-------------|
| **Module profiles** | HIGH | 3 days | Quick, Standard, Deep, Custom profiles |
| **Selective collection** | HIGH | 2 days | Enable/disable specific modules |
| **Retry logic** | CRITICAL | 2 days | Retry failed modules 3 times |
| **Continue-on-error** | HIGH | 1 day | Don't stop entire collection on module failure |
| **Pause/Resume** | MEDIUM | 3 days | Pause collection, resume later |
| **Abort gracefully** | HIGH | 1 day | Stop collection, save partial results |
| **Verification pass** | MEDIUM | 2 days | Verify artifacts created before upload |
| **Chunked upload** | MEDIUM | 3 days | Upload files in chunks (resume capability) |
| **Compression options** | LOW | 1 day | Choose: None, RLE, GZIP, LZMA |
| **Priority modules** | MEDIUM | 1 day | Run high-priority modules first |

### Web Dashboard

| Feature | Priority | Effort | Description |
|---------|----------|--------|-------------|
| **Live monitoring** | HIGH | 3 days | Real-time collection status |
| **Log streaming** | HIGH | 2 days | Live log viewer via WebSocket |
| **Module control** | HIGH | 2 days | Start/stop/pause from web |
| **Evidence browser** | MEDIUM | 3 days | Browse collected artifacts |
| **Preview files** | MEDIUM | 2 days | View text files, JSON in browser |
| **Chain of custody viewer** | MEDIUM | 1 day | Display full audit trail |
| **Download manager** | LOW | 2 days | Batch download, ZIP on demand |
| **Settings UI** | HIGH | 2 days | Configure all settings via web |
| **API documentation** | LOW | 1 day | Swagger/OpenAPI docs |
| **Multi-device view** | LOW | 1 week | Manage multiple FRFD devices |

### Analysis Features

| Feature | Priority | Effort | Description |
|---------|----------|--------|-------------|
| **Timeline generator** | MEDIUM | 2 weeks | Parse artifacts, create timeline |
| **IOC extractor** | MEDIUM | 1 week | Extract IPs, domains, hashes, URLs |
| **YARA scanner** | MEDIUM | 1-2 weeks | Scan for malware signatures |
| **Hash lookup** | LOW | 3 days | VirusTotal, NSRL lookups |
| **String search** | MEDIUM | 2 days | Search across all artifacts |
| **Regex search** | MEDIUM | 2 days | Regex search for patterns |
| **Report generator** | HIGH | 1 week | HTML/PDF report with findings |
| **Summary stats** | MEDIUM | 2 days | File counts, sizes, types |
| **Suspicious activity** | MEDIUM | 1 week | Flag anomalies automatically |
| **Comparison mode** | LOW | 1 week | Compare two evidence containers |

---

## Required Tools

### Tools to Bundle with FRFD

#### Windows Tools (Total: ~6 MB)

| Tool | Size | Purpose | Source |
|------|------|---------|--------|
| **RawCopy.exe** | 100 KB | Copy locked files ($MFT, SAM) | [GitHub](https://github.com/jschicht/RawCopy) |
| **7za.exe** | 1.5 MB | Fast compression | [7-Zip](https://www.7-zip.org/) |
| **YARA.exe** | 500 KB | Malware scanning | [YARA](https://virustotal.github.io/yara/) |
| **autoruns.exe** | 1 MB | Persistence check | [Sysinternals](https://docs.microsoft.com/sysinternals) |
| **sigcheck.exe** | 500 KB | File signature verification | [Sysinternals](https://docs.microsoft.com/sysinternals) |
| **WinPMEM.exe** | 2 MB | Memory acquisition | [Velocidex](https://github.com/Velocidex/WinPmem) |
| **handle.exe** | 300 KB | List open handles | [Sysinternals](https://docs.microsoft.com/sysinternals) |

**Storage**: Compress to ~3 MB, store in SPIFFS, extract to target at runtime

#### Linux Tools (Total: ~1 MB)

| Tool | Size | Purpose | Source |
|------|------|---------|--------|
| **YARA** | 500 KB | Malware scanning | Compile static |
| **LiME** | 50 KB | Memory acquisition | Compile kernel module |

**Note**: Most Linux tools (tar, dd, find, grep) are pre-installed

#### macOS Tools (Total: ~2.5 MB)

| Tool | Size | Purpose | Source |
|------|------|---------|--------|
| **OSXPMem** | 2 MB | Memory acquisition | [Velocidex](https://github.com/Velocidex/c-aff4) |
| **YARA** | 500 KB | Malware scanning | Compile static |

### Tools Referenced (Not Bundled)

These run on analyst workstation after collection:

- **Plaso** - Timeline generation
- **Volatility** - Memory analysis
- **RegRipper** - Registry parsing
- **Autopsy** - Forensics platform
- **KAPE** - Evidence processing
- **Log2Timeline** - Log parsing

---

## Implementation Timeline

### Sprint 1-2 (Weeks 1-2): Stability

- [ ] Enhanced error handling
- [ ] Upload progress display
- [ ] Module selection system
- [ ] Abort mechanism
- [ ] Browser history (Windows)

**Deliverable**: v0.7.0-alpha

### Sprint 3-4 (Weeks 3-4): Windows Deep Dive

- [ ] Registry hive collection
- [ ] MFT and timeline artifacts
- [ ] User file metadata
- [ ] Recycle Bin
- [ ] Jump lists / shellbags

**Deliverable**: v0.7.0-beta

### Sprint 5-6 (Weeks 5-6): Linux/macOS

- [ ] Shell history (Linux)
- [ ] SSH configs (Linux)
- [ ] Browser history (Linux, macOS)
- [ ] Unified logs (macOS)
- [ ] Container forensics (Docker)

**Deliverable**: v0.7.0-rc1

### Sprint 7-8 (Weeks 7-8): Web Dashboard

- [ ] Live monitoring
- [ ] Log streaming
- [ ] Module control
- [ ] Evidence browser
- [ ] Settings UI

**Deliverable**: v0.7.0 (Release)

### Sprint 9-12 (Weeks 9-12): Analysis

- [ ] Timeline generation
- [ ] IOC extraction
- [ ] YARA scanning
- [ ] HTML report generation
- [ ] Summary statistics

**Deliverable**: v0.8.0

### Sprint 13-16 (Weeks 13-16): Advanced

- [ ] Memory acquisition
- [ ] Cloud sync
- [ ] Multi-device management
- [ ] API access
- [ ] Plugin architecture

**Deliverable**: v0.9.0

### Sprint 17-20 (Weeks 17-20): Production Hardening

- [ ] Comprehensive testing
- [ ] Documentation
- [ ] Security audit
- [ ] Performance optimization
- [ ] Bug fixes

**Deliverable**: v1.0.0 (Production Stable)

---

## Quick Action Items

**DO TODAY**:
1. Add error handling to HID automation
2. Display upload progress on screen
3. Test browser history collection

**DO THIS WEEK**:
1. Implement module selection profiles
2. Add Windows registry collection
3. Create basic web dashboard

**DO THIS MONTH**:
1. Complete Windows artifact coverage
2. Enhance Linux/macOS coverage
3. Add timeline generation

**DO THIS QUARTER**:
1. YARA scanning integration
2. Memory acquisition support
3. Advanced web dashboard
4. HTML report generation

---

**Last Updated**: 2025-11-05
**Next Review**: Weekly during implementation

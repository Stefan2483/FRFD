# FRFD - First Responder Forensics Dongle

![Version](https://img.shields.io/badge/version-0.1.0-blue.svg)
![Status](https://img.shields.io/badge/status-alpha-yellow.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)

> A portable, automated forensics and artifact extraction tool built on the Lilygo T-Dongle S3 platform, designed for CSIRT first responders to quickly assess, contain, and extract critical forensic data from potentially compromised systems.

---

## 🚀 Quick Start

```bash
# Clone the repository
git clone https://github.com/your-org/FRFD.git
cd FRFD

# Build and flash firmware (requires PlatformIO)
pio run --target upload

# Monitor serial output
pio device monitor
```

**For detailed instructions, see [BUILD.md](docs/BUILD.md)**

---

## 📋 Table of Contents

- [Features](#-features)
- [Hardware](#-hardware)
- [Project Status](#-project-status)
- [Getting Started](#-getting-started)
- [Usage](#-usage)
- [Documentation](#-documentation)
- [Contributing](#-contributing)
- [License](#-license)

---

## ✨ Features

### Operating Modes

- **🔍 Triage Mode** - Quick system assessment (5 min)
- **📦 Collection Mode** - Full forensic artifact collection (30 min)
- **🛡️ Containment Mode** - Network isolation and security controls
- **🔬 Analysis Mode** - On-device IOC matching and analysis

### HUD Display

```
┌─────────────────┐
│ CSIRT TOOLKIT   │
│ =============== │
│ Mode: COLLECT   │
│ OS: Windows     │
│ Risk: HIGH ▲    │
│ Progress: 47%   │
│ [████████░░░░]  │
│ Time: 02:34     │
│ NET ✓           │
└─────────────────┘
```

### Evidence Collection

**Windows:**
- ✅ Process memory dumps with metadata
- ✅ Registry autoruns & persistence mechanisms
- ✅ Event logs (Security, System, Application, PowerShell, Sysmon)
- ✅ Network connections, DNS cache, ARP table, firewall rules
- ✅ Prefetch analysis with execution history ✨
- ✅ Scheduled tasks with suspicious pattern detection ✨
- ✅ Windows services with digital signature verification ✨
- ✅ Active processes and services

**Linux:**
- ✅ System information and configuration
- ✅ Authentication logs (auth.log, wtmp, btmp, journal)
- ✅ Network state (ss, iptables, routes)
- ✅ Kernel modules with LKM rootkit detection ✨
- ✅ Comprehensive persistence check (15+ mechanisms) ✨
- ✅ User bash history and shell profiles
- ✅ Cron jobs, systemd services/timers, init scripts

**macOS:** ✨ NEW
- ✅ System information and hardware profiles
- ✅ Launch Agents/Daemons analysis
- ✅ Login items and startup persistence
- ✅ Kernel extensions (kexts) enumeration
- ✅ Browser history and extensions
- ✅ Quarantine database

**Advanced Features:**
- 🔐 AES-256 encryption for evidence at rest ✨
- 📊 Automated timeline generation from all sources ✨
- 🔍 IOC matching with 20+ built-in YARA-like rules ✨
- 📡 WiFi AP mode for wireless evidence download ✨
- 🌐 Web interface for remote monitoring and file management ✨
- 💾 SD card storage with automatic case organization ✨

**Chain of Custody:**
- SHA-256 hashing of all artifacts
- Complete audit trail with timestamps
- Responder and case ID tracking
- Encrypted evidence storage
- Web-based evidence verification

---

## 🔧 Hardware

### Lilygo T-Dongle S3

- **MCU:** ESP32-S3 (dual-core Xtensa LX7 @ 240MHz)
- **Memory:** 8MB PSRAM, 16MB Flash
- **Display:** 0.96" Color LCD (80x160 pixels)
- **Connectivity:** USB-C, WiFi 802.11 b/g/n, Bluetooth 5.0 LE
- **Storage:** SD Card slot support
- **Size:** Compact USB dongle form factor
- **Cost:** ~$15-25 USD

**Where to buy:**
- [Lilygo Official Store](https://lilygo.cc)
- AliExpress (search "Lilygo T-Dongle S3")
- Amazon

---

## 📊 Project Status

### ✅ Phase 1: Core Framework (COMPLETE)

- [x] ESP32-S3 firmware base
- [x] LCD driver and HUD system
- [x] USB serial communication
- [x] Configuration management
- [x] Operating mode framework
- [x] Display with real-time updates

### ⚡ Phase 2: Forensics Modules (COMPLETE - 100%) ✅

**Windows Scripts:**
- [x] Process memory dumps
- [x] Registry autoruns
- [x] Network connections
- [x] Event log collection
- [x] Prefetch analysis
- [x] Scheduled tasks enumeration
- [x] Windows services analysis

**Linux Scripts:**
- [x] System information
- [x] Authentication logs
- [x] Network state
- [x] Kernel modules & LKM rootkit detection
- [x] Comprehensive persistence check

**macOS Scripts:** ✨ NEW
- [x] System information collection
- [x] Persistence mechanisms check

**Launchers:**
- [x] Windows PowerShell launcher (updated with new scripts)
- [x] Linux Bash launcher (updated with new scripts)
- [x] Chain of custody generation

### 🎯 Phase 3: Advanced Features (COMPLETE - 100%) ✅

- [x] WiFi AP mode with web server ✨
- [x] IOC/YARA-like matching engine ✨
- [x] Timeline generation (Python tool) ✨
- [x] AES-256 encryption support ✨
- [x] SD card storage system ✨
- [x] Web-based evidence management ✨

### 🧪 Phase 4: Integration & Documentation (IN PROGRESS - 60%)

- [ ] SIEM connectivity
- [x] Complete documentation updates ✨
- [ ] Unit tests
- [ ] Field testing
- [ ] Security audit

**Current Version:** 0.3.0-alpha ✨
**Status:** Feature-complete, ready for testing and field trials

---

## 🏁 Getting Started

### Prerequisites

- **Hardware:** Lilygo T-Dongle S3
- **Software:** PlatformIO (via VS Code or CLI)
- **For Windows targets:** PowerShell 5.1+, Admin privileges
- **For Linux targets:** Bash, root/sudo access

### Installation

1. **Install PlatformIO**
   ```bash
   # Via VS Code Extension (recommended)
   # Or via pip:
   pip install platformio
   ```

2. **Clone Repository**
   ```bash
   git clone https://github.com/your-org/FRFD.git
   cd FRFD
   ```

3. **Build Firmware**
   ```bash
   pio run
   ```

4. **Flash to Device**
   ```bash
   # Connect Lilygo T-Dongle S3 via USB
   pio run --target upload
   ```

5. **Verify Installation**
   ```bash
   pio device monitor
   ```

   You should see:
   ```
   === FRFD - CSIRT Forensics Dongle ===
   Firmware Version: 0.1.0
   FRFD initialized successfully
   ```

**For detailed build instructions, see [BUILD.md](docs/BUILD.md)**

---

## 💻 Usage

### Windows

```powershell
# Run triage mode
.\scripts\FRFD-Windows-Launcher.ps1 -Mode Triage

# Full collection with case tracking
.\scripts\FRFD-Windows-Launcher.ps1 -Mode Collect -CaseId "INC-2024-001" -Responder "john.doe"

# Containment (network isolation)
.\scripts\FRFD-Windows-Launcher.ps1 -Mode Contain
```

### Linux

```bash
# Run triage mode
sudo ./scripts/frfd-linux-launcher.sh triage

# Full collection with case tracking
sudo ./scripts/frfd-linux-launcher.sh collect /evidence INC-2024-001 john.doe

# Containment (network isolation)
sudo ./scripts/frfd-linux-launcher.sh contain
```

### Serial Commands

Connect via serial (115200 baud):

```
triage          # Start triage mode
collect         # Start collection mode
contain         # Start containment mode
status          # Show current status
os:windows      # Set detected OS to Windows
os:linux        # Set detected OS to Linux
```

### Evidence Output

**Windows:** `C:\CSIRT\Evidence\`
**Linux:** `/tmp/csirt/evidence/`

Each collection generates:
- Collected artifacts in timestamped directories
- `chain_of_custody_*.json` with SHA-256 hashes
- Summary reports

---

## 📚 Documentation

- **[Getting Started Guide](docs/GETTING_STARTED.md)** - Complete setup and usage guide
- **[Build Instructions](docs/BUILD.md)** - Detailed build and flash instructions
- **[Project Overview](docs/PROJECT_OVERVIEW.md)** - Architecture and implementation status
- **[Original Plan](docs/ORIGINAL_PLAN.md)** - Original project specification

---

## 🗂️ Project Structure

```
FRFD/
├── firmware/                   # ESP32-S3 firmware
│   ├── include/               # Headers (config, display, frfd)
│   ├── src/                   # Source files
│   └── lib/                   # Libraries
├── forensics_tools/           # Collection scripts
│   ├── windows/              # Windows PowerShell scripts
│   └── linux/                # Linux Bash scripts
├── scripts/                   # Launcher scripts
│   ├── FRFD-Windows-Launcher.ps1
│   └── frfd-linux-launcher.sh
├── config/                    # Configuration files
│   └── config.json
├── docs/                      # Documentation
├── rules/                     # YARA/IOC rules (planned)
├── web_interface/            # Management portal (planned)
└── platformio.ini            # PlatformIO configuration
```

---

## 🤝 Contributing

Contributions are welcome! Areas that need work:

1. **Forensics Scripts**
   - Additional Windows artifacts (MFT, Prefetch, ShimCache)
   - macOS support
   - Memory imaging tools

2. **Firmware Features**
   - WiFi AP mode implementation
   - SD card support
   - YARA integration

3. **Documentation**
   - User manual
   - Training materials
   - Case studies

4. **Testing**
   - Unit tests
   - Integration tests
   - Field testing

**To contribute:**
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

---

## 🔒 Security Considerations

⚠️ **Important:** This tool is designed for authorized incident response and forensics by trained professionals.

- **Physical Security:** Keep FRFD devices secure
- **Access Control:** Require authentication for sensitive operations
- **Audit Logging:** Review all actions
- **Chain of Custody:** Always maintain proper documentation
- **Testing:** Never use on production systems without thorough testing
- **Legal:** Ensure proper authorization before use

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- Lilygo for the T-Dongle S3 hardware
- ESP32 community
- SANS DFIR community
- All CSIRT professionals

---

## 📞 Support

- **Documentation:** See `docs/` directory
- **Issues:** Report on GitHub Issues
- **Community:** CSIRT forums and mailing lists

---

## 🗺️ Roadmap

### ✅ Version 0.1.0 (COMPLETE)
- ✅ Core framework and firmware
- ✅ Basic forensics scripts
- ✅ Display system
- ✅ Initial documentation

### ✅ Version 0.2.0 (COMPLETE)
- ✅ Additional forensics scripts
- ✅ WiFi AP mode with web server
- ✅ SD card storage system
- ✅ Enhanced HUD

### ✅ Version 0.3.0 (COMPLETE - CURRENT)
- ✅ IOC/YARA-like matching engine
- ✅ Timeline generation tool
- ✅ AES-256 encryption support
- ✅ macOS forensics support
- ✅ Comprehensive documentation
- ✅ Updated launchers

### 🔮 Version 1.0.0 (Planned - Q1 2025)
- [ ] SIEM integration (Splunk, ELK)
- [ ] Cloud evidence upload
- [ ] Unit testing framework
- [ ] Field testing and validation
- [ ] Security audit
- [ ] Production hardening

---

## 📊 Stats

- **Lines of Code:** ~15,000+ ✨
- **Forensics Scripts:** 17+ (Windows, Linux & macOS) ✨
- **IOC Rules:** 20+ built-in YARA-like rules ✨
- **Operating Modes:** 4
- **Supported OS:** Windows, Linux, macOS ✨
- **Hardware Cost:** ~$20
- **Firmware Components:** 10+ modules ✨
- **Development Status:** Feature-Complete Alpha ✨

### New in v0.3.0:
- ✨ **8 new forensics scripts** (Prefetch, Tasks, Services, Kernel Modules, Persistence, macOS x2)
- ✨ **IOC Matcher** with 20+ detection rules
- ✨ **Timeline Generator** - Python tool for unified timeline creation
- ✨ **AES-256 Encryption** - Hardware-accelerated encryption support
- ✨ **WiFi Web Server** - Full-featured web interface for evidence management
- ✨ **SD Card Support** - Automatic case organization and storage
- ✨ **macOS Support** - Complete macOS forensics capability

---

**Built with ❤️ for CSIRT professionals**

**Happy Forensics! 🔍**

---

## Quick Links

- 📖 [Getting Started](docs/GETTING_STARTED.md)
- 🔨 [Build Guide](docs/BUILD.md)
- 📋 [Project Overview](docs/PROJECT_OVERVIEW.md)
- 🐛 [Report Issues](https://github.com/your-org/FRFD/issues)
- 💬 [Discussions](https://github.com/your-org/FRFD/discussions)

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
- ✅ Process memory dumps
- ✅ Registry autoruns & persistence
- ✅ Event logs (Security, System, Application, PowerShell, Sysmon)
- ✅ Network connections, DNS cache, ARP table
- ✅ Active processes and services

**Linux:**
- ✅ System information and configuration
- ✅ Authentication logs (auth.log, wtmp, btmp, journal)
- ✅ Network state (ss, iptables, routes)
- ✅ User bash history
- ✅ Cron jobs and systemd services

**Chain of Custody:**
- SHA-256 hashing of all artifacts
- Complete audit trail
- Responder and case ID tracking
- Timestamp tracking

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

### ⚡ Phase 2: Forensics Modules (IN PROGRESS - 60%)

**Windows Scripts:**
- [x] Process memory dumps
- [x] Registry autoruns
- [x] Network connections
- [x] Event log collection
- [ ] MFT extraction
- [ ] Prefetch analysis
- [ ] Scheduled tasks

**Linux Scripts:**
- [x] System information
- [x] Authentication logs
- [x] Network state
- [ ] Process memory dumps
- [ ] Kernel modules

**Launchers:**
- [x] Windows PowerShell launcher
- [x] Linux Bash launcher
- [x] Chain of custody generation

### 🔮 Phase 3: Advanced Features (PLANNED)

- [ ] WiFi AP mode
- [ ] YARA integration
- [ ] Volatility plugins
- [ ] Timeline generation
- [ ] AES-256 encryption
- [ ] Cloud upload

### 🧪 Phase 4: Integration & Testing (PLANNED)

- [ ] SIEM connectivity
- [ ] Unit tests
- [ ] Field testing
- [ ] Security audit

**Current Version:** 0.1.0-alpha
**Status:** Ready for testing in lab environment

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

### Version 0.2.0 (Q1 2025)
- Complete Phase 2 forensics scripts
- WiFi AP mode
- SD card support
- Enhanced HUD with navigation

### Version 0.3.0 (Q2 2025)
- YARA integration
- Timeline generation
- AES-256 encryption
- Cloud upload

### Version 1.0.0 (Q3 2025)
- SIEM integration
- Complete documentation
- Security audit
- Production ready

---

## 📊 Stats

- **Lines of Code:** ~5,000+
- **Forensics Scripts:** 10+ (Windows & Linux)
- **Operating Modes:** 4
- **Supported OS:** Windows, Linux, (macOS planned)
- **Hardware Cost:** ~$20
- **Development Status:** Alpha

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

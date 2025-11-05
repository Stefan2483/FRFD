# FRFD - Getting Started Guide

## First Responder Forensics Dongle

This guide will help you get started with building and deploying the FRFD (First Responder Forensics Dongle) for your CSIRT team.

---

## Table of Contents

1. [Hardware Requirements](#hardware-requirements)
2. [Software Requirements](#software-requirements)
3. [Building the Firmware](#building-the-firmware)
4. [Deploying Scripts](#deploying-scripts)
5. [Using FRFD](#using-frfd)
6. [Troubleshooting](#troubleshooting)

---

## Hardware Requirements

### Lilygo T-Dongle S3

- **MCU**: ESP32-S3 (dual-core Xtensa LX7 @ 240MHz)
- **Memory**: 8MB PSRAM, 16MB Flash
- **Display**: 0.96" Color LCD (80x160 pixels)
- **Connectivity**: USB-C, WiFi 802.11 b/g/n, Bluetooth 5.0 LE
- **Storage**: SD Card slot (optional, for evidence storage)

### Where to Buy

- Official Lilygo Store: [lilygo.cc](https://lilygo.cc)
- AliExpress: Search for "Lilygo T-Dongle S3"
- Amazon: Available from various sellers

**Cost**: Approximately $15-25 USD

---

## Software Requirements

### Development Tools

1. **PlatformIO** (recommended)
   - Install via VS Code extension
   - Or install PlatformIO Core CLI

2. **Arduino IDE** (alternative)
   - Version 2.0 or higher
   - ESP32 board support

3. **USB Drivers**
   - CP210x or CH340 drivers (usually auto-installed)

### Host System Requirements

#### For Windows Targets
- PowerShell 5.1 or higher
- Administrator privileges
- .NET Framework 4.5+

#### For Linux Targets
- Bash shell
- Root/sudo access
- Common utilities (ss, iptables, journalctl, etc.)

---

## Building the Firmware

### Option 1: Using PlatformIO (Recommended)

1. **Install PlatformIO**
   ```bash
   # Using VS Code
   # Install "PlatformIO IDE" extension from marketplace

   # Or using pip
   pip install platformio
   ```

2. **Clone or navigate to FRFD directory**
   ```bash
   cd /path/to/FRFD
   ```

3. **Build the firmware**
   ```bash
   pio run
   ```

4. **Upload to device**
   ```bash
   # Connect Lilygo T-Dongle S3 via USB
   pio run --target upload
   ```

5. **Monitor serial output**
   ```bash
   pio device monitor
   ```

### Option 2: Using Arduino IDE

1. **Install ESP32 Board Support**
   - Open Arduino IDE
   - Go to File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools → Board → Boards Manager
   - Search for "ESP32" and install

2. **Install Required Libraries**
   - TFT_eSPI
   - ArduinoJson
   - Adafruit TinyUSB

3. **Configure TFT_eSPI**
   - Locate TFT_eSPI library folder
   - Edit `User_Setup.h` for Lilygo T-Dongle S3:
     ```cpp
     #define ST7735_DRIVER
     #define TFT_WIDTH  80
     #define TFT_HEIGHT 160
     #define TFT_CS    4
     #define TFT_DC    2
     #define TFT_RST   8
     ```

4. **Open and Upload**
   - Open `firmware/src/main.cpp`
   - Select Board: "ESP32S3 Dev Module"
   - Select Port
   - Click Upload

### Verifying Installation

After upload, you should see on the LCD:
```
┌─────────────────┐
│    FRFD         │
│  CSIRT Toolkit  │
│  v0.1.0         │
└─────────────────┘
```

Serial output should show:
```
=== FRFD - CSIRT Forensics Dongle ===
Firmware Version: 0.1.0
FRFD initialized successfully
```

---

## Deploying Scripts

### Windows Deployment

1. **Copy scripts to target system** (or prepare them on the dongle's SD card)
   ```powershell
   # Copy forensics tools
   Copy-Item -Recurse forensics_tools\windows C:\CSIRT\Tools\

   # Copy launcher
   Copy-Item scripts\FRFD-Windows-Launcher.ps1 C:\CSIRT\
   ```

2. **Set execution policy** (if needed)
   ```powershell
   Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
   ```

### Linux Deployment

1. **Copy scripts to target system**
   ```bash
   # Copy forensics tools
   sudo cp -r forensics_tools/linux /opt/csirt/tools/

   # Copy launcher
   sudo cp scripts/frfd-linux-launcher.sh /opt/csirt/
   sudo chmod +x /opt/csirt/frfd-linux-launcher.sh
   ```

2. **Make scripts executable**
   ```bash
   sudo chmod +x /opt/csirt/tools/linux/**/*.sh
   ```

---

## Using FRFD

### Basic Workflow

1. **Power on FRFD** by connecting via USB
2. **Device auto-detects OS** (or set manually via serial)
3. **Select mode** via button or serial commands
4. **Scripts execute** automatically
5. **Collect evidence** from output directory

### Operating Modes

#### 1. Triage Mode (Default)
Quick system assessment without full collection.

**Windows:**
```powershell
.\FRFD-Windows-Launcher.ps1 -Mode Triage
```

**Linux:**
```bash
sudo ./frfd-linux-launcher.sh triage
```

**What it does:**
- Quick system scan
- Network connections check
- Running processes enumeration
- Risk level assessment
- Takes ~5 minutes

#### 2. Collection Mode
Full forensic artifact collection.

**Windows:**
```powershell
.\FRFD-Windows-Launcher.ps1 -Mode Collect -CaseId "INC-2024-001" -Responder "john.doe"
```

**Linux:**
```bash
sudo ./frfd-linux-launcher.sh collect /evidence INC-2024-001 john.doe
```

**What it collects:**
- Memory dumps (processes)
- Registry keys (Windows) / Config files (Linux)
- Event logs / System logs
- Network state
- User artifacts
- Persistence mechanisms
- Takes ~30 minutes

#### 3. Containment Mode
Implements isolation and security controls.

**Windows:**
```powershell
.\FRFD-Windows-Launcher.ps1 -Mode Contain
```

**Linux:**
```bash
sudo ./frfd-linux-launcher.sh contain
```

**What it does:**
- Network isolation
- Firewall rules
- Blocks outbound connections
- Logs all actions
- **⚠️ Use with caution!**

#### 4. Analysis Mode
On-device analysis (limited implementation).

**Windows:**
```powershell
.\FRFD-Windows-Launcher.ps1 -Mode Analyze
```

**Linux:**
```bash
sudo ./frfd-linux-launcher.sh analyze
```

### Using Serial Commands

Connect to FRFD via serial terminal (115200 baud):

```
# Available commands:
triage          # Start triage mode
collect         # Start collection mode
contain         # Start containment mode
analyze         # Start analysis mode
status          # Show current status
os:windows      # Set detected OS to Windows
os:linux        # Set detected OS to Linux
os:macos        # Set detected OS to macOS
```

**Example:**
```bash
# Using screen
screen /dev/ttyUSB0 115200

# Using minicom
minicom -D /dev/ttyUSB0 -b 115200

# Then type commands:
> os:linux
> collect
```

### LCD Display

The HUD shows real-time status:
```
┌─────────────────┐
│ CSIRT TOOLKIT   │
│ =============== │
│ Mode: COLLECT   │
│ OS: Linux       │
│ Risk: MEDIUM    │
│ Progress: 47%   │
│ [████████░░░░]  │
│ Time: 02:34     │
│ NET             │
└─────────────────┘
```

---

## Evidence Collection

### Output Structure

**Windows:**
```
C:\CSIRT\Evidence\
├── ProcessDumps_20241105_143022\
├── Autoruns_20241105_143045\
├── NetworkConnections_20241105_143112\
├── EventLogs_20241105_143201\
└── chain_of_custody_20241105_143500.json
```

**Linux:**
```
/tmp/csirt/evidence/
├── SystemInfo_20241105_143022/
├── AuthLogs_20241105_143045/
├── Network_20241105_143112/
└── chain_of_custody_20241105_143500.json
```

### Chain of Custody

Every operation generates a `chain_of_custody_*.json` file:

```json
{
  "case_id": "INC-2024-001",
  "responder": "john.doe",
  "device_id": "FRFD-001",
  "hostname": "suspect-pc",
  "start_time": "2024-11-05T14:30:00Z",
  "end_time": "2024-11-05T14:45:00Z",
  "duration_seconds": 900,
  "mode": "collect",
  "artifacts": [
    {
      "filename": "lsass_672.dmp",
      "size": 45678901,
      "hash": "sha256:abc123..."
    }
  ]
}
```

---

## Configuration

### Device Configuration

Edit `config/config.json` and copy to device SPIFFS:

```json
{
  "device_config": {
    "device_id": "FRFD-001",
    "organization": "YOUR-CSIRT",
    "wifi_ssid": "CSIRT-FORENSICS",
    "wifi_password": "ChangeThisPassword123!"
  },
  "operational_config": {
    "default_mode": "triage",
    "collection_timeout": 300
  }
}
```

### Custom Scripts

Add your own collection scripts:

**Windows:**
```
forensics_tools/windows/custom/
└── my_script.ps1
```

**Linux:**
```
forensics_tools/linux/custom/
└── my_script.sh
```

---

## Troubleshooting

### Firmware Issues

**Device not recognized:**
- Install CP210x or CH340 drivers
- Try different USB cable
- Press BOOT button while connecting

**Upload fails:**
- Hold BOOT button during upload
- Check correct COM/tty port selected
- Reduce upload speed in platformio.ini

**Display not working:**
- Verify TFT_eSPI configuration
- Check pin definitions in `config.h`
- Test with example sketches first

### Script Issues

**PowerShell execution policy:**
```powershell
Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process
```

**Linux permission denied:**
```bash
sudo chmod +x script.sh
sudo ./script.sh
```

**Scripts not found:**
- Verify paths in launcher scripts
- Check FORENSICS_DIR variable
- Ensure scripts copied correctly

### Collection Issues

**No evidence collected:**
- Verify admin/root privileges
- Check output path permissions
- Review error messages in serial output

**Timeout errors:**
- Increase timeout in config
- Split large collections into parts
- Check system resources

---

## Next Steps

1. **Test in Lab Environment**
   - Practice with test VMs
   - Validate evidence collection
   - Review chain of custody

2. **Customize for Your Organization**
   - Add custom IOC rules
   - Configure SIEM integration
   - Set up evidence server

3. **Training**
   - Train CSIRT team on usage
   - Document procedures
   - Conduct tabletop exercises

4. **Advanced Features** (Phase 2+)
   - WiFi AP for wireless collection
   - YARA rule matching
   - Volatility integration
   - Timeline generation

---

## Support and Resources

- **Documentation**: `docs/` directory
- **Examples**: `examples/` directory
- **Issues**: Report on GitHub
- **Community**: CSIRT forums and mailing lists

---

## Security Considerations

⚠️ **Important Security Notes:**

1. **Physical Security**: Keep FRFD device secure
2. **Encryption**: Enable encryption for sensitive data
3. **Access Control**: Require authentication for operations
4. **Audit Logging**: Review all actions in logs
5. **Chain of Custody**: Always generate and preserve
6. **Testing**: Never use on production without testing

---

## License

See LICENSE file for details.

---

**Happy Forensics! 🔍**

# FRFD Build Instructions

Complete guide for building and flashing the FRFD firmware to Lilygo T-Dongle S3.

---

## Prerequisites

### Hardware
- Lilygo T-Dongle S3 device
- USB-C cable (data capable)
- Computer (Windows, Linux, or macOS)

### Software

#### Install PlatformIO

**Option 1: VS Code Extension (Recommended)**
1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Open VS Code
3. Go to Extensions (Ctrl+Shift+X)
4. Search for "PlatformIO IDE"
5. Click Install
6. Restart VS Code

**Option 2: PlatformIO Core CLI**
```bash
# Using Python pip
pip install platformio

# Or using Homebrew (macOS)
brew install platformio
```

---

## Build Process

### 1. Clone or Download FRFD

```bash
git clone https://github.com/your-org/FRFD.git
cd FRFD
```

### 2. Open Project in PlatformIO

**Using VS Code:**
1. Open VS Code
2. File → Open Folder
3. Select the FRFD directory
4. PlatformIO will auto-detect the project

**Using CLI:**
```bash
cd FRFD
pio project init --ide vscode
```

### 3. Install Dependencies

PlatformIO will automatically install:
- ESP32 framework
- TFT_eSPI library
- ArduinoJson library
- Other dependencies listed in `platformio.ini`

**Manual installation (if needed):**
```bash
pio pkg install
```

### 4. Configure Display (Important!)

The TFT_eSPI library needs configuration for the Lilygo T-Dongle S3 display.

**Option A: Pre-configured (Recommended)**

Create `firmware/lib/TFT_eSPI_Config/User_Setup.h`:

```cpp
// Lilygo T-Dongle S3 Configuration
#define ST7735_DRIVER
#define TFT_WIDTH  80
#define TFT_HEIGHT 160

#define TFT_CS    4
#define TFT_DC    2
#define TFT_RST   8
#define TFT_MOSI  3
#define TFT_SCLK  5
#define TFT_BL    38

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8

#define SMOOTH_FONT

#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
```

**Option B: Edit Library (Alternative)**

Find TFT_eSPI library folder:
```bash
# Location varies by installation
# Common paths:
# ~/.platformio/lib/TFT_eSPI/
# or in project: .pio/libdeps/lilygo-t-dongle-s3/TFT_eSPI/

# Edit User_Setup.h with the configuration above
```

### 5. Build Firmware

**Using VS Code:**
1. Click PlatformIO icon in sidebar
2. Click "Build" under PROJECT TASKS
3. Wait for compilation to complete

**Using CLI:**
```bash
pio run
```

**Expected output:**
```
Processing lilygo-t-dongle-s3 (platform: espressif32; board: esp32-s3-devkitc-1; framework: arduino)
...
RAM:   [==        ]  15.2% (used 49856 bytes from 327680 bytes)
Flash: [====      ]  35.8% (used 591201 bytes from 1638400 bytes)
========================= [SUCCESS] Took 45.23 seconds =========================
```

### 6. Flash to Device

#### Connect Device

1. Connect Lilygo T-Dongle S3 to USB port
2. Device should be recognized as COM/ttyUSB/ttyACM device

**Check port:**

*Windows:*
```powershell
# Device Manager → Ports (COM & LPT)
# Look for "USB-SERIAL CH340" or "CP210x"
# Note COM port number (e.g., COM3)
```

*Linux:*
```bash
ls /dev/ttyUSB* /dev/ttyACM*
# Usually /dev/ttyUSB0 or /dev/ttyACM0

# Add user to dialout group (if needed)
sudo usermod -a -G dialout $USER
# Log out and back in
```

*macOS:*
```bash
ls /dev/cu.*
# Look for /dev/cu.usbserial-* or /dev/cu.wchusbserial*
```

#### Upload Firmware

**Using VS Code:**
1. Click PlatformIO icon
2. Click "Upload" under PROJECT TASKS
3. Wait for upload to complete

**Using CLI:**
```bash
# Auto-detect port
pio run --target upload

# Or specify port manually
pio run --target upload --upload-port /dev/ttyUSB0  # Linux
pio run --target upload --upload-port COM3          # Windows
```

**If upload fails with "Timed out waiting for packet header":**

1. Hold the BOOT button (GPIO0)
2. While holding BOOT, click Upload
3. Release BOOT when "Connecting..." appears
4. Upload should proceed

### 7. Monitor Serial Output

**Using VS Code:**
1. Click PlatformIO icon
2. Click "Monitor" under PROJECT TASKS

**Using CLI:**
```bash
pio device monitor
```

**Using other tools:**
```bash
# Linux - screen
screen /dev/ttyUSB0 115200

# Linux - minicom
minicom -D /dev/ttyUSB0 -b 115200

# Windows - PuTTY
# Set Serial, COM3, 115200 baud

# macOS - screen
screen /dev/cu.usbserial-* 115200
```

**Expected serial output:**
```
=== FRFD - CSIRT Forensics Dongle ===
Firmware Version: 0.1.0
Loading configuration...
Device ID: FRFD-001
Organization: CSIRT-TEAM
Initializing USB...
Initializing storage...
FRFD initialized successfully

FRFD ready for operations
Available commands:
  triage  - Run triage mode
  collect - Run collection mode
  contain - Run containment mode
  analyze - Run analysis mode
  status  - Show status and chain of custody
  os:<windows|linux|macos> - Set detected OS
```

---

## Troubleshooting

### Common Issues

#### 1. Device Not Detected

**Symptoms:**
- No COM/tty port appears
- "Device not found" error

**Solutions:**
```bash
# Install drivers

# Windows - CH340 driver
# Download from: http://www.wch.cn/downloads/CH341SER_EXE.html

# Or CP210x driver
# Download from: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

# Linux - usually works out of box, but check:
lsusb  # Should show device
dmesg | tail -20  # Check for errors

# Add udev rules if needed:
sudo nano /etc/udev/rules.d/99-platformio-udev.rules
# Add:
# SUBSYSTEMS=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", MODE="0666"
sudo udevadm control --reload-rules
```

#### 2. Upload Fails

**Symptoms:**
- "Timed out waiting for packet header"
- "Failed to connect to ESP32"

**Solutions:**
1. **Hold BOOT button** during upload
2. Try different USB cable (must support data)
3. Reduce upload speed in `platformio.ini`:
   ```ini
   upload_speed = 460800  # Instead of 921600
   ```
4. Try different USB port
5. Restart device

#### 3. Display Not Working

**Symptoms:**
- Firmware uploads but display stays blank
- Serial works but no LCD output

**Solutions:**
1. Check TFT_eSPI configuration
2. Verify pin definitions in `config.h`
3. Test backlight:
   ```cpp
   pinMode(38, OUTPUT);
   digitalWrite(38, HIGH);
   ```
4. Check for hardware issues

#### 4. Compilation Errors

**Symptoms:**
- "fatal error: TFT_eSPI.h: No such file or directory"
- Library not found errors

**Solutions:**
```bash
# Clean and rebuild
pio run --target clean
pio pkg install
pio run

# Check library dependencies
pio pkg list

# Update libraries
pio pkg update
```

#### 5. Out of Memory Errors

**Symptoms:**
- "region `dram0_0_seg' overflowed"
- RAM or Flash overflow

**Solutions:**
1. Check partition scheme in `platformio.ini`
2. Reduce debug level
3. Optimize code
4. Use PSRAM for large buffers

---

## Advanced Configuration

### Custom Partitions

Edit `platformio.ini`:

```ini
board_build.partitions = custom_partitions.csv
```

Create `custom_partitions.csv`:
```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x300000,
app1,     app,  ota_1,   0x310000,0x300000,
spiffs,   data, spiffs,  0x610000,0x1F0000,
```

### Over-The-Air (OTA) Updates

Enable OTA in `platformio.ini`:

```ini
upload_protocol = espota
upload_port = 192.168.4.1  # Device IP
upload_flags =
    --port=3232
    --auth=otapassword
```

### Debug Settings

Increase debug output:

```ini
build_flags =
    -DCORE_DEBUG_LEVEL=5  # Verbose
    -DFRFD_DEBUG=1
```

---

## Production Build

For production deployment:

1. **Update configuration**
   ```bash
   # Edit config/config.json
   # Set proper device_id, organization, credentials
   ```

2. **Disable debug**
   ```ini
   # platformio.ini
   build_flags =
       -DCORE_DEBUG_LEVEL=0  # None
   ```

3. **Enable security features**
   ```ini
   board_build.flash_mode = dio
   board_build.f_flash = 80000000L
   board_security.flash_encryption = true
   board_security.secure_boot = true
   ```

4. **Build release**
   ```bash
   pio run --environment release
   ```

5. **Flash firmware**
   ```bash
   pio run --target upload --environment release
   ```

6. **Upload SPIFFS data** (config files)
   ```bash
   pio run --target uploadfs
   ```

---

## Firmware Updates

### Update Process

1. **Build new firmware**
   ```bash
   pio run
   ```

2. **Backup current version** (if needed)
   ```bash
   esptool.py --port /dev/ttyUSB0 read_flash 0x0 0x400000 backup.bin
   ```

3. **Upload new version**
   ```bash
   pio run --target upload
   ```

4. **Verify**
   - Check serial output
   - Test on display
   - Verify functionality

---

## Testing Firmware

### Basic Tests

1. **Boot Test**
   - Verify boot screen appears
   - Check serial initialization

2. **Display Test**
   - Verify HUD renders correctly
   - Test progress bars
   - Check color display

3. **Serial Test**
   ```
   # Send commands via serial:
   os:windows
   triage
   status
   ```

4. **Mode Test**
   - Test all operating modes
   - Verify state transitions
   - Check display updates

5. **Integration Test**
   - Connect to real Windows/Linux system
   - Run collection scripts
   - Verify evidence collection

---

## Build Automation

### CI/CD Pipeline

Example GitHub Actions workflow:

```yaml
name: Build FRFD

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-python@v4
        with:
          python-version: '3.x'
      - name: Install PlatformIO
        run: pip install platformio
      - name: Build firmware
        run: pio run
      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: firmware
          path: .pio/build/*/firmware.bin
```

---

## Support

If you encounter issues:

1. Check this documentation
2. Review PlatformIO documentation
3. Check ESP32-S3 documentation
4. Review Lilygo T-Dongle S3 specifications
5. Open GitHub issue with:
   - Error messages
   - Build output
   - System information
   - Steps to reproduce

---

## Next Steps

After successful build and flash:

1. Read [GETTING_STARTED.md](GETTING_STARTED.md)
2. Test basic functionality
3. Deploy forensics scripts
4. Configure for your environment
5. Train your team

---

**Happy Building! 🔨**

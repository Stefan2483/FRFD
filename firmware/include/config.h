#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Device Information
#define DEVICE_NAME "FRFD"
#define FIRMWARE_VERSION "0.8.0-alpha"
#define HARDWARE_VERSION "Lilygo T-Dongle S3"

// Display Configuration (ST7735 - 80x160 pixels)
#define TFT_WIDTH 80
#define TFT_HEIGHT 160
#define TFT_ROTATION 1

// Pin Definitions for Lilygo T-Dongle S3
#define TFT_CS    4
#define TFT_RST   8
#define TFT_DC    2
#define TFT_MOSI  3
#define TFT_SCK   5
#define TFT_BL    38

// Button Configuration
#define BUTTON_PIN 0  // Boot button

// SD Card Configuration
#define SD_CS 10
#define SD_MOSI 11
#define SD_MISO 13
#define SD_SCK 12

// USB Configuration
#define USB_VENDOR_ID 0x303A
#define USB_PRODUCT_ID 0x8200
#define USB_MANUFACTURER "CSIRT-TEAM"
#define USB_PRODUCT "FRFD Forensics Dongle"

// Memory Configuration
#define PSRAM_SIZE (8 * 1024 * 1024)  // 8MB PSRAM
#define FLASH_SIZE (16 * 1024 * 1024) // 16MB Flash

// WiFi Configuration
#define WIFI_AP_SSID "CSIRT-FORENSICS"
#define WIFI_AP_PASSWORD "ChangeThisPassword123!"
#define WIFI_AP_CHANNEL 6
#define WIFI_MAX_CLIENTS 4

// Operational Timeouts
#define OS_DETECTION_TIMEOUT 1000      // 1 second
#define TRIAGE_TIMEOUT 300000          // 5 minutes
#define COLLECTION_TIMEOUT 1800000     // 30 minutes
#define COMMAND_TIMEOUT 30000          // 30 seconds

// Buffer Sizes
#define SERIAL_BUFFER_SIZE 4096
#define JSON_BUFFER_SIZE 8192
#define COMMAND_BUFFER_SIZE 1024

// Operating Modes
enum OperatingMode {
    MODE_TRIAGE,
    MODE_COLLECTION,
    MODE_CONTAINMENT,
    MODE_ANALYSIS,
    MODE_CONFIG,
    MODE_IDLE
};

// Operating Systems
enum OperatingSystem {
    OS_UNKNOWN,
    OS_WINDOWS,
    OS_LINUX,
    OS_MACOS
};

// Risk Levels
enum RiskLevel {
    RISK_UNKNOWN,
    RISK_LOW,
    RISK_MEDIUM,
    RISK_HIGH,
    RISK_CRITICAL
};

// Collection Status
enum CollectionStatus {
    STATUS_IDLE,
    STATUS_DETECTING,
    STATUS_COLLECTING,
    STATUS_ANALYZING,
    STATUS_TRANSFERRING,
    STATUS_COMPLETE,
    STATUS_ERROR
};

#endif // CONFIG_H

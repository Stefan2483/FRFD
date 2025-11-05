#include "storage.h"

FRFDStorage::FRFDStorage() {
    sdCardAvailable = false;
    spiffsAvailable = false;
    sdCardSize = 0;
    sdCardUsed = 0;
    spiffsSize = 0;
    spiffsUsed = 0;
    currentCaseDir = "";
}

bool FRFDStorage::begin() {
    Serial.println("[Storage] Initializing storage systems...");

    bool success = true;

    // Initialize SPIFFS for configuration
    if (initSPIFFS()) {
        Serial.println("[Storage] SPIFFS initialized successfully");
    } else {
        Serial.println("[Storage] SPIFFS initialization failed");
        success = false;
    }

    // Initialize SD card for evidence storage
    if (initSDCard()) {
        Serial.println("[Storage] SD Card initialized successfully");
    } else {
        Serial.println("[Storage] SD Card not available (optional)");
        // SD card is optional, not a fatal error
    }

    printStorageInfo();

    return success;
}

bool FRFDStorage::initSDCard() {
    Serial.println("[Storage] Initializing SD Card...");

    // Initialize SD card with SPI pins
    if (!SD.begin(SD_CS)) {
        Serial.println("[Storage] SD Card mount failed");
        sdCardAvailable = false;
        return false;
    }

    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE) {
        Serial.println("[Storage] No SD card attached");
        sdCardAvailable = false;
        return false;
    }

    // Print card info
    Serial.print("[Storage] SD Card Type: ");
    switch (cardType) {
        case CARD_MMC:
            Serial.println("MMC");
            break;
        case CARD_SD:
            Serial.println("SD");
            break;
        case CARD_SDHC:
            Serial.println("SDHC");
            break;
        default:
            Serial.println("UNKNOWN");
    }

    sdCardSize = SD.cardSize() / (1024 * 1024); // MB
    sdCardUsed = SD.usedBytes() / (1024 * 1024); // MB

    Serial.printf("[Storage] SD Card Size: %llu MB\n", sdCardSize);
    Serial.printf("[Storage] SD Card Used: %llu MB\n", sdCardUsed);

    // Create base evidence directory
    if (!SD.exists("/evidence")) {
        SD.mkdir("/evidence");
        Serial.println("[Storage] Created /evidence directory");
    }

    sdCardAvailable = true;
    return true;
}

bool FRFDStorage::initSPIFFS() {
    Serial.println("[Storage] Initializing SPIFFS...");

    if (!SPIFFS.begin(true)) { // format on fail
        Serial.println("[Storage] SPIFFS mount failed");
        spiffsAvailable = false;
        return false;
    }

    spiffsSize = SPIFFS.totalBytes() / 1024; // KB
    spiffsUsed = SPIFFS.usedBytes() / 1024; // KB

    Serial.printf("[Storage] SPIFFS Size: %llu KB\n", spiffsSize);
    Serial.printf("[Storage] SPIFFS Used: %llu KB\n", spiffsUsed);

    spiffsAvailable = true;
    return true;
}

bool FRFDStorage::isSDCardAvailable() {
    return sdCardAvailable;
}

bool FRFDStorage::isSPIFFSAvailable() {
    return spiffsAvailable;
}

uint64_t FRFDStorage::getSDCardSize() {
    if (sdCardAvailable) {
        return SD.cardSize() / (1024 * 1024); // MB
    }
    return 0;
}

uint64_t FRFDStorage::getSDCardFree() {
    if (sdCardAvailable) {
        uint64_t total = SD.cardSize();
        uint64_t used = SD.usedBytes();
        return (total - used) / (1024 * 1024); // MB
    }
    return 0;
}

uint64_t FRFDStorage::getSPIFFSSize() {
    if (spiffsAvailable) {
        return SPIFFS.totalBytes() / 1024; // KB
    }
    return 0;
}

uint64_t FRFDStorage::getSPIFFSFree() {
    if (spiffsAvailable) {
        return (SPIFFS.totalBytes() - SPIFFS.usedBytes()) / 1024; // KB
    }
    return 0;
}

bool FRFDStorage::createCaseDirectory(const String& caseId) {
    if (!sdCardAvailable) {
        Serial.println("[Storage] SD Card not available");
        return false;
    }

    // Create directory path
    currentCaseDir = "/evidence/" + caseId;

    if (SD.exists(currentCaseDir.c_str())) {
        Serial.printf("[Storage] Case directory already exists: %s\n", currentCaseDir.c_str());
        return true;
    }

    if (SD.mkdir(currentCaseDir.c_str())) {
        Serial.printf("[Storage] Created case directory: %s\n", currentCaseDir.c_str());
        return true;
    }

    Serial.printf("[Storage] Failed to create case directory: %s\n", currentCaseDir.c_str());
    return false;
}

String FRFDStorage::getCaseDirectory() {
    return currentCaseDir;
}

bool FRFDStorage::createSubDirectory(const String& subDir) {
    if (!sdCardAvailable || currentCaseDir.isEmpty()) {
        return false;
    }

    String fullPath = currentCaseDir + "/" + subDir;

    if (SD.exists(fullPath.c_str())) {
        return true;
    }

    return SD.mkdir(fullPath.c_str());
}

bool FRFDStorage::createDirectory(const String& path) {
    if (!sdCardAvailable) {
        Serial.println("[Storage] SD Card not available");
        return false;
    }

    // Check if directory already exists
    if (SD.exists(path.c_str())) {
        return true;
    }

    // Create parent directories if needed
    String currentPath = "";
    int start = (path.startsWith("/")) ? 1 : 0;
    int nextSlash = path.indexOf('/', start);

    while (nextSlash != -1) {
        currentPath = path.substring(0, nextSlash);

        if (!SD.exists(currentPath.c_str())) {
            if (!SD.mkdir(currentPath.c_str())) {
                Serial.printf("[Storage] Failed to create directory: %s\n", currentPath.c_str());
                return false;
            }
        }

        start = nextSlash + 1;
        nextSlash = path.indexOf('/', start);
    }

    // Create final directory
    if (!SD.exists(path.c_str())) {
        if (!SD.mkdir(path.c_str())) {
            Serial.printf("[Storage] Failed to create final directory: %s\n", path.c_str());
            return false;
        }
    }

    return true;
}

bool FRFDStorage::directoryExists(const String& path) {
    if (!sdCardAvailable) {
        return false;
    }

    return SD.exists(path.c_str());
}

bool FRFDStorage::writeFile(const String& path, const String& data) {
    return writeFile(path, (const uint8_t*)data.c_str(), data.length());
}

bool FRFDStorage::writeFile(const String& path, const uint8_t* data, size_t len) {
    // Determine which storage to use
    bool useSD = sdCardAvailable && path.startsWith("/evidence");

    File file;

    if (useSD) {
        file = SD.open(path.c_str(), FILE_WRITE);
    } else if (spiffsAvailable) {
        file = SPIFFS.open(path.c_str(), FILE_WRITE);
    } else {
        Serial.println("[Storage] No storage available");
        return false;
    }

    if (!file) {
        Serial.printf("[Storage] Failed to open file for writing: %s\n", path.c_str());
        return false;
    }

    size_t written = file.write(data, len);
    file.close();

    if (written != len) {
        Serial.printf("[Storage] Write failed. Expected %d, wrote %d bytes\n", len, written);
        return false;
    }

    Serial.printf("[Storage] Wrote %d bytes to: %s\n", written, path.c_str());
    return true;
}

bool FRFDStorage::appendFile(const String& path, const String& data) {
    bool useSD = sdCardAvailable && path.startsWith("/evidence");

    File file;

    if (useSD) {
        file = SD.open(path.c_str(), FILE_APPEND);
    } else if (spiffsAvailable) {
        file = SPIFFS.open(path.c_str(), FILE_APPEND);
    } else {
        return false;
    }

    if (!file) {
        return false;
    }

    size_t written = file.print(data);
    file.close();

    return (written == data.length());
}

String FRFDStorage::readFile(const String& path) {
    bool useSD = sdCardAvailable && path.startsWith("/evidence");

    File file;

    if (useSD) {
        file = SD.open(path.c_str(), FILE_READ);
    } else if (spiffsAvailable) {
        file = SPIFFS.open(path.c_str(), FILE_READ);
    } else {
        return "";
    }

    if (!file) {
        return "";
    }

    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }

    file.close();
    return content;
}

bool FRFDStorage::fileExists(const String& path) {
    bool useSD = sdCardAvailable && path.startsWith("/evidence");

    if (useSD) {
        return SD.exists(path.c_str());
    } else if (spiffsAvailable) {
        return SPIFFS.exists(path.c_str());
    }

    return false;
}

bool FRFDStorage::deleteFile(const String& path) {
    bool useSD = sdCardAvailable && path.startsWith("/evidence");

    if (useSD) {
        return SD.remove(path.c_str());
    } else if (spiffsAvailable) {
        return SPIFFS.remove(path.c_str());
    }

    return false;
}

size_t FRFDStorage::getFileSize(const String& path) {
    bool useSD = sdCardAvailable && path.startsWith("/evidence");

    File file;

    if (useSD) {
        file = SD.open(path.c_str(), FILE_READ);
    } else if (spiffsAvailable) {
        file = SPIFFS.open(path.c_str(), FILE_READ);
    } else {
        return 0;
    }

    if (!file) {
        return 0;
    }

    size_t size = file.size();
    file.close();
    return size;
}

bool FRFDStorage::saveArtifact(const String& filename, const String& data) {
    return saveArtifact(filename, (const uint8_t*)data.c_str(), data.length());
}

bool FRFDStorage::saveArtifact(const String& filename, const uint8_t* data, size_t len) {
    if (currentCaseDir.isEmpty()) {
        Serial.println("[Storage] No case directory set");
        return false;
    }

    String fullPath = currentCaseDir + "/" + filename;
    return writeFile(fullPath, data, len);
}

bool FRFDStorage::loadConfiguration(String& configJson) {
    if (!spiffsAvailable) {
        Serial.println("[Storage] SPIFFS not available for configuration");
        return false;
    }

    File file = SPIFFS.open("/config.json", FILE_READ);

    if (!file) {
        Serial.println("[Storage] Configuration file not found");
        return false;
    }

    configJson = "";
    while (file.available()) {
        configJson += (char)file.read();
    }

    file.close();
    Serial.printf("[Storage] Loaded configuration: %d bytes\n", configJson.length());
    return true;
}

bool FRFDStorage::saveConfiguration(const String& configJson) {
    if (!spiffsAvailable) {
        return false;
    }

    return writeFile("/config.json", configJson);
}

bool FRFDStorage::saveChainOfCustody(const String& custodyJson) {
    if (currentCaseDir.isEmpty()) {
        Serial.println("[Storage] No case directory set");
        return false;
    }

    String filename = "chain_of_custody_" + String(millis()) + ".json";
    String fullPath = currentCaseDir + "/" + filename;

    bool success = writeFile(fullPath, custodyJson);

    if (success) {
        Serial.printf("[Storage] Saved chain of custody: %s\n", filename.c_str());
    }

    return success;
}

void FRFDStorage::listDirectory(const String& path) {
    bool useSD = sdCardAvailable && path.startsWith("/evidence");

    File root;

    if (useSD) {
        root = SD.open(path.c_str());
    } else if (spiffsAvailable) {
        root = SPIFFS.open(path.c_str());
    } else {
        Serial.println("[Storage] No storage available");
        return;
    }

    if (!root) {
        Serial.printf("[Storage] Failed to open directory: %s\n", path.c_str());
        return;
    }

    if (!root.isDirectory()) {
        Serial.println("[Storage] Not a directory");
        return;
    }

    Serial.printf("[Storage] Listing directory: %s\n", path.c_str());

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.printf("  DIR : %s\n", file.name());
        } else {
            Serial.printf("  FILE: %s\t\tSIZE: %d\n", file.name(), file.size());
        }
        file = root.openNextFile();
    }
}

std::vector<String> FRFDStorage::getFileList(const String& path) {
    std::vector<String> files;
    bool useSD = sdCardAvailable && path.startsWith("/evidence");

    File root;

    if (useSD) {
        root = SD.open(path.c_str());
    } else if (spiffsAvailable) {
        root = SPIFFS.open(path.c_str());
    } else {
        return files;
    }

    if (!root || !root.isDirectory()) {
        return files;
    }

    File file = root.openNextFile();
    while (file) {
        files.push_back(String(file.name()));
        file = root.openNextFile();
    }

    return files;
}

bool FRFDStorage::cleanupOldFiles(uint32_t daysOld) {
    // Placeholder for cleanup functionality
    // Would need to implement timestamp checking and file deletion
    Serial.printf("[Storage] Cleanup files older than %d days\n", daysOld);
    return true;
}

void FRFDStorage::printStorageInfo() {
    Serial.println("\n=== Storage Status ===");

    if (sdCardAvailable) {
        Serial.println("SD Card: Available");
        Serial.printf("  Size: %llu MB\n", getSDCardSize());
        Serial.printf("  Free: %llu MB\n", getSDCardFree());
        Serial.printf("  Used: %llu MB\n", getSDCardSize() - getSDCardFree());
    } else {
        Serial.println("SD Card: Not Available");
    }

    if (spiffsAvailable) {
        Serial.println("SPIFFS: Available");
        Serial.printf("  Size: %llu KB\n", getSPIFFSSize());
        Serial.printf("  Free: %llu KB\n", getSPIFFSFree());
        Serial.printf("  Used: %llu KB\n", getSPIFFSSize() - getSPIFFSFree());
    } else {
        Serial.println("SPIFFS: Not Available");
    }

    if (!currentCaseDir.isEmpty()) {
        Serial.printf("Current Case: %s\n", currentCaseDir.c_str());
    }

    Serial.println("=====================\n");
}

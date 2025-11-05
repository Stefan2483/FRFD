#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <SPIFFS.h>
#include "config.h"

class FRFDStorage {
private:
    bool sdCardAvailable;
    bool spiffsAvailable;
    uint64_t sdCardSize;
    uint64_t sdCardUsed;
    uint64_t spiffsSize;
    uint64_t spiffsUsed;

    String currentCaseDir;

public:
    FRFDStorage();

    // Initialization
    bool begin();
    bool initSDCard();
    bool initSPIFFS();

    // Status
    bool isSDCardAvailable();
    bool isSPIFFSAvailable();
    uint64_t getSDCardSize();
    uint64_t getSDCardFree();
    uint64_t getSPIFFSSize();
    uint64_t getSPIFFSFree();

    // Directory management
    bool createCaseDirectory(const String& caseId);
    String getCaseDirectory();
    bool createSubDirectory(const String& subDir);

    // File operations
    bool writeFile(const String& path, const String& data);
    bool writeFile(const String& path, const uint8_t* data, size_t len);
    bool appendFile(const String& path, const String& data);
    String readFile(const String& path);
    bool fileExists(const String& path);
    bool deleteFile(const String& path);
    size_t getFileSize(const String& path);

    // Artifact storage
    bool saveArtifact(const String& filename, const String& data);
    bool saveArtifact(const String& filename, const uint8_t* data, size_t len);

    // Configuration
    bool loadConfiguration(String& configJson);
    bool saveConfiguration(const String& configJson);

    // Chain of custody
    bool saveChainOfCustody(const String& custodyJson);

    // Directory listing
    void listDirectory(const String& path);
    std::vector<String> getFileList(const String& path);

    // Cleanup
    bool cleanupOldFiles(uint32_t daysOld);

    // Storage info
    void printStorageInfo();
};

#endif // STORAGE_H

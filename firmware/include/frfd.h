#ifndef FRFD_H
#define FRFD_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "config.h"
#include "display.h"
#include "storage.h"
#include "hid_automation.h"
#include "evidence_container.h"

struct DeviceState {
    OperatingMode mode;
    OperatingSystem os;
    RiskLevel risk;
    CollectionStatus status;
    uint8_t progress;
    unsigned long startTime;
    String caseId;
    String responder;
    uint32_t artifactCount;
    uint32_t totalSize;
};

struct ForensicsArtifact {
    String type;
    String path;
    uint32_t size;
    String hash;
    unsigned long timestamp;
};

class FRFD {
private:
    FRFDDisplay* display;
    FRFDStorage* storage;
    HIDAutomation* hid_automation;
    EvidenceContainer* evidence_container;
    DeviceState state;

    // Configuration
    JsonDocument config;
    String deviceId;
    String organization;

    // Communication
    bool wifiActive;
    bool serialActive;

    // Evidence collection
    std::vector<ForensicsArtifact> artifacts;

    // HID automation tracking
    unsigned long automation_start_time;

    // Private methods
    void loadConfiguration();
    void initializeUSB();
    void initializeWiFi();
    void initializeStorage();

public:
    FRFD();
    ~FRFD();

    // Initialization
    bool begin();
    void loop();

    // Mode management
    void setMode(OperatingMode mode);
    OperatingMode getMode();

    // OS Detection
    OperatingSystem detectOS();
    void setOS(OperatingSystem os);

    // Triage operations
    void runTriage();
    void quickSystemAssessment();
    void displayCriticalIndicators();

    // Collection operations
    void runCollection();
    void collectWindowsArtifacts();
    void collectLinuxArtifacts();
    void collectMacOSArtifacts();

    // Containment operations
    void runContainment();
    void isolateNetwork();
    void terminateSuspiciousProcesses();
    void implementFirewallRules();
    void lockdownAccounts();

    // Analysis operations
    void runAnalysis();
    void matchIOCs();
    void generateTimeline();
    void detectAnomalies();

    // HID Automation operations
    bool enableHIDAutomation();
    bool runHIDAutomation();
    OSDetectionResult detectOSViaHID();
    bool automateForensicsCollection();
    bool automateWindowsWithDisplay(uint8_t totalModules);
    bool automateLinuxWithDisplay(uint8_t totalModules);
    bool automateMacOSWithDisplay(uint8_t totalModules);
    void saveHIDLog();

    // Script execution
    bool executeScript(const char* scriptPath);
    bool executePowerShell(const char* command);
    bool executeBash(const char* command);

    // Communication
    void setupWiFiAP();
    void transferData();
    void uploadToCloud();

    // Evidence management
    void addArtifact(const ForensicsArtifact& artifact);
    void generateChainOfCustody();
    String calculateHash(const String& data);

    // Status updates
    void updateProgress(uint8_t percent);
    void updateStatus(CollectionStatus status);
    void updateRisk(RiskLevel risk);

    // Input handling
    void handleButton();
    void handleSerial();
    void handleUSB();

    // Utility functions
    String getDeviceId();
    String getCaseId();
    void setCaseId(const String& caseId);
    void setResponder(const String& responder);
    unsigned long getElapsedTime();
};

#endif // FRFD_H

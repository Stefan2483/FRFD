#include "frfd.h"
#include <SD.h>
#include <FS.h>
#include <SPIFFS.h>

FRFD::FRFD() {
    display = new FRFDDisplay();
    storage = new FRFDStorage();
    state.mode = MODE_IDLE;
    state.os = OS_UNKNOWN;
    state.risk = RISK_UNKNOWN;
    state.status = STATUS_IDLE;
    state.progress = 0;
    state.startTime = 0;
    state.artifactCount = 0;
    state.totalSize = 0;

    wifiActive = false;
    serialActive = false;
}

FRFD::~FRFD() {
    delete display;
    delete storage;
}

bool FRFD::begin() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n=== FRFD - CSIRT Forensics Dongle ===");
    Serial.println("Firmware Version: " FIRMWARE_VERSION);

    // Initialize display
    display->begin();
    display->showBootScreen();

    // Load configuration
    loadConfiguration();

    // Initialize subsystems
    initializeUSB();

    // Initialize storage
    storage->begin();

    Serial.println("FRFD initialized successfully");
    display->showMainHUD();

    return true;
}

void FRFD::loadConfiguration() {
    Serial.println("Loading configuration...");

    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return;
    }

    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("Config file not found, using defaults");
        deviceId = "FRFD-001";
        organization = "CSIRT-TEAM";
        return;
    }

    DeserializationError error = deserializeJson(config, configFile);
    configFile.close();

    if (error) {
        Serial.print("Failed to parse config: ");
        Serial.println(error.c_str());
        return;
    }

    // Extract configuration values
    deviceId = config["device_config"]["device_id"].as<String>();
    organization = config["device_config"]["organization"].as<String>();

    Serial.print("Device ID: ");
    Serial.println(deviceId);
    Serial.print("Organization: ");
    Serial.println(organization);
}

void FRFD::initializeUSB() {
    Serial.println("Initializing USB...");
    serialActive = true;
}

void FRFD::initializeWiFi() {
    Serial.println("Initializing WiFi AP...");

    WiFi.mode(WIFI_AP);
    bool success = WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, 0, WIFI_MAX_CLIENTS);

    if (success) {
        Serial.print("WiFi AP started: ");
        Serial.println(WiFi.softAPIP());
        wifiActive = true;
        display->updateNetwork(true);
    } else {
        Serial.println("Failed to start WiFi AP");
        wifiActive = false;
    }
}

void FRFD::loop() {
    handleButton();
    handleSerial();

    // Update elapsed time display every second
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 1000) {
        display->updateElapsedTime();
        lastUpdate = millis();
    }
}

void FRFD::setMode(OperatingMode mode) {
    state.mode = mode;
    state.startTime = millis();
    state.progress = 0;
    display->updateMode(mode);

    Serial.print("Mode changed to: ");
    Serial.println(display->getModeString(mode));
}

OperatingMode FRFD::getMode() {
    return state.mode;
}

OperatingSystem FRFD::detectOS() {
    Serial.println("Detecting operating system...");
    display->updateStatus(STATUS_DETECTING);

    // In a real implementation, this would analyze USB descriptors,
    // filesystem patterns, or execute detection scripts
    // For now, this is a placeholder

    // Simulate detection delay
    delay(500);

    // Default to unknown - actual detection would happen via host scripts
    state.os = OS_UNKNOWN;

    display->updateOS(state.os);
    Serial.print("Detected OS: ");
    Serial.println(display->getOSString(state.os));

    return state.os;
}

void FRFD::setOS(OperatingSystem os) {
    state.os = os;
    display->updateOS(os);
    display->showOSDetection(os);
    delay(1000);
    display->showMainHUD();
}

void FRFD::runTriage() {
    Serial.println("=== Starting Triage Mode ===");
    setMode(MODE_TRIAGE);

    state.status = STATUS_DETECTING;
    detectOS();

    quickSystemAssessment();
    displayCriticalIndicators();

    state.status = STATUS_COMPLETE;
    display->updateStatus(STATUS_COMPLETE);
    display->showSuccess("Triage Complete");
}

void FRFD::quickSystemAssessment() {
    Serial.println("Performing quick system assessment...");

    updateProgress(10);

    // Execute triage scripts based on detected OS
    switch (state.os) {
        case OS_WINDOWS:
            Serial.println("Running Windows triage...");
            // Execute Windows triage scripts
            break;
        case OS_LINUX:
            Serial.println("Running Linux triage...");
            // Execute Linux triage scripts
            break;
        case OS_MACOS:
            Serial.println("Running macOS triage...");
            // Execute macOS triage scripts
            break;
        default:
            Serial.println("Unknown OS - skipping automated assessment");
            break;
    }

    updateProgress(100);
}

void FRFD::displayCriticalIndicators() {
    // Analyze collected data and update risk level
    // This would be based on actual findings from triage
    state.risk = RISK_MEDIUM; // Placeholder
    display->updateRisk(state.risk);
}

void FRFD::runCollection() {
    Serial.println("=== Starting Collection Mode ===");
    setMode(MODE_COLLECTION);

    state.status = STATUS_COLLECTING;

    switch (state.os) {
        case OS_WINDOWS:
            collectWindowsArtifacts();
            break;
        case OS_LINUX:
            collectLinuxArtifacts();
            break;
        case OS_MACOS:
            collectMacOSArtifacts();
            break;
        default:
            Serial.println("Cannot collect - OS not detected");
            display->showError("OS Unknown");
            return;
    }

    state.status = STATUS_COMPLETE;
    display->updateStatus(STATUS_COMPLETE);
    display->showSuccess("Collection Complete");
}

void FRFD::collectWindowsArtifacts() {
    Serial.println("Collecting Windows artifacts...");

    const char* categories[] = {"memory", "registry", "filesystem", "network", "persistence"};
    uint8_t categoryCount = 5;

    for (uint8_t i = 0; i < categoryCount; i++) {
        Serial.print("Collecting: ");
        Serial.println(categories[i]);

        // Execute collection scripts
        String scriptPath = String("/forensics_tools/windows/") + categories[i] + "/collect.ps1";
        executeScript(scriptPath.c_str());

        updateProgress((i + 1) * 100 / categoryCount);
        delay(500); // Simulate collection time
    }
}

void FRFD::collectLinuxArtifacts() {
    Serial.println("Collecting Linux artifacts...");

    const char* categories[] = {"system", "logs", "network", "persistence"};
    uint8_t categoryCount = 4;

    for (uint8_t i = 0; i < categoryCount; i++) {
        Serial.print("Collecting: ");
        Serial.println(categories[i]);

        // Execute collection scripts
        String scriptPath = String("/forensics_tools/linux/") + categories[i] + "/collect.sh";
        executeScript(scriptPath.c_str());

        updateProgress((i + 1) * 100 / categoryCount);
        delay(500); // Simulate collection time
    }
}

void FRFD::collectMacOSArtifacts() {
    Serial.println("Collecting macOS artifacts...");
    // Similar to Linux for now
    collectLinuxArtifacts();
}

void FRFD::runContainment() {
    Serial.println("=== Starting Containment Mode ===");
    setMode(MODE_CONTAINMENT);

    isolateNetwork();
    implementFirewallRules();

    state.status = STATUS_COMPLETE;
    display->showSuccess("Containment Active");
}

void FRFD::isolateNetwork() {
    Serial.println("Initiating network isolation...");
    // Execute network isolation scripts
}

void FRFD::terminateSuspiciousProcesses() {
    Serial.println("Terminating suspicious processes...");
    // Execute process termination scripts
}

void FRFD::implementFirewallRules() {
    Serial.println("Implementing firewall rules...");
    // Execute firewall configuration scripts
}

void FRFD::lockdownAccounts() {
    Serial.println("Locking down accounts...");
    // Execute account lockdown scripts
}

void FRFD::runAnalysis() {
    Serial.println("=== Starting Analysis Mode ===");
    setMode(MODE_ANALYSIS);

    matchIOCs();
    generateTimeline();
    detectAnomalies();

    state.status = STATUS_COMPLETE;
    display->showSuccess("Analysis Complete");
}

void FRFD::matchIOCs() {
    Serial.println("Matching IOCs...");
    // Execute IOC matching with YARA rules
}

void FRFD::generateTimeline() {
    Serial.println("Generating timeline...");
    // Generate forensic timeline
}

void FRFD::detectAnomalies() {
    Serial.println("Detecting anomalies...");
    // Run anomaly detection algorithms
}

bool FRFD::executeScript(const char* scriptPath) {
    Serial.print("Executing script: ");
    Serial.println(scriptPath);

    // In real implementation, this would:
    // 1. Read script from SD/SPIFFS
    // 2. Send to host via USB serial
    // 3. Capture output
    // 4. Store results

    return true;
}

bool FRFD::executePowerShell(const char* command) {
    Serial.print("PowerShell: ");
    Serial.println(command);
    // Send command via USB serial to host
    return true;
}

bool FRFD::executeBash(const char* command) {
    Serial.print("Bash: ");
    Serial.println(command);
    // Send command via USB serial to host
    return true;
}

void FRFD::setupWiFiAP() {
    initializeWiFi();
}

void FRFD::transferData() {
    Serial.println("Transferring data...");
    state.status = STATUS_TRANSFERRING;
    display->updateStatus(STATUS_TRANSFERRING);

    // Transfer via WiFi, serial, or SD card
    delay(2000); // Simulate transfer

    state.status = STATUS_COMPLETE;
    display->showSuccess("Transfer Complete");
}

void FRFD::uploadToCloud() {
    Serial.println("Uploading to cloud...");
    // Upload evidence to cloud storage
}

void FRFD::addArtifact(const ForensicsArtifact& artifact) {
    artifacts.push_back(artifact);
    state.artifactCount++;
    state.totalSize += artifact.size;

    Serial.print("Artifact added: ");
    Serial.print(artifact.type);
    Serial.print(" (");
    Serial.print(artifact.size);
    Serial.println(" bytes)");
}

void FRFD::generateChainOfCustody() {
    Serial.println("=== Chain of Custody ===");

    JsonDocument doc;
    doc["case_id"] = state.caseId;
    doc["responder"] = state.responder;
    doc["device_id"] = deviceId;
    doc["timestamp"] = millis();

    JsonArray artifactsArray = doc["artifacts"].to<JsonArray>();
    for (const auto& artifact : artifacts) {
        JsonObject obj = artifactsArray.add<JsonObject>();
        obj["type"] = artifact.type;
        obj["path"] = artifact.path;
        obj["size"] = artifact.size;
        obj["hash"] = artifact.hash;
        obj["timestamp"] = artifact.timestamp;
    }

    serializeJsonPretty(doc, Serial);
    Serial.println();
}

String FRFD::calculateHash(const String& data) {
    // Implement SHA-256 hashing
    // For now, return placeholder
    return "sha256:placeholder";
}

void FRFD::updateProgress(uint8_t percent) {
    state.progress = percent;
    display->updateProgress(percent);
}

void FRFD::updateStatus(CollectionStatus status) {
    state.status = status;
    display->updateStatus(status);
}

void FRFD::updateRisk(RiskLevel risk) {
    state.risk = risk;
    display->updateRisk(risk);
}

void FRFD::handleButton() {
    static bool lastButtonState = HIGH;
    static unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 50;

    bool reading = digitalRead(BUTTON_PIN);

    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading == LOW) {
            // Button pressed
            Serial.println("Button pressed");

            // Cycle through modes
            if (state.mode == MODE_IDLE) {
                setMode(MODE_TRIAGE);
            }
        }
    }

    lastButtonState = reading;
}

void FRFD::handleSerial() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        Serial.print("Command received: ");
        Serial.println(command);

        // Parse and execute commands
        if (command == "triage") {
            runTriage();
        } else if (command == "collect") {
            runCollection();
        } else if (command == "contain") {
            runContainment();
        } else if (command == "analyze") {
            runAnalysis();
        } else if (command == "status") {
            generateChainOfCustody();
        } else if (command.startsWith("os:")) {
            String osStr = command.substring(3);
            if (osStr == "windows") setOS(OS_WINDOWS);
            else if (osStr == "linux") setOS(OS_LINUX);
            else if (osStr == "macos") setOS(OS_MACOS);
        }
    }
}

void FRFD::handleUSB() {
    // Handle USB HID/Mass Storage events
}

String FRFD::getDeviceId() {
    return deviceId;
}

String FRFD::getCaseId() {
    return state.caseId;
}

void FRFD::setCaseId(const String& caseId) {
    state.caseId = caseId;
    Serial.print("Case ID set: ");
    Serial.println(caseId);

    // Create case directory on SD card
    if (storage->isSDCardAvailable()) {
        storage->createCaseDirectory(caseId);
    }
}

void FRFD::setResponder(const String& responder) {
    state.responder = responder;
    Serial.print("Responder set: ");
    Serial.println(responder);
}

unsigned long FRFD::getElapsedTime() {
    return millis() - state.startTime;
}

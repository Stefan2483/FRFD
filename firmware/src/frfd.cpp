#include "frfd.h"
#include <SD.h>
#include <FS.h>
#include <SPIFFS.h>

FRFD::FRFD() {
    display = new FRFDDisplay();
    storage = new FRFDStorage();
    hid_automation = new HIDAutomation();
    evidence_container = nullptr;  // Created per case
    wifi_manager = nullptr;  // Initialized after storage is ready
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
    delete hid_automation;
    if (evidence_container) {
        delete evidence_container;
    }
    if (wifi_manager) {
        delete wifi_manager;
    }
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

    // Initialize HID automation
    if (hid_automation->begin(storage)) {
        Serial.println("HID Automation enabled");
    } else {
        Serial.println("HID Automation disabled");
    }

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
    Serial.println("Initializing WiFi Manager...");

    if (!wifi_manager) {
        wifi_manager = new WiFiManager(storage);
    }

    // Set device info for web interface
    wifi_manager->setDeviceId(deviceId);
    wifi_manager->setMode("Initializing");
    wifi_manager->setStatus("Starting WiFi AP");

    // Connect evidence container if available
    if (evidence_container) {
        wifi_manager->setEvidenceContainer(evidence_container);
    }

    // Start WiFi AP with web server
    bool success = wifi_manager->begin(WIFI_AP_SSID, WIFI_AP_PASSWORD);

    if (success) {
        Serial.print("[WiFi] AP started: ");
        Serial.println(wifi_manager->getAPSSID());
        Serial.print("[WiFi] IP address: ");
        Serial.println(wifi_manager->getAPIP());
        Serial.println("[WiFi] Upload endpoint: http://192.168.4.1/upload");
        wifiActive = true;
        display->updateNetwork(true);
    } else {
        Serial.println("[WiFi] Failed to start AP");
        wifiActive = false;
    }
}

void FRFD::loop() {
    handleButton();
    handleSerial();

    // Handle WiFi client requests
    if (wifi_manager && wifi_manager->isActive()) {
        wifi_manager->handleClient();
    }

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
        } else if (command == "hid") {
            // Run full HID automation
            runHIDAutomation();
        } else if (command == "hid_detect") {
            // Just detect OS via HID
            OSDetectionResult result = detectOSViaHID();
            Serial.print("OS Detected: ");
            Serial.println(result.os_version);
        } else if (command == "status") {
            generateChainOfCustody();
        } else if (command.startsWith("os:")) {
            String osStr = command.substring(3);
            if (osStr == "windows") setOS(OS_WINDOWS);
            else if (osStr == "linux") setOS(OS_LINUX);
            else if (osStr == "macos") setOS(OS_MACOS);
        } else if (command == "help") {
            Serial.println("\n=== FRFD Commands ===");
            Serial.println("triage       - Run triage mode");
            Serial.println("collect      - Run collection mode");
            Serial.println("contain      - Run containment mode");
            Serial.println("analyze      - Run analysis mode");
            Serial.println("hid          - Run full HID automation");
            Serial.println("hid_detect   - Detect OS via HID");
            Serial.println("status       - Show chain of custody");
            Serial.println("os:windows   - Set OS to Windows");
            Serial.println("os:linux     - Set OS to Linux");
            Serial.println("os:macos     - Set OS to macOS");
            Serial.println("help         - Show this help");
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

// ============================================================================
// HID AUTOMATION METHODS
// ============================================================================

bool FRFD::enableHIDAutomation() {
    if (!hid_automation) {
        Serial.println("HID Automation not initialized");
        return false;
    }

    if (!hid_automation->isHIDReady()) {
        Serial.println("HID not ready");
        return false;
    }

    Serial.println("HID Automation enabled");
    display->showMessage("HID Mode Active");
    delay(1000);

    return true;
}

bool FRFD::runHIDAutomation() {
    Serial.println("=== Starting HID Automation ===");

    // Start HID display mode
    display->startHIDMode();

    if (!enableHIDAutomation()) {
        display->showHIDError("HID Init Failed");
        return false;
    }

    // Step 1: Detect OS via HID
    display->showHIDDetecting("Keyboard");

    OSDetectionResult osResult = detectOSViaHID();

    if (osResult.confidence_score < 80) {
        Serial.println("OS detection failed or low confidence");
        display->showHIDError("OS Detect Failed");
        return false;
    }

    // Update state with detected OS
    state.os = osResult.detected_os;

    Serial.print("Detected: ");
    Serial.println(osResult.os_version);

    // Show OS detected
    display->showHIDOSDetected(state.os, osResult.confidence_score);

    // Step 2: Run automated forensics collection
    bool success = automateForensicsCollection();

    if (success) {
        // Save HID log
        saveHIDLog();

        // Show completion screen
        unsigned long duration = millis() - automation_start_time;
        display->showHIDComplete(hid_automation->getActionCount(), duration);

        Serial.println("HID automation completed successfully");
    } else {
        display->showHIDError("Collection Failed");
        Serial.println("HID automation failed");
    }

    // End HID mode and return to normal display
    display->endHIDMode();

    return success;
}

OSDetectionResult FRFD::detectOSViaHID() {
    Serial.println("Detecting OS via HID keyboard automation...");

    // Log to forensics log
    hid_automation->logAction("AUTO_DETECT_START", "Automated OS detection initiated", "STARTED");

    // Run OS detection
    OSDetectionResult result = hid_automation->detectOS();

    Serial.print("Detection result: ");
    Serial.print("OS = ");
    Serial.print((int)result.detected_os);
    Serial.print(", Confidence = ");
    Serial.print(result.confidence_score);
    Serial.println("%");

    return result;
}

bool FRFD::automateForensicsCollection() {
    Serial.println("Starting automated forensics collection via HID...");

    if (!hid_automation || !hid_automation->isHIDReady()) {
        Serial.println("HID not available");
        return false;
    }

    // Set case ID if not already set
    if (state.caseId.length() == 0) {
        String autoCase = "AUTO_" + String(millis());
        setCaseId(autoCase);
    }

    // Create evidence container
    if (evidence_container) {
        delete evidence_container;
    }

    evidence_container = new EvidenceContainer(storage);

    if (!evidence_container->createContainer(state.caseId, state.responder)) {
        Serial.println("Failed to create evidence container");
        display->showHIDError("Container Failed");
        delete evidence_container;
        evidence_container = nullptr;
        return false;
    }

    // Connect evidence container to WiFi manager for uploads
    if (wifi_manager) {
        wifi_manager->setEvidenceContainer(evidence_container);
        Serial.println("[FRFD] Evidence container connected to WiFi manager");
    }

    // Set target system information
    TargetSystemInfo targetInfo;
    targetInfo.os_name = display->getOSString(state.os);
    targetInfo.os_version = ""; // Would be populated from detection
    targetInfo.hostname = "";  // Would be populated from detection
    targetInfo.system_time = millis();
    targetInfo.is_admin = false; // Would be detected

    evidence_container->setTargetSystemInfo(targetInfo);

    // Track automation start time
    automation_start_time = millis();

    // Determine number of modules based on OS
    uint8_t totalModules = 0;
    switch (state.os) {
        case OperatingSystem::OS_WINDOWS: totalModules = 7; break;
        case OperatingSystem::OS_LINUX: totalModules = 5; break;
        case OperatingSystem::OS_MACOS: totalModules = 2; break;
        default: totalModules = 1; break;
    }

    // Run collection with display updates
    bool success = false;

    switch (state.os) {
        case OperatingSystem::OS_WINDOWS:
            success = automateWindowsWithDisplay(totalModules);
            break;
        case OperatingSystem::OS_LINUX:
            success = automateLinuxWithDisplay(totalModules);
            break;
        case OperatingSystem::OS_MACOS:
            success = automateMacOSWithDisplay(totalModules);
            break;
        default:
            success = false;
    }

    if (success) {
        Serial.println("Automated collection completed");

        // Finalize evidence container
        evidence_container->finalizeContainer();

        // Update artifact count from container
        state.artifactCount = evidence_container->getArtifactCount();
        state.totalSize = evidence_container->getTotalSize();

        // Print statistics
        Serial.printf("Collection Stats:\n");
        Serial.printf("  Artifacts: %d\n", state.artifactCount);
        Serial.printf("  Total Size: %d bytes\n", state.totalSize);
        Serial.printf("  Compressed: %d bytes\n", evidence_container->getCompressedSize());
        Serial.printf("  Ratio: %.2f%%\n", evidence_container->getCompressionRatio() * 100.0);
        Serial.printf("  Duration: %lu ms\n", evidence_container->getCollectionDuration());

        // Generate chain of custody (old method for compatibility)
        generateChainOfCustody();
    } else {
        Serial.println("Automated collection encountered errors");

        if (evidence_container) {
            evidence_container->finalizeContainer(); // Finalize even on error
        }
    }

    return success;
}

bool FRFD::automateWindowsWithDisplay(uint8_t totalModules) {
    const char* modules[] = {"Memory", "Autoruns", "Network", "EventLogs", "Prefetch", "Tasks", "Services"};
    const char* types[] = {"memory", "registry", "network", "logs", "filesystem", "persistence", "persistence"};

    for (uint8_t i = 0; i < totalModules; i++) {
        // Show current module
        display->showHIDCollection(String(modules[i]), i + 1, totalModules);

        // Simulate module execution with progress updates
        for (uint8_t progress = 0; progress <= 100; progress += 25) {
            display->showHIDProgress(i + 1, totalModules, String(modules[i]), progress);
            delay(500); // Simulated work time
        }

        // Create simulated artifact (in real implementation, this would be actual collected data)
        String artifactName = String(modules[i]) + "_" + String(millis()) + ".dat";
        String artifactData = "Simulated artifact data from " + String(modules[i]) + " module\n";
        artifactData += "Collected at: " + String(millis()) + " ms\n";
        artifactData += "Module type: " + String(types[i]) + "\n";

        // Add to evidence container
        if (evidence_container) {
            String artifactId = evidence_container->addArtifact(
                types[i],
                artifactName,
                (const uint8_t*)artifactData.c_str(),
                artifactData.length(),
                true  // Enable compression
            );

            if (artifactId.length() > 0) {
                Serial.printf("[Collection] Added artifact: %s (%d bytes)\n",
                             artifactName.c_str(), artifactData.length());
            }
        }

        // Log action
        hid_automation->logAction("WIN_MODULE", modules[i], "SUCCESS");
    }

    return true;
}

bool FRFD::automateLinuxWithDisplay(uint8_t totalModules) {
    const char* modules[] = {"SysInfo", "AuthLogs", "Network", "Kernel", "Persist"};
    const char* types[] = {"filesystem", "logs", "network", "filesystem", "persistence"};

    for (uint8_t i = 0; i < totalModules; i++) {
        // Show current module
        display->showHIDCollection(String(modules[i]), i + 1, totalModules);

        // Simulate module execution with progress updates
        for (uint8_t progress = 0; progress <= 100; progress += 25) {
            display->showHIDProgress(i + 1, totalModules, String(modules[i]), progress);
            delay(500); // Simulated work time
        }

        // Create simulated artifact
        String artifactName = String(modules[i]) + "_" + String(millis()) + ".dat";
        String artifactData = "Simulated Linux artifact from " + String(modules[i]) + "\n";
        artifactData += "Timestamp: " + String(millis()) + "\n";

        // Add to evidence container
        if (evidence_container) {
            evidence_container->addArtifact(
                types[i],
                artifactName,
                (const uint8_t*)artifactData.c_str(),
                artifactData.length(),
                true
            );
        }

        // Log action
        hid_automation->logAction("LNX_MODULE", modules[i], "SUCCESS");
    }

    return true;
}

bool FRFD::automateMacOSWithDisplay(uint8_t totalModules) {
    const char* modules[] = {"SysInfo", "Persist"};
    const char* types[] = {"filesystem", "persistence"};

    for (uint8_t i = 0; i < totalModules; i++) {
        // Show current module
        display->showHIDCollection(String(modules[i]), i + 1, totalModules);

        // Simulate module execution with progress updates
        for (uint8_t progress = 0; progress <= 100; progress += 25) {
            display->showHIDProgress(i + 1, totalModules, String(modules[i]), progress);
            delay(500); // Simulated work time
        }

        // Create simulated artifact
        String artifactName = String(modules[i]) + "_" + String(millis()) + ".dat";
        String artifactData = "Simulated macOS artifact from " + String(modules[i]) + "\n";
        artifactData += "Timestamp: " + String(millis()) + "\n";

        // Add to evidence container
        if (evidence_container) {
            evidence_container->addArtifact(
                types[i],
                artifactName,
                (const uint8_t*)artifactData.c_str(),
                artifactData.length(),
                true
            );
        }

        // Log action
        hid_automation->logAction("MAC_MODULE", modules[i], "SUCCESS");
    }

    return true;
}

void FRFD::saveHIDLog() {
    if (!hid_automation) {
        return;
    }

    Serial.println("Saving HID automation log...");

    // Save forensic action log
    bool saved = hid_automation->saveForensicLog();

    if (saved) {
        Serial.println("HID log saved successfully");

        // Generate and print chain of custody
        String custody = hid_automation->generateChainOfCustody();
        Serial.println(custody);
    } else {
        Serial.println("Failed to save HID log");
    }
}

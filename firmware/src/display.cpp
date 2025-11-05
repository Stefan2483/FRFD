#include "display.h"

FRFDDisplay::FRFDDisplay() {
    currentMode = MODE_IDLE;
    detectedOS = OS_UNKNOWN;
    riskLevel = RISK_UNKNOWN;
    status = STATUS_IDLE;
    progress = 0;
    startTime = 0;
    networkActive = false;

    // Initialize HID automation tracking
    hidMode = false;
    animFrame = 0;
    hidPhase.phase = HID_PHASE_INIT;
    hidPhase.currentStepNum = 0;
    hidPhase.totalSteps = 0;
    hidPhase.phaseProgress = 0;
    hidPhase.phaseStartTime = 0;
}

void FRFDDisplay::begin() {
    tft.init();
    tft.setRotation(TFT_ROTATION);
    tft.fillScreen(COLOR_BG);

    // Enable backlight
    pinMode(TFT_BL, OUTPUT);
    setBrightness(80);
}

void FRFDDisplay::clear() {
    tft.fillScreen(COLOR_BG);
}

void FRFDDisplay::setBrightness(uint8_t brightness) {
    analogWrite(TFT_BL, brightness);
}

void FRFDDisplay::showBootScreen() {
    clear();
    tft.setTextColor(COLOR_HEADER, COLOR_BG);
    tft.setTextDatum(MC_DATUM);

    tft.setTextSize(2);
    tft.drawString("FRFD", TFT_WIDTH/2, TFT_HEIGHT/2 - 20);

    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("CSIRT Toolkit", TFT_WIDTH/2, TFT_HEIGHT/2 + 10);
    tft.drawString("v" FIRMWARE_VERSION, TFT_WIDTH/2, TFT_HEIGHT/2 + 25);

    delay(2000);
}

void FRFDDisplay::showMainHUD() {
    clear();
    drawHeader();

    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);

    // Mode
    tft.setTextColor(COLOR_INFO, COLOR_BG);
    tft.drawString("Mode:", 5, 20);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(getModeString(currentMode), 35, 20);

    // OS
    tft.setTextColor(COLOR_INFO, COLOR_BG);
    tft.drawString("OS:", 5, 35);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(getOSString(detectedOS), 25, 35);

    // Risk Level
    tft.setTextColor(COLOR_INFO, COLOR_BG);
    tft.drawString("Risk:", 5, 50);
    tft.setTextColor(getRiskColor(riskLevel), COLOR_BG);
    tft.drawString(getRiskString(riskLevel), 35, 50);

    // Progress
    tft.setTextColor(COLOR_INFO, COLOR_BG);
    tft.drawString("Progress:", 5, 70);
    drawProgressBar(5, 85, TFT_WIDTH - 10, 10, progress);

    // Time
    tft.setTextColor(COLOR_INFO, COLOR_BG);
    tft.drawString("Time:", 5, 105);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(getElapsedTimeString(), 35, 105);

    // Network status
    if (networkActive) {
        tft.setTextColor(COLOR_SUCCESS, COLOR_BG);
        tft.drawString("NET", 5, TFT_HEIGHT - 15);
    }
}

void FRFDDisplay::showModeSelection() {
    clear();
    tft.setTextColor(COLOR_HEADER, COLOR_BG);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("SELECT MODE", TFT_WIDTH/2, 5);

    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);

    tft.drawString("1. Triage", 10, 30);
    tft.drawString("2. Collect", 10, 50);
    tft.drawString("3. Contain", 10, 70);
    tft.drawString("4. Analyze", 10, 90);
}

void FRFDDisplay::showOSDetection(OperatingSystem os) {
    clear();
    tft.setTextColor(COLOR_INFO, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Detecting OS...", TFT_WIDTH/2, TFT_HEIGHT/2 - 10);

    tft.setTextColor(COLOR_SUCCESS, COLOR_BG);
    tft.setTextSize(2);
    tft.drawString(getOSString(os), TFT_WIDTH/2, TFT_HEIGHT/2 + 15);
    tft.setTextSize(1);
}

void FRFDDisplay::showProgress(uint8_t percent) {
    progress = percent;

    // Update progress bar area only
    tft.fillRect(5, 70, TFT_WIDTH - 10, 30, COLOR_BG);
    tft.setTextColor(COLOR_INFO, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Progress:", 5, 70);
    drawProgressBar(5, 85, TFT_WIDTH - 10, 10, progress);
}

void FRFDDisplay::showError(const char* message) {
    tft.fillRect(0, TFT_HEIGHT - 30, TFT_WIDTH, 30, COLOR_DANGER);
    tft.setTextColor(COLOR_BG, COLOR_DANGER);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(message, TFT_WIDTH/2, TFT_HEIGHT - 15);
}

void FRFDDisplay::showSuccess(const char* message) {
    tft.fillRect(0, TFT_HEIGHT - 30, TFT_WIDTH, 30, COLOR_SUCCESS);
    tft.setTextColor(COLOR_BG, COLOR_SUCCESS);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(message, TFT_WIDTH/2, TFT_HEIGHT - 15);
}

void FRFDDisplay::showStatus(const char* message) {
    tft.fillRect(0, TFT_HEIGHT - 15, TFT_WIDTH, 15, COLOR_BG);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(message, TFT_WIDTH/2, TFT_HEIGHT - 7);
}

void FRFDDisplay::updateMode(OperatingMode mode) {
    currentMode = mode;
    showMainHUD();
}

void FRFDDisplay::updateOS(OperatingSystem os) {
    detectedOS = os;
}

void FRFDDisplay::updateRisk(RiskLevel risk) {
    riskLevel = risk;
}

void FRFDDisplay::updateProgress(uint8_t percent) {
    showProgress(percent);
}

void FRFDDisplay::updateStatus(CollectionStatus newStatus) {
    status = newStatus;
    showStatus(getStatusString(status).c_str());
}

void FRFDDisplay::updateNetwork(bool active) {
    networkActive = active;
}

void FRFDDisplay::updateElapsedTime() {
    // Update time display
    tft.fillRect(35, 105, 40, 10, COLOR_BG);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(getElapsedTimeString(), 35, 105);
}

String FRFDDisplay::getModeString(OperatingMode mode) {
    switch (mode) {
        case MODE_TRIAGE: return "TRIAGE";
        case MODE_COLLECTION: return "COLLECT";
        case MODE_CONTAINMENT: return "CONTAIN";
        case MODE_ANALYSIS: return "ANALYZE";
        case MODE_CONFIG: return "CONFIG";
        case MODE_IDLE: return "IDLE";
        default: return "UNKNOWN";
    }
}

String FRFDDisplay::getOSString(OperatingSystem os) {
    switch (os) {
        case OS_WINDOWS: return "Windows";
        case OS_LINUX: return "Linux";
        case OS_MACOS: return "macOS";
        case OS_UNKNOWN: return "Unknown";
        default: return "???";
    }
}

String FRFDDisplay::getRiskString(RiskLevel risk) {
    switch (risk) {
        case RISK_LOW: return "LOW";
        case RISK_MEDIUM: return "MED";
        case RISK_HIGH: return "HIGH";
        case RISK_CRITICAL: return "CRIT!";
        case RISK_UNKNOWN: return "---";
        default: return "???";
    }
}

String FRFDDisplay::getStatusString(CollectionStatus status) {
    switch (status) {
        case STATUS_IDLE: return "Idle";
        case STATUS_DETECTING: return "Detecting...";
        case STATUS_COLLECTING: return "Collecting...";
        case STATUS_ANALYZING: return "Analyzing...";
        case STATUS_TRANSFERRING: return "Transferring...";
        case STATUS_COMPLETE: return "Complete";
        case STATUS_ERROR: return "Error";
        default: return "Unknown";
    }
}

String FRFDDisplay::getElapsedTimeString() {
    unsigned long elapsed = (millis() - startTime) / 1000;
    uint16_t minutes = elapsed / 60;
    uint8_t seconds = elapsed % 60;

    char buffer[10];
    sprintf(buffer, "%02d:%02d", minutes, seconds);
    return String(buffer);
}

uint16_t FRFDDisplay::getRiskColor(RiskLevel risk) {
    switch (risk) {
        case RISK_LOW: return COLOR_SUCCESS;
        case RISK_MEDIUM: return COLOR_WARNING;
        case RISK_HIGH: return COLOR_DANGER;
        case RISK_CRITICAL: return TFT_MAGENTA;
        default: return COLOR_TEXT;
    }
}

void FRFDDisplay::drawHeader() {
    tft.fillRect(0, 0, TFT_WIDTH, 15, COLOR_INFO);
    tft.setTextColor(COLOR_BG, COLOR_INFO);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("CSIRT TOOLKIT", TFT_WIDTH/2, 7);
}

void FRFDDisplay::drawProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t percent) {
    // Draw border
    tft.drawRect(x, y, w, h, COLOR_TEXT);

    // Draw fill
    uint8_t fillWidth = (w - 4) * percent / 100;
    if (fillWidth > 0) {
        uint16_t fillColor = (percent < 33) ? COLOR_DANGER :
                            (percent < 66) ? COLOR_WARNING : COLOR_SUCCESS;
        tft.fillRect(x + 2, y + 2, fillWidth, h - 4, fillColor);
    }

    // Draw percentage text
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(TC_DATUM);
    char buffer[8];
    sprintf(buffer, "%d%%", percent);
    tft.drawString(buffer, x + w/2, y + h + 2);
}

void FRFDDisplay::drawRiskIndicator() {
    uint8_t x = TFT_WIDTH - 15;
    uint8_t y = 20;

    // Draw triangle indicator
    uint16_t color = getRiskColor(riskLevel);
    tft.fillTriangle(x, y + 10, x + 10, y + 10, x + 5, y, color);
    tft.drawString("!", x + 5, y + 5);
}

// ============================================================================
// HID AUTOMATION DISPLAY METHODS
// ============================================================================

void FRFDDisplay::showHIDAutomationStart() {
    clear();

    // Draw header
    tft.fillRect(0, 0, TFT_WIDTH, 18, TFT_PURPLE);
    tft.setTextColor(TFT_WHITE, TFT_PURPLE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("HID AUTO", TFT_WIDTH/2, 9);

    // Draw keyboard icon representation
    tft.setTextColor(TFT_CYAN, COLOR_BG);
    tft.setTextSize(3);
    tft.drawString("K", TFT_WIDTH/2, TFT_HEIGHT/2 - 10);

    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString("Automation", TFT_WIDTH/2, TFT_HEIGHT/2 + 25);
    tft.drawString("Starting...", TFT_WIDTH/2, TFT_HEIGHT/2 + 38);

    delay(1000);
}

void FRFDDisplay::showHIDPhase(HIDPhase phase, const String& phaseName) {
    hidPhase.phase = phase;
    hidPhase.phaseName = phaseName;
    hidPhase.phaseStartTime = millis();

    clear();

    // Header
    tft.fillRect(0, 0, TFT_WIDTH, 18, TFT_PURPLE);
    tft.setTextColor(TFT_WHITE, TFT_PURPLE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("HID AUTO", TFT_WIDTH/2, 9);

    // Phase name
    tft.setTextColor(TFT_CYAN, COLOR_BG);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(phaseName, TFT_WIDTH/2, 25);

    // Draw phase indicators (dots)
    drawPhaseIndicator(4, 4, (uint8_t)phase);
}

void FRFDDisplay::showHIDDetecting(const String& method) {
    clear();

    // Header
    tft.fillRect(0, 0, TFT_WIDTH, 18, TFT_PURPLE);
    tft.setTextColor(TFT_WHITE, TFT_PURPLE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("HID AUTO", TFT_WIDTH/2, 9);

    // Phase title
    tft.setTextColor(TFT_CYAN, COLOR_BG);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("OS DETECTION", TFT_WIDTH/2, 25);

    // Spinner
    drawSpinner(TFT_WIDTH/2, 60, 12, TFT_YELLOW);

    // Status
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Detecting...", TFT_WIDTH/2, 90);
    tft.drawString(method, TFT_WIDTH/2, 103);

    // Phase indicator
    drawPhaseIndicator(4, 4, 1);
}

void FRFDDisplay::showHIDOSDetected(OperatingSystem os, uint8_t confidence) {
    clear();

    // Header
    tft.fillRect(0, 0, TFT_WIDTH, 18, TFT_PURPLE);
    tft.setTextColor(TFT_WHITE, TFT_PURPLE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("HID AUTO", TFT_WIDTH/2, 9);

    // Success checkmark
    drawCheckmark(TFT_WIDTH/2, 40, TFT_GREEN);

    // OS Detected
    tft.setTextColor(TFT_GREEN, COLOR_BG);
    tft.setTextSize(2);
    tft.drawString(getOSString(os), TFT_WIDTH/2, 68);

    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);

    char confBuf[16];
    sprintf(confBuf, "Conf: %d%%", confidence);
    tft.drawString(confBuf, TFT_WIDTH/2, 90);

    // Phase indicator
    drawPhaseIndicator(4, 4, 2);

    delay(1500);
}

void FRFDDisplay::showHIDCollection(const String& moduleName, uint8_t current, uint8_t total) {
    clear();

    // Header
    tft.fillRect(0, 0, TFT_WIDTH, 18, TFT_PURPLE);
    tft.setTextColor(TFT_WHITE, TFT_PURPLE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("HID AUTO", TFT_WIDTH/2, 9);

    // Phase title
    tft.setTextColor(TFT_CYAN, COLOR_BG);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("COLLECTING", TFT_WIDTH/2, 25);

    // Module name (truncated if needed)
    tft.setTextColor(TFT_YELLOW, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    String shortName = moduleName.substring(0, min(12, (int)moduleName.length()));
    tft.drawString(shortName, TFT_WIDTH/2, 50);

    // Step counter
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    char stepBuf[16];
    sprintf(stepBuf, "Step %d/%d", current, total);
    tft.drawString(stepBuf, TFT_WIDTH/2, 65);

    // Progress bar
    uint8_t percent = (current * 100) / total;
    drawProgressBar(10, 80, TFT_WIDTH - 20, 10, percent);

    // Activity spinner
    drawSpinner(TFT_WIDTH/2, 110, 8, TFT_CYAN);

    // Phase indicator
    drawPhaseIndicator(4, 4, 3);

    // Time elapsed
    tft.setTextColor(TFT_DARKGREY, COLOR_BG);
    tft.setTextDatum(BC_DATUM);
    tft.drawString(getPhaseTimeString(), TFT_WIDTH/2, TFT_HEIGHT - 25);
}

void FRFDDisplay::showHIDProgress(uint8_t stepNum, uint8_t totalSteps, const String& stepName, uint8_t progress) {
    // Update existing display without full clear

    // Module name area
    tft.fillRect(5, 40, TFT_WIDTH - 10, 40, COLOR_BG);

    tft.setTextColor(TFT_YELLOW, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    String shortName = stepName.substring(0, min(12, (int)stepName.length()));
    tft.drawString(shortName, TFT_WIDTH/2, 50);

    // Step counter
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    char stepBuf[16];
    sprintf(stepBuf, "Step %d/%d", stepNum, totalSteps);
    tft.drawString(stepBuf, TFT_WIDTH/2, 65);

    // Update progress bar
    tft.fillRect(10, 80, TFT_WIDTH - 20, 22, COLOR_BG);
    drawProgressBar(10, 80, TFT_WIDTH - 20, 10, progress);

    // Animate spinner
    animFrame = (animFrame + 1) % 8;
    tft.fillCircle(TFT_WIDTH/2, 110, 10, COLOR_BG);
    drawSpinner(TFT_WIDTH/2, 110, 8, TFT_CYAN);
}

void FRFDDisplay::showHIDComplete(uint8_t totalActions, unsigned long durationMs) {
    clear();

    // Header
    tft.fillRect(0, 0, TFT_WIDTH, 18, TFT_GREEN);
    tft.setTextColor(TFT_WHITE, TFT_GREEN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("COMPLETE", TFT_WIDTH/2, 9);

    // Large checkmark
    drawCheckmark(TFT_WIDTH/2, 45, TFT_GREEN);

    // Stats
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);

    char actionsBuf[20];
    sprintf(actionsBuf, "%d Actions", totalActions);
    tft.drawString(actionsBuf, TFT_WIDTH/2, 80);

    // Duration
    uint16_t seconds = durationMs / 1000;
    uint8_t mins = seconds / 60;
    uint8_t secs = seconds % 60;
    char timeBuf[20];
    sprintf(timeBuf, "%dm %02ds", mins, secs);
    tft.drawString(timeBuf, TFT_WIDTH/2, 95);

    // Success message
    tft.setTextColor(TFT_GREEN, COLOR_BG);
    tft.drawString("Evidence", TFT_WIDTH/2, 115);
    tft.drawString("Collected", TFT_WIDTH/2, 128);

    // Phase indicator - all complete
    drawPhaseIndicator(4, 4, 4);

    delay(3000);
}

void FRFDDisplay::showHIDError(const String& error) {
    clear();

    // Header
    tft.fillRect(0, 0, TFT_WIDTH, 18, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("ERROR", TFT_WIDTH/2, 9);

    // Error X
    tft.setTextColor(TFT_RED, COLOR_BG);
    tft.setTextSize(4);
    tft.drawString("X", TFT_WIDTH/2, 50);

    // Error message
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);

    // Word wrap error message
    String msg = error.substring(0, min(24, (int)error.length()));
    if (msg.length() > 12) {
        String line1 = msg.substring(0, 12);
        String line2 = msg.substring(12);
        tft.drawString(line1, TFT_WIDTH/2, 95);
        tft.drawString(line2, TFT_WIDTH/2, 108);
    } else {
        tft.drawString(msg, TFT_WIDTH/2, 100);
    }

    delay(3000);
}

// ============================================================================
// HID PHASE MANAGEMENT
// ============================================================================

void FRFDDisplay::startHIDMode() {
    hidMode = true;
    hidPhase.phase = HID_PHASE_INIT;
    hidPhase.phaseStartTime = millis();
    showHIDAutomationStart();
}

void FRFDDisplay::updateHIDPhase(HIDPhase phase, const String& phaseName) {
    hidPhase.phase = phase;
    if (phaseName.length() > 0) {
        hidPhase.phaseName = phaseName;
    }
    hidPhase.phaseStartTime = millis();
    hidPhase.currentStepNum = 0;
    hidPhase.phaseProgress = 0;
}

void FRFDDisplay::updateHIDStep(uint8_t current, uint8_t total, const String& stepName) {
    hidPhase.currentStepNum = current;
    hidPhase.totalSteps = total;
    hidPhase.currentStep = stepName;
    hidPhase.phaseProgress = (current * 100) / total;
}

void FRFDDisplay::updateHIDProgress(uint8_t percent) {
    hidPhase.phaseProgress = percent;
}

void FRFDDisplay::endHIDMode() {
    hidMode = false;
    delay(2000);
    showMainHUD();
}

// ============================================================================
// VISUAL EFFECTS
// ============================================================================

void FRFDDisplay::drawSpinner(uint8_t x, uint8_t y, uint8_t radius, uint16_t color) {
    // Animated spinner based on animFrame
    const float angleStep = PI / 4.0;
    float angle = animFrame * angleStep;

    for (uint8_t i = 0; i < 8; i++) {
        float a = angle + i * angleStep;
        uint8_t lineLen = (i == 0) ? radius : radius * (8 - i) / 8;
        uint8_t x1 = x + cos(a) * (radius - lineLen);
        uint8_t y1 = y + sin(a) * (radius - lineLen);
        uint8_t x2 = x + cos(a) * radius;
        uint8_t y2 = y + sin(a) * radius;

        uint16_t lineColor = (i == 0) ? color :
                            tft.color565(
                                (color >> 11) * (8 - i) / 8,
                                ((color >> 5) & 0x3F) * (8 - i) / 8,
                                (color & 0x1F) * (8 - i) / 8
                            );
        tft.drawLine(x1, y1, x2, y2, lineColor);
    }

    animFrame = (animFrame + 1) % 8;
}

void FRFDDisplay::drawCheckmark(uint8_t x, uint8_t y, uint16_t color) {
    // Draw a checkmark symbol
    tft.drawLine(x - 8, y, x - 2, y + 6, color);
    tft.drawLine(x - 8, y + 1, x - 2, y + 7, color);
    tft.drawLine(x - 2, y + 6, x + 8, y - 6, color);
    tft.drawLine(x - 2, y + 7, x + 8, y - 5, color);
}

void FRFDDisplay::drawPhaseIndicator(uint8_t totalPhases, uint8_t totalPhasesMax, uint8_t currentPhase) {
    // Draw phase dots at bottom
    uint8_t y = TFT_HEIGHT - 10;
    uint8_t spacing = 14;
    uint8_t startX = (TFT_WIDTH - (totalPhases * spacing)) / 2;

    for (uint8_t i = 0; i < totalPhases; i++) {
        uint8_t dotX = startX + (i * spacing) + 7;

        if (i < currentPhase) {
            // Completed phase - filled circle with green
            tft.fillCircle(dotX, y, 4, TFT_GREEN);
        } else if (i == currentPhase) {
            // Current phase - filled circle with cyan
            tft.fillCircle(dotX, y, 5, TFT_CYAN);
        } else {
            // Future phase - empty circle
            tft.drawCircle(dotX, y, 4, TFT_DARKGREY);
        }
    }
}

void FRFDDisplay::drawStepProgress(uint8_t current, uint8_t total, uint8_t y) {
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextDatum(MC_DATUM);

    char stepBuf[16];
    sprintf(stepBuf, "%d/%d", current, total);
    tft.drawString(stepBuf, TFT_WIDTH/2, y);

    // Draw mini progress dots
    uint8_t dotY = y + 12;
    uint8_t spacing = 6;
    uint8_t startX = (TFT_WIDTH - (total * spacing)) / 2;

    for (uint8_t i = 0; i < total; i++) {
        uint8_t dotX = startX + (i * spacing) + 3;
        if (i < current) {
            tft.fillCircle(dotX, dotY, 2, TFT_CYAN);
        } else {
            tft.drawCircle(dotX, dotY, 2, TFT_DARKGREY);
        }
    }
}

void FRFDDisplay::animateActivity() {
    // Increment animation frame
    animFrame = (animFrame + 1) % 8;
}

// ============================================================================
// HID HELPER METHODS
// ============================================================================

String FRFDDisplay::getHIDPhaseString(HIDPhase phase) {
    switch (phase) {
        case HID_PHASE_INIT: return "Init";
        case HID_PHASE_DETECTING: return "Detecting";
        case HID_PHASE_OS_DETECTED: return "OS Found";
        case HID_PHASE_COLLECTING: return "Collecting";
        case HID_PHASE_COMPLETE: return "Complete";
        case HID_PHASE_ERROR: return "Error";
        default: return "Unknown";
    }
}

String FRFDDisplay::getPhaseTimeString() {
    unsigned long elapsed = (millis() - hidPhase.phaseStartTime) / 1000;
    char buffer[12];
    sprintf(buffer, "%lus", elapsed);
    return String(buffer);
}

// ============================================================================
// ENHANCED MODULE TRACKING (Phase 8.0+)
// ============================================================================

void FRFDDisplay::showModuleStart(const String& moduleName, uint8_t moduleNum, uint8_t totalModules) {
    tft.fillRect(0, 40, TFT_WIDTH, TFT_HEIGHT - 40, COLOR_BG);

    // Module header
    tft.setTextColor(COLOR_HEADER, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(2, 45);
    tft.print("Module ");
    tft.print(moduleNum);
    tft.print("/");
    tft.print(totalModules);

    // Module name (truncate if too long)
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(2, 58);
    String displayName = moduleName;
    if (displayName.length() > 16) {
        displayName = displayName.substring(0, 13) + "...";
    }
    tft.print(displayName);

    // Starting indicator
    tft.setTextColor(COLOR_INFO, COLOR_BG);
    tft.setCursor(2, 71);
    tft.print("STARTING...");

    // Progress bar at bottom
    drawCompactProgressBar(85, 0, "");

    animateActivity();
}

void FRFDDisplay::showModuleProgress(const String& moduleName, uint8_t progressPercent) {
    // Update progress bar
    drawCompactProgressBar(85, progressPercent, "");

    // Show percentage
    tft.fillRect(2, 71, 76, 12, COLOR_BG);
    tft.setTextColor(COLOR_INFO, COLOR_BG);
    tft.setCursor(2, 71);
    tft.print("Progress: ");
    tft.print(progressPercent);
    tft.print("%");

    animateActivity();
}

void FRFDDisplay::showModuleComplete(const String& moduleName, bool success, unsigned long durationMs) {
    tft.fillRect(0, 71, TFT_WIDTH, 26, COLOR_BG);

    if (success) {
        tft.setTextColor(COLOR_SUCCESS, COLOR_BG);
        tft.setCursor(2, 71);
        tft.print("COMPLETE");
        drawCheckmark(65, 71, COLOR_SUCCESS);
    } else {
        tft.setTextColor(COLOR_DANGER, COLOR_BG);
        tft.setCursor(2, 71);
        tft.print("FAILED");
    }

    // Duration
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(2, 84);
    tft.print(durationMs / 1000);
    tft.print(".");
    tft.print((durationMs % 1000) / 100);
    tft.print("s");

    drawCompactProgressBar(100, success ? 100 : 0, "");
}

void FRFDDisplay::showModuleList(const std::vector<String>& modules, const std::vector<bool>& completed) {
    tft.fillRect(0, 40, TFT_WIDTH, TFT_HEIGHT - 40, COLOR_BG);

    tft.setTextColor(COLOR_HEADER, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(2, 42);
    tft.print("Modules:");

    uint8_t y = 55;
    uint8_t maxDisplay = 8;  // Max modules to show
    uint8_t startIdx = 0;

    if (modules.size() > maxDisplay) {
        // Show last modules if we have too many
        startIdx = modules.size() - maxDisplay;
    }

    for (uint8_t i = startIdx; i < modules.size() && i < startIdx + maxDisplay; i++) {
        String modName = modules[i];
        if (modName.length() > 12) {
            modName = modName.substring(0, 9) + "...";
        }

        tft.setTextColor(completed[i] ? COLOR_SUCCESS : COLOR_TEXT, COLOR_BG);
        tft.setCursor(4, y);
        tft.print(completed[i] ? "+" : "-");
        tft.print(" ");
        tft.print(modName);

        y += 12;
    }
}

void FRFDDisplay::showLiveStats(uint8_t modulesCompleted, uint8_t modulesTotal, unsigned long elapsedMs, uint8_t artifactsCollected) {
    tft.fillRect(0, 100, TFT_WIDTH, 60, COLOR_BG);

    tft.setTextColor(COLOR_HEADER, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(2, 102);
    tft.print("Live Stats:");

    // Modules completed
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(2, 115);
    tft.print("Modules: ");
    tft.setTextColor(COLOR_INFO, COLOR_BG);
    tft.print(modulesCompleted);
    tft.print("/");
    tft.print(modulesTotal);

    // Elapsed time
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(2, 128);
    tft.print("Time: ");
    tft.setTextColor(COLOR_INFO, COLOR_BG);
    unsigned long seconds = elapsedMs / 1000;
    unsigned long minutes = seconds / 60;
    seconds = seconds % 60;
    tft.print(minutes);
    tft.print("m ");
    tft.print(seconds);
    tft.print("s");

    // Artifacts collected
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setCursor(2, 141);
    tft.print("Artifacts: ");
    tft.setTextColor(COLOR_SUCCESS, COLOR_BG);
    tft.print(artifactsCollected);

    // Overall progress bar
    uint8_t overallProgress = modulesTotal > 0 ? (modulesCompleted * 100) / modulesTotal : 0;
    drawCompactProgressBar(154, overallProgress, "Overall");
}

void FRFDDisplay::drawCompactProgressBar(uint8_t y, uint8_t percent, const String& label) {
    if (percent > 100) percent = 100;

    // Label
    if (label.length() > 0) {
        tft.setTextColor(COLOR_TEXT, COLOR_BG);
        tft.setTextSize(1);
        tft.setCursor(2, y);
        tft.print(label);
        y += 12;
    }

    // Background
    tft.fillRect(2, y, TFT_WIDTH - 4, 8, TFT_DARKGREY);

    // Filled portion
    uint16_t fillWidth = ((TFT_WIDTH - 4) * percent) / 100;
    uint16_t fillColor = percent < 33 ? COLOR_DANGER : (percent < 66 ? COLOR_WARNING : COLOR_SUCCESS);
    tft.fillRect(2, y, fillWidth, 8, fillColor);

    // Border
    tft.drawRect(2, y, TFT_WIDTH - 4, 8, COLOR_TEXT);
}

void FRFDDisplay::drawModuleStatus(uint8_t y, const String& moduleName, const String& status, uint16_t statusColor) {
    String displayName = moduleName;
    if (displayName.length() > 10) {
        displayName = displayName.substring(0, 7) + "...";
    }

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(2, y);
    tft.print(displayName);

    tft.setTextColor(statusColor, COLOR_BG);
    tft.setCursor(55, y);
    tft.print(status);
}

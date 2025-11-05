#include "display.h"

FRFDDisplay::FRFDDisplay() {
    currentMode = MODE_IDLE;
    detectedOS = OS_UNKNOWN;
    riskLevel = RISK_UNKNOWN;
    status = STATUS_IDLE;
    progress = 0;
    startTime = 0;
    networkActive = false;
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

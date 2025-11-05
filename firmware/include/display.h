#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

class FRFDDisplay {
private:
    TFT_eSPI tft;
    OperatingMode currentMode;
    OperatingSystem detectedOS;
    RiskLevel riskLevel;
    CollectionStatus status;
    uint8_t progress;
    unsigned long startTime;
    bool networkActive;

    // Color scheme
    const uint16_t COLOR_BG = TFT_BLACK;
    const uint16_t COLOR_HEADER = TFT_CYAN;
    const uint16_t COLOR_TEXT = TFT_WHITE;
    const uint16_t COLOR_WARNING = TFT_YELLOW;
    const uint16_t COLOR_DANGER = TFT_RED;
    const uint16_t COLOR_SUCCESS = TFT_GREEN;
    const uint16_t COLOR_INFO = TFT_BLUE;

public:
    FRFDDisplay();

    void begin();
    void clear();
    void setBrightness(uint8_t brightness);

    // Main display functions
    void showBootScreen();
    void showMainHUD();
    void showModeSelection();
    void showOSDetection(OperatingSystem os);
    void showProgress(uint8_t percent);
    void showError(const char* message);
    void showSuccess(const char* message);
    void showStatus(const char* message);

    // Update functions
    void updateMode(OperatingMode mode);
    void updateOS(OperatingSystem os);
    void updateRisk(RiskLevel risk);
    void updateProgress(uint8_t percent);
    void updateStatus(CollectionStatus status);
    void updateNetwork(bool active);
    void updateElapsedTime();

    // Helper functions
    String getModeString(OperatingMode mode);
    String getOSString(OperatingSystem os);
    String getRiskString(RiskLevel risk);
    String getStatusString(CollectionStatus status);
    String getElapsedTimeString();
    uint16_t getRiskColor(RiskLevel risk);

    // Drawing primitives
    void drawHeader();
    void drawProgressBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t percent);
    void drawRiskIndicator();
};

#endif // DISPLAY_H

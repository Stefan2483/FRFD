#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

// HID Automation Phase Tracking
enum HIDPhase {
    HID_PHASE_INIT,
    HID_PHASE_DETECTING,
    HID_PHASE_OS_DETECTED,
    HID_PHASE_COLLECTING,
    HID_PHASE_COMPLETE,
    HID_PHASE_ERROR
};

struct HIDPhaseInfo {
    HIDPhase phase;
    String phaseName;
    String currentStep;
    uint8_t currentStepNum;
    uint8_t totalSteps;
    uint8_t phaseProgress;
    unsigned long phaseStartTime;
};

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

    // HID Automation tracking
    HIDPhaseInfo hidPhase;
    bool hidMode;
    uint8_t animFrame;

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

    // HID Automation Display Methods
    void showHIDAutomationStart();
    void showHIDPhase(HIDPhase phase, const String& phaseName);
    void showHIDDetecting(const String& method);
    void showHIDOSDetected(OperatingSystem os, uint8_t confidence);
    void showHIDCollection(const String& moduleName, uint8_t current, uint8_t total);
    void showHIDProgress(uint8_t stepNum, uint8_t totalSteps, const String& stepName, uint8_t progress);
    void showHIDComplete(uint8_t totalActions, unsigned long durationMs);
    void showHIDError(const String& error);

    // HID Phase Management
    void startHIDMode();
    void updateHIDPhase(HIDPhase phase, const String& phaseName = "");
    void updateHIDStep(uint8_t current, uint8_t total, const String& stepName);
    void updateHIDProgress(uint8_t percent);
    void endHIDMode();

    // Visual Effects
    void drawSpinner(uint8_t x, uint8_t y, uint8_t radius, uint16_t color);
    void drawCheckmark(uint8_t x, uint8_t y, uint16_t color);
    void drawPhaseIndicator(uint8_t phase, uint8_t totalPhases, uint8_t currentPhase);
    void drawStepProgress(uint8_t current, uint8_t total, uint8_t y);
    void animateActivity();

    // Helper methods for HID display
    String getHIDPhaseString(HIDPhase phase);
    String getPhaseTimeString();
};

#endif // DISPLAY_H

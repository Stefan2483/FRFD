#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "config.h"
#include "storage.h"

class WiFiManager {
private:
    WebServer* server;
    FRFDStorage* storage;

    bool apActive;
    String apSSID;
    String apPassword;
    IPAddress apIP;

    // Device state for web interface
    String deviceId;
    String currentMode;
    String currentStatus;
    uint8_t currentProgress;

    // Web handlers
    void handleRoot();
    void handleStatus();
    void handleFiles();
    void handleDownload();
    void handleUpload();
    void handleConfig();
    void handleNotFound();

    // Helper functions
    String getContentType(String filename);
    String generateHTML(const String& title, const String& body);
    String generateStatusJSON();

public:
    WiFiManager(FRFDStorage* storagePtr);
    ~WiFiManager();

    // Initialization
    bool begin(const String& ssid, const String& password);
    bool startAP();
    void stop();

    // Server management
    void handleClient();
    bool isActive();

    // Status updates (for web interface)
    void setDeviceId(const String& id);
    void setMode(const String& mode);
    void setStatus(const String& status);
    void setProgress(uint8_t progress);

    // Network info
    String getAPIP();
    String getAPSSID();
    uint8_t getConnectedClients();
};

#endif // WIFI_MANAGER_H

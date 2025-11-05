#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "config.h"
#include "storage.h"
#include "evidence_container.h"

class WiFiManager {
private:
    WebServer* server;
    FRFDStorage* storage;
    EvidenceContainer* evidence_container;

    bool apActive;
    String apSSID;
    String apPassword;
    IPAddress apIP;

    // Device state for web interface
    String deviceId;
    String currentMode;
    String currentStatus;
    uint8_t currentProgress;

    // Module execution tracking
    struct ModuleStatus {
        String name;
        String status;  // "pending", "running", "completed", "failed"
        uint8_t progress;
        unsigned long start_time;
        unsigned long duration_ms;
        String error_message;
    };
    std::vector<ModuleStatus> module_statuses;

    // Forensic log buffer for web display
    std::vector<String> recent_logs;
    const size_t max_log_entries = 100;

    // Upload progress tracking
    struct UploadProgress {
        bool active;
        String filename;
        String artifact_type;
        unsigned long total_bytes;
        unsigned long uploaded_bytes;
        unsigned long start_time;
        float speed_kbps;
        uint8_t percent;
    } upload_progress;

    // Web handlers
    void handleRoot();
    void handleDashboard();
    void handleLogs();
    void handleModules();
    void handleControl();
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

    // Module status tracking
    void addModule(const String& module_name);
    void updateModuleStatus(const String& module_name, const String& status, uint8_t progress = 0);
    void setModuleError(const String& module_name, const String& error);
    void clearModules();

    // Log management
    void addLog(const String& log_entry);
    String getRecentLogs(size_t count = 50);

    // Evidence container integration
    void setEvidenceContainer(EvidenceContainer* container);

    // Network info
    String getAPIP();
    String getAPSSID();
    uint8_t getConnectedClients();

    // Upload progress
    bool isUploadActive();
    String getUploadFilename();
    unsigned long getUploadBytes();
    unsigned long getUploadTotal();
    uint8_t getUploadPercent();
    float getUploadSpeed();
};

#endif // WIFI_MANAGER_H

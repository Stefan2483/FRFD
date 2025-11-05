#include "wifi_manager.h"

WiFiManager::WiFiManager(FRFDStorage* storagePtr) {
    storage = storagePtr;
    server = nullptr;
    evidence_container = nullptr;
    apActive = false;
    apSSID = "";
    apPassword = "";
    currentProgress = 0;

    // Initialize upload progress tracking
    upload_progress.active = false;
    upload_progress.filename = "";
    upload_progress.artifact_type = "";
    upload_progress.total_bytes = 0;
    upload_progress.uploaded_bytes = 0;
    upload_progress.start_time = 0;
    upload_progress.speed_kbps = 0.0;
    upload_progress.percent = 0;
}

WiFiManager::~WiFiManager() {
    stop();
    if (server) {
        delete server;
    }
}

bool WiFiManager::begin(const String& ssid, const String& password) {
    apSSID = ssid;
    apPassword = password;

    Serial.println("[WiFi] Initializing WiFi Manager...");

    return startAP();
}

bool WiFiManager::startAP() {
    Serial.println("[WiFi] Starting Access Point...");

    // Stop any existing WiFi
    WiFi.mode(WIFI_OFF);
    delay(100);

    // Configure AP
    WiFi.mode(WIFI_AP);

    // Set static IP
    apIP = IPAddress(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);

    if (!WiFi.softAPConfig(apIP, gateway, subnet)) {
        Serial.println("[WiFi] Failed to configure AP");
        return false;
    }

    // Start AP
    bool success = WiFi.softAP(apSSID.c_str(), apPassword.c_str(), WIFI_AP_CHANNEL, 0, WIFI_MAX_CLIENTS);

    if (!success) {
        Serial.println("[WiFi] Failed to start AP");
        apActive = false;
        return false;
    }

    Serial.print("[WiFi] AP started: ");
    Serial.println(apSSID);
    Serial.print("[WiFi] IP address: ");
    Serial.println(WiFi.softAPIP());

    // Start mDNS
    if (MDNS.begin("frfd")) {
        Serial.println("[WiFi] mDNS responder started: http://frfd.local");
    }

    // Initialize web server
    if (!server) {
        server = new WebServer(80);
    }

    // Setup routes
    server->on("/", HTTP_GET, [this]() { handleRoot(); });
    server->on("/status", HTTP_GET, [this]() { handleStatus(); });
    server->on("/files", HTTP_GET, [this]() { handleFiles(); });
    server->on("/download", HTTP_GET, [this]() { handleDownload(); });
    server->on("/config", HTTP_GET, [this]() { handleConfig(); });
    server->on("/upload", HTTP_POST,
        [this]() { server->send(200, "application/json", "{\"status\":\"success\"}"); },
        [this]() { handleUpload(); }
    );
    server->onNotFound([this]() { handleNotFound(); });

    server->begin();
    Serial.println("[WiFi] Web server started on port 80");

    apActive = true;
    return true;
}

void WiFiManager::stop() {
    if (server) {
        server->stop();
    }

    if (apActive) {
        WiFi.softAPdisconnect(true);
        apActive = false;
        Serial.println("[WiFi] AP stopped");
    }
}

void WiFiManager::handleClient() {
    if (server && apActive) {
        server->handleClient();
    }
}

bool WiFiManager::isActive() {
    return apActive;
}

void WiFiManager::setDeviceId(const String& id) {
    deviceId = id;
}

void WiFiManager::setMode(const String& mode) {
    currentMode = mode;
}

void WiFiManager::setStatus(const String& status) {
    currentStatus = status;
}

void WiFiManager::setProgress(uint8_t progress) {
    currentProgress = progress;
}

void WiFiManager::setEvidenceContainer(EvidenceContainer* container) {
    evidence_container = container;
}

String WiFiManager::getAPIP() {
    return WiFi.softAPIP().toString();
}

String WiFiManager::getAPSSID() {
    return apSSID;
}

uint8_t WiFiManager::getConnectedClients() {
    return WiFi.softAPgetStationNum();
}

bool WiFiManager::isUploadActive() {
    return upload_progress.active;
}

String WiFiManager::getUploadFilename() {
    return upload_progress.filename;
}

unsigned long WiFiManager::getUploadBytes() {
    return upload_progress.uploaded_bytes;
}

unsigned long WiFiManager::getUploadTotal() {
    return upload_progress.total_bytes;
}

uint8_t WiFiManager::getUploadPercent() {
    return upload_progress.percent;
}

float WiFiManager::getUploadSpeed() {
    return upload_progress.speed_kbps;
}

void WiFiManager::handleRoot() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>FRFD - Forensics Dongle</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background: #2c3e50;
            color: #ecf0f1;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: #34495e;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.3);
        }
        h1 {
            color: #3498db;
            text-align: center;
            margin-top: 0;
        }
        .status-box {
            background: #2c3e50;
            padding: 15px;
            border-radius: 5px;
            margin: 15px 0;
        }
        .status-item {
            display: flex;
            justify-content: space-between;
            padding: 8px 0;
            border-bottom: 1px solid #34495e;
        }
        .status-label {
            font-weight: bold;
            color: #3498db;
        }
        .menu {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin: 20px 0;
        }
        .menu-item {
            background: #3498db;
            color: white;
            padding: 20px;
            text-align: center;
            border-radius: 5px;
            text-decoration: none;
            transition: background 0.3s;
        }
        .menu-item:hover {
            background: #2980b9;
        }
        .footer {
            text-align: center;
            margin-top: 30px;
            padding-top: 20px;
            border-top: 1px solid #34495e;
            color: #7f8c8d;
        }
    </style>
    <script>
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('mode').textContent = data.mode;
                    document.getElementById('status').textContent = data.status;
                    document.getElementById('progress').textContent = data.progress + '%';
                    document.getElementById('clients').textContent = data.connected_clients;
                });
        }
        setInterval(updateStatus, 2000);
        window.onload = updateStatus;
    </script>
</head>
<body>
    <div class="container">
        <h1>🔍 FRFD Control Panel</h1>

        <div class="status-box">
            <h2>Device Status</h2>
            <div class="status-item">
                <span class="status-label">Device ID:</span>
                <span>)" + deviceId + R"(</span>
            </div>
            <div class="status-item">
                <span class="status-label">Mode:</span>
                <span id="mode">)" + currentMode + R"(</span>
            </div>
            <div class="status-item">
                <span class="status-label">Status:</span>
                <span id="status">)" + currentStatus + R"(</span>
            </div>
            <div class="status-item">
                <span class="status-label">Progress:</span>
                <span id="progress">)" + String(currentProgress) + R"(%</span>
            </div>
            <div class="status-item">
                <span class="status-label">Connected Clients:</span>
                <span id="clients">)" + String(getConnectedClients()) + R"(</span>
            </div>
        </div>

        <div class="menu">
            <a href="/files" class="menu-item">
                📁 Browse Files
            </a>
            <a href="/status" class="menu-item">
                📊 Status (JSON)
            </a>
            <a href="/config" class="menu-item">
                ⚙️ Configuration
            </a>
        </div>

        <div class="footer">
            FRFD v)" + String(FIRMWARE_VERSION) + R"( - CSIRT Forensics Toolkit
        </div>
    </div>
</body>
</html>
)";

    server->send(200, "text/html", html);
}

void WiFiManager::handleStatus() {
    String json = generateStatusJSON();
    server->send(200, "application/json", json);
}

String WiFiManager::generateStatusJSON() {
    String json = "{";
    json += "\"device_id\":\"" + deviceId + "\",";
    json += "\"mode\":\"" + currentMode + "\",";
    json += "\"status\":\"" + currentStatus + "\",";
    json += "\"progress\":" + String(currentProgress) + ",";
    json += "\"connected_clients\":" + String(getConnectedClients()) + ",";
    json += "\"ip_address\":\"" + getAPIP() + "\",";
    json += "\"ssid\":\"" + getAPSSID() + "\",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"sd_card\":" + String(storage->isSDCardAvailable() ? "true" : "false") + ",";

    if (storage->isSDCardAvailable()) {
        json += "\"sd_size_mb\":" + String(storage->getSDCardSize()) + ",";
        json += "\"sd_free_mb\":" + String(storage->getSDCardFree()) + ",";
    }

    // Add upload progress
    json += "\"upload\":{";
    json += "\"active\":" + String(upload_progress.active ? "true" : "false") + ",";
    if (upload_progress.active) {
        json += "\"filename\":\"" + upload_progress.filename + "\",";
        json += "\"type\":\"" + upload_progress.artifact_type + "\",";
        json += "\"bytes\":" + String(upload_progress.uploaded_bytes) + ",";
        json += "\"speed_kbps\":" + String(upload_progress.speed_kbps, 2) + ",";
        json += "\"percent\":" + String(upload_progress.percent);
    }
    json += "},";

    json += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\"";
    json += "}";

    return json;
}

void WiFiManager::handleFiles() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>FRFD - Files</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #2c3e50; color: #ecf0f1; }
        .container { max-width: 1000px; margin: 0 auto; background: #34495e; padding: 20px; border-radius: 10px; }
        h1 { color: #3498db; }
        table { width: 100%; border-collapse: collapse; margin: 20px 0; }
        th, td { padding: 10px; text-align: left; border-bottom: 1px solid #2c3e50; }
        th { background: #3498db; color: white; }
        tr:hover { background: #2c3e50; }
        a { color: #3498db; text-decoration: none; }
        a:hover { text-decoration: underline; }
        .back { display: inline-block; margin: 10px 0; padding: 10px 20px; background: #3498db; color: white; border-radius: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>📁 Evidence Files</h1>
        <a href="/" class="back">← Back to Home</a>
)";

    if (!storage->isSDCardAvailable()) {
        html += "<p>⚠️ SD Card not available</p>";
    } else {
        String caseDir = storage->getCaseDirectory();

        if (caseDir.isEmpty()) {
            html += "<p>No active case. Files will appear here once collection starts.</p>";
        } else {
            html += "<p><strong>Case Directory:</strong> " + caseDir + "</p>";
            html += "<table><tr><th>Filename</th><th>Size</th><th>Action</th></tr>";

            std::vector<String> files = storage->getFileList(caseDir);

            for (const String& file : files) {
                String fullPath = caseDir + "/" + file;
                size_t size = storage->getFileSize(fullPath);

                html += "<tr>";
                html += "<td>" + file + "</td>";
                html += "<td>" + String(size) + " bytes</td>";
                html += "<td><a href='/download?file=" + fullPath + "'>Download</a></td>";
                html += "</tr>";
            }

            html += "</table>";

            if (files.size() == 0) {
                html += "<p>No files collected yet.</p>";
            }
        }
    }

    html += R"(
    </div>
</body>
</html>
)";

    server->send(200, "text/html", html);
}

void WiFiManager::handleDownload() {
    if (!server->hasArg("file")) {
        server->send(400, "text/plain", "Missing file parameter");
        return;
    }

    String filePath = server->arg("file");

    if (!storage->fileExists(filePath)) {
        server->send(404, "text/plain", "File not found");
        return;
    }

    String content = storage->readFile(filePath);
    String contentType = getContentType(filePath);

    // Extract filename from path
    int lastSlash = filePath.lastIndexOf('/');
    String filename = (lastSlash >= 0) ? filePath.substring(lastSlash + 1) : filePath;

    server->sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    server->send(200, contentType, content);

    Serial.printf("[WiFi] Downloaded: %s (%d bytes)\n", filename.c_str(), content.length());
}

void WiFiManager::handleConfig() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>FRFD - Configuration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #2c3e50; color: #ecf0f1; }
        .container { max-width: 800px; margin: 0 auto; background: #34495e; padding: 20px; border-radius: 10px; }
        h1 { color: #3498db; }
        .config-item { margin: 15px 0; padding: 10px; background: #2c3e50; border-radius: 5px; }
        .back { display: inline-block; margin: 10px 0; padding: 10px 20px; background: #3498db; color: white; border-radius: 5px; text-decoration: none; }
    </style>
</head>
<body>
    <div class="container">
        <h1>⚙️ Device Configuration</h1>
        <a href="/" class="back">← Back to Home</a>

        <div class="config-item">
            <strong>Device ID:</strong> )" + deviceId + R"(
        </div>
        <div class="config-item">
            <strong>Firmware Version:</strong> )" + String(FIRMWARE_VERSION) + R"(
        </div>
        <div class="config-item">
            <strong>WiFi SSID:</strong> )" + apSSID + R"(
        </div>
        <div class="config-item">
            <strong>IP Address:</strong> )" + getAPIP() + R"(
        </div>
        <div class="config-item">
            <strong>SD Card:</strong> )" + String(storage->isSDCardAvailable() ? "Available" : "Not Available") + R"(
        </div>
)";

    if (storage->isSDCardAvailable()) {
        html += R"(
        <div class="config-item">
            <strong>SD Card Size:</strong> )" + String(storage->getSDCardSize()) + R"( MB
        </div>
        <div class="config-item">
            <strong>SD Card Free:</strong> )" + String(storage->getSDCardFree()) + R"( MB
        </div>
)";
    }

    html += R"(
    </div>
</body>
</html>
)";

    server->send(200, "text/html", html);
}

void WiFiManager::handleNotFound() {
    server->send(404, "text/plain", "404: Not Found");
}

void WiFiManager::handleUpload() {
    static String currentArtifactType;
    static String currentFilename;
    static String currentSourcePath;
    static std::vector<uint8_t> uploadBuffer;
    static unsigned long uploadStartTime;

    HTTPUpload& upload = server->upload();

    if (upload.status == UPLOAD_FILE_START) {
        // Initialize upload
        uploadStartTime = millis();
        currentFilename = upload.filename;
        uploadBuffer.clear();

        // Extract artifact type from form parameter
        if (server->hasArg("type")) {
            currentArtifactType = server->arg("type");
        } else {
            currentArtifactType = "unknown";
        }

        // Extract source path if provided
        if (server->hasArg("source_path")) {
            currentSourcePath = server->arg("source_path");
        } else {
            currentSourcePath = "";
        }

        // Initialize progress tracking
        upload_progress.active = true;
        upload_progress.filename = currentFilename;
        upload_progress.artifact_type = currentArtifactType;
        upload_progress.total_bytes = 0;  // Unknown until complete
        upload_progress.uploaded_bytes = 0;
        upload_progress.start_time = uploadStartTime;
        upload_progress.speed_kbps = 0.0;
        upload_progress.percent = 0;

        Serial.printf("[WiFi] Upload started: %s (%s)\n",
                     currentFilename.c_str(),
                     currentArtifactType.c_str());

    } else if (upload.status == UPLOAD_FILE_WRITE) {
        // Append chunk to buffer
        size_t oldSize = uploadBuffer.size();
        uploadBuffer.resize(oldSize + upload.currentSize);
        memcpy(&uploadBuffer[oldSize], upload.buf, upload.currentSize);

        // Update progress tracking
        upload_progress.uploaded_bytes = uploadBuffer.size();
        unsigned long elapsed = millis() - uploadStartTime;
        if (elapsed > 0) {
            upload_progress.speed_kbps = (uploadBuffer.size() / 1024.0) / (elapsed / 1000.0);
        }

        // Estimate percent (if we knew total size, but we don't for chunked uploads)
        // For now, just track that we're receiving data
        upload_progress.percent = 0;  // Will be 100 when complete

        // Update progress (every 10KB)
        if (uploadBuffer.size() % 10240 == 0) {
            Serial.printf("[WiFi] Received: %d bytes (%.2f KB/s)\n",
                         uploadBuffer.size(),
                         upload_progress.speed_kbps);
        }

    } else if (upload.status == UPLOAD_FILE_END) {
        // Upload complete - save to evidence container
        unsigned long uploadDuration = millis() - uploadStartTime;

        Serial.printf("[WiFi] Upload complete: %s (%d bytes in %lu ms)\n",
                     currentFilename.c_str(),
                     uploadBuffer.size(),
                     uploadDuration);

        if (!evidence_container) {
            Serial.println("[WiFi] ERROR: No evidence container available!");
            server->send(500, "application/json",
                        "{\"status\":\"error\",\"message\":\"No evidence container\"}");
            uploadBuffer.clear();
            return;
        }

        if (!evidence_container->isOpen()) {
            Serial.println("[WiFi] ERROR: Evidence container not open!");
            server->send(500, "application/json",
                        "{\"status\":\"error\",\"message\":\"Evidence container not open\"}");
            uploadBuffer.clear();
            return;
        }

        // Add artifact to evidence container
        String artifactId = evidence_container->addArtifact(
            currentArtifactType,
            currentFilename,
            uploadBuffer.data(),
            uploadBuffer.size(),
            true  // Enable compression
        );

        if (artifactId.isEmpty()) {
            Serial.println("[WiFi] ERROR: Failed to add artifact to container!");
            server->send(500, "application/json",
                        "{\"status\":\"error\",\"message\":\"Failed to save artifact\"}");
            uploadBuffer.clear();
            return;
        }

        // Update metadata with source path if provided
        if (!currentSourcePath.isEmpty()) {
            ArtifactMetadata meta;
            const auto& artifacts = evidence_container->getArtifacts();
            for (const auto& artifact : artifacts) {
                if (artifact.artifact_id == artifactId) {
                    meta = artifact;
                    meta.source_path = currentSourcePath;
                    evidence_container->addArtifactMetadata(artifactId, meta);
                    break;
                }
            }
        }

        // Log successful collection
        evidence_container->logAction(
            "ARTIFACT_UPLOAD",
            "Received " + currentArtifactType + ": " + currentFilename,
            "SUCCESS - " + String(uploadBuffer.size()) + " bytes"
        );

        // Calculate transfer speed
        float speedKBps = (uploadBuffer.size() / 1024.0) / (uploadDuration / 1000.0);

        Serial.printf("[WiFi] Artifact saved: %s (%.2f KB/s)\n",
                     artifactId.c_str(), speedKBps);

        // Send success response with artifact ID
        String response = "{";
        response += "\"status\":\"success\",";
        response += "\"artifact_id\":\"" + artifactId + "\",";
        response += "\"filename\":\"" + currentFilename + "\",";
        response += "\"size\":" + String(uploadBuffer.size()) + ",";
        response += "\"duration_ms\":" + String(uploadDuration) + ",";
        response += "\"speed_kbps\":" + String(speedKBps, 2);
        response += "}";

        server->send(200, "application/json", response);

        // Clear buffer
        uploadBuffer.clear();

        // Finalize progress tracking
        upload_progress.total_bytes = uploadBuffer.size();
        upload_progress.uploaded_bytes = uploadBuffer.size();
        upload_progress.percent = 100;
        upload_progress.active = false;

    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Serial.println("[WiFi] Upload aborted!");
        uploadBuffer.clear();

        // Reset progress tracking
        upload_progress.active = false;
        upload_progress.percent = 0;

        server->send(500, "application/json",
                    "{\"status\":\"error\",\"message\":\"Upload aborted\"}");
    }
}

String WiFiManager::getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".txt")) return "text/plain";
    else if (filename.endsWith(".csv")) return "text/csv";
    else if (filename.endsWith(".zip")) return "application/zip";
    else if (filename.endsWith(".gz")) return "application/gzip";
    else if (filename.endsWith(".pdf")) return "application/pdf";
    return "application/octet-stream";
}

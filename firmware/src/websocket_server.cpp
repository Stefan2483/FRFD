#include "websocket_server.h"

// Global instance for static callback
WebSocketServer* g_ws_server_instance = nullptr;

WebSocketServer::WebSocketServer(AsyncWebServer* server)
    : web_server(server),
      event_queue_enabled(true),
      max_queue_size(50),
      max_messages_per_second(20),
      max_clients_allowed(5),
      total_clients_connected(0),
      total_messages_sent(0),
      total_events_broadcast(0),
      failed_sends(0),
      start_time(0),
      last_message_time(0),
      messages_this_second(0) {

    ws = new AsyncWebSocket("/ws");
    g_ws_server_instance = this;
}

WebSocketServer::~WebSocketServer() {
    if (ws) {
        delete ws;
    }
    g_ws_server_instance = nullptr;
}

void WebSocketServer::begin() {
    // Setup WebSocket event handler
    ws->onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client,
                   AwsEventType type, void* arg, uint8_t* data, size_t len) {
        if (g_ws_server_instance) {
            g_ws_server_instance->handleWebSocketEvent(server, client, type, arg, data, len);
        }
    });

    // Add WebSocket handler to web server
    web_server->addHandler(ws);

    start_time = millis();

    Serial.println("[WebSocketServer] Started on /ws");
}

void WebSocketServer::end() {
    disconnectAll();
    ws->enable(false);
    Serial.println("[WebSocketServer] Stopped");
}

// ===========================
// Event Broadcasting
// ===========================

void WebSocketServer::broadcastModuleStart(const String& module_name) {
    String json = createModuleStartJSON(module_name);
    broadcastJSON(json);
    total_events_broadcast++;

    Serial.println("[WebSocketServer] Broadcast: Module started - " + module_name);
}

void WebSocketServer::broadcastModuleComplete(const String& module_name, bool success) {
    String json = createModuleCompleteJSON(module_name, success);
    broadcastJSON(json);
    total_events_broadcast++;

    Serial.println("[WebSocketServer] Broadcast: Module complete - " + module_name +
                   (success ? " (success)" : " (failed)"));
}

void WebSocketServer::broadcastModuleFailed(const String& module_name, const String& error) {
    WSEvent event;
    event.type = WS_EVENT_MODULE_FAILED;
    event.priority = WS_PRIORITY_HIGH;
    event.title = "Module Failed";
    event.message = module_name;
    event.details = error;
    event.timestamp = millis();

    broadcastEvent(event);
}

void WebSocketServer::broadcastFileCreated(const String& file_path, uint32_t file_size) {
    WSEvent event;
    event.type = WS_EVENT_FILE_CREATED;
    event.priority = WS_PRIORITY_LOW;
    event.title = "File Created";
    event.message = file_path;
    event.details = "Size: " + String(file_size) + " bytes";
    event.timestamp = millis();

    broadcastEvent(event);
}

void WebSocketServer::broadcastProgressUpdate(uint8_t progress_percent, const String& current_task) {
    String json = createProgressJSON(progress_percent, current_task);
    broadcastJSON(json);
    total_events_broadcast++;
}

void WebSocketServer::broadcastAlert(const String& alert_type, const String& message, const String& severity) {
    String json = createAlertJSON(alert_type, message, severity);
    broadcastJSON(json);
    total_events_broadcast++;

    Serial.println("[WebSocketServer] Broadcast: ALERT - " + alert_type + " (" + severity + ")");
}

void WebSocketServer::broadcastStatistics(const String& stats_json) {
    String json = createStatsJSON(stats_json);
    broadcastJSON(json);
}

void WebSocketServer::broadcastStatusChange(const String& old_status, const String& new_status) {
    WSEvent event;
    event.type = WS_EVENT_STATUS_CHANGE;
    event.priority = WS_PRIORITY_NORMAL;
    event.title = "Status Change";
    event.message = new_status;
    event.details = "Previous: " + old_status;
    event.timestamp = millis();

    broadcastEvent(event);
}

void WebSocketServer::broadcastLogMessage(const String& level, const String& message) {
    WSEvent event;
    event.type = WS_EVENT_LOG_MESSAGE;
    event.priority = WS_PRIORITY_LOW;
    event.title = level;
    event.message = message;
    event.timestamp = millis();

    broadcastEvent(event);
}

void WebSocketServer::broadcastIOCFound(const String& ioc_type, const String& ioc_value) {
    WSEvent event;
    event.type = WS_EVENT_IOC_FOUND;
    event.priority = WS_PRIORITY_HIGH;
    event.title = "IOC Detected";
    event.message = ioc_type;
    event.details = ioc_value;
    event.timestamp = millis();

    broadcastEvent(event);

    Serial.println("[WebSocketServer] Broadcast: IOC found - " + ioc_type + ": " + ioc_value);
}

void WebSocketServer::broadcastCorrelationFound(const String& correlation_type, const String& description) {
    WSEvent event;
    event.type = WS_EVENT_CORRELATION_FOUND;
    event.priority = WS_PRIORITY_HIGH;
    event.title = "Correlation Found";
    event.message = correlation_type;
    event.details = description;
    event.timestamp = millis();

    broadcastEvent(event);
}

void WebSocketServer::broadcastThreatDetected(const String& threat_name, const String& severity) {
    WSEvent event;
    event.type = WS_EVENT_THREAT_DETECTED;
    event.priority = WS_PRIORITY_CRITICAL;
    event.title = "Threat Detected";
    event.message = threat_name;
    event.details = "Severity: " + severity;
    event.timestamp = millis();

    broadcastEvent(event);

    Serial.println("[WebSocketServer] Broadcast: THREAT DETECTED - " + threat_name + " (" + severity + ")");
}

void WebSocketServer::broadcastScanComplete(uint32_t artifacts_collected, uint32_t alerts_generated) {
    WSEvent event;
    event.type = WS_EVENT_SCAN_COMPLETE;
    event.priority = WS_PRIORITY_NORMAL;
    event.title = "Scan Complete";
    event.message = "Collected " + String(artifacts_collected) + " artifacts";
    event.details = "Generated " + String(alerts_generated) + " alerts";
    event.timestamp = millis();

    broadcastEvent(event);

    Serial.println("[WebSocketServer] Broadcast: Scan complete - " + String(artifacts_collected) +
                   " artifacts, " + String(alerts_generated) + " alerts");
}

void WebSocketServer::broadcastEvent(const WSEvent& event) {
    if (event_queue_enabled && event_queue.size() < max_queue_size) {
        addToQueue(event);
    } else {
        String json = createEventJSON(event);
        broadcastJSON(json);
    }
}

void WebSocketServer::broadcastJSON(const String& json) {
    if (!checkRateLimit()) {
        Serial.println("[WebSocketServer] Rate limit exceeded, message queued");
        return;
    }

    // Clean up disconnected clients first
    cleanupDisconnectedClients();

    // Broadcast to all connected clients
    ws->textAll(json);
    total_messages_sent++;

    // Update rate limiting
    messages_this_second++;
    last_message_time = millis();
}

void WebSocketServer::broadcastToClient(uint32_t client_id, const String& message) {
    AsyncWebSocketClient* client = ws->client(client_id);
    if (client && client->status() == WS_CONNECTED) {
        client->text(message);
        updateClientInfo(client_id);
    }
}

// ===========================
// Client Management
// ===========================

uint32_t WebSocketServer::getClientCount() const {
    return ws->count();
}

std::vector<ClientInfo> WebSocketServer::getConnectedClients() const {
    std::vector<ClientInfo> connected;
    for (const auto& client : clients) {
        AsyncWebSocketClient* ws_client = ws->client(client.id);
        if (ws_client && ws_client->status() == WS_CONNECTED) {
            connected.push_back(client);
        }
    }
    return connected;
}

void WebSocketServer::disconnectClient(uint32_t client_id) {
    AsyncWebSocketClient* client = ws->client(client_id);
    if (client) {
        client->close();
        Serial.println("[WebSocketServer] Disconnected client " + String(client_id));
    }
}

void WebSocketServer::disconnectAll() {
    ws->closeAll();
    clients.clear();
    Serial.println("[WebSocketServer] Disconnected all clients");
}

// ===========================
// Subscription Management
// ===========================

void WebSocketServer::setClientSubscription(uint32_t client_id, const String& subscription_type, bool enabled) {
    ClientInfo* client = getClientInfo(client_id);
    if (!client) return;

    if (subscription_type == "alerts") {
        client->subscribed_alerts = enabled;
    } else if (subscription_type == "progress") {
        client->subscribed_progress = enabled;
    } else if (subscription_type == "logs") {
        client->subscribed_logs = enabled;
    }
}

bool WebSocketServer::isClientSubscribed(uint32_t client_id, const String& subscription_type) {
    ClientInfo* client = getClientInfo(client_id);
    if (!client) return false;

    if (subscription_type == "alerts") return client->subscribed_alerts;
    if (subscription_type == "progress") return client->subscribed_progress;
    if (subscription_type == "logs") return client->subscribed_logs;

    return false;
}

// ===========================
// Statistics
// ===========================

WSStatistics WebSocketServer::getStatistics() const {
    WSStatistics stats;
    stats.total_clients_connected = total_clients_connected;
    stats.current_clients = getClientCount();
    stats.total_messages_sent = total_messages_sent;
    stats.total_events_broadcast = total_events_broadcast;
    stats.failed_sends = failed_sends;
    stats.uptime_ms = millis() - start_time;

    return stats;
}

void WebSocketServer::resetStatistics() {
    total_clients_connected = 0;
    total_messages_sent = 0;
    total_events_broadcast = 0;
    failed_sends = 0;
    start_time = millis();
}

// ===========================
// Configuration
// ===========================

void WebSocketServer::setMaxMessagesPerSecond(uint16_t max_rate) {
    max_messages_per_second = max_rate;
}

void WebSocketServer::setMaxClientsAllowed(uint8_t max_clients) {
    max_clients_allowed = max_clients;
}

void WebSocketServer::enableEventQueue(bool enabled) {
    event_queue_enabled = enabled;
}

void WebSocketServer::setEventQueueSize(uint16_t size) {
    max_queue_size = size;
}

void WebSocketServer::flushEventQueue() {
    processQueue();
}

// ===========================
// Event Handlers
// ===========================

void WebSocketServer::handleWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                           AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            onConnect(client);
            break;

        case WS_EVT_DISCONNECT:
            onDisconnect(client);
            break;

        case WS_EVT_DATA:
            onMessage(client, arg, data, len);
            break;

        case WS_EVT_ERROR:
            onError(client, arg, data, len);
            break;

        default:
            break;
    }
}

void WebSocketServer::onConnect(AsyncWebSocketClient* client) {
    if (getClientCount() > max_clients_allowed) {
        Serial.println("[WebSocketServer] Max clients reached, rejecting connection");
        client->close();
        return;
    }

    ClientInfo info;
    info.id = client->id();
    info.ip_address = client->remoteIP().toString();
    info.connect_time = millis();
    info.messages_sent = 0;
    info.subscribed_alerts = true;
    info.subscribed_progress = true;
    info.subscribed_logs = false;

    clients.push_back(info);
    total_clients_connected++;

    Serial.println("[WebSocketServer] Client connected: " + String(info.id) +
                   " from " + info.ip_address);

    // Send welcome message
    String welcome = "{\"type\":\"welcome\",\"message\":\"Connected to FRFD WebSocket Server\",";
    welcome += "\"server_time\":" + String(millis()) + "}";
    client->text(welcome);
}

void WebSocketServer::onDisconnect(AsyncWebSocketClient* client) {
    Serial.println("[WebSocketServer] Client disconnected: " + String(client->id()));

    // Remove from clients list
    for (size_t i = 0; i < clients.size(); i++) {
        if (clients[i].id == client->id()) {
            clients.erase(clients.begin() + i);
            break;
        }
    }
}

void WebSocketServer::onMessage(AsyncWebSocketClient* client, void* arg, uint8_t* data, size_t len) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;

    if (info->final && info->index == 0 && info->len == len) {
        // Complete message received
        String message = "";
        for (size_t i = 0; i < len; i++) {
            message += (char)data[i];
        }

        Serial.println("[WebSocketServer] Received from " + String(client->id()) + ": " + message);

        // Parse commands (simple JSON parsing)
        if (message.indexOf("\"cmd\":\"subscribe\"") >= 0) {
            // Handle subscription
            if (message.indexOf("\"alerts\"") >= 0) {
                setClientSubscription(client->id(), "alerts", true);
            }
            if (message.indexOf("\"progress\"") >= 0) {
                setClientSubscription(client->id(), "progress", true);
            }
            if (message.indexOf("\"logs\"") >= 0) {
                setClientSubscription(client->id(), "logs", true);
            }

            client->text("{\"status\":\"subscribed\"}");
        }
        else if (message.indexOf("\"cmd\":\"unsubscribe\"") >= 0) {
            // Handle unsubscribe
            if (message.indexOf("\"alerts\"") >= 0) {
                setClientSubscription(client->id(), "alerts", false);
            }
            if (message.indexOf("\"progress\"") >= 0) {
                setClientSubscription(client->id(), "progress", false);
            }
            if (message.indexOf("\"logs\"") >= 0) {
                setClientSubscription(client->id(), "logs", false);
            }

            client->text("{\"status\":\"unsubscribed\"}");
        }
        else if (message.indexOf("\"cmd\":\"ping\"") >= 0) {
            client->text("{\"pong\":" + String(millis()) + "}");
        }
        else if (message.indexOf("\"cmd\":\"stats\"") >= 0) {
            WSStatistics stats = getStatistics();
            String stats_json = "{\"clients\":" + String(stats.current_clients) + ",";
            stats_json += "\"messages\":" + String(stats.total_messages_sent) + ",";
            stats_json += "\"events\":" + String(stats.total_events_broadcast) + "}";
            client->text(stats_json);
        }
    }
}

void WebSocketServer::onError(AsyncWebSocketClient* client, void* arg, uint8_t* data, size_t len) {
    Serial.println("[WebSocketServer] Error from client " + String(client->id()));
    failed_sends++;
}

// ===========================
// Message Formatting
// ===========================

String WebSocketServer::createEventJSON(const WSEvent& event) {
    String json = "{";
    json += "\"type\":\"" + getEventTypeName(event.type) + "\",";
    json += "\"priority\":\"" + getPriorityName(event.priority) + "\",";
    json += "\"title\":\"" + event.title + "\",";
    json += "\"message\":\"" + event.message + "\",";
    json += "\"details\":\"" + event.details + "\",";
    json += "\"timestamp\":" + String(event.timestamp);
    json += "}";

    return json;
}

String WebSocketServer::createModuleStartJSON(const String& module_name) {
    String json = "{";
    json += "\"type\":\"module_start\",";
    json += "\"module\":\"" + module_name + "\",";
    json += "\"timestamp\":" + String(millis());
    json += "}";

    return json;
}

String WebSocketServer::createModuleCompleteJSON(const String& module_name, bool success) {
    String json = "{";
    json += "\"type\":\"module_complete\",";
    json += "\"module\":\"" + module_name + "\",";
    json += "\"success\":" + String(success ? "true" : "false") + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";

    return json;
}

String WebSocketServer::createProgressJSON(uint8_t progress, const String& task) {
    String json = "{";
    json += "\"type\":\"progress\",";
    json += "\"percent\":" + String(progress) + ",";
    json += "\"task\":\"" + task + "\",";
    json += "\"timestamp\":" + String(millis());
    json += "}";

    return json;
}

String WebSocketServer::createAlertJSON(const String& alert_type, const String& message, const String& severity) {
    String json = "{";
    json += "\"type\":\"alert\",";
    json += "\"alert_type\":\"" + alert_type + "\",";
    json += "\"message\":\"" + message + "\",";
    json += "\"severity\":\"" + severity + "\",";
    json += "\"timestamp\":" + String(millis());
    json += "}";

    return json;
}

String WebSocketServer::createStatsJSON(const String& stats) {
    String json = "{";
    json += "\"type\":\"statistics\",";
    json += "\"data\":" + stats + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";

    return json;
}

// ===========================
// Helper Methods
// ===========================

ClientInfo* WebSocketServer::getClientInfo(uint32_t client_id) {
    for (size_t i = 0; i < clients.size(); i++) {
        if (clients[i].id == client_id) {
            return &clients[i];
        }
    }
    return nullptr;
}

void WebSocketServer::addToQueue(const WSEvent& event) {
    if (event_queue.size() < max_queue_size) {
        event_queue.push_back(event);
    }
}

void WebSocketServer::processQueue() {
    for (const auto& event : event_queue) {
        String json = createEventJSON(event);
        broadcastJSON(json);
    }
    event_queue.clear();
}

bool WebSocketServer::checkRateLimit() {
    unsigned long current_time = millis();

    // Reset counter every second
    if (current_time - last_message_time > 1000) {
        messages_this_second = 0;
    }

    return messages_this_second < max_messages_per_second;
}

void WebSocketServer::updateClientInfo(uint32_t client_id) {
    ClientInfo* client = getClientInfo(client_id);
    if (client) {
        client->messages_sent++;
    }
}

void WebSocketServer::cleanupDisconnectedClients() {
    for (int i = clients.size() - 1; i >= 0; i--) {
        AsyncWebSocketClient* client = ws->client(clients[i].id);
        if (!client || client->status() != WS_CONNECTED) {
            clients.erase(clients.begin() + i);
        }
    }
}

String WebSocketServer::getEventTypeName(WSEventType type) {
    switch (type) {
        case WS_EVENT_MODULE_START: return "module_start";
        case WS_EVENT_MODULE_COMPLETE: return "module_complete";
        case WS_EVENT_MODULE_FAILED: return "module_failed";
        case WS_EVENT_FILE_CREATED: return "file_created";
        case WS_EVENT_PROGRESS_UPDATE: return "progress";
        case WS_EVENT_ALERT: return "alert";
        case WS_EVENT_STATISTICS: return "statistics";
        case WS_EVENT_STATUS_CHANGE: return "status_change";
        case WS_EVENT_LOG_MESSAGE: return "log";
        case WS_EVENT_IOC_FOUND: return "ioc_found";
        case WS_EVENT_CORRELATION_FOUND: return "correlation_found";
        case WS_EVENT_THREAT_DETECTED: return "threat_detected";
        case WS_EVENT_SCAN_COMPLETE: return "scan_complete";
        default: return "unknown";
    }
}

String WebSocketServer::getPriorityName(WSEventPriority priority) {
    switch (priority) {
        case WS_PRIORITY_LOW: return "low";
        case WS_PRIORITY_NORMAL: return "normal";
        case WS_PRIORITY_HIGH: return "high";
        case WS_PRIORITY_CRITICAL: return "critical";
        default: return "normal";
    }
}

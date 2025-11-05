#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <vector>
#include "storage.h"

/**
 * @brief WebSocket Event Types
 */
enum WSEventType {
    WS_EVENT_MODULE_START,
    WS_EVENT_MODULE_COMPLETE,
    WS_EVENT_MODULE_FAILED,
    WS_EVENT_FILE_CREATED,
    WS_EVENT_PROGRESS_UPDATE,
    WS_EVENT_ALERT,
    WS_EVENT_STATISTICS,
    WS_EVENT_STATUS_CHANGE,
    WS_EVENT_LOG_MESSAGE,
    WS_EVENT_IOC_FOUND,
    WS_EVENT_CORRELATION_FOUND,
    WS_EVENT_THREAT_DETECTED,
    WS_EVENT_SCAN_COMPLETE
};

/**
 * @brief WebSocket Event Priority
 */
enum WSEventPriority {
    WS_PRIORITY_LOW = 1,
    WS_PRIORITY_NORMAL = 2,
    WS_PRIORITY_HIGH = 3,
    WS_PRIORITY_CRITICAL = 4
};

/**
 * @brief WebSocket Event
 */
struct WSEvent {
    WSEventType type;
    WSEventPriority priority;
    String title;
    String message;
    String details;
    unsigned long timestamp;
};

/**
 * @brief Client Connection Info
 */
struct ClientInfo {
    uint32_t id;
    String ip_address;
    unsigned long connect_time;
    uint32_t messages_sent;
    bool subscribed_alerts;
    bool subscribed_progress;
    bool subscribed_logs;
};

/**
 * @brief WebSocket Statistics
 */
struct WSStatistics {
    uint32_t total_clients_connected;
    uint32_t current_clients;
    uint32_t total_messages_sent;
    uint32_t total_events_broadcast;
    uint32_t failed_sends;
    unsigned long uptime_ms;
};

/**
 * @brief WebSocket Real-Time Server
 *
 * Provides real-time updates to web clients via WebSocket
 * Broadcasts forensic collection progress, alerts, IOCs, and statistics
 */
class WebSocketServer {
public:
    WebSocketServer(AsyncWebServer* server);
    ~WebSocketServer();

    // Initialization
    void begin();
    void end();

    // Event Broadcasting
    void broadcastModuleStart(const String& module_name);
    void broadcastModuleComplete(const String& module_name, bool success);
    void broadcastModuleFailed(const String& module_name, const String& error);
    void broadcastFileCreated(const String& file_path, uint32_t file_size);
    void broadcastProgressUpdate(uint8_t progress_percent, const String& current_task);
    void broadcastAlert(const String& alert_type, const String& message, const String& severity);
    void broadcastStatistics(const String& stats_json);
    void broadcastStatusChange(const String& old_status, const String& new_status);
    void broadcastLogMessage(const String& level, const String& message);
    void broadcastIOCFound(const String& ioc_type, const String& ioc_value);
    void broadcastCorrelationFound(const String& correlation_type, const String& description);
    void broadcastThreatDetected(const String& threat_name, const String& severity);
    void broadcastScanComplete(uint32_t artifacts_collected, uint32_t alerts_generated);

    // Generic event broadcast
    void broadcastEvent(const WSEvent& event);
    void broadcastJSON(const String& json);
    void broadcastToClient(uint32_t client_id, const String& message);

    // Client Management
    uint32_t getClientCount() const;
    std::vector<ClientInfo> getConnectedClients() const;
    void disconnectClient(uint32_t client_id);
    void disconnectAll();

    // Subscription Management
    void setClientSubscription(uint32_t client_id, const String& subscription_type, bool enabled);
    bool isClientSubscribed(uint32_t client_id, const String& subscription_type);

    // Statistics
    WSStatistics getStatistics() const;
    void resetStatistics();

    // Rate Limiting
    void setMaxMessagesPerSecond(uint16_t max_rate);
    void setMaxClientsAllowed(uint8_t max_clients);

    // Event Queue Management
    void enableEventQueue(bool enabled);
    void setEventQueueSize(uint16_t size);
    void flushEventQueue();

private:
    AsyncWebSocket* ws;
    AsyncWebServer* web_server;

    std::vector<ClientInfo> clients;
    std::vector<WSEvent> event_queue;

    // Configuration
    bool event_queue_enabled;
    uint16_t max_queue_size;
    uint16_t max_messages_per_second;
    uint8_t max_clients_allowed;

    // Statistics
    uint32_t total_clients_connected;
    uint32_t total_messages_sent;
    uint32_t total_events_broadcast;
    uint32_t failed_sends;
    unsigned long start_time;

    // Rate limiting
    unsigned long last_message_time;
    uint16_t messages_this_second;

    // Event handlers
    void handleWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                             AwsEventType type, void* arg, uint8_t* data, size_t len);
    static void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                AwsEventType type, void* arg, uint8_t* data, size_t len);

    void onConnect(AsyncWebSocketClient* client);
    void onDisconnect(AsyncWebSocketClient* client);
    void onMessage(AsyncWebSocketClient* client, void* arg, uint8_t* data, size_t len);
    void onError(AsyncWebSocketClient* client, void* arg, uint8_t* data, size_t len);

    // Message formatting
    String createEventJSON(const WSEvent& event);
    String createModuleStartJSON(const String& module_name);
    String createModuleCompleteJSON(const String& module_name, bool success);
    String createProgressJSON(uint8_t progress, const String& task);
    String createAlertJSON(const String& alert_type, const String& message, const String& severity);
    String createStatsJSON(const String& stats);

    // Helper methods
    ClientInfo* getClientInfo(uint32_t client_id);
    void addToQueue(const WSEvent& event);
    void processQueue();
    bool checkRateLimit();
    void updateClientInfo(uint32_t client_id);
    String getEventTypeName(WSEventType type);
    String getPriorityName(WSEventPriority priority);

    // Cleanup
    void cleanupDisconnectedClients();
};

// Global instance pointer for static callback
extern WebSocketServer* g_ws_server_instance;

#endif // WEBSOCKET_SERVER_H

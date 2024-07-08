#include "WebSocketsCommander.h"
#include <esp_task_wdt.h>

#define TAG "WebSocketsCommander"

WebSocketsCommander* WebSocketsCommander::instance = nullptr;

WebSocketsCommander::WebSocketsCommander(const char* ssid, const char* password, void (*onEvent)(JsonDocument& json), BaseType_t core)
    : ssid(ssid), password(password), onEvent(onEvent), core(core), server(7032), ws("/ws") {
    instance = this;
}

void WebSocketsCommander::init() {
    esp_log_level_set(TAG, ESP_LOG_INFO); // Set log level for this tag

    ESP_LOGI(TAG, "Initializing WebSocketsCommander with SSID: %s", ssid);

    WiFi.onEvent(WiFiEvent);

    WiFi.setHostname("EWCTRLMINI");
    WiFi.begin(ssid, password);
    ESP_LOGI(TAG, "Connecting to WiFi...");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        ESP_LOGI(TAG, ".");
    }
    ESP_LOGI(TAG, "\nWiFi connected");
    ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());

    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        onWebSocketEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&ws);

    server.begin();
    ESP_LOGI(TAG, "WebSocket server started");

    xTaskCreatePinnedToCore(
        WebSocketsCommander::listenForConnectionsTask, 
        "ListenTask", 
        4096, 
        this, 
        1, 
        NULL, 
        core
    );
    ESP_LOGI(TAG, "Listening task created and pinned to core %d", core);
}

void WebSocketsCommander::WiFiEvent(WiFiEvent_t event) {
    if (instance == nullptr) return;

    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            ESP_LOGI(TAG, "WiFi connected");
            ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            ESP_LOGI(TAG, "WiFi lost connection, attempting to reconnect...");
            WiFi.begin(instance->ssid, instance->password);
            break;
        default:
            break;
    }
}

void WebSocketsCommander::listenForConnectionsTask(void* pvParameters) {
    WebSocketsCommander* commander = static_cast<WebSocketsCommander*>(pvParameters);
    commander->listenForConnections();
}

void WebSocketsCommander::listenForConnections() {
    esp_task_wdt_add(NULL);
    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
            ESP_LOGI(TAG, "WiFi disconnected, waiting to reconnect...");
            delay(1000);
            continue;
        }
        ws.cleanupClients();
        esp_task_wdt_reset();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    esp_task_wdt_delete(NULL);
}

void WebSocketsCommander::handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    DynamicJsonDocument jsonDoc(1024);

    DeserializationError error = deserializeJson(jsonDoc, data, len);
    if (error) {
        ESP_LOGE(TAG, "deserializeJson() failed: %s", error.c_str());
        return;
    }

    if (!jsonDoc.containsKey("type") || !jsonDoc.containsKey("data")) {
        ESP_LOGE(TAG, "Invalid JSON format");
        return;
    }

    onEvent(jsonDoc);
}

void WebSocketsCommander::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            ESP_LOGI(TAG, "WebSocket client connected");
            break;
        case WS_EVT_DISCONNECT:
            ESP_LOGI(TAG, "WebSocket client disconnected");
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(client, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

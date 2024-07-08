#include "WebSocketsCommander.h"
#include <esp_task_wdt.h>

WebSocketsCommander* WebSocketsCommander::instance = nullptr;

WebSocketsCommander::WebSocketsCommander(const char* ssid, const char* password, void (*onEvent)(uint8_t type, uint16_t data), BaseType_t core)
    : ssid(ssid), password(password), onEvent(onEvent), core(core), server(80), ws("/ws") {
    instance = this;
}

void WebSocketsCommander::init() {
    Serial.begin(115200);
    Serial.println("Initializing WebSocketsCommander with SSID: " + String(ssid));

    WiFi.onEvent(WiFiEvent);

    WiFi.setHostname("EWCTRLMINI");
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.println("IP address: " + WiFi.localIP().toString());

    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        onWebSocketEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&ws);

    server.begin();
    Serial.println("WebSocket server started");

    xTaskCreatePinnedToCore(
        WebSocketsCommander::listenForConnectionsTask, 
        "ListenTask", 
        4096, 
        this, 
        1, 
        NULL, 
        core
    );
    Serial.println("Listening task created and pinned to core " + String(core));
}

void WebSocketsCommander::WiFiEvent(WiFiEvent_t event) {
    if (instance == nullptr) return;

    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.println("WiFi connected");
            Serial.println("IP address: " + WiFi.localIP().toString());
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("WiFi lost connection, attempting to reconnect...");
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
            Serial.println("WiFi disconnected, waiting to reconnect...");
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
    if (len < 4 || data[0] != 0x00) {
        return;
    }
    uint8_t type = data[1];
    uint16_t value = (data[2] << 8) | data[3];

    if (type == 0xFF) { // Heartbeat message
        ((AsyncWebSocketClient*)arg)->text("ACK");
    } else {
        onEvent(type, value);
    }
}

void WebSocketsCommander::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.println("WebSocket client connected");
            break;
        case WS_EVT_DISCONNECT:
            Serial.println("WebSocket client disconnected");
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(client, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

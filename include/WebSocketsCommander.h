#ifndef WEBSOCKETSCOMMANDER_H
#define WEBSOCKETSCOMMANDER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_log.h>

class WebSocketsCommander {
public:
    WebSocketsCommander(const char* ssid, const char* password, void (*onEvent)(uint8_t type, uint16_t data), BaseType_t core);
    void init();

private:
    const char* ssid;
    const char* password;
    void (*onEvent)(uint8_t type, uint16_t data);
    BaseType_t core;
    AsyncWebServer server;
    AsyncWebSocket ws;

    static void WiFiEvent(WiFiEvent_t event);
    static void listenForConnectionsTask(void* pvParameters);
    void listenForConnections();
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    static WebSocketsCommander* instance; // Singleton instance
};

#endif // WEBSOCKETSCOMMANDER_H

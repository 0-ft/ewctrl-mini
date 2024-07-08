#ifndef WEBSOCKETSCOMMANDER_H
#define WEBSOCKETSCOMMANDER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <esp_log.h>

class WebSocketsCommander {
public:
    WebSocketsCommander(const char* ssid, const char* password, void (*onEvent)(JsonDocument& json), BaseType_t core);
    void init();
    enum CommandTypes {
        COMMAND_SET_PATTERN = 0x01,
        COMMAND_SET_GAIN = 0x02,
        COMMAND_SET_FRAMERATE = 0x03,
        COMMAND_SET_PATTERNS = 0x04
    };

private:
    const char* ssid;
    const char* password;
    void (*onEvent)(JsonDocument& json);
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

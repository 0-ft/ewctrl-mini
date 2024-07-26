#ifndef WEBSOCKETSCOMMANDER_H
#define WEBSOCKETSCOMMANDER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <esp_log.h>
#include <vector>
#include <esp_task_wdt.h>
#include <numeric>

class WebSocketsCommander {
public:
    enum CommandTypes {
        COMMAND_SET_PATTERN = 0x01,
        COMMAND_SET_GAIN = 0x02,
        COMMAND_SET_FRAMERATE = 0x03,
        COMMAND_SET_PATTERNS = 0x04,
        COMMAND_ADD_PATTERN = 0x05,
        COMMAND_CLEAR_PATTERNS = 0x06,
    };
    WebSocketsCommander(const char* ssid, const char* password, void (*onEvent)(JsonDocument& json), BaseType_t core);
    void init();

private:
    const char* ssid;
    const char* password;
    void (*onEvent)(JsonDocument& json);
    BaseType_t core;
    AsyncWebServer server;
    AsyncWebSocket ws;
    char* messageBuffer;
    size_t messageBufferLength;
    
    static void WiFiEvent(WiFiEvent_t event);
    // static void listenForConnectionsTask(void* pvParameters);
    // void listenForConnections();
    uint8_t handleWebSocketMessage(AwsFrameInfo *info, uint8_t *data, size_t len);
    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    static WebSocketsCommander* instance; // Singleton instance
};

#endif // WEBSOCKETSCOMMANDER_H

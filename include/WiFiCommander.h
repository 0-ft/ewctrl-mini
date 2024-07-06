#ifndef WIFICOMMANDER_H
#define WIFICOMMANDER_H

#include <WiFi.h>
#include <esp_task_wdt.h>

#define WIFI_COMMANDER_PORT 7032

class WiFiCommander {
public:
    enum CommandTypes {
        COMMAND_SET_PATTERN = 0x01,
        COMMAND_SET_GAIN = 0x02,
        COMMAND_SET_FRAMERATE = 0x03,
    };
    
    // Constructor
    WiFiCommander(const char* ssid, const char* password, void (*onEvent)(uint8_t type, uint16_t data), BaseType_t core = 0);

    // Initializes the WiFiCommander
    void init();

    // Public enum for event types
    enum EventType {
        EVENT_TYPE_1 = 1,
        EVENT_TYPE_2 = 2,
        // Add other event types here
    };

private:
    const char* ssid;                  // SSID of the Wi-Fi network
    const char* password;              // Password of the Wi-Fi network
    WiFiServer server;                 // Wi-Fi server instance
    void (*onEvent)(uint8_t type, uint16_t data);  // Event callback function
    BaseType_t core;                   // Core to pin the task to

    // Singleton instance
    static WiFiCommander* instance;

    // Task function to listen for connections
    static void listenForConnectionsTask(void* pvParameters);

    // Handles listening for connections
    void listenForConnections();

    // Handles a connected client
    void handleClient(WiFiClient& client);

    // Reads an event from the client
    bool readEvent(WiFiClient& client, uint8_t& type, uint16_t& data);

    // WiFi event handler
    static void WiFiEvent(WiFiEvent_t event);
};

#endif // WIFICOMMANDER_H

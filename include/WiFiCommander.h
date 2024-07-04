#ifndef WIFICOMMANDER_H
#define WIFICOMMANDER_H

#include <WiFi.h>
#include <esp_task_wdt.h>

class WiFiCommander {
public:
    enum CommandTypes {
        COMMAND_SET_PATTERN = 0x01,
        COMMAND_SET_GAIN = 0x02,
        COMMAND_SET_FRAMERATE = 0x03,
    };

    // Constructor
    WiFiCommander(const char* ssid, void (*onCommand)(uint8_t type, uint16_t data), BaseType_t core = 0);

    // Initializes the WiFiCommander
    void init();

private:
    const char* ssid;                  // SSID of the Wi-Fi network
    WiFiServer server;                 // Wi-Fi server instance
    void (*onCommand)(uint8_t type, uint16_t data);  // Event callback function
    BaseType_t core;                   // Core to pin the task to

    // Task function to listen for connections
    static void listenForConnectionsTask(void* pvParameters);

    // Handles listening for connections
    void listenForConnections();

    // Handles a connected client
    void handleClient(WiFiClient& client);

    // Reads an event from the client
    bool readCommand(WiFiClient& client, uint8_t& type, uint16_t& data);

    // WiFi event handler
    static void WiFiEvent(WiFiEvent_t event);
};

#endif // WIFICOMMANDER_H

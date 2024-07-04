#include "WiFiCommander.h"
#include <WiFi.h>

static const char* TAG = "WiFiCommander";

// Static IP configuration
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Constructor implementation
WiFiCommander::WiFiCommander(const char* ssid, void (*onCommand)(uint8_t type, uint16_t data), BaseType_t core)
    : ssid(ssid), onCommand(onCommand), server(80), core(core) {}

// Initializes the WiFiCommander
void WiFiCommander::init() {
    ESP_LOGI(TAG, "Initializing WiFiCommander with SSID: %s", ssid);
    
    WiFi.onEvent(WiFiEvent);
    
    WiFi.softAPConfig(local_IP, gateway, subnet);  // Configure the static IP address
    WiFi.softAP(ssid);
    ESP_LOGI(TAG, "WiFi Access Point started");
    ESP_LOGI(TAG, "Hotspot IP: %s", WiFi.softAPIP().toString());
    
    xTaskCreatePinnedToCore(
        WiFiCommander::listenForConnectionsTask, // Task function
        "ListenTask",                            // Task name
        4096,                                    // Stack size
        this,                                    // Task parameter
        1,                                       // Task priority
        NULL,                                    // Task handle
        core                                     // Core to pin the task to
    );
    ESP_LOGI(TAG, "Listening task created and pinned to core %d", core);
}

// WiFi event handler
void WiFiCommander::WiFiEvent(WiFiEvent_t event) {
    switch(event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            ESP_LOGI(TAG, "Device connected to the hotspot");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "Device disconnected from the hotspot");
            break;
        default:
            break;
    }
}

// Task function to listen for connections
void WiFiCommander::listenForConnectionsTask(void* pvParameters) {
    WiFiCommander* commander = static_cast<WiFiCommander*>(pvParameters);
    commander->listenForConnections();
}

// Handles listening for connections
void WiFiCommander::listenForConnections() {
    esp_task_wdt_add(NULL);  // Add current task to watchdog timer
    server.begin();
    ESP_LOGI(TAG, "Server started, waiting for connections...");
    while (true) {
        WiFiClient client = server.available();
        if (client) {
            ESP_LOGI(TAG, "Client connected to the server");
            handleClient(client);
            ESP_LOGI(TAG, "Client disconnected from the server");
        }
        esp_task_wdt_reset();  // Reset watchdog timer
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay to yield to other tasks
    }
    esp_task_wdt_delete(NULL);  // Remove task from watchdog timer (though this will never be reached)
}

// Handles a connected client
void WiFiCommander::handleClient(WiFiClient& client) {
    while (client.connected()) {
        if (client.available()) {
            uint8_t type;
            uint16_t data;
            if (readCommand(client, type, data)) {
                ESP_LOGI(TAG, "Event received: type=%d, data=%d", type, data);
                onCommand(type, data);
            }
        }
        esp_task_wdt_reset();  // Reset watchdog timer
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay to yield to other tasks
    }
    client.stop();
}

// Reads an event from the client
bool WiFiCommander::readCommand(WiFiClient& client, uint8_t& type, uint16_t& data) {
    bool receiving = false;
    uint8_t buffer[3];
    uint8_t index = 0;

    // Read until start marker (0x00) is found
    while (client.available()) {
        uint8_t byte = client.read();

        if (!receiving && byte == 0x00) {
            receiving = true;
            index = 0;
            continue;
        }

        if (receiving) {
            buffer[index++] = byte;
            if (index == 3) {  // Expecting exactly 3 bytes after start marker
                type = buffer[0];
                data = (buffer[1] << 8) | buffer[2];
                return true;
            }
        }
    }
    return false;
}
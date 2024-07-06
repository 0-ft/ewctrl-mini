#include "WiFiCommander.h"

// Singleton instance
WiFiCommander* WiFiCommander::instance = nullptr;

// Constructor implementation
WiFiCommander::WiFiCommander(const char* ssid, const char* password, void (*onEvent)(uint8_t type, uint16_t data), BaseType_t core)
    : ssid(ssid), password(password), onEvent(onEvent), server(WIFI_COMMANDER_PORT), core(core) {
    instance = this;
}

// Initializes the WiFiCommander
void WiFiCommander::init() {
    Serial.begin(115200);
    Serial.println("Initializing WiFiCommander with SSID: " + String(ssid));

    WiFi.onEvent(WiFiEvent);

    WiFi.setHostname("EWCTRLMINI");
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");

    // Attempt to connect to Wi-Fi in a loop
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.println("IP address: " + WiFi.localIP().toString());

    xTaskCreatePinnedToCore(
        WiFiCommander::listenForConnectionsTask, // Task function
        "ListenTask",                            // Task name
        4096,                                    // Stack size
        this,                                    // Task parameter
        1,                                       // Task priority
        NULL,                                    // Task handle
        core                                     // Core to pin the task to
    );
    Serial.println("Listening task created and pinned to core " + String(core));
}

// WiFi event handler
void WiFiCommander::WiFiEvent(WiFiEvent_t event) {
    if (instance == nullptr) return; // Ensure instance is valid

    switch(event) {
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

// Task function to listen for connections
void WiFiCommander::listenForConnectionsTask(void* pvParameters) {
    WiFiCommander* commander = static_cast<WiFiCommander*>(pvParameters);
    commander->listenForConnections();
}

// Handles listening for connections
void WiFiCommander::listenForConnections() {
    esp_task_wdt_add(NULL);  // Add current task to watchdog timer
    server.begin();
    Serial.println("Server started, waiting for connections...");
    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected, waiting to reconnect...");
            delay(1000);
            continue;  // Skip this iteration if Wi-Fi is not connected
        }

        WiFiClient client = server.available();
        if (client) {
            Serial.println("Client connected to the server");
            handleClient(client);
            Serial.println("Client disconnected from the server");
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
            if (readEvent(client, type, data)) {
                Serial.println("Event received: Type=" + String(type) + ", Data=" + String(data));
                onEvent(type, data);
            }
        }
        esp_task_wdt_reset();  // Reset watchdog timer
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Small delay to yield to other tasks
    }
    client.stop();
}

// Reads an event from the client
bool WiFiCommander::readEvent(WiFiClient& client, uint8_t& type, uint16_t& data) {
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

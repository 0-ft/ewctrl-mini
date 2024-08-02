#include "WebSocketsCommander.h"

static const char *TAG = "WebSocketsCommander";

WebSocketsCommander *WebSocketsCommander::instance = nullptr;

WebSocketsCommander::WebSocketsCommander(const char *ssid, const char *password, bool (*onEvent)(JsonDocument &json), BaseType_t core)
    : ssid(ssid), password(password), onEvent(onEvent), core(core), server(7032), ws("/ws")
{
    instance = this;
}

void WebSocketsCommander::init()
{
    ESP_LOGI(TAG, "Initializing WebSocketsCommander with SSID: %s", ssid);

    WiFi.onEvent(WiFiEvent);

    WiFi.setHostname("EWCTRLMINI");
    WiFi.begin(ssid, password);
    ESP_LOGI(TAG, "Connecting to WiFi...");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        ESP_LOGI(TAG, ".");
    }
    ESP_LOGI(TAG, "\nWiFi connected");
    ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());

    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
               { onWebSocketEvent(server, client, type, arg, data, len); });
    server.addHandler(&ws);

    server.begin();
    ESP_LOGI(TAG, "WebSocket server started");

    // xTaskCreatePinnedToCore(
    //     WebSocketsCommander::listenForConnectionsTask,
    //     "ListenTask",
    //     4096,
    //     this,
    //     configMAX_PRIORITIES - 1,
    //     NULL,
    //     core);
    // ESP_LOGI(TAG, "Listening task created and pinned to core %d", core);
}

void WebSocketsCommander::WiFiEvent(WiFiEvent_t event)
{
    if (instance == nullptr)
        return;

    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        ESP_LOGI(TAG, "WiFi connected");
        ESP_LOGE(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        ESP_LOGI(TAG, "WiFi lost connection, attempting to reconnect...");
        WiFi.begin(instance->ssid, instance->password);
        break;
    default:
        break;
    }
}

// void WebSocketsCommander::listenForConnectionsTask(void *pvParameters)
// {
//     WebSocketsCommander *commander = static_cast<WebSocketsCommander *>(pvParameters);
//     commander->listenForConnections();
// }

// void WebSocketsCommander::listenForConnections()
// {
//     esp_task_wdt_add(NULL);
//     while (true)
//     {
//         if (WiFi.status() != WL_CONNECTED)
//         {
//             ESP_LOGI(TAG, "WiFi disconnected, waiting to reconnect...");
//             delay(1000);
//             continue;
//         }
//         ws.cleanupClients();
//         esp_task_wdt_reset();
//         vTaskDelay(1 / portTICK_PERIOD_MS);
//     }
//     esp_task_wdt_delete(NULL);
// }

uint8_t WebSocketsCommander::handleWebSocketMessage(AwsFrameInfo *info, uint8_t *data, size_t len)
{
    if (info->index == 0)
    {
        if (messageBuffer != nullptr)
        {
            delete[] messageBuffer;
        }
        messageBufferLength = info->len + 1024;
        messageBuffer = new char[messageBufferLength + 1];
        memset(messageBuffer, 0, messageBufferLength + 1);
    }

    // check if message buffer is null or message too long
    if (messageBuffer == nullptr || info->index + len > messageBufferLength)
    {
        ESP_LOGE(TAG, "Message buffer is null or message too long");
        return 1;
    }

    memcpy(messageBuffer + info->index, data, len);

    if ((info->index + len) == info->len && info->final)
    {
        ESP_LOGI(TAG, "Received complete message: %s", messageBuffer);
        // check if message is empty
        if (strlen(messageBuffer) == 0)
        {
            ESP_LOGE(TAG, "Message is empty");
            delete[] messageBuffer;
            messageBuffer = nullptr;
            return 1;
        }
        JsonDocument jsonDoc;
        DeserializationError error = deserializeJson(jsonDoc, messageBuffer);
        if (error)
        {
            ESP_LOGE(TAG, "deserializeJson() failed: %s", error.c_str());
            delete[] messageBuffer;
            messageBuffer = nullptr;
            return 1;
        }

        if (!jsonDoc.containsKey("type") || !jsonDoc.containsKey("data"))
        {
            ESP_LOGE(TAG, "Invalid JSON format");
            delete[] messageBuffer;
            messageBuffer = nullptr;
            return 1;
        }

        ESP_LOGI(TAG, "Finished deserialising, calling onEvent");
        auto shouldAck = onEvent(jsonDoc);
        // ESP_LOGE(TAG, "DELETING MESSAGE BUFFER");
        delete[] messageBuffer;
        messageBuffer = nullptr;
        return shouldAck ? 0 : 1;
    }
    return 2;
}

void WebSocketsCommander::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        ESP_LOGI(TAG, "WebSocket client connected");
        break;
    case WS_EVT_DISCONNECT:
        ESP_LOGI(TAG, "WebSocket client disconnected");
        break;
    case WS_EVT_DATA:
    {
        auto result = handleWebSocketMessage((AwsFrameInfo *)arg, data, len);
        // if(result == 0) {
        //     ESP_LOGE(TAG, "Sending ACK");
        //     client->text("ACK");
        // }
        break;
    }
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}
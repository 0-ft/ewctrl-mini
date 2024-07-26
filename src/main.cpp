#include <Arduino.h>
#include <FaderPlayback.h>
#include <WebSocketsCommander.h>
#include <BezierJsonParser.h>

static const char *TAG = "Main";

FaderPlayback faderPlayback(60, 2, new uint8_t[2]{0x40, 0x41}, std::vector<uint16_t>(16, 4096));

void receivePattern(const JsonObject &doc) {
  ESP_LOGI(TAG, "Received pattern");
  auto [patternName, pattern] = parseJsonToBezierPattern(doc);
  faderPlayback.addPattern(patternName, pattern);
}

void handleWifiCommand(JsonDocument& doc)
{
  // ESP_LOGI(TAG, "Handling WifiCommander command");
  uint8_t type = doc["type"];
  switch(type) {
    case WebSocketsCommander::COMMAND_SET_PATTERN:
      faderPlayback.goToPattern(doc["data"]);
      break;
    case WebSocketsCommander::COMMAND_SET_GAIN:
      faderPlayback.setGain(doc["data"]);
      break;
    case WebSocketsCommander::COMMAND_SET_FRAMERATE:
      faderPlayback.frameRate = doc["data"];
      break;
    case WebSocketsCommander::COMMAND_SET_PATTERNS:
    {
      auto patterns = parseJsonToBezierPatterns(doc["data"]);
      faderPlayback.setPatterns(patterns);
      break;
    }
    case WebSocketsCommander::COMMAND_ADD_PATTERN:
    {
      JsonObject data = doc["data"];
      receivePattern(data);
      break;
    }
    case WebSocketsCommander::COMMAND_CLEAR_PATTERNS:
    {
      faderPlayback.setPatterns({});
      break;
    }
    default:
      ESP_LOGW(TAG, "Unknown event type");
      break;
  }
  ESP_LOGI(TAG, "Handled WifiCommander command type %d", type);
}

// void handlePostCommand(JsonDocument& doc)
// {
//   // ESP_LOGI(TAG, "Handling WifiCommander post");
//   uint8_t type = doc["type"];
//   switch(type) {
//     case WebSocketsCommander::COMMAND_SET_PATTERNS:
//     {
//       auto patterns = parseJsonToBezierPatterns(doc["data"]);
//       faderPlayback.setPatterns(patterns);
//       break;
//     }
//     default:
//       ESP_LOGW(TAG, "bad POST with event type %d", type);
//       break;
//   }
//   ESP_LOGI(TAG, "Handled WifiCommander post type %d", type);
// }


// WebSocketsCommander wifiCommander("COMMANDER", "fadercommand", handleWifiCommand, 0);
// WiFiCommander wifiCommander("Queens", "trlguest021275", handleWifiCommand);
// WebSocketsCommander wifiCommander("Queens", "trlguest021275", handleWifiCommand, 0);
WebSocketsCommander wifiCommander("190bpm hardcore steppas", "fungible", handleWifiCommand, 0);
// WiFiCommander wifiCommander("190bpm hardcore steppas", "fungible", handleWifiCommand);

void setup()
{
  ESP_LOGI(TAG, "Setting up");

  faderPlayback.setup();
  faderPlayback.goToPattern("test");
  faderPlayback.setGain(4095);

  wifiCommander.init();
}

void loop()
{
  faderPlayback.sendFrame();
  vTaskDelay(1 / portTICK_PERIOD_MS);
  // delayMicroseconds(300);
}

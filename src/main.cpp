#include <Arduino.h>
#include <FaderPlayback.h>
#include <WebSocketsCommander.h>
#include <BezierJsonParser.h>

static const char *TAG = "Main";

FaderPlayback faderPlayback(0, {}, std::vector<uint16_t>(OUTPUTS_COUNT, 0));

void receivePattern(const JsonObject &doc) {
  ESP_LOGI(TAG, "Received pattern");
  auto [patternName, pattern] = parseJsonToBezierPattern(doc);
  faderPlayback.addPattern(patternName, pattern);
}

bool handleWifiCommand(JsonDocument& doc)
{
  // ESP_LOGI(TAG, "Handling WifiCommander command");
  uint8_t type = doc["type"];
  switch(type) {
    case FaderPlayback::COMMAND_START_PATTERN:
      faderPlayback.setPaused(false);
      faderPlayback.startPattern(doc["data"]["name"], doc["data"]["loop"]);
      break;
    case FaderPlayback::COMMAND_STOP_PATTERN:
      faderPlayback.stopPattern(doc["data"]["name"]);
      break;
    case FaderPlayback::COMMAND_SET_GAIN:
      faderPlayback.setGain(doc["data"]);
      break;
    case FaderPlayback::COMMAND_SET_SPEED:
    {
      auto rawSpeed = doc["data"]["speed"];
      // check if it's a number
      if (rawSpeed.is<float>() || rawSpeed.is<int>()) {
        faderPlayback.setSpeed(rawSpeed);
      } else if(rawSpeed == "+") {
        faderPlayback.setSpeed(faderPlayback.getSpeed() * 1.05);
      } else if(rawSpeed == "-") {
        faderPlayback.setSpeed(faderPlayback.getSpeed() * 0.95);
      }
      break;
    }
    case FaderPlayback::COMMAND_SET_PATTERNS:
    {
      auto patterns = parseJsonToBezierPatterns(doc["data"]);
      faderPlayback.setPatterns(patterns);
      break;
    }
    case FaderPlayback::COMMAND_ADD_PATTERN:
    {
      JsonObject data = doc["data"];
      receivePattern(data);
      return true;
      break;
    }
    case FaderPlayback::COMMAND_CLEAR_PATTERNS:
    {
      faderPlayback.setPatterns({});
      break;
    }
    case FaderPlayback::COMMAND_SET_MULTIPLIER:
    {
      std::vector<uint16_t> multiplier;
      for (JsonVariant value : doc["data"].as<JsonArray>()) {
        multiplier.push_back(value);
      }
      faderPlayback.setMultiplier(multiplier);
      break;
    }
    case FaderPlayback::COMMAND_STOP_ALL:
      faderPlayback.stopAll();
      break;
    case FaderPlayback::COMMAND_SET_PAUSED:
      faderPlayback.setPaused(doc["data"]["paused"]);
      break;
    default:
      ESP_LOGW(TAG, "Unknown event type");
      break;
  }
  ESP_LOGI(TAG, "Handled WifiCommander command type %d", type);
  return false;
}


// WebSocketsCommander wifiCommander("COMMANDER", "fadercommand", handleWifiCommand, 0);
// WiFiCommander wifiCommander("Queens", "trlguest021275", handleWifiCommand);
// WebSocketsCommander wifiCommander("Queens", "trlguest021275", handleWifiCommand, 0);
WebSocketsCommander wifiCommander("190bpm hardcore steppas", "fungible", handleWifiCommand, 0);
// WebSocketsCommander wifiCommander("TP-LINK_2C5EE8", "85394919", handleWifiCommand, 0);
// WiFiCommander wifiCommander("190bpm hardcore steppas", "fungible", handleWifiCommand);

void sendFrameCallback(void *arg) {
  faderPlayback.sendFrame();
}

void setup()
{
  ESP_LOGI(TAG, "Setting up");

  faderPlayback.setup();
  faderPlayback.startPattern("test");
  faderPlayback.setGain(4095);

  faderPlayback.testSequence();
  faderPlayback.setPaused(false);
  wifiCommander.init();


  esp_timer_create_args_t timerArgs;
  timerArgs.callback = &sendFrameCallback;
  timerArgs.arg = NULL;
  timerArgs.dispatch_method = ESP_TIMER_TASK;
  timerArgs.name = "frameTimer";
    
  esp_timer_handle_t myTimer;
  esp_timer_create(&timerArgs, &myTimer);

    // Start the timer (200 times per second -> 5ms interval)
  esp_timer_start_periodic(myTimer, 5000); // interval is in microseconds
}

void loop()
{
  // faderPlayback.sendFrame();
  // vTaskDelay(1 / portTICK_PERIOD_MS);
  // delayMicroseconds(300);
}

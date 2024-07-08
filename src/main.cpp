#include <Arduino.h>
#include <ControlPad.h>
#include <FaderPlayback.h>
#include <FaderPatterns.h>
#include <WiFiCommander.h>
#include <WebSocketsCommander.h>

static const char *TAG = "Main";

FaderPlayback faderPlayback(30, 2, new uint8_t[2]{0x40, 0x41});

void handleWifiCommand(uint8_t type, uint16_t data)
{
  // ESP_LOGI(TAG, "Handling WifiCommander command type %d, data %d", type, data);
  switch(type) {
    case WiFiCommander::COMMAND_SET_PATTERN:
      faderPlayback.goToPattern(data);
      break;
    case WiFiCommander::COMMAND_SET_GAIN:
      faderPlayback.setGain(data);
      break;
    case WiFiCommander::COMMAND_SET_FRAMERATE:
      faderPlayback.frameRate = data;
      break;
    default:
      ESP_LOGW(TAG, "Unknown event type");
      break;
  }
}


// WiFiCommander wifiCommander("COMMANDER", "fadercommand", handleWifiCommand);
// WiFiCommander wifiCommander("Queens", "trlguest021275", handleWifiCommand);
WebSocketsCommander wifiCommander("Queens", "trlguest021275", handleWifiCommand);
// WiFiCommander wifiCommander("190bpm hardcore steppas", "fungible", handleWifiCommand);

void setup()
{
  ESP_LOGI(TAG, "Setting up");

  wifiCommander.init();

  faderPlayback.setup();
  faderPlayback.goToPattern(0);
  faderPlayback.setGain(4095);
}

void loop()
{
  faderPlayback.sendFrame();
  delayMicroseconds(300);
}

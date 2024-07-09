// #include <Arduino.h>
// #include <BezierEnvelope.h>
// #include <BezierPattern.h>
// #include <BezierJsonParser.h>

// static const char *TAG = "Main";

// void setup()
// {
//     ESP_LOGI(TAG, "Setting up");
//     // std::vector<FloatEvent> events = {
//     //     { -63072000, 0, 0, 0, 0, 0, false },
//     //     { 60, 0, 0, 0, 0, 0, false },
//     //     { 60, 1, 0.187709427053866423, 0.448497744974886869, 0.522837743941625854, 0.783283187880314924, true },
//     //     { 60.175690975690976, 0, 0, 0, 0, 0, false },
//     //     { 60.25, 0, 0, 0, 0, 0, false },
//     //     { 60.25, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
//     //     { 60.425690975690976, 0, 0, 0, 0, 0, false },
//     //     { 60.5, 0, 0, 0, 0, 0, false },
//     //     { 60.5, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
//     //     { 60.675690975690976, 0, 0, 0, 0, 0, false },
//     //     { 60.75, 0, 0, 0, 0, 0, false },
//     //     { 60.75, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
//     //     { 60.925690975690976, 0, 0, 0, 0, 0, false },
//     //     { 61, 0, 0, 0, 0, 0, false },
//     //     { 61, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
//     //     { 61.175690975690976, 0, 0, 0, 0, 0, false },
//     //     { 61.25, 0, 0, 0, 0, 0, false },
//     //     { 61.25, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
//     //     { 61.425690975690976, 0, 0, 0, 0, 0, false },
//     //     { 61.5, 0, 0, 0, 0, 0, false },
//     //     { 61.5, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
//     //     { 61.675690975690976, 0, 0, 0, 0, 0, false },
//     //     { 61.75, 0, 0, 0, 0, 0, false },
//     //     { 61.75, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
//     //     { 61.925690975690976, 0, 0, 0, 0, 0, false },
//     //     { 62, 0, 0, 0, 0, 0, false }
//     // };

//     // BezierEnvelope envelope(events);

//     String jsonString = R"(
    // [
    //     [
    //         [
    //             {
    //                 "Time": 0.0,
    //                 "Value": 0.0
    //             },
    //             {
    //                 "Time": 1.0,
    //                 "Value": 1.0,
    //                 "CurveControl1X": 0.18770942705386642,
    //                 "CurveControl1Y": 0.44849774497488687,
    //                 "CurveControl2X": 0.5228377439416259,
    //                 "CurveControl2Y": 0.7832831878803149
    //             },
    //             {
    //                 "Time": 2.0,
    //                 "Value": 0.0,
    //                 "CurveControl1X": 0.18770942705386642,
    //                 "CurveControl1Y": 0.44849774497488687,
    //                 "CurveControl2X": 0.5228377439416259,
    //                 "CurveControl2Y": 0.7832831878803149
    //             }
    //         ]
    //     ]
    // ]
//     )";

//     const std::vector<BezierPattern> patterns = parseJsonToBezierPatterns(jsonString); // std::vector<BezierSegment> bezierSegments = loadEnvelope(events);

//     const auto envelope = patterns[0].envelopes[0];
//     for (double queryTime = 0; queryTime < 2; queryTime += 0.005)
//     {
//         float result = envelope.sampleAtTime(queryTime);
//         int resultInt = (int)(result * 50);
//         char spaces[resultInt + 1];
//         for (int i = 0; i < resultInt; i++)
//         {
//             spaces[i] = ' ';
//         }
//         spaces[resultInt] = '\0';
//         ESP_LOGI(TAG, "%f |%s.|", queryTime, spaces);
//     }

//     // double queryTime = 60.5;
//     // float result = sampleBezierCurve(events, queryTime);
//     // ESP_LOGI(TAG, "Result at time %f: %f", queryTime, result);
// }

// void loop()
// {
// }

#include <Arduino.h>
#include <ControlPad.h>
#include <FaderPlayback.h>
#include <FaderPatterns.h>
#include <WiFiCommander.h>
#include <WebSocketsCommander.h>
#include <BezierJsonParser.h>

static const char *TAG = "Main";

FaderPlayback faderPlayback(60, 2, new uint8_t[2]{0x40, 0x41});

// void handleWifiCommand(uint8_t type, uint16_t data)
// {
//   // ESP_LOGI(TAG, "Handling WifiCommander command type %d, data %d", type, data);
//   switch(type) {
//     case WiFiCommander::COMMAND_SET_PATTERN:
//       faderPlayback.goToPattern(data);
//       break;
//     case WiFiCommander::COMMAND_SET_GAIN:
//       faderPlayback.setGain(data);
//       break;
//     case WiFiCommander::COMMAND_SET_FRAMERATE:
//       faderPlayback.frameRate = data;
//       break;
//     default:
//       ESP_LOGW(TAG, "Unknown event type");
//       break;
//   }
// }

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


WebSocketsCommander wifiCommander("COMMANDER", "fadercommand", handleWifiCommand, 0);
// WiFiCommander wifiCommander("Queens", "trlguest021275", handleWifiCommand);
// WebSocketsCommander wifiCommander("Queens", "trlguest021275", handleWifiCommand, 0);
// WebSocketsCommander wifiCommander("190bpm hardcore steppas", "fungible", handleWifiCommand, 0);
// WiFiCommander wifiCommander("190bpm hardcore steppas", "fungible", handleWifiCommand);

void setup()
{
  ESP_LOGI(TAG, "Setting up");

  wifiCommander.init();

  faderPlayback.setup();
  faderPlayback.goToPattern("test");
  faderPlayback.setGain(4095);
}

void loop()
{
  faderPlayback.sendFrame();
  vTaskDelay(1 / portTICK_PERIOD_MS);
  // delayMicroseconds(300);
}

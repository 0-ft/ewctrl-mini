#include <Arduino.h>
#include <BezierEnvelope.h>

static const char *TAG = "Main";

// struct FloatEvent {
//     double Time;
//     float Value;
//     double CurveControl1X;
//     double CurveControl1Y;
//     double CurveControl2X;
//     double CurveControl2Y;
//     bool HasCurveControls;
// };

// struct BezierSegment {
//     double StartTime;
//     double EndTime;
//     bezier::Bezier<3> BezierCurve;
// };

// std::vector<BezierSegment> loadEnvelope(const std::vector<FloatEvent>& events) {
//     std::vector<BezierSegment> bezierSegments;

//     for (size_t i = 0; i < events.size() - 1; ++i) {
//         const FloatEvent& startEvent = events[i];
//         const FloatEvent& endEvent = events[i + 1];

//         if(startEvent.Time == endEvent.Time) {
//             continue;
//         }

//         std::vector<bezier::Point> controlPoints;
//         if (startEvent.HasCurveControls) {
//             controlPoints = {
//                 bezier::Point(startEvent.Time, startEvent.Value),
//                 bezier::Point(startEvent.Time + (endEvent.Time - startEvent.Time) * startEvent.CurveControl1X, startEvent.Value + (endEvent.Value - startEvent.Value) * startEvent.CurveControl1Y),
//                 bezier::Point(startEvent.Time + (endEvent.Time - startEvent.Time) * startEvent.CurveControl2X, startEvent.Value + (endEvent.Value - startEvent.Value) * startEvent.CurveControl2Y),
//                 bezier::Point(endEvent.Time, endEvent.Value)
//             };
//         } else {
//             // For linear interpolation, create a cubic Bezier curve with collinear control points
//             controlPoints = {
//                 bezier::Point(startEvent.Time, startEvent.Value),
//                 bezier::Point((2 * startEvent.Time + endEvent.Time) / 3, (2 * startEvent.Value + endEvent.Value) / 3),
//                 bezier::Point((startEvent.Time + 2 * endEvent.Time) / 3, (startEvent.Value + 2 * endEvent.Value) / 3),
//                 bezier::Point(endEvent.Time, endEvent.Value)
//             };
//         }

//         // log all control points
//         ESP_LOGI(TAG, "SEG");
//         for (auto& point : controlPoints) {
//             ESP_LOGI(TAG, "Control point: %f, %f", point.x, point.y);
//         }
//         bezier::Bezier<3> bezierCurve(controlPoints);
//         bezierSegments.push_back({startEvent.Time, endEvent.Time, bezierCurve});
//     }

//     return bezierSegments;
// }

// double sampleEnvelope(const std::vector<BezierSegment>& bezierSegments, double time) {
//     for (const auto& segment : bezierSegments) {
//         if (segment.StartTime <= time && time <= segment.EndTime) {
//             // ESP_LOGI(TAG, "t %f, Segment start time: %f, end time: %f", time, segment.StartTime, segment.EndTime);
//             double t = (time - segment.StartTime) / (segment.EndTime - segment.StartTime);
//             return segment.BezierCurve.valueAt(t).y;
//         }
//     }

//     // If the time is not within any segment, return NaN
//     ESP_LOGE(TAG, "Time %f is not within any segment", time);
//     return std::numeric_limits<double>::quiet_NaN();
// }

void setup()
{
  ESP_LOGI(TAG, "Setting up");
std::vector<FloatEvent> events = {
        { -63072000, 0, 0, 0, 0, 0, false },
        { 60, 0, 0, 0, 0, 0, false },
        { 60, 1, 0.187709427053866423, 0.448497744974886869, 0.522837743941625854, 0.783283187880314924, true },
        { 60.175690975690976, 0, 0, 0, 0, 0, false },
        { 60.25, 0, 0, 0, 0, 0, false },
        { 60.25, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
        { 60.425690975690976, 0, 0, 0, 0, 0, false },
        { 60.5, 0, 0, 0, 0, 0, false },
        { 60.5, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
        { 60.675690975690976, 0, 0, 0, 0, 0, false },
        { 60.75, 0, 0, 0, 0, 0, false },
        { 60.75, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
        { 60.925690975690976, 0, 0, 0, 0, 0, false },
        { 61, 0, 0, 0, 0, 0, false },
        { 61, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
        { 61.175690975690976, 0, 0, 0, 0, 0, false },
        { 61.25, 0, 0, 0, 0, 0, false },
        { 61.25, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
        { 61.425690975690976, 0, 0, 0, 0, 0, false },
        { 61.5, 0, 0, 0, 0, 0, false },
        { 61.5, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
        { 61.675690975690976, 0, 0, 0, 0, 0, false },
        { 61.75, 0, 0, 0, 0, 0, false },
        { 61.75, 1, 0.187709427053878941, 0.448497744974886814, 0.522837743941613975, 0.783283187880314924, true },
        { 61.925690975690976, 0, 0, 0, 0, 0, false },
        { 62, 0, 0, 0, 0, 0, false }
    };

    BezierEnvelope envelope(events);
    // std::vector<BezierSegment> bezierSegments = loadEnvelope(events);

    for(double queryTime = 60; queryTime < 62; queryTime += 0.005) {
        float result = envelope.sampleAtTime(queryTime);
        int resultInt = (int)(result * 50);
        char spaces[resultInt + 1];
        for (int i = 0; i < resultInt; i++) {
            spaces[i] = ' ';
        }
        spaces[resultInt] = '\0';
        ESP_LOGI(TAG, "%f |%s.|", queryTime, spaces);
    }

    // double queryTime = 60.5;
    // float result = sampleBezierCurve(events, queryTime);
    // ESP_LOGI(TAG, "Result at time %f: %f", queryTime, result);
}

void loop()
{
}



// #include <Arduino.h>
// #include <ControlPad.h>
// #include <FaderPlayback.h>
// #include <FaderPatterns.h>
// #include <WiFiCommander.h>
// #include <WebSocketsCommander.h>

// static const char *TAG = "Main";

// FaderPlayback faderPlayback(60, 2, new uint8_t[2]{0x40, 0x41});

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


// // WiFiCommander wifiCommander("COMMANDER", "fadercommand", handleWifiCommand);
// // WiFiCommander wifiCommander("Queens", "trlguest021275", handleWifiCommand);
// WebSocketsCommander wifiCommander("Queens", "trlguest021275", handleWifiCommand, 0);
// // WiFiCommander wifiCommander("190bpm hardcore steppas", "fungible", handleWifiCommand);

// void setup()
// {
//   ESP_LOGI(TAG, "Setting up");

//   wifiCommander.init();

//   faderPlayback.setup();
//   faderPlayback.goToPattern(0);
//   faderPlayback.setGain(4095);
// }

// void loop()
// {
//   faderPlayback.sendFrame();
//   delayMicroseconds(300);
// }

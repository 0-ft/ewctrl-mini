#include "BezierJsonParser.h"
#include <esp_log.h>

static const char *TAG = "BezierJsonParser";

std::vector<BezierPattern> parseJsonToBezierPatterns(const JsonArray &doc) {
    ESP_LOGI(TAG, "Parsing JSON to BezierPatterns");
    std::vector<BezierPattern> patterns;

    // // Allocate a temporary JsonDocument
    // StaticJsonDocument<2000> doc;

    // // Deserialize the JSON document
    // DeserializationError error = deserializeJson(doc, jsonString);
    // if (error) {
    //     Serial.print(F("deserializeJson() failed: "));
    //     Serial.println(error.f_str());
    //     return patterns;
    // }

    // Parse the JSON array
    for (JsonArray envelopeArray : doc) {
        std::vector<BezierEnvelope> envelopes;

        for (JsonArray eventArray : envelopeArray) {
            std::vector<FloatEvent> events;

            for (JsonArray eventJson : eventArray) {
                FloatEvent event;
                event.Time = eventJson[0].as<double>();
                event.Value = eventJson[1].as<float>();

                if (eventJson.size() == 6) {
                    event.CurveControl1X = eventJson[2].as<double>();
                    event.CurveControl1Y = eventJson[3].as<double>();
                    event.CurveControl2X = eventJson[4].as<double>();
                    event.CurveControl2Y = eventJson[5].as<double>();
                    event.HasCurveControls = true;
                } else {
                    event.HasCurveControls = false;
                }

                events.push_back(event);
            }

            envelopes.push_back(BezierEnvelope(events));
        }

        patterns.push_back(BezierPattern(envelopes));
    }

    ESP_LOGI(TAG, "Parsed %d patterns", patterns.size());

    return patterns;
}

#include "BezierJsonParser.h"

static const char* TAG = "BezierJsonParser";

std::pair<std::string, BezierPattern> parseJsonToBezierPattern(const JsonObject &patternObj) {
        if (!patternObj.containsKey("name") || !patternObj.containsKey("data")) {
            ESP_LOGE(TAG, "Pattern object missing required keys");
            return {"", BezierPattern(std::vector<BezierEnvelope>())};
        }

        String patternName = patternObj["name"].as<String>();
        JsonArray envelopeArray = patternObj["data"].as<JsonArray>();

        std::vector<BezierEnvelope> envelopes;

        for (JsonArray eventArray : envelopeArray) {
            std::vector<FloatEvent> events;

            for (JsonArray eventJson : eventArray) {
                if (eventJson.size() < 2) {
                    ESP_LOGE(TAG, "Event JSON array does not have the minimum 2 elements");
                    continue;
                }

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

                // ESP_LOGI(TAG, "Initialized event for pattern %s", patternName.c_str());
                events.push_back(event);
            }
            // ESP_LOGI(TAG, "Initialized envelope for pattern %s", patternName.c_str());
            envelopes.push_back(BezierEnvelope(events));
        }
        ESP_LOGI(TAG, "Initialized pattern %s", patternName.c_str());

        return {patternName.c_str(), BezierPattern(envelopes)};
}

std::map<std::string, BezierPattern> parseJsonToBezierPatterns(const JsonArray& doc) {
    std::map<std::string, BezierPattern> patterns;

    ESP_LOGI(TAG, "Parsing JSON to BezierPatterns, array length %d", doc.size());

    // Parse the JSON array of patterns
    for (JsonObject patternObj : doc) {
        auto [patternName, pattern] = parseJsonToBezierPattern(patternObj);
        patterns.insert({patternName, pattern});
    }
    ESP_LOGI(TAG, "JSON pattern parsing finished");

    return patterns;
}

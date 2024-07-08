#include "BezierJsonParser.h"

std::vector<BezierPattern> parseJsonToBezierPatterns(const JsonArray &doc) {
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

            for (JsonObject eventObj : eventArray) {
                FloatEvent event;
                event.Time = eventObj["Time"].as<double>();
                event.Value = eventObj["Value"].as<float>();

                if (eventObj.containsKey("CurveControl1X") && eventObj.containsKey("CurveControl1Y") &&
                    eventObj.containsKey("CurveControl2X") && eventObj.containsKey("CurveControl2Y")) {
                    event.CurveControl1X = eventObj["CurveControl1X"].as<double>();
                    event.CurveControl1Y = eventObj["CurveControl1Y"].as<double>();
                    event.CurveControl2X = eventObj["CurveControl2X"].as<double>();
                    event.CurveControl2Y = eventObj["CurveControl2Y"].as<double>();
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

    return patterns;
}

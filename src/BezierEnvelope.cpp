#include "BezierEnvelope.h"

static const char *TAG = "BezierEnvelope";

BezierEnvelope::BezierEnvelope(const std::vector<FloatEvent>& events) {
    bezierSegments = loadEnvelope(events);
    duration = events.empty() ? 0.0 : events.back().Time;
}

std::vector<BezierSegment> BezierEnvelope::loadEnvelope(const std::vector<FloatEvent>& events) {
    std::vector<BezierSegment> bezierSegments;

    if(events.size() < 2) {
        // ESP_LOGE(TAG, "At least two events are required to create a Bezier envelope");
        return bezierSegments;
    }

    for (size_t i = 0; i < events.size() - 1; ++i) {
        const FloatEvent& startEvent = events[i];
        const FloatEvent& endEvent = events[i + 1];

        if(startEvent.Time == endEvent.Time) {
            continue;
        }

        // std::vector<bezier::Point> controlPoints;
        CurveSegment curveSegment = startEvent.HasCurveControls ? 
            CurveSegment({
                Vec2(startEvent.Time, startEvent.Value),
                Vec2(startEvent.Time + (endEvent.Time - startEvent.Time) * startEvent.CurveControl1X, startEvent.Value + (endEvent.Value - startEvent.Value) * startEvent.CurveControl1Y),
                Vec2(startEvent.Time + (endEvent.Time - startEvent.Time) * startEvent.CurveControl2X, startEvent.Value + (endEvent.Value - startEvent.Value) * startEvent.CurveControl2Y),
                Vec2(endEvent.Time, endEvent.Value)
            }) : 
            CurveSegment(Vec2(startEvent.Time, startEvent.Value), Vec2(endEvent.Time, endEvent.Value));
            // controlPoints = {
            //     Vec2(startEvent.Time, startEvent.Value),
            //     Vec2((2 * startEvent.Time + endEvent.Time) / 3, (2 * startEvent.Value + endEvent.Value) / 3),
            //     Vec2((startEvent.Time + 2 * endEvent.Time) / 3, (startEvent.Value + 2 * endEvent.Value) / 3),
            //     Vec2(endEvent.Time, endEvent.Value)
            // };

        // Log all control points
        // std::cout << "SEG\n";
        // for (auto& point : controlPoints) {
        //     std::cout << "Control point: " << point.x << ", " << point.y << '\n';
        // }
        // bezier::Bezier<3> bezierCurve(controlPoints);
        bezierSegments.push_back({startEvent.Time, endEvent.Time, curveSegment});
    }

    return bezierSegments;
}

float BezierEnvelope::sampleAtTime(float time) const {
    if(bezierSegments.empty()) {
        return 0;
    }
    for (const auto& segment : bezierSegments) {
        if (segment.StartTime <= time && time <= segment.EndTime) {
            float t = (time - segment.StartTime) / (segment.EndTime - segment.StartTime);
            return segment.curve.valueAt(t).y;
        }
    }

    // If the time is not within any segment, return NaN
    // std::cerr << "Time " << time << " is not within any segment\n";
    ESP_LOGD(TAG, "Time %.2f is not within any segment", time);
    return 0;
}

std::string BezierEnvelope::toString() const {
    std::string str = "Envelope:\n";
    for (const auto& segment : bezierSegments) {
        str += "    " + std::to_string(segment.StartTime) + " -> " + std::to_string(segment.EndTime) + '\n';
    }
    return str;
}
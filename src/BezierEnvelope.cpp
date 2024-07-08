#include "BezierEnvelope.h"
#include <iostream> // For logging, assuming using std::cout for simplicity

BezierEnvelope::BezierEnvelope(const std::vector<FloatEvent>& events) {
    bezierSegments = loadEnvelope(events);
}

std::vector<BezierSegment> BezierEnvelope::loadEnvelope(const std::vector<FloatEvent>& events) {
    std::vector<BezierSegment> bezierSegments;

    for (size_t i = 0; i < events.size() - 1; ++i) {
        const FloatEvent& startEvent = events[i];
        const FloatEvent& endEvent = events[i + 1];

        if(startEvent.Time == endEvent.Time) {
            continue;
        }

        std::vector<bezier::Point> controlPoints;
        if (startEvent.HasCurveControls) {
            controlPoints = {
                bezier::Point(startEvent.Time, startEvent.Value),
                bezier::Point(startEvent.Time + (endEvent.Time - startEvent.Time) * startEvent.CurveControl1X, startEvent.Value + (endEvent.Value - startEvent.Value) * startEvent.CurveControl1Y),
                bezier::Point(startEvent.Time + (endEvent.Time - startEvent.Time) * startEvent.CurveControl2X, startEvent.Value + (endEvent.Value - startEvent.Value) * startEvent.CurveControl2Y),
                bezier::Point(endEvent.Time, endEvent.Value)
            };
        } else {
            // For linear interpolation, create a cubic Bezier curve with collinear control points
            controlPoints = {
                bezier::Point(startEvent.Time, startEvent.Value),
                bezier::Point((2 * startEvent.Time + endEvent.Time) / 3, (2 * startEvent.Value + endEvent.Value) / 3),
                bezier::Point((startEvent.Time + 2 * endEvent.Time) / 3, (startEvent.Value + 2 * endEvent.Value) / 3),
                bezier::Point(endEvent.Time, endEvent.Value)
            };
        }

        // Log all control points
        std::cout << "SEG\n";
        for (auto& point : controlPoints) {
            std::cout << "Control point: " << point.x << ", " << point.y << '\n';
        }
        bezier::Bezier<3> bezierCurve(controlPoints);
        bezierSegments.push_back({startEvent.Time, endEvent.Time, bezierCurve});
    }

    return bezierSegments;
}

double BezierEnvelope::sampleAtTime(double time) const {
    for (const auto& segment : bezierSegments) {
        if (segment.StartTime <= time && time <= segment.EndTime) {
            double t = (time - segment.StartTime) / (segment.EndTime - segment.StartTime);
            return segment.BezierCurve.valueAt(t).y;
        }
    }

    // If the time is not within any segment, return NaN
    std::cerr << "Time " << time << " is not within any segment\n";
    return std::numeric_limits<double>::quiet_NaN();
}

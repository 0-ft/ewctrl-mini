#ifndef BEZIERENVELOPE_H
#define BEZIERENVELOPE_H

#include <vector>
#include <limits>
#include "bezier.h"  // Assuming you have a Bezier library

struct FloatEvent {
    double Time;
    float Value;
    double CurveControl1X;
    double CurveControl1Y;
    double CurveControl2X;
    double CurveControl2Y;
    bool HasCurveControls;
};

struct BezierSegment {
    double StartTime;
    double EndTime;
    bezier::Bezier<3> BezierCurve;
};

class BezierEnvelope {
public:
    BezierEnvelope(const std::vector<FloatEvent>& events);
    double sampleAtTime(double time) const;
    double duration;

private:
    std::vector<BezierSegment> bezierSegments;

    std::vector<BezierSegment> loadEnvelope(const std::vector<FloatEvent>& events);
};

#endif // BEZIERENVELOPE_H

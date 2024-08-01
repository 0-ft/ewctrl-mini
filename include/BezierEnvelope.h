#ifndef BEZIERENVELOPE_H
#define BEZIERENVELOPE_H

#include <vector>
#include <limits>
#include "bezier.h"  // Assuming you have a Bezier library
#include <esp_log.h>
#include "Bezier.h"

struct FloatEvent {
    float Time;
    float Value;
    float CurveControl1X;
    float CurveControl1Y;
    float CurveControl2X;
    float CurveControl2Y;
    bool HasCurveControls;
};

struct BezierSegment {
    float StartTime;
    float EndTime;
    CurveSegment curve;
};

class BezierEnvelope {
public:
    BezierEnvelope(const std::vector<FloatEvent>& events);
    float sampleAtTime(float time) const;
    float duration;
    std::string toString() const;

private:
    std::vector<BezierSegment> bezierSegments;

    std::vector<BezierSegment> loadEnvelope(const std::vector<FloatEvent>& events);
};

#endif // BEZIERENVELOPE_H

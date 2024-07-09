#ifndef BEZIERPATTERN_H
#define BEZIERPATTERN_H

#include "BezierEnvelope.h"
#include <vector>
#include <cstdint>
#include <limits>
#include <esp_log.h>

class BezierPattern {
public:
    BezierPattern(const std::vector<BezierEnvelope>& envelopes);
    std::vector<uint16_t> getFrameAtTime(double time) const;
    std::vector<BezierEnvelope> envelopes;
    double duration;
    double numOutputs;

private:
};

#endif // BEZIERPATTERN_H

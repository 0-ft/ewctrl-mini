#ifndef BEZIERPATTERN_H
#define BEZIERPATTERN_H

#include <vector>
#include <cstdint>
#include "BezierEnvelope.h"

class BezierPattern {
public:
    BezierPattern(const std::vector<BezierEnvelope>& envelopes);
    std::vector<uint16_t> getFrameAtTime(double time) const;

private:
    std::vector<BezierEnvelope> envelopes;
};

#endif // BEZIERPATTERN_H

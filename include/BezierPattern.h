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
    float duration;
    uint8_t numOutputs;


    std::string toString() const {
        std::string str = "BezierPattern with " + std::to_string(envelopes.size()) + " envelopes, duration " + std::to_string(duration) + ", numOutputs " + std::to_string(numOutputs);
        for (const auto& envelope : envelopes) {
            str += "\n" + envelope.toString();
        }
        return str + "\n";
    }

    void printSamples();
private:
};

#endif // BEZIERPATTERN_H

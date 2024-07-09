#include "BezierPattern.h"

static const char* TAG = "BezierPattern";

BezierPattern::BezierPattern(const std::vector<BezierEnvelope>& envelopes) : envelopes(envelopes) {
    duration = 0.0;

    for (const auto& envelope : envelopes) {
        duration = std::max(duration, envelope.duration);
    }

    numOutputs = envelopes.size();
    ESP_LOGI(TAG, "BezierPattern created with %d envelopes, duration %.2f, numOutputs %.2f", envelopes.size(), duration, numOutputs);
}

std::vector<uint16_t> BezierPattern::getFrameAtTime(double time) const {
    std::vector<uint16_t> frame;
    frame.reserve(envelopes.size());
    ESP_LOGI(TAG, "GFAT: Time %.2f", time);
    for (const auto& envelope : envelopes) {
        double sample = envelope.sampleAtTime(time);
        if(sample != 0) {
            ESP_LOGI(TAG, "Sample at time %.2f: %.2f", time, sample);
        }
        // Scale the sample to the 0-4095 range and clamp to [0, 4095]
        uint16_t scaledSample = static_cast<uint16_t>(std::max(0.0, std::min(4095.0, sample * 4095.0)));
        frame.push_back(scaledSample);
    }

    return frame;
}

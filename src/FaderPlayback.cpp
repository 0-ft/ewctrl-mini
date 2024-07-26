#include "FaderPlayback.h"

static const char *TAG = "FaderPlayback";

#define OUTPUTS_PER_DRIVER 8

void FaderPlayback::setup()
{
    for (uint8_t i = 0; i < driverCount; i++)
    {
        Adafruit_PWMServoDriver driver(driverAddresses[i]);
        driver.begin();
        driver.setOscillatorFrequency(27000000);
        driver.setPWMFreq(1600);
        drivers.push_back(driver);
    }
    ESP_LOGI(TAG, "Set up %d drivers", drivers.size());
    availableOutputs = driverCount * OUTPUTS_PER_DRIVER;
}

std::vector<uint16_t> FaderPlayback::makeFrame(int64_t time)
{
    std::vector<uint16_t> combinedFrame(availableOutputs, 0);
    double deltaTime;

    std::vector<std::string> patternsToRemove;

    for (const auto& patternName : activePatterns) {
        const auto maybePattern = patterns.find(patternName);
        if (maybePattern == patterns.end()) {
            patternsToRemove.push_back(patternName);
            continue;
        }
        const auto pattern = maybePattern->second;
        deltaTime = (time - patternStartTime[patternName]) / 1000000.0;

        if (deltaTime > pattern.duration) {
            patternsToRemove.push_back(patternName);
            continue;
        }

        const auto frame = pattern.getFrameAtTime(fmod(deltaTime, pattern.duration));

        for (uint8_t i = 0; i < pattern.numOutputs && i < availableOutputs; i++) {
            combinedFrame[i] += frame[i]; // Summing up the frames
        }
    }

    // clamp the frame to 0-4095
    for (uint8_t i = 0; i < availableOutputs; i++) {
        combinedFrame[i] = std::min(4095, (int)combinedFrame[i]);
    }

    for (const auto& patternName : patternsToRemove) {
        activePatterns.erase(std::remove(activePatterns.begin(), activePatterns.end(), patternName), activePatterns.end());
        patternStartTime.erase(patternName);
        ESP_LOGI(TAG, "Removed pattern %s from active patterns after duration expired", patternName.c_str());
    }
    return combinedFrame;
}

void FaderPlayback::sendFrame()
{
    const auto now = esp_timer_get_time();

    const auto newFrame = activePatterns.empty() ? defaultFrame : makeFrame(now);
    if(newFrame == currentFrame) {
        return;
    } else {
        currentFrame = newFrame;
        measFramesWritten++;
    }

    for (uint8_t i = 0; i < availableOutputs; i++) {
        const uint16_t scaled = ((uint32_t)currentFrame[i] * gain) >> 12;
        drivers[i / OUTPUTS_PER_DRIVER].setPWM(i % OUTPUTS_PER_DRIVER, 0, scaled);
    }

    const auto measDeltaTime = now - measStartTime;
    if (measDeltaTime > 1000000) {
        const double fps = measFramesWritten / (measDeltaTime / 1000000.0);
        ESP_LOGE(TAG, "FPS: %f", fps);
        measFramesWritten = 0;
        measStartTime = now;
    }
}

void FaderPlayback::goToPattern(std::string patternName)
{
    if(activePatterns.size() > MAX_CONCURRENT_PATTERNS) {
        ESP_LOGI(TAG, "Max concurrent patterns reached, not adding %s", patternName.c_str());
        return;
    }
    if (patterns.find(patternName) == patterns.end()) {
        ESP_LOGE(TAG, "Invalid pattern name %s", patternName.c_str());
        return;
    }
    if (std::find(activePatterns.begin(), activePatterns.end(), patternName) == activePatterns.end()) {
        activePatterns.push_back(patternName);
    }
    patternStartTime[patternName] = esp_timer_get_time();
    ESP_LOGI(TAG, "Started pattern %s", patternName.c_str());
}

void FaderPlayback::removePattern(std::string patternName)
{
    activePatterns.erase(std::remove(activePatterns.begin(), activePatterns.end(), patternName), activePatterns.end());
    patternStartTime.erase(patternName);
    ESP_LOGI(TAG, "Removed pattern %s from active patterns", patternName.c_str());
}

void FaderPlayback::setGain(uint16_t gain)
{
    gain = gain > 4095 ? 4095 : gain;
    this->gain = gain;
    ESP_LOGI(TAG, "Set gain to %d", gain);
}

void FaderPlayback::setPatterns(std::map<std::string, BezierPattern> patterns)
{
    this->patterns = patterns;
    ESP_LOGI(TAG, "Set patterns, count %d", patterns.size());
}

void FaderPlayback::addPattern(std::string patternName, BezierPattern pattern)
{
    patterns.insert({patternName, pattern});
    ESP_LOGE(TAG, "Added pattern %s, now have %d total", patternName.c_str(), patterns.size());
    ESP_LOGE(TAG, "Used Heap: %u bytes, Free Heap: %u bytes\n", ESP.getHeapSize() - ESP.getFreeHeap(), ESP.getFreeHeap());
}

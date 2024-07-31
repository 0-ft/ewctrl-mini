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
    std::vector<uint32_t> combinedFrame(availableOutputs, 0);
    double deltaTime;

    std::vector<std::string> patternsToRemove;

    for (const auto& patternPlayback : activePatterns) {
        const auto maybePattern = patterns.find(patternPlayback.name);
        if (maybePattern == patterns.end()) {
            patternsToRemove.push_back(patternPlayback.name);
            continue;
        }
        const auto pattern = maybePattern->second;
        deltaTime = ((time - patternPlayback.startTime) / 1000000.0) * speed;

        if (!patternPlayback.loop && deltaTime > pattern.duration) {
            patternsToRemove.push_back(patternPlayback.name);
            continue;
        }

        const auto frame = pattern.getFrameAtTime(fmod(deltaTime, pattern.duration));

        for (uint8_t i = 0; i < pattern.numOutputs && i < availableOutputs; i++) {
            combinedFrame[i] += frame[i]; // Summing up the frames
        }
    }

    // clamp the frame to 0-4095, apply gain and multiplier
    for (uint8_t i = 0; i < availableOutputs; i++) {
        combinedFrame[i] = std::min(4095, (int)combinedFrame[i]);
        combinedFrame[i] = (combinedFrame[i] * gain) >> 12;
        combinedFrame[i] = (combinedFrame[i] * currentMultiplier[i]) >> 12;
    }

    for (const auto& patternName : patternsToRemove) {
        stopPattern(patternName);
        // activePatterns.erase(std::remove(activePatterns.begin(), activePatterns.end(), patternName), activePatterns.end());
        // ESP_LOGI(TAG, "Removed pattern %s from active patterns after duration expired", patternName.c_str());
    }
    std::vector<uint16_t> result(combinedFrame.begin(), combinedFrame.end());
    return result;
}

void FaderPlayback::sendFrame()
{
    measFrameLoops++;
    const auto now = esp_timer_get_time();
    const auto measDeltaTime = now - measStartTime;
    if (measDeltaTime > measReportTime) {
        const double fps = measFramesWritten / (measDeltaTime / float(measReportTime));
        const double lps = measFrameLoops / (measDeltaTime / float(measReportTime));
        ESP_LOGE(TAG, "FPS: %f, LPS: %f", fps, lps);
        // ESP_LOGE(TAG, "Sendframe on core %d", xPortGetCoreID());
        measFramesWritten = 0;
        measFrameLoops = 0;
        measStartTime = now;
    }

    const auto newFrame = activePatterns.empty() ? defaultFrame : makeFrame(now);
    if(newFrame == currentFrame) {
        return;
    } else {
        currentFrame = newFrame;
        measFramesWritten++;
    }

    for (uint8_t i = 0; i < availableOutputs; i++) {
        drivers[i / OUTPUTS_PER_DRIVER].setPWM(i % OUTPUTS_PER_DRIVER, 0, currentFrame[i]);
    }

    // std::string outputLine;
    // std::string brightnessLevels = " .:-=+*#%@";
    // for (uint8_t i = 0; i < availableOutputs; i++) {
    //     outputLine += brightnessLevels[currentFrame[i] * (brightnessLevels.size() - 1) / 4095];
    // }
    // ESP_LOGI(TAG, "Output Line: %s", outputLine.c_str());

    
}

void FaderPlayback::flashAll(uint8_t times) {
    for(uint8_t f=0; f<times*2+1;f++) {
        // ESP_LOGE(TAG, "Flashing all %d", f);
        for (uint8_t i = 0; i < availableOutputs; i++) {
            drivers[i / OUTPUTS_PER_DRIVER].setPWM(i % OUTPUTS_PER_DRIVER, 0, 4095 * (f % 2));
        }
        delay(60);
    }
}

void FaderPlayback::startPattern(std::string patternName, bool loop)
{
    // ESP_LOGE(TAG, "Go to pattern on core %d", xPortGetCoreID());
    if(activePatterns.size() > MAX_CONCURRENT_PATTERNS) {
        ESP_LOGI(TAG, "Max concurrent patterns reached, not adding %s", patternName.c_str());
        return;
    }
    if (patterns.find(patternName) == patterns.end()) {
        ESP_LOGE(TAG, "Invalid pattern name %s", patternName.c_str());
        return;
    }
    if (std::find_if(activePatterns.begin(), activePatterns.end(), [patternName](const PatternPlayback& pattern) {
        return pattern.name == patternName;
    }) == activePatterns.end()) {
        activePatterns.push_back({
            .name = patternName,
            .startTime = esp_timer_get_time(),
            .loop = loop
        });
    }
    ESP_LOGI(TAG, "Started pattern %s", patternName.c_str());
}

void FaderPlayback::stopPattern(std::string patternName)
{
    // remove from activepatterns
    activePatterns.erase(std::remove_if(activePatterns.begin(), activePatterns.end(), [patternName](const PatternPlayback& pattern) {
        return pattern.name == patternName;
    }), activePatterns.end());
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
    
    uint32_t heapSize  = ESP.getHeapSize();
    uint32_t usedHeap = heapSize - ESP.getFreeHeap();
    float heapUsage = float(usedHeap) / heapSize;
    ESP_LOGE(TAG, "Added pattern %s, now have %d total, heap at %f (%d / %d)", patternName.c_str(), patterns.size(), heapUsage, usedHeap, heapSize);
}

void FaderPlayback::setMultiplier(std::vector<uint16_t> multiplier)
{
    this->currentMultiplier = multiplier;
    ESP_LOGI(TAG, "Set multiplier");
}

void FaderPlayback::setSpeed(float speed)
{
    this->speed = speed;
    ESP_LOGI(TAG, "Set speed multiplier to %.2f", speed);
}

float FaderPlayback::getSpeed()
{
    return speed;
}
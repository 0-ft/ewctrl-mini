#include "FaderPlayback.h"
#include <numeric>
static const char *TAG = "FaderPlayback";

#define OUTPUTS_PER_DRIVER 8

std::vector<uint8_t> FaderPlayback::scanI2C()
{
    ESP_LOGD(TAG, "Scanning I2C");
    std::vector<uint8_t> addresses;
    Wire.begin();
    for (byte i = 8; i < 120; i++)
    {
        Wire.beginTransmission(i);       // Begin I2C transmission Address (i)
        if (Wire.endTransmission() == 0) // Receive 0 = success (ACK response)
        {
            ESP_LOGD(TAG, "Found device at %d", i);
            addresses.push_back(i);
        }
    }
    ESP_LOGD(TAG, "Found %d I2C devices", addresses.size()); // numbers of devices
    return addresses;
}

void FaderPlayback::setup()
{
    if (driverCount == 0)
    {
        const auto foundAddresses = scanI2C();
        driverCount = foundAddresses.size();

        const auto addrString = std::accumulate(std::next(foundAddresses.begin()), foundAddresses.end(), std::to_string(foundAddresses[0]), [](std::string a, uint8_t b)
                                                                                 { return a + ", " + std::to_string(b); });
        ESP_LOGE(TAG, "No drivers specified, scan found %d: %s", driverCount, addrString.c_str());

        driverAddresses = new uint8_t[driverCount];
        for (uint8_t i = 0; i < driverCount; i++)
        {
            driverAddresses[i] = foundAddresses[i];
        }
    }
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
    currentFrame = std::vector<uint16_t>(availableOutputs, 0);
}

std::vector<uint16_t> FaderPlayback::makeFrame(int64_t time)
{
    std::vector<uint32_t> combinedFrame(availableOutputs, 0);
    double deltaTime;

    std::vector<std::string> patternsToRemove;

    for (const auto &patternPlayback : activePatterns)
    {
        const auto maybePattern = patterns.find(patternPlayback.name);
        if (maybePattern == patterns.end())
        {
            patternsToRemove.push_back(patternPlayback.name);
            continue;
        }
        const auto pattern = maybePattern->second;
        deltaTime = ((time - patternPlayback.startTime) / 1000000.0) * speed;

        if (!patternPlayback.loop && deltaTime > pattern.duration)
        {
            patternsToRemove.push_back(patternPlayback.name);
            continue;
        }

        const auto frame = pattern.getFrameAtTime(fmod(deltaTime, pattern.duration));

        for (uint8_t i = 0; i < pattern.numOutputs && i < availableOutputs; i++)
        {
            combinedFrame[i] += frame[i]; // Summing up the frames
        }
    }

    // clamp the frame to 0-4095, apply gain and multiplier
    for (uint8_t i = 0; i < availableOutputs; i++)
    {
        combinedFrame[i] = std::min(4095, (int)combinedFrame[i]);
        combinedFrame[i] = (combinedFrame[i] * gain) >> 12;
        combinedFrame[i] = (combinedFrame[i] * currentMultiplier[i]) >> 12;
    }

    for (const auto &patternName : patternsToRemove)
    {
        stopPattern(patternName);
        // activePatterns.erase(std::remove(activePatterns.begin(), activePatterns.end(), patternName), activePatterns.end());
        // ESP_LOGI(TAG, "Removed pattern %s from active patterns after duration expired", patternName.c_str());
    }
    std::vector<uint16_t> result(combinedFrame.begin(), combinedFrame.end());
    return result;
}

void FaderPlayback::sendFrame()
{
    if (paused)
    {
        return;
    }

    measFrameLoops++;
    const auto now = esp_timer_get_time();
    const auto measDeltaTime = now - measStartTime;
    if (measDeltaTime > measReportTime)
    {
        const double fps = measFramesWritten / (measDeltaTime / float(measReportTime));
        const double lps = measFrameLoops / (measDeltaTime / float(measReportTime));
        ESP_LOGE(TAG, "FPS: %f, LPS: %f", fps, lps);
        // ESP_LOGE(TAG, "Sendframe on core %d", xPortGetCoreID());
        measFramesWritten = 0;
        measFrameLoops = 0;
        measStartTime = now;
    }

    const auto newFrame = activePatterns.empty() ? defaultFrame : makeFrame(now);
    if (newFrame == currentFrame)
    {
        return;
    }

    for (uint8_t i = 0; i < availableOutputs; i++)
    {
        if (currentFrame[i] == newFrame[i])
        {
            continue;
        }
        drivers[i / OUTPUTS_PER_DRIVER].setPWM(i % OUTPUTS_PER_DRIVER, 0, newFrame[i]);
    }

    currentFrame = newFrame;
    measFramesWritten++;

    // std::string outputLine;
    // std::string brightnessLevels = " .:-=+*#%@";
    // for (uint8_t i = 0; i < availableOutputs; i++) {
    //     outputLine += brightnessLevels[currentFrame[i] * (brightnessLevels.size() - 1) / 4095];
    // }
    // ESP_LOGI(TAG, "Output Line: %s", outputLine.c_str());
}

void FaderPlayback::flashAll(uint8_t times, uint16_t duration)
{
    for (uint8_t f = 0; f < times * 2 + 1; f++)
    {
        // ESP_LOGE(TAG, "Flashing all %d", f);
        for (uint8_t i = 0; i < availableOutputs; i++)
        {
            drivers[i / OUTPUTS_PER_DRIVER].setPWM(i % OUTPUTS_PER_DRIVER, 0, 4095 * (f % 2));
        }
        delay(duration);
    }
}

void FaderPlayback::testSequence()
{
    setPaused(true);
    flashAll(3, 100);
    delay(100);
    for (uint8_t i = 0; i < availableOutputs; i++)
    {
        drivers[i / OUTPUTS_PER_DRIVER].setPWM(i % OUTPUTS_PER_DRIVER, 0, 4095);
        delay(60);
        drivers[i / OUTPUTS_PER_DRIVER].setPWM(i % OUTPUTS_PER_DRIVER, 0, 0);
    }
    setPaused(false);
}

void FaderPlayback::startPattern(std::string patternName, bool loop)
{
    // ESP_LOGE(TAG, "Go to pattern on core %d", xPortGetCoreID());
    if (activePatterns.size() > MAX_CONCURRENT_PATTERNS)
    {
        ESP_LOGI(TAG, "Max concurrent patterns reached, not adding %s", patternName.c_str());
        return;
    }
    if (patterns.find(patternName) == patterns.end())
    {
        ESP_LOGE(TAG, "Invalid pattern name %s", patternName.c_str());
        return;
    }
    auto now = esp_timer_get_time();
    auto alreadyActive = std::find_if(activePatterns.begin(), activePatterns.end(), [patternName](const PatternPlayback &pattern)
                                      { return pattern.name == patternName; });
    if (alreadyActive == activePatterns.end())
    {

        // find other patterns started less than quantizeTime ago, update startTime to the earliest
        if (quantizeTime > 0)
        {
            for (const auto &pattern : activePatterns)
            {
                if (now - pattern.startTime < quantizeTime)
                {
                    now = pattern.startTime;
                }
            }
        }

        activePatterns.push_back({.name = patternName,
                                  .startTime = now,
                                  .loop = loop});
    }
    else
    {
        // pattern already active, restart it
        (*alreadyActive).startTime = now;
    }
    ESP_LOGI(TAG, "Started pattern %s", patternName.c_str());
}

void FaderPlayback::stopPattern(std::string patternName)
{
    // remove from activepatterns
    activePatterns.erase(std::remove_if(activePatterns.begin(), activePatterns.end(), [patternName](const PatternPlayback &pattern)
                                        { return pattern.name == patternName; }),
                         activePatterns.end());
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

    uint32_t heapSize = ESP.getHeapSize();
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

void FaderPlayback::setPaused(bool paused)
{
    this->paused = paused;
    ESP_LOGI(TAG, paused ? "Paused" : "Unpaused");
}
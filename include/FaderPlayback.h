#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <Adafruit_PWMServoDriver.h>
#include <vector>
#include "BezierPattern.h"
#include <string>
#include <map>
#include <esp_log.h>


class FaderPlayback {
private:
    uint8_t driverCount;
    uint8_t* driverAddresses;
    std::vector<Adafruit_PWMServoDriver> drivers;
    const uint8_t MAX_CONCURRENT_PATTERNS = 10;


    std::map<std::string, int64_t> patternStartTime; // Map to store start time for each active pattern
    uint16_t gain;
    uint16_t lastFrameIndex;
    uint8_t availableOutputs;
    int64_t measStartTime;
    std::vector<uint16_t> makeFrame(int64_t time);

    uint16_t measFramesWritten;
    uint16_t measFrameLoops;
    uint64_t measReportTime = 2000000;

public:
    std::vector<std::string> activePatterns; // Vector to store currently active patterns
    uint16_t frameRate;
    std::map<std::string, BezierPattern> patterns;
    std::vector<uint16_t> defaultFrame;
    std::vector<uint16_t> currentFrame;

    std::vector<uint16_t> multiplier = std::vector<uint16_t>(16, 4096);

    FaderPlayback(uint16_t frameRate, uint8_t driverCount, uint8_t* driverAddresses, std::vector<uint16_t> defaultFrame = std::vector<uint16_t>(16, 0))
        : frameRate(frameRate), driverCount(driverCount), driverAddresses(driverAddresses), defaultFrame(defaultFrame) {}

    void setup();
    void goToPattern(std::string patternName);
    void removePattern(std::string patternName);
    void sendFrame();
    void setGain(uint16_t gain);
    void setPatterns(std::map<std::string, BezierPattern> patterns);
    void addPattern(std::string patternName, BezierPattern pattern);
    void setMultiplier(std::vector<uint16_t> multiplier);
};

#endif // PLAYBACK_H

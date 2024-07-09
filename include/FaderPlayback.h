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
    uint64_t measFramesWritten;

public:
    std::vector<std::string> activePatterns; // Vector to store currently active patterns
    uint16_t frameRate;
    std::map<std::string, BezierPattern> patterns;

    FaderPlayback(uint16_t frameRate, uint8_t driverCount, uint8_t* driverAddresses)
        : frameRate(frameRate), driverCount(driverCount), driverAddresses(driverAddresses) {}

    void setup();
    void goToPattern(std::string patternName);
    void removePattern(std::string patternName);
    void sendFrame();
    void setGain(uint16_t gain);
    void setPatterns(std::map<std::string, BezierPattern> patterns);
    void addPattern(std::string patternName, BezierPattern pattern);
};

#endif // PLAYBACK_H

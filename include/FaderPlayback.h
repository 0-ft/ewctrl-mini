#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <Adafruit_PWMServoDriver.h>
#include <vector>
#include "BezierPattern.h"

class FaderPlayback {
private:

    uint8_t driverCount;
    uint8_t* driverAddresses;
    std::vector<Adafruit_PWMServoDriver> drivers;

    int64_t patternStartTime;

    uint16_t gain;
    // uint16_t frameIndex;
    uint16_t lastFrameIndex;
    uint16_t frameCount;
    uint8_t availableOutputs;
    int64_t measTime;

    void setPin(uint16_t value);
    void setAll(uint16_t* values);


public:
    uint16_t patternIndex;
    uint16_t frameRate;
    std::vector<BezierPattern> patterns;
    FaderPlayback(uint16_t frameRate, uint8_t driverCount, uint8_t* driverAddresses) : frameRate(frameRate), driverCount(driverCount), driverAddresses(driverAddresses) {};
    void setup();
    void goToPattern(uint16_t patternIndex);
    void sendFrame();
    void setGain(uint16_t gain);
    void setPatterns(std::vector<BezierPattern> patterns);
};

#endif // PLAYBACK_H
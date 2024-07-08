
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
        // for(int j=0; j<16; j++) {
        //     driver.setPin(0, 0, 0);
        // }
        drivers.push_back(driver);
    }
    ESP_LOGI(TAG, "Set up %d drivers", drivers.size());
    availableOutputs = driverCount * OUTPUTS_PER_DRIVER;
}

void FaderPlayback::sendFrame()
{
    if(patternIndex >= patterns.size()) {
        ESP_LOGE(TAG, "Invalid pattern index %d", patternIndex);
        return;
    }
    const auto now = esp_timer_get_time();
    const auto deltaTime = (now - patternStartTime) / 1000000.0;

    // const auto pattern = FADER_PATTERNS[patternIndex];
    // const auto patternLength = FADER_PATTERN_LENGTHS[patternIndex];
    // const auto frameIndex = (deltaTime * frameRate / 1000000);
    // const uint16_t *frame;
    // if(frameIndex >= patternLength) {
    //     frame = FADER_FRAME_ZEROES;
    // } else {
    //     frame = pattern + (frameIndex * FADER_PATTERN_OUTPUTS_NUM);
    // }
    // if (frameIndex != lastFrameIndex)
    // {
    //     lastFrameIndex = frameIndex;
    // }
    // else
    // {
    //     return;
    // }

    const auto pattern = patterns[patternIndex];
    const auto frame = pattern.getFrameAtTime(fmod(deltaTime, pattern.duration));
    for (uint8_t i = 0; i < pattern.numOutputs && i < availableOutputs; i++)
    {
        // const auto val = pattern[frameIndex * FADER_PATTERN_OUTPUTS_NUM + i];
        // Serial.print(String(frame[i]) + "|");
        // drivers[i / OUTPUTS_PER_DRIVER].setPin(i % OUTPUTS_PER_DRIVER, 4095, false);
        const uint16_t scaled = ((uint32_t)frame[i] * gain) >> 12;
        drivers[i / OUTPUTS_PER_DRIVER].setPWM(i % OUTPUTS_PER_DRIVER, 0, scaled);
    }

    // if(deltaTime > 1000000) {
    //     const auto fps = frameIndex / (deltaTime / 1000000.0);
    //     ESP_LOGI(TAG, "FPS: %f", fps);
    // }
    // Serial.println();
    // frameIndex++;
}

void FaderPlayback::goToPattern(uint16_t patternIndex)
{
    if (patternIndex >= patterns.size())
    {
        ESP_LOGE(TAG, "Invalid pattern index %d", patternIndex);
        return;
    }
    this->patternIndex = patternIndex;
    this->patternStartTime = esp_timer_get_time();
    ESP_LOGI(TAG, "Changed to pattern %d", patternIndex);
    // Serial.println("Went to pattern " + String(patternIndex));
    // frameIndex = 0;
    // sendFrame();
}

void FaderPlayback::setGain(uint16_t gain)
{
    gain = gain > 4095 ? 4095 : gain;
    this->gain = gain;
    ESP_LOGI(TAG, "Set gain to %d", gain);
    // sendFrame();
}

void FaderPlayback::setPatterns(std::vector<BezierPattern> patterns)
{
    this->patterns = patterns;
    ESP_LOGI(TAG, "Set patterns, count %d" , patterns.size());
    // sendFrame();
}
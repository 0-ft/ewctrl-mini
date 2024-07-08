
#include "FaderPlayback.h"
#include <FaderPatterns.h>

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
    availableOutputs = (driverCount * OUTPUTS_PER_DRIVER) > FADER_PATTERN_OUTPUTS_NUM ? FADER_PATTERN_OUTPUTS_NUM : (driverCount * OUTPUTS_PER_DRIVER);
}

void FaderPlayback::sendFrame()
{
    const auto pattern = FADER_PATTERNS[patternIndex];
    const auto patternLength = FADER_PATTERN_LENGTHS[patternIndex];
    // Serial.println("Pattern length: " + String(patternLength) + " pattern index: " + String(patternIndex) + " pattern start time: " + String(patternStartTime));
    const auto now = esp_timer_get_time();
    const auto frameIndex = ((now - patternStartTime) * frameRate / 1000000) % patternLength;
    // Serial.println("Frame index: " + String(frameIndex) + " last frame index: " + String(lastFrameIndex));
    if (frameIndex != lastFrameIndex)
    {
        lastFrameIndex = frameIndex;
    }
    else
    {
        return;
    }
    // Serial.println("Writing frame " + String(frameIndex) + " of pattern " + String(patternIndex));
    const auto *frame = pattern + (frameIndex * FADER_PATTERN_OUTPUTS_NUM);
    // Serial.println("available outputs: " + String(availableOutputs) + " pattern outputs: " + String(FADER_PATTERN_OUTPUTS_NUM));
    // Serial.print("P " + String(patternIndex) + " F " + String(frameIndex) + ":\t");
    // Serial.println("driver" + String(i / OUTPUTS_PER_DRIVER));
    for (uint8_t i = 0; i < availableOutputs; i++)
    {
        // const auto val = pattern[frameIndex * FADER_PATTERN_OUTPUTS_NUM + i];
        // Serial.print(String(frame[i]) + "|");
        // drivers[i / OUTPUTS_PER_DRIVER].setPin(i % OUTPUTS_PER_DRIVER, 4095, false);
        const uint16_t scaled = ((uint32_t)frame[i] * gain) >> 12;
        drivers[i / OUTPUTS_PER_DRIVER].setPWM(i % OUTPUTS_PER_DRIVER, 0, scaled);
    }
    // Serial.println();
    // frameIndex++;
}

void FaderPlayback::goToPattern(uint16_t patternIndex)
{
    if (patternIndex >= FADER_PATTERNS_NUM)
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
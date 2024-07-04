#ifndef CONTROLPAD_H
#define CONTROLPAD_H

#if ARDUINO_USB_MODE
  #error "This sketch should be used when USB is in OTG mode"
#endif

// // USB Host Part (handles detection and input from the physical keyboard)
// #define DP_P0  15  // USB Host Data+ Pin (must be an analog pin)
// #define DM_P0  16  // USB Host Data- Pin (must be an analog pin)
#define FORCE_TEMPLATED_NOPS
#include <Arduino.h>
// Include Wire Library for I2C
#include <SPI.h>
#include <ESP32-USB-Soft-Host.h>

#include <functional>

enum CONTROLPAD_EVENT_TYPE {
    CONTROLPAD_EVENT_KEYBOARD = 0,
};

class ControlPad {
public:
    ControlPad(uint8_t dataPos, uint8_t dataNeg, std::function<void(uint8_t type, uint16_t data)> eventCallback) : dataPos(dataPos), dataNeg(dataNeg), eventCallback(eventCallback) {};
    void init();

private:
    static ControlPad* instance;

    const uint8_t dataPos;
    const uint8_t dataNeg;
    std::function<void(uint8_t type, uint16_t data)> eventCallback;

    static void onKeyboardDataStatic(uint8_t usbNum, uint8_t byte_depth, uint8_t *data, uint8_t data_len);
    static void onKeyboardDetectedStatic(uint8_t usbNum, void *dev);

    void onKeyboardDetected(uint8_t usbNum, void *dev);
    void onKeyboardData(uint8_t usbNum, uint8_t byte_depth, uint8_t *data, uint8_t data_len);
};

#endif // CONTROLPAD_H

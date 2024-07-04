#include "ControlPad.h"

ControlPad* ControlPad::instance = nullptr;

void ControlPad::onKeyboardDataStatic(uint8_t usbNum, uint8_t byte_depth, uint8_t *data, uint8_t data_len)
{
    Serial.println("Keyboard data");
    if (instance)
    {
        instance->onKeyboardData(usbNum, byte_depth, data, data_len);
    } else {
        Serial.println("Keyboard data but no instance");
    }
}

void ControlPad::onKeyboardDetectedStatic(uint8_t usbNum, void *dev)
{
    Serial.println("Keyboard detected");
    if (instance)
    {
        instance->onKeyboardDetected(usbNum, dev);
    } else {
        Serial.println("Keyboard detected but no instance");
    }
}

String *keyboardDataToKeyName(uint8_t *data, uint8_t data_len)
{
}

void ControlPad::onKeyboardData(uint8_t usbNum, uint8_t byte_depth, uint8_t *data, uint8_t data_len)
{
    // if( myListenUSBPort != usbNum ) return;
    printf("in: ");
    for (int k = 0; k < data_len; k++)
    {
        printf("0x%02x ", data[k]);
    }
    printf("\n");
    eventCallback(CONTROLPAD_EVENT_KEYBOARD, data[0]);

    // implement keylogger here, before forwarding

    // Keyboard.sendReport( (KeyReport*)data );
}

void ControlPad::onKeyboardDetected(uint8_t usbNum, void *dev)
{
    sDevDesc *device = (sDevDesc *)dev;
    Serial.println("New device detected on USB#" + String(usbNum));
    Serial.println("desc.bcdUSB             = 0x" + String(device->bcdUSB, HEX));
    Serial.println("desc.bDeviceClass       = 0x" + String(device->bDeviceClass, HEX));
    Serial.println("desc.bDeviceSubClass    = 0x" + String(device->bDeviceSubClass, HEX));
    Serial.println("desc.bDeviceProtocol    = 0x" + String(device->bDeviceProtocol, HEX));
    Serial.println("desc.bMaxPacketSize0    = 0x" + String(device->bMaxPacketSize0, HEX));
    Serial.println("desc.idVendor           = 0x" + String(device->idVendor, HEX));
    Serial.println("desc.idProduct          = 0x" + String(device->idProduct, HEX));
    Serial.println("desc.bcdDevice          = 0x" + String(device->bcdDevice, HEX));
    Serial.println("desc.iManufacturer      = 0x" + String(device->iManufacturer, HEX));
    Serial.println("desc.iProduct           = 0x" + String(device->iProduct, HEX));
    Serial.println("desc.iSerialNumber      = 0x" + String(device->iSerialNumber, HEX));
    Serial.println("desc.bNumConfigurations = 0x" + String(device->bNumConfigurations, HEX));
    // if( device->iProduct == mySupportedIdProduct && device->iManufacturer == mySupportedManufacturer ) {
    //   myListenUSBPort = usbNum;
    // }

    // static bool usb_dev_begun = false;

    // if( !usb_dev_begun ) {
    //   printf("Starting USB");
    //   // Keyboard.begin();
    //   // USB.begin();
    // }
}

void ControlPad::init()
{
    instance = this;
    printf("ESP32-S3 Keylogger\n");
    printf("TIMER_BASE_CLK: %d, TIMER_DIVIDER:%d, TIMER_SCALE: %d\n", TIMER_BASE_CLK, TIMER_DIVIDER, TIMER_SCALE);
    USH.setBlinkPin( (gpio_num_t) 2 );
    // USH.setTaskPriority( 16 );
    // USH.setTaskCore(1);
    USH.setOnConfigDescCB(Default_USB_ConfigDescCB);
    USH.setOnIfaceDescCb(Default_USB_IfaceDescCb);
    USH.setOnHIDDevDescCb(Default_USB_HIDDevDescCb);
    USH.setOnEPDescCb(Default_USB_EPDescCb);
    usb_pins_config_t USB_Pins_Config =
        {
            dataPos, dataNeg,
            -1, -1,
            -1, -1,
            -1, -1};

    USH.init(USB_Pins_Config, onKeyboardDetectedStatic, onKeyboardDataStatic);
}
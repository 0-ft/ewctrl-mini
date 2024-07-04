
// #define DEBUG_ALL
// #define FORCE_TEMPLATED_NOPS
// #include <ESP32-USB-Soft-Host.h>

// #if defined ARDUINO_LOLIN_S3 || defined ARDUINO_LOLIN_S3_PRO

//   #define PROFILE_NAME "LoLin S3"
//   #define DP_P0  15  // always enabled
//   #define DM_P0  16  // always enabled
//   #define DP_P1  -1
//   #define DM_P1  -1
//   #define DP_P2  -1
//   #define DM_P2  -1
//   #define DP_P3  -1
//   #define DM_P3  -1

// #elif defined ARDUINO_LOLIN_D32_PRO
//   // [KO] 15/13 : unassigned pins seem unresponsive
//   // [OK] 22/23 : pins for MOSI/SCK work just fine
//   #define PROFILE_NAME "LoLin D32 Pro"
//   #define DP_P0  22  // always enabled
//   #define DM_P0  21  // always enabled
//   #define DP_P1  -1
//   #define DM_P1  -1
//   #define DP_P2  -1
//   #define DM_P2  -1
//   #define DP_P3  -1
//   #define DM_P3  -1
// #elif defined ARDUINO_M5STACK_Core2
//   // [OK] 33/32 are the GROVE port pins for I2C, but there are other devices interferring on the bus
//   #define PROFILE_NAME "M5Stack Core2"
//   #define DP_P0  33  // always enabled
//   #define DM_P0  32  // always enabled
//   #define DP_P1  -1
//   #define DM_P1  -1
//   #define DP_P2  -1
//   #define DM_P2  -1
//   #define DP_P3  -1
//   #define DM_P3  -1
// #elif defined ARDUINO_M5STACK_FIRE
//   // [KO] 16/17 : GROVE port pins for RX2/TX2 but seem unresponsive
//   // [KO] 21/22 : GROVE port pins for SDA/SCL, but there are other devices interferring on the bus
//   // [KO] 26/36 : GROVE port pins for I/O but seem unresponsive
//   #define PROFILE_NAME "M5Stack Fire"
//   #define DP_P0  16  // always enabled
//   #define DM_P0  17  // always enabled
//   #define DP_P1  -1
//   #define DM_P1  -1
//   #define DP_P2  -1
//   #define DM_P2  -1
//   #define DP_P3  -1
//   #define DM_P3  -1
// #elif defined ARDUINO_M5Stack_Core_ESP32
//   // [OK] 16/17 : M5Bottom pins for RX2/TX2 work just fine
//   #define PROFILE_NAME "M5Stack Gray"
//   #define DP_P0  16  // always enabled
//   #define DM_P0  17  // always enabled
//   #define DP_P1  -1
//   #define DM_P1  -1
//   #define DP_P2  -1
//   #define DM_P2  -1
//   #define DP_P3  -1
//   #define DM_P3  -1
// #elif CONFIG_IDF_TARGET_ESP32C3 || defined ESP32C3
//   #define PROFILE_NAME "ESP32 C3 Dev module"
//   #define DP_P0   6
//   #define DM_P0   8
//   #define DP_P1  -1
//   #define DM_P1  -1
//   #define DP_P2  -1
//   #define DM_P2  -1
//   #define DP_P3  -1
//   #define DM_P3  -1
// #elif CONFIG_IDF_TARGET_ESP32S2 || defined CONFIG_IDF_TARGET_ESP32S2
//   #define LOAD_TINYUSB
//   #define PROFILE_NAME "ESP32 S2 Dev module"
//   // [/!\] 20/19 = (USB RS/TS), might be unresponsive
//   #define DP_P0  20 // ok ESP32-S2-Kaluga
//   #define DM_P0  19 // ok ESP32-S2-Kaluga
//   #define DP_P1  9  // ok ESP32-S2-Kaluga
//   #define DM_P1  8  // ok ESP32-S2-Kaluga
//   #define DP_P2  11 // ok ESP32-S2-Kaluga
//   #define DM_P2  10 // ok ESP32-S2-Kaluga
//   #define DP_P3  13 // ok ESP32-S2-Kaluga
//   #define DM_P3  12 // ok ESP32-S2-Kaluga

// #else
//   // default pins tested on ESP32-Wroom
//   #define PROFILE_NAME "Default Wroom"
//   #define DP_P0  16  // always enabled
//   #define DM_P0  17  // always enabled
//   #define DP_P1  -1 // -1 to disable
//   #define DM_P1  -1 // -1 to disable
//   #define DP_P2  -1 // -1 to disable
//   #define DM_P2  -1 // -1 to disable
//   #define DP_P3  -1 // -1 to disable
//   #define DM_P3  -1 // -1 to disable
// #endif

// static void my_USB_DetectCB( uint8_t usbNum, void * dev )
// {
//   sDevDesc *device = (sDevDesc*)dev;
//   printf("New device detected on USB#%d\n", usbNum);
//   printf("desc.bcdUSB             = 0x%04x\n", device->bcdUSB);
//   printf("desc.bDeviceClass       = 0x%02x\n", device->bDeviceClass);
//   printf("desc.bDeviceSubClass    = 0x%02x\n", device->bDeviceSubClass);
//   printf("desc.bDeviceProtocol    = 0x%02x\n", device->bDeviceProtocol);
//   printf("desc.bMaxPacketSize0    = 0x%02x\n", device->bMaxPacketSize0);
//   printf("desc.idVendor           = 0x%04x\n", device->idVendor);
//   printf("desc.idProduct          = 0x%04x\n", device->idProduct);
//   printf("desc.bcdDevice          = 0x%04x\n", device->bcdDevice);
//   printf("desc.iManufacturer      = 0x%02x\n", device->iManufacturer);
//   printf("desc.iProduct           = 0x%02x\n", device->iProduct);
//   printf("desc.iSerialNumber      = 0x%02x\n", device->iSerialNumber);
//   printf("desc.bNumConfigurations = 0x%02x\n", device->bNumConfigurations);
//   // if( device->iProduct == mySupportedIdProduct && device->iManufacturer == mySupportedManufacturer ) {
//   //   myListenUSBPort = usbNum;
//   // }
// }

// static void my_USB_PrintCB(uint8_t usbNum, uint8_t byte_depth, uint8_t* data, uint8_t data_len)
// {
//   // if( myListenUSBPort != usbNum ) return;
//   printf("in: ");
//   for(int k=0;k<data_len;k++) {
//     printf("0x%02x ", data[k] );
//   }
//   printf("\n");
// }

// usb_pins_config_t USB_Pins_Config =
// {
//   DP_P0, DM_P0,
//   DP_P1, DM_P1,
//   DP_P2, DM_P2,
//   DP_P3, DM_P3
// };

// void setup()
// {
//   //Serial.begin(115200);
//   delay(5000);
//   printf("USB Soft Host Test for %s\n", PROFILE_NAME );
//   printf("TIMER_BASE_CLK: %d, TIMER_DIVIDER:%d, TIMER_SCALE: %d\n", TIMER_BASE_CLK, TIMER_DIVIDER, TIMER_SCALE );
//   // USH.setTaskCore( 0 );
//   // USH.setBlinkPin( (gpio_num_t) 2 );
//   // USH.setTaskPriority( 16 );
//   USH.setOnConfigDescCB( Default_USB_ConfigDescCB );
//   USH.setOnIfaceDescCb( Default_USB_IfaceDescCb );
//   USH.setOnHIDDevDescCb( Default_USB_HIDDevDescCb );
//   USH.setOnEPDescCb( Default_USB_EPDescCb );

//   USH.init( USB_Pins_Config, my_USB_DetectCB, my_USB_PrintCB );
// }

// void loop()
// {
//   vTaskDelete(NULL);
// }

#include <Arduino.h>
#include <ControlPad.h>
#include <FaderPlayback.h>
#include <FaderPatterns.h>
#include <WiFiCommander.h>

static const char *TAG = "Main";

FaderPlayback faderPlayback(30, 1, new uint8_t[1]{0x40});

// void onControlPadEvent(uint8_t type, uint16_t data)
// {
//     Serial.println("ControlPad event: " + String(type) + " " + String(data));
//     // faderPlayback.goToPattern(random(0, FADER_PATTERNS_NUM));
//     faderPlayback.goToPattern((faderPlayback.patternIndex + 1) % FADER_PATTERNS_NUM);
// }

// USB WHITE IS POSITIVE, GREEN IS NEGATIVE

// ControlPad controlPad(16, 17, onControlPadEvent);

void handleWifiCommand(uint8_t type, uint16_t data)
{
  ESP_LOGI(TAG, "Handling WifiCommander command type %d, data %d", type, data);
  switch(type) {
    case WiFiCommander::COMMAND_SET_PATTERN:
      faderPlayback.goToPattern(data);
      break;
    case WiFiCommander::COMMAND_SET_GAIN:
      faderPlayback.setGain(data);
      break;
    case WiFiCommander::COMMAND_SET_FRAMERATE:
      faderPlayback.frameRate = data;
      break;
    default:
      ESP_LOGW(TAG, "Unknown event type");
      break;
  }
}

WiFiCommander wifiCommander("fader", handleWifiCommand);

void setup()
{
  ESP_LOGI(TAG, "Setting up");

  wifiCommander.init();

  faderPlayback.setup();
  faderPlayback.goToPattern(0);
  // Serial.println("A" + String(FADER_PATTERN_1[0][17]));
  // Serial.println("A" + String(FADER_PATTERNS[0][0]));
}

void loop()
{
  faderPlayback.sendFrame();

  // for(int j=0; j<8; j++) {
  //     driver.setPWM(j, 0, random(0, 4095));
  // }
  // Serial.println("sent frame");
  // delay(1000);
}

// #include <Arduino.h>
// // Include Wire Library for I2C
// #include <SPI.h>
// #include <Adafruit_PWMServoDriver.h>

// // put function declarations here:

// Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// int t = 0;

// void loop() {

//   for(int i=0; i<8; i++){
//     pwm.setPWM(i, 0, i==t ? 4095 : 0);
//   }
//   t = (t + 1) % 8;
//   delay(100);
// }

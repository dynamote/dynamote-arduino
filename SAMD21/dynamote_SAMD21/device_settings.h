
const int receiverPin = 2;                // pin to use for the ir receiver, when recording remote commands
//senderPin = 9                           // pin used for the ir sender
// The senderPin default from the IRLib2 library seems to be 9.
// Check the IRLib2/IRLibProtocols/IRLibSAMD21.h file for more information, you can change it there.

// for WiFi
#define SECRET_SSID "your_ssid"
#define SECRET_PASS "your_password"
#define DEVICE_NAME "dynamote"

// for BLE
char bleAdvertisingName[] = "dynamote";

// for Assistant integration
// make sure you upload the SSL certificates to your device, as described here
// https://github.com/GoogleCloudPlatform/google-cloud-iot-arduino#notes-on-the-certificate
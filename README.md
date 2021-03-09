# Dynamote Arduino

<p align="center">
	<img src="https://dynamote.ca/icons/logo2.png" alt="" width="200" height="200">
</p>

<p align="center">
	<a href='https://play.google.com/store/apps/details?id=com.electricedge.dynamote'>
		<img alt='Get it on Google Play' src='https://play.google.com/intl/en_us/badges/static/images/badges/en_badge_web_generic.png' width="150"/>
	</a>
</p>

<p align="center">
	<a href="https://www.Dynamote.ca">www.Dynamote.ca</a>
</p>

This repository contains the code for Dynamote remotes, which are based on the Arduino platform. The app is currently in closed beta, stay tuned!

- [Dynamote Arduino](#dynamote-arduino)
- [Connectivity](#connectivity)
- [Custom Commands](#custom-commands)
- [Supported Hardware](#supported-hardware)
	- [SAMD21](#samd21)
	- [ESP32](#esp32)
- [Dependencies](#dependencies)
- [Installation](#installation)
- [IR Remote Circuit](#ir-remote-circuit)

# Connectivity

Dynamote supports either WiFi or BLE for communicating with your remote. Which one should you use? In general, we recommend WiFi. With WiFi you can get better range (only need to be connected to the same network, not close proximity like BLE), quicker response (do not need to connect/pair first like BLE), and the option to use Google Assistant integration with Dynamote Pro (in-app purchase). BLE can be useful in environments where you may not have access to a WiFi network, or the WiFI is unreliable.

# Custom Commands

Dynamote is primarily built as an IR remote solution. However, since it is Arduino based and the code is provided directly to you, you are able to extend upon it for your own purposes. The Dynamote app provides a way to interface with your own code through "custom commands". When configuring a button in the app you will also see the option to manually type in a custom command. You can then react to that custom command in your code, see the examples for how to register your own custom command handlers. This allows you to use Dynamote as a remote for your own projects.

# Supported Hardware

There are SAMD21 and ESP32 versions of the project. For each platform, the following boards are supported:

## SAMD21
- [Arduino nano 33 IOT](https://store.arduino.cc/usa/nano-33-iot)

## ESP32
- [Adafruit HUZZAH32](https://www.adafruit.com/product/3405)
- Other ESP32 boards (should theoretically work, but others are not tested)

For now, the WiFi variant of Dynamote only supports these boards. This will be expanded in the future. This limitation is mostly due to getting some of the third party libraries working and tested to confirm they work. There is no limitation for the BLE variant of Dynamote, as this variant of the Dynamote library does not actually handle any BLE code directly or use any extra third party libraries. BLE is all part of the user application code (see the examples). As long as you can reimplement the functionality shown in the examples, any BLE capable board should be sufficient.

# Dependencies

Some third party libraries are required, they are listed below.

All variants:
- [IRLib2](https://github.com/cyborg5/IRLib2)
- [ArduinoJson](https://arduinojson.org/)

WiFi variant:
- [google-cloud-iot-arduino](https://github.com/GoogleCloudPlatform/google-cloud-iot-arduino)
- [arduino-mqtt](https://github.com/256dpi/arduino-mqtt)
- [FlashStorage](https://github.com/cmaglie/FlashStorage), SAMD21 boards only

Note: The latest release of IRLib2 does not support the ESP32. However, a modified version [here](https://github.com/4lloyd/IRLib2/tree/esp32-support) does.

# Installation

Clone/download this repository and then paste it into your Arduino libraries folder. 

Inside the "src/Dynamote.h" file you will find the following block of code, which configures a few things in the Dynamote library. These are the default settings, you do not need to worry about modifying this if you are ok with them, and the provided circuit diagram (in the "extras" folder) applies to these defaults. Inside this code you can select either the WiFi or BLE variant of Dynamote, as well as the pin connected to your IR receiver. Unfortunately, the IR emitter pin tends to be hardcoded within the IRLib2 library, and cannot always be easily changed.

```c++
// Dynamote.h

/********************************************************************************
*    User options
********************************************************************************/

// Uncomment one of these to either select WiFi or BLE operation
#define DYNAMOTE_WIFI
//#define DYNAMOTE_BLE

// Select the pin input you would like to use for the IR receiver
// Not all pins will work, results may vary.
#if defined(ESP32)
#define RECEIVER_PIN				14
#elif defined(ARDUINO_SAMD_NANO_33_IOT)
#define RECEIVER_PIN				2
#endif

// The SEND pin is hardcoded in the IRLib2 library for each board. Listed below are the pins for our supported boards.
//   Adafruit HUZZAH32 = digital pin 26 (same as analog pin A0)
//   Nano 33 IoT = digital pin 9

/********************************************************************************
*    End user options
********************************************************************************/
```

Your board will need to be programmed using the [Arduino IDE](https://www.arduino.cc/en/software). You will also need to have the required board support files for your particular board. The easiest way to get them is to use the "board manager" in the Arduino IDE. Unfortunately, the "board manager" may not include built in support for your particular board and you will have to get it from the third-party manufacturer of the one you have. Some examples of where to get them are below, but will ultimately depend on which one you have, which might not be shown:

**Arduino nano 33 IOT**

Install the "Arduino SAMD Core" from the boards manager. Further information can be found on Arduino's [getting started](https://www.arduino.cc/en/Guide/NANO33IoT) page.

**Adafruit HUZZAH32**

You will not be able to use the boards manager in this case. Adafruit has a good [guide](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide) on where to get the board files and how to get the Arduino IDE set up for them.

# IR Remote Circuit

In order to use Dynamote as a universal remote you will have to add infrared circuitry to your chosen hardware. This means adding an infrared emitter (LED) for sending remote commands, and an infrared receiver so you can record new ones. You are free to design your own circuit, there are also many guides online that you can use to find circuits for universal remotes. For simplicity, we recommend using off the shelf modules that you can buy, such as the following ones.

* [Infrared Emitter](https://www.seeedstudio.com/Grove-Infrared-Emitter.html)
* [Infrared Receiver](https://www.seeedstudio.com/Grove-Infrared-Receiver.html)

You can find wiring diagrams that use these modules in the "extras/wiring_diagram" subfolder. In the case of the Arduino Nano IoT 33 you can also find files for a 3d printable case that uses these modules in the "3d_printable_case" subfolder.

Note: The 5V pin on the Nano 33 IoT is only enabled after you short a jumper on the underside of the board.

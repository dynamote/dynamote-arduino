# Dynamote Arduino

<p align="center">
    <img src="https://dynamote.ca/icons/logo2.png" alt="" width="200" height="200">
</p>

This repository contains the code for Dynamote remotes, which are based on the Arduino platform. Get the app on [Google Play](https://play.google.com/store/apps/details?id=com.dmhuisma.dynamote).

- [Dynamote Arduino](#dynamote-arduino)
- [Supported Hardware](#supported-hardware)
    - [SAMD21](#samd21)
    - [ESP32](#esp32)
- [Dependencies](#dependencies)
    - [Arduino nano 33 IOT](#arduino-nano-33-iot)
    - [Adafruit HUZZAH32](#adafruit-huzzah32)
- [Connectivity](#connectivity)
- [Custom Commands](#custom-commands)

# Supported Hardware

There are SAMD21 and ESP32 versions of the project. For each platform, the following boards are tested:

### SAMD21
- [Arduino nano 33 IOT](https://store.arduino.cc/usa/nano-33-iot)

### ESP32
- [Adafruit HUZZAH32](https://www.adafruit.com/product/3405)

There is a good possibility that other boards that use the same MCU/platform will also work (for example, other ESP32 boards). However, we cannot confirm this until actually testing them. It is also possible that similar MCUs/platforms will also work with little to no code changes, such as the ESP8622 (WiFi only). Do you have a board other than one listed above that works? We would love to hear about it, and add it to the list.

# Dependencies

Some third party libraries are required. Information on which ones and where to find them are located in the respective .ino/.h files.

Your board will need to be programmed using the [Arduino IDE](https://www.arduino.cc/en/software). You will also need to have the required board support files for your particular board. The easiest way to get them is to use the "board manager" in the Arduino IDE. Unfortunately, the "board manager" may not include built in support for your particular board and you will have to get it from the third-party manufacturer of the one you have. Some examples of where to get them are below, but will ultimately depend on which one you have, which might not be shown:

### Arduino nano 33 IOT

Install the "Arduino SAMD Core" from the boards manager. Further information can be found on Arduino's [getting started](https://www.arduino.cc/en/Guide/NANO33IoT) page.

### Adafruit HUZZAH32

You will not be able to use the boards manager in this case. Adafruit has a good [guide](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide) on where to get them and how to get the Arduino IDE set up for it.

There is currently a bug in the most recently released version of the BLE library for the ESP32, which causes every BLE characteristic to fire when only a single one is written to. Fortunately, this has already been fixed in the master source but has not made it into a release yet. See [here](https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/ArduinoBLE.md#replacing-the-version-that-comes-with-arduino-esp32) for more information on how to replace the library files.

# Connectivity

Dynamote supports either WiFi or BLE for communicating with your remote. You can choose which one is used when programming your board. This is done by uncommenting one of the following lines in the code (but not both at the same time). These #define's will cause the proper code to be included when you program your board.

> #define ENABLE_WIFI
>
> #define ENABLE_BLE

Which one should you use? In general, we recommend WiFi. With WiFi you can get better range (only need to be connected to the same network, not close proximity like BLE), quicker response (do not need to connect/pair first like BLE), and the option to use Google Assistant integration with Dynamote Pro (in-app purchase). BLE can be useful in environments where you may not have access to a WiFi network, or the WiFI is unreliable.

When using WiFi, you can input your network credentials in the "device_settings.h" file. Also make sure you change the "DEVICE_NAME" in this file to something unique (for example, "living_room" or "basement"). This is what is used by the Dynamote app to identify your device on the network, and you cannot have multiple devices with the same device name.

# Custom Commands

Dynamote is primarily built as an IR remote solution. However, since it is Arduino based and the code is provided directly to you, you are able to extend upon it for your own purposes. The Dynamote app provides a way to interface with your own code through "custom commands". When configuring a button in the app you will also see the option to manually type in a custom command. You can then react to that custom command in your code, see the "custom_code_handlers.h" file for how to do this, as well as an example. This allows you to use Dynamote as a remote for your own projects.
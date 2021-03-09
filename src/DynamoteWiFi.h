/******************************************************************************
 * Copyright (C) 2021 Darcy Huisman
 * This program is free software: you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *****************************************************************************/

#ifndef DYNAMOTEWIFI_H
#define DYNAMOTEWIFI_H

#include <Dynamote.h>
#ifndef DYNAMOTE_BLE
#include <WiFi.h>

#if defined(ESP32)
#define __DYNAMOTE_ESP32__
#elif defined(ARDUINO_SAMD_NANO_33_IOT)
#define __DYNAMOTE_SAMD21__
#else
#error "error, unsupported/untested board type for DynamoteWiFi.h"
#endif

class DynamoteWiFi : public Dynamote {

  public:
    DynamoteWiFi(void);
    void begin(void);
    void loop(void);

  private:
    WiFiServer _server;
    void handleCommandFromClient(String command, String commandData);
};

#endif		
#endif

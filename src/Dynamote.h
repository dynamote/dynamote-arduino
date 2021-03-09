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

#ifndef DYNAMOTE_H
#define DYNAMOTE_H

/********************************************************************************
*    User options
********************************************************************************/

// Uncomment one of these to either select WiFi or BLE operation
#define DYNAMOTE_WIFI
//#define DYNAMOTE_BLE

// Select the pin input you would like to use for the IR receiver
#define RECEIVER_PIN				14

// The SEND pin is hardcoded in the IRLib2 library for each board. Listed below are the pins for our supported boards.
//   Adafruit HUZZAH32 = digital pin 26 (same as analog pin A0)
//   Nano 33 IoT = digital pin 9

/********************************************************************************
*    End user options
********************************************************************************/

// Check a few conditions
#if defined(DYNAMOTE_WIFI) && defined(DYNAMOTE_BLE)
#error "Error, both DYNAMOTE_WIFI and DYNAMOTE_BLE cannot be defined at the same time"
#endif
#if !defined(DYNAMOTE_WIFI) && !defined(DYNAMOTE_BLE)
#error "Error, one of DYNAMOTE_WIFI or DYNAMOTE_BLE must be defined"
#endif
#ifndef RECEIVER_PIN
#error "Error, RECEIVER_PIN is not defined"
#endif

#include <IRLibDecodeBase.h>                  // IRLib2 https://github.com/cyborg5/IRLib2
#include <IRLibSendBase.h>                    // with the following pull request for ESP32 support: https://github.com/cyborg5/IRLib2/pull/77
#include <IRLib_P01_NEC.h>
#include <IRLib_P02_Sony.h>
#include <IRLib_P03_RC5.h>
#include <IRLib_P04_RC6.h>
#include <IRLib_P05_Panasonic_Old.h>
#include <IRLib_P06_JVC.h>
#include <IRLib_P07_NECx.h>
#include <IRLib_P08_Samsung36.h>
#include <IRLib_P09_GICable.h>
#include <IRLib_P10_DirecTV.h>
#include <IRLib_P11_RCMM.h>
#include <IRLib_P12_CYKM.h>
#include <IRLib_HashRaw.h>
#include <IRLibCombo.h>
#include <IRLibRecvPCI.h>
#include <DynamoteLinkedList.h>
#include <ArduinoJson.h>              // https://arduinojson.org/

typedef struct
{
	uint8_t codeProtocol;         			// The type of IR code
	uint32_t codeValue;           			// The data bits if IR type is not raw
	DynamoteLinkedList codeValueRaw;    // The data bits if IR type is raw
	uint8_t codeLength;           			// The length of the IR code in bits
	String customCode;        					// custom code specified by the user
	bool useCustomCode;      						// whether to use the received custom code over the IR code
} RemoteCommand;

enum RemoteState {
	SEND,
	RECORD
};

class Dynamote
{
	public:
		Dynamote(void);
		RemoteCommand dynamoteLoop(void);
		void sendRemoteCommand(RemoteCommand command);
		void setCustomCommandHandlerFxn(void (*fxn)(RemoteCommand));
		uint8_t sendJsonRemoteCommand(String command);

	protected:
		void setRemoteState(RemoteState state);
		RemoteState remoteState = SEND;
		unsigned long remoteRecordRequestTime = 0;
		void serializeRemoteCommandToJsonString(RemoteCommand command, String &destinationBuffer);

	private:
		IRsend remoteSender;
		IRsendRaw remoteRawSender;
		IRdecode remoteDecoder;
		RemoteCommand getReceiverInput(void);
		IRrecvPCI remoteReceiver;
		DeserializationError deserializeJsonStringToRemoteCommand(String jsonString, RemoteCommand *command);
		void (*customCommandHandlerFxn)(RemoteCommand);
};

#endif

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

#include "Dynamote.h"

/******************************************************************************************************************
* Dynamote constructor
******************************************************************************************************************/
Dynamote::Dynamote(void) : remoteReceiver(RECEIVER_PIN) {}

/******************************************************************************************************************
* dynamoteLoop
******************************************************************************************************************/
RemoteCommand Dynamote::dynamoteLoop(void)
{
	RemoteCommand recordedCommand = RemoteCommand();
	if (remoteState == RECORD){
			recordedCommand = getReceiverInput();
	}

	// If we do not receive the next command within several seconds, we will go back to SEND state
	if (remoteRecordRequestTime != 0 && remoteRecordRequestTime + 5000 < millis()) {
		remoteRecordRequestTime = 0;
		if (remoteState != SEND)
			setRemoteState(SEND);
	}

	return recordedCommand;
}

/******************************************************************************************************************
* setRemoteState
******************************************************************************************************************/
void Dynamote::setRemoteState(RemoteState state) {
	Serial.print("setting remote state: ");
	Serial.println(state);
	remoteState = state;

	if (state == RECORD) {
	// Start the receiver
		remoteReceiver.enableIRIn();
	}
	else if (state == SEND){
		// Stop the receiver
		remoteReceiver.disableIRIn();
	}
}

/******************************************************************************************************************
* sendJsonRemoteCommand
******************************************************************************************************************/
uint8_t Dynamote::sendJsonRemoteCommand(String command) 
{
	// the command has been sent as a json string
	// this will parse it before sending the command
	RemoteCommand remoteCommand = RemoteCommand();
	DeserializationError error = deserializeJsonStringToRemoteCommand(command, &remoteCommand);

	if (!error) {
		if (remoteCommand.useCustomCode && customCommandHandlerFxn != NULL) {
			Serial.print("Received custom command: ");
			Serial.println(remoteCommand.customCode);
			(*customCommandHandlerFxn)(remoteCommand);
		}
		else if (remoteCommand.useCustomCode && customCommandHandlerFxn == NULL) {
			Serial.println("Warning, a custom command was sent but a custom command handler function was not provided");
		}
		else
			sendRemoteCommand(remoteCommand);
		return 0;
	}
	else
		return 1;
}

/******************************************************************************************************************
* sendRemoteCommand
******************************************************************************************************************/
void Dynamote::sendRemoteCommand(RemoteCommand command) 
{
	Serial.print("Sending: ");
	if (command.codeProtocol == UNKNOWN) {
		remoteRawSender.send(command.codeValueRaw.toArray(), command.codeLength, 36);
		Serial.println(F("Sent raw"));
	}
	else {
		Serial.print(F("Sent "));
		Serial.print(Pnames(command.codeProtocol));
		Serial.print(F(" Value:0x"));
		Serial.println(command.codeValue, HEX);
		remoteSender.send(command.codeProtocol, command.codeValue, command.codeLength);
	}
}

/******************************************************************************************************************
* getReceiverInput
******************************************************************************************************************/
RemoteCommand Dynamote::getReceiverInput(void) 
{
	RemoteCommand recordedRemoteCommand = RemoteCommand();

	if (remoteReceiver.getResults()) {
		
		remoteDecoder.decode();

		recordedRemoteCommand.codeProtocol = remoteDecoder.protocolNum;
		Serial.print(F("Received "));
		Serial.print(Pnames(recordedRemoteCommand.codeProtocol));

		//
		// unknown protocol, save raw data
		//
		if (recordedRemoteCommand.codeProtocol == UNKNOWN) {
			Serial.println(F(", saving raw data."));

			recordedRemoteCommand.codeLength = recvGlobal.decodeLength-1;
		
			// copy the contents of the decode buffer into the recordedRemoteCommand raw field
			volatile uint16_t* decodeBuffer = &(recvGlobal.decodeBuffer[1]);
			recordedRemoteCommand.codeValueRaw.clear();
			for (uint8_t x = 0; x < recordedRemoteCommand.codeLength; x++) {
				recordedRemoteCommand.codeValueRaw.add(decodeBuffer[x]);
				Serial.print(decodeBuffer[x]);
				Serial.print(',');
			}
			Serial.println(' ');
		}
		//
		// known protocol
		//
		else {
			if (remoteDecoder.value == REPEAT_CODE) {
				// Don't record a NEC repeat value as that's useless.
				Serial.println(F("repeat; ignoring."));
			} else {
				recordedRemoteCommand.codeValue = remoteDecoder.value;
				recordedRemoteCommand.codeLength = remoteDecoder.bits;
			}
			Serial.print(F(" Value:0x"));
			Serial.println(recordedRemoteCommand.codeValue, HEX);
		}
		remoteReceiver.enableIRIn();
	}
	return recordedRemoteCommand;
}

/******************************************************************************************************************
* deserializeJsonStringToRemoteCommand
******************************************************************************************************************/
DeserializationError Dynamote::deserializeJsonStringToRemoteCommand(String jsonString, RemoteCommand *command)
{
	char jsonStringCharArray[jsonString.length()+1];
	jsonString.toCharArray(jsonStringCharArray, jsonString.length()+1);

	StaticJsonDocument<RECV_BUF_LENGTH*10> jsonDoc;                  // <- an estimate for the maximum byte size of a command string, plus some extra
	DeserializationError deserializeStatus = deserializeJson(jsonDoc, jsonStringCharArray);

	if (deserializeStatus)
		return deserializeStatus;

	command->codeProtocol = jsonDoc["protocol"];
	command->codeValue = jsonDoc["codeValue"];
	command->codeLength = jsonDoc["codeLength"];
	command->customCode = jsonDoc["customCode"].as<String>();
	command->useCustomCode = jsonDoc["useCustomCode"];

	// get the raw code values, if there are any
	command->codeValueRaw.clear();
	JsonArray codeValueRawArray = jsonDoc["codeValueRaw"].as<JsonArray>();
	for (JsonVariant value : codeValueRawArray) {
		command->codeValueRaw.add(value.as<int>());
	}

	return deserializeStatus;
}

/******************************************************************************************************************
* serializeRemoteCommandToJsonString
******************************************************************************************************************/
void Dynamote::serializeRemoteCommandToJsonString(RemoteCommand command, String &destinationBuffer)
{
	StaticJsonDocument<RECV_BUF_LENGTH*10> jsonDoc;                  // <- an estimate for the maximum byte size of a command string, plus some extra
	jsonDoc["protocol"] = command.codeProtocol;
	jsonDoc["codeValue"] = command.codeValue;
	jsonDoc["codeLength"] = command.codeLength;
	jsonDoc["customCode"] = command.customCode;
	jsonDoc["useCustomCode"] = command.useCustomCode;
	if (command.codeProtocol == UNKNOWN) {
		for (uint8_t x = 0; x < command.codeLength; x++)
			jsonDoc["codeValueRaw"][x] = command.codeValueRaw.get(x);
	}
	else
		// create empty array entry
		jsonDoc.createNestedArray("codeValueRaw"); 

	serializeJson(jsonDoc, destinationBuffer);
}

/******************************************************************************************************************
* setCustomCommandHandlerFxn
******************************************************************************************************************/
void Dynamote::setCustomCommandHandlerFxn(void (*fxn)(RemoteCommand)) {
	customCommandHandlerFxn = fxn;
}
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

/******************************************************************************************************************
* includes
******************************************************************************************************************/
#include <DynamoteWiFi.h>
#ifndef DYNAMOTE_BLE
#include <DynamoteMqtt.h>

/******************************************************************************************************************
* constructor
******************************************************************************************************************/
DynamoteWiFi::DynamoteWiFi(void) : _server(80) {}

/******************************************************************************************************************
* setup
******************************************************************************************************************/
void DynamoteWiFi::begin()
{
	_server.begin();
	setupMqtt(this);
}

/******************************************************************************************************************
* loop
******************************************************************************************************************/
void DynamoteWiFi::loop(void)
{
	RemoteCommand recordedCommand = dynamoteLoop();

	if (WiFi.status() != WL_CONNECTED)
		return;

	WiFiClient client = _server.available();;

	if (client) {                             // if you get a client,
		String currentLine = "";                // make a String to hold incoming data from the client
		String requestCommand = "";
		String requestData = "";
		while (client.connected()) {            // loop while the client's connected
			if (client.available()) {             // if there's bytes to read from the client,
				char c = client.read();             // read a byte, then
				if (c == '\n') {                    // if the byte is a newline character

					// if the current line is blank, you got two newline characters in a row.
					// that's the end of the client HTTP request, so send a response:
					if (currentLine.length() == 0) {

						// header
						// send an OK response to the client
						client.println("HTTP/1.1 200 OK");
						client.println("Content-type:text/html");
						client.println();

						// send the recorded command to the client.
						if (recordedCommand.codeLength != 0) {
							String recordedRemoteCommandJsonString;
							serializeRemoteCommandToJsonString(recordedCommand, recordedRemoteCommandJsonString);
							client.print(recordedRemoteCommandJsonString);
							client.println("");
						}
						
						// get the data from the HTTP request, then handle it
						while(client.available()) {
							requestData += (char)client.read();
						}
						handleCommandFromClient(requestCommand, requestData);

						// break out of the while loop
						break;
					} 

					else {

						//
						// parse POST commands
						//
						if (currentLine.indexOf("POST /") != -1) {
							// remove the "POST /"
							requestCommand = currentLine.substring(currentLine.indexOf("POST /") + 6);
							// remove anything after a space
							requestCommand = requestCommand.substring(0, requestCommand.indexOf(' '));
						} 

						//
						// Parse GET commands
						//
						if (currentLine.indexOf("GET /") != -1) {
							// remove the "GET /"
							requestCommand = currentLine.substring(currentLine.indexOf("GET /") + 5);
							// remove anything after a space
							requestCommand = requestCommand.substring(0, requestCommand.indexOf(' '));
						} 

						currentLine = "";
					}
				} else if (c != '\r') {  // if you got anything else but a carriage return character,
					currentLine += c;      // add it to the end of the currentLine
				}
			}
		}
		// close the connection
		client.stop();
	}

	mqttloop();
}

/******************************************************************************************************************
* handleCommandFromClient
******************************************************************************************************************/
void DynamoteWiFi::handleCommandFromClient(String command, String commandData) 
{
	//
	// Are we trying to send a remote command?
	//
	if (command.equals("sendRemoteCommand")) {
		sendJsonRemoteCommand(commandData);
	}

	//
	// Are we trying to configure MQTT?
	//
	if (command.equals("configureMQTT")) {
		configureMqtt(commandData);
		Serial.println("Saved new MQTT config");
	}

	//
	// determine if a new recorded command is requested by the client
	//
	if (command.equals("getRecordedCommand")) {
		if (remoteState != RECORD)
			setRemoteState(RECORD);
		// If we do not receive the next command within several seconds, we will go back to SEND state
		remoteRecordRequestTime = millis();
	}
	//
	// determine if the client is done recording commands
	//
	else if (command.equals("getRecordedCommandDone")) {
		setRemoteState(SEND);
	}
}

#endif

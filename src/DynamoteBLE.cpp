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
#include <DynamoteBLE.h>
#ifndef DYNAMOTE_WIFI

/******************************************************************************************************************
* constructor
******************************************************************************************************************/
DynamoteBLE::DynamoteBLE(void) {}

/******************************************************************************************************************
* begin
******************************************************************************************************************/
void DynamoteBLE::begin(void (*fxn)(byte*, uint8_t))
{
	sendDataToRemoteRecordCharacteristicFxn = fxn;
}

/******************************************************************************************************************
* loop
******************************************************************************************************************/
void DynamoteBLE::loop(void)
{
	if (sendDataToRemoteRecordCharacteristicFxn == NULL) {
		static bool errorShown = false;
		if (!errorShown) {
				errorShown = true;
				Serial.println("Warning, no sendDataToRemoteRecordCharacteristic function was provided");
		}
	}

	if (sendRemoteCommandFlag) {
		sendRemoteCommandFlag = false;
		// the command has been sent as a json string
		uint8_t result = sendJsonRemoteCommand(remoteCommandJsonString);
		if (result == 0) {
				remoteSendTime = 0;
				remoteCommandJsonString = "";
		}
	}

  // If we do not receive the rest of the remote command over BLE within 1 second, we assume it failed
  if (remoteSendTime != 0 && remoteSendTime + 1000 < millis()) {
    remoteSendTime = 0;
    remoteCommandJsonString = "";
    Serial.println("remote command cleared");
  }

  RemoteCommand recordedCommand = dynamoteLoop();
  if (recordedCommand.codeLength != 0)
    sendRecordedCommandOverBle(recordedCommand);
}

/******************************************************************************************************************
* setMtu
******************************************************************************************************************/
void DynamoteBLE::setMtu(uint16_t _mtu)
{
	mtu = _mtu;
}

/******************************************************************************************************************
* onBleDisconnected
******************************************************************************************************************/
void DynamoteBLE::onBleDisconnected(void)
{
	setRemoteState(SEND);
}

/******************************************************************************************************************
* onRemoteSendCharacteristic
******************************************************************************************************************/
void DynamoteBLE::onRemoteSendCharacteristic(uint8_t *data, uint16_t length)
{
	// If we do not receive the next value within 1 second, we will stop and assume it failed
	remoteSendTime = millis();

	for (uint16_t x = 0; x < length; x++) {
		remoteCommandJsonString += (char)data[x];
	}

	// Don't do the heavy lifting in the callback. Set a flag to do it in the loop.
	sendRemoteCommandFlag = true;
}

/******************************************************************************************************************
* onRemoteRecordEnableCharacteristic
******************************************************************************************************************/
void DynamoteBLE::onRemoteRecordEnableCharacteristic(uint8_t *data)
{
	if (data[0])
		setRemoteState(RECORD);
	else
		setRemoteState(SEND);
}
void DynamoteBLE::onRemoteRecordEnableCharacteristic(bool data)
{
	if (data)
		setRemoteState(RECORD);
	else
		setRemoteState(SEND);
}

/******************************************************************************************************************
* sendRecordedCommandOverBle
******************************************************************************************************************/
void DynamoteBLE::sendRecordedCommandOverBle(RemoteCommand command) {

  if (sendDataToRemoteRecordCharacteristicFxn == NULL)
    return;

  //
  // serialize command to json string
  //
  String jsonString = "";
  serializeRemoteCommandToJsonString(command, jsonString);

  uint16_t index = 0;
  while(index < jsonString.length()) {

    uint16_t charactreristicLength = min(mtu, (uint16_t)(jsonString.length()-index));
    byte characteristicArray[charactreristicLength] = {0};
    for (uint16_t x = 0; x < charactreristicLength; x++) 
      characteristicArray[x] = jsonString.charAt(index++);
    // write the array chunk over BLE
    (*sendDataToRemoteRecordCharacteristicFxn)((byte*)&characteristicArray, charactreristicLength);
    delay(3);
  }
}

#endif
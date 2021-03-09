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

#ifndef DYNAMOTEBLE_H
#define DYNAMOTEBLE_H

#include <Dynamote.h>

#ifndef DYNAMOTE_WIFI

#define DYNAMOTE_REMOTE_SERVICE_UUID                        "91193088-70ec-4eba-8a27-4802e6b4f81f"
#define DYNAMOTE_REMOTE_SEND_CHARACTERISTIC_UUID            "5e4d2bf6-29ec-4a1b-8649-259568488e7b"
#define DYNAMOTE_REMOTE_RECORD_CHARACTERISTIC_UUID          "608b70d9-5ee2-4380-a0f4-8a629578f19b"
#define DYNAMOTE_REMOTE_RECORD_ENABLE_CHARACTERISTIC_UUID   "37141628-d6d9-45bd-af90-e297a92b6953"

#define DEFAULT_MTU     20

class DynamoteBLE : public Dynamote {

	public:
		DynamoteBLE(void);
		void begin(void (*fxn)(byte*, uint8_t));
		void loop(void);
		void setMtu(uint16_t _mtu);
		void onBleDisconnected(void);
		void onRemoteSendCharacteristic(uint8_t *data, uint16_t dataLength);
		void onRemoteRecordEnableCharacteristic(uint8_t *data);
		void onRemoteRecordEnableCharacteristic(bool data);

	private:
		uint16_t mtu = DEFAULT_MTU;
		unsigned long remoteSendTime = 0;
		String remoteCommandJsonString = "";
		bool sendRemoteCommandFlag = false;
		void sendRecordedCommandOverBle(RemoteCommand command);

		void (*sendDataToRemoteRecordCharacteristicFxn)(byte*, uint8_t);
};
		
#endif
#endif

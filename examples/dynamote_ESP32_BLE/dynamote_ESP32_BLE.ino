/******************************************************************************************************************
* includes
******************************************************************************************************************/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <DynamoteBLE.h>

// Change to your preffered advertising name.
// Suggestions include "Living Room", "Basement", etc.
#define BLE_ADVERTISING_NAME "dynamote"

/******************************************************************************************************************
* global
******************************************************************************************************************/
DynamoteBLE dynamote;

// service UUID
BLEServer* bleServer = NULL;
// remote send characteristic, used to send commands from the app
BLECharacteristic* remoteSendCharacteristic = NULL;
// remote record characteristic, used to record new commands and send to the app
BLECharacteristic* remoteRecordCharacteristic = NULL;
// remote record enable characteristic, used to enable/disable record mode
BLECharacteristic* remoteRecordEnableCharacteristic = NULL;

// Change to your preffered advertising name.
// Suggestions include "Living Room", "Basement", etc.
char bleAdvertisingName[] = BLE_ADVERTISING_NAME;

/******************************************************************************************************************
* bleServer callbacks
******************************************************************************************************************/
class bleServerCallbacks: public BLEServerCallbacks {

	void onConnect(BLEServer* bleServer) {
		Serial.println("Connected event");
	};

	void onDisconnect(BLEServer* bleServer) {
		dynamote.onBleDisconnected();
		Serial.println("Disconnected event");
		// re-advertise
		BLEDevice::startAdvertising();
	}
};

/******************************************************************************************************************
* remoteSendCharacteristic callbacks
******************************************************************************************************************/
class remoteSendCharacteristicCallbacks: public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic *pCharacteristic) {
		dynamote.onRemoteSendCharacteristic(pCharacteristic->getData(), pCharacteristic->getValue().length());
	}
};

/******************************************************************************************************************
* remoteRecordEnableCharacteristic callbacks
******************************************************************************************************************/
class remoteRecordEnableCharacteristicCallbacks: public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic *pCharacteristic) {
		dynamote.onRemoteRecordEnableCharacteristic(pCharacteristic->getData());
	}
};

/******************************************************************************************************************
* setup
******************************************************************************************************************/
void setup() {

  Serial.begin(9600);
  //while (!Serial);

  // Create the BLE Device
  BLEDevice::init(bleAdvertisingName);

  // Create the BLE Server
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new bleServerCallbacks());

  // Create the remote Service
  BLEService *remoteService = bleServer->createService(DYNAMOTE_REMOTE_SERVICE_UUID);

  // Create the Characteristics
  remoteSendCharacteristic = remoteService->createCharacteristic(
                              DYNAMOTE_REMOTE_SEND_CHARACTERISTIC_UUID,
                              BLECharacteristic::PROPERTY_READ      |
                              BLECharacteristic::PROPERTY_WRITE
                            );
  remoteRecordCharacteristic = remoteService->createCharacteristic(
                              DYNAMOTE_REMOTE_RECORD_CHARACTERISTIC_UUID,
                              BLECharacteristic::PROPERTY_READ      |
                              BLECharacteristic::PROPERTY_NOTIFY
                            );
  remoteRecordEnableCharacteristic = remoteService->createCharacteristic(
                              DYNAMOTE_REMOTE_RECORD_ENABLE_CHARACTERISTIC_UUID,
                              BLECharacteristic::PROPERTY_READ      |
                              BLECharacteristic::PROPERTY_WRITE
                            );

  // Create BLE Descriptors
  remoteSendCharacteristic->addDescriptor(new BLE2902());
  remoteRecordCharacteristic->addDescriptor(new BLE2902());
  remoteRecordEnableCharacteristic->addDescriptor(new BLE2902());

  // set characteristic callbacks
  remoteSendCharacteristic->setCallbacks(new remoteSendCharacteristicCallbacks());
  remoteRecordEnableCharacteristic->setCallbacks(new remoteRecordEnableCharacteristicCallbacks());

  // Start the service
  remoteService->start();

  // Start advertising
  BLEAdvertising *bleAdvertising = BLEDevice::getAdvertising();
  bleAdvertising->addServiceUUID(DYNAMOTE_REMOTE_SERVICE_UUID);
  bleAdvertising->setScanResponse(true);
  bleAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  Serial.println(("Bluetooth device active, waiting for connections..."));

  // DynamoteBLE does not handle BLE code directly, because different boards require different code.
  // You must provide a function that sends data over the remoteRecordCharacteristic for Dynamote to use.
  dynamote.begin(&sendDataToRemoteRecordCharacteristic);

  // This is optional.
  // You may provide your own function for responding to custom commands from Dynamote.
  dynamote.setCustomCommandHandlerFxn(&customCommandHandler);
}

/******************************************************************************************************************
* loop
******************************************************************************************************************/
void loop() {
  dynamote.loop();
  delay(1);
}

/******************************************************************************************************************
* sendDataToRemoteRecordCharacteristic
******************************************************************************************************************/
void sendDataToRemoteRecordCharacteristic(byte* data, uint8_t dataLength) {
  remoteRecordCharacteristic->setValue(data, dataLength);
  remoteRecordCharacteristic->notify();
}

/******************************************************************************************************************
* customCommandHandler
******************************************************************************************************************/
void customCommandHandler(RemoteCommand command) {
  
  /*
  *  Here you can add your own application code for the custom code handlers.
  *  This can be anything you want, not just IR remote specific.
  *  However, below is an example relating to IR remotes.
  *
  *  Some other possible ideas include:
  *  - toggle a relay
  *  - control an RGB light strip. Turn on/off, change brightness, change color, change pattern, etc.  
  */

  /*
  *  Example:
  *  Many TV's have a source button that brings up the source menu before actually changing the source with a second press.
  *  This can make it impossible to change the source with the Google assistant, because you cannot send it twice fast enough with voice.
  *  In the dynamote app, selecting a custom command AFTER recording an IR command still keeps the recorded IR command data,
  *  it just sets the useCustomCode flag to true. Therefore, you can still send an IR command within a custom code handler.
  *  We can use this to create a custom command that sends the IR command multiple times to solve the described issue, see below.
  */
  if (command.customCode == "send_IR_code_twice") {
    // verify that an IR command is present
    if (command.codeLength == 0) {
      return;
    }
    dynamote.sendRemoteCommand(command);
    delay(2000);
    dynamote.sendRemoteCommand(command);
  }
}

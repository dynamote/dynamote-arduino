/******************************************************************************************************************
* includes
******************************************************************************************************************/
#include <ArduinoBLE.h>
#include <DynamoteBLE.h>

// Change to your preffered advertising name.
// Suggestions include "Living Room", "Basement", etc.
#define BLE_ADVERTISING_NAME "dynamote"

/******************************************************************************************************************
* globals
******************************************************************************************************************/
DynamoteBLE dynamote;

#define CHARACTERISTIC_LENGTH                       512
// service UUID
BLEService remoteService(DYNAMOTE_REMOTE_SERVICE_UUID);
// remote send characteristic, used to send commands from the app
BLECharacteristic remoteSendCharacteristic(DYNAMOTE_REMOTE_SEND_CHARACTERISTIC_UUID, BLEWrite, CHARACTERISTIC_LENGTH);
// remote record characteristic, used to record new commands and send to the app
BLECharacteristic remoteRecordCharacteristic(DYNAMOTE_REMOTE_RECORD_CHARACTERISTIC_UUID, BLERead | BLENotify, CHARACTERISTIC_LENGTH);
// remote record enable characteristic, used to enable/disable record mode
BLEBoolCharacteristic remoteRecordEnableCharacteristic(DYNAMOTE_REMOTE_RECORD_ENABLE_CHARACTERISTIC_UUID, BLERead | BLEWrite);

char bleAdvertisingName[] = BLE_ADVERTISING_NAME;

/******************************************************************************************************************
* setup
******************************************************************************************************************/
void setup() {
  Serial.begin(9600);
  //while (!Serial);

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  // set the local name the peripheral advertises
  BLE.setLocalName(bleAdvertisingName);
  BLE.setDeviceName(bleAdvertisingName);

  // set the UUID for the service this peripheral advertises
  BLE.setAdvertisedService(remoteService);

  // add the characteristics to the service
  remoteService.addCharacteristic(remoteSendCharacteristic);
  remoteService.addCharacteristic(remoteRecordCharacteristic);
  remoteService.addCharacteristic(remoteRecordEnableCharacteristic);

  // add service
  BLE.addService(remoteService);

  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristic
  remoteSendCharacteristic.setEventHandler(BLEWritten, remoteSendCharacteristicWritten);
  remoteRecordEnableCharacteristic.setEventHandler(BLEWritten, remoteRecordEnableCharacteristicWritten);

  // set an initial value for the characteristics
  remoteSendCharacteristic.setValue(0);
  remoteRecordCharacteristic.setValue(0);
  remoteRecordEnableCharacteristic.setValue(0);

  // start advertising
  BLE.advertise();
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
  // poll for BLE events
  BLE.poll();
  dynamote.loop();
  delay(1);
}

/******************************************************************************************************************
* blePeripheralConnectHandler
******************************************************************************************************************/
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

/******************************************************************************************************************
* blePeripheralDisconnectHandler
******************************************************************************************************************/
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  
  dynamote.onBleDisconnected();

  // restart advertising
  BLE.stopAdvertise();
  BLE.advertise();
}

/******************************************************************************************************************
* remoteSendCharacteristicWritten
******************************************************************************************************************/
void remoteSendCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
  dynamote.onRemoteSendCharacteristic((uint8_t*)remoteSendCharacteristic.value(), remoteSendCharacteristic.valueLength());
}

/******************************************************************************************************************
* remoteRecordEnableCharacteristicWritten
******************************************************************************************************************/
void remoteRecordEnableCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
  dynamote.onRemoteRecordEnableCharacteristic(remoteRecordEnableCharacteristic.value());
}

/******************************************************************************************************************
* sendDataToRemoteRecordCharacteristic
******************************************************************************************************************/
void sendDataToRemoteRecordCharacteristic(byte* data, uint8_t dataLength) {
  remoteRecordCharacteristic.writeValue(data, dataLength);
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

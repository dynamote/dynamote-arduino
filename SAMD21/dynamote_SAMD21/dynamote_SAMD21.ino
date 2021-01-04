// define to enable BLE or WiFi functionality
//#define ENABLE_WIFI
#define ENABLE_BLE

// Only one can be enabled at a time
#if defined(ENABLE_WIFI) && defined(ENABLE_BLE)
#error "both WiFi and BLE cannot be defined at the same time"
#endif

/******************************************************************************************************************
* includes
******************************************************************************************************************/
#ifdef ENABLE_BLE
#include <ArduinoBLE.h>
#endif
#ifdef ENABLE_WIFI
#include <SPI.h>
#include <WiFiNINA.h>
#include "samd21-mqtt.h"
#include <WiFiUdp.h>
#include <ArduinoMDNS.h>                      // ArduinoMDNS https://github.com/arduino-libraries/ArduinoMDNS
#endif 
#include "device_settings.h"
#include "dynamote_SAMD21.h"
#include "custom_code_handlers.h"
#include <IRLibDecodeBase.h>                  // IRLib2 https://github.com/cyborg5/IRLib2
#include <IRLibSendBase.h>
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
#include <ArduinoJson.h>                      // https://arduinojson.org/

/******************************************************************************************************************
* globals
******************************************************************************************************************/
#ifdef ENABLE_BLE

// it cannot be guaranteed that all central devices (i.e. phones) support larger characteristic lengths
// since we cannot determine the connection mtu, we must assume the min length when sending to the central device
#define MIN_CHARACTERISTIC_LENGTH                   20
// central devices can still request higher mtu's and send data using them, so make the characteristics larger
#define CHARACTERISTIC_LENGTH                       512
// service UUID
BLEService remoteService("91193088-70ec-4eba-8a27-4802e6b4f81f");
// remote send characteristic, used to receive commands to send from the app
BLECharacteristic remoteSendCharacteristic("5e4d2bf6-29ec-4a1b-8649-259568488e7b", BLEWrite, CHARACTERISTIC_LENGTH);
// remote record characteristic, used to record new commands and send to the app
BLECharacteristic remoteRecordCharacteristic("608b70d9-5ee2-4380-a0f4-8a629578f19b", BLERead | BLENotify, CHARACTERISTIC_LENGTH);
// remote record enable characteristic, used to enable/disable record mode
BLEBoolCharacteristic remoteRecordEnableCharacteristic("37141628-d6d9-45bd-af90-e297a92b6953", BLERead | BLEWrite);

// It might take multiple packets to receive the remote command over BLE.
// This times the events and resets the command if it was incomplete, after not receiving a new packet after a while
unsigned long remoteSendTime = 0;
// Json string containing the serialized remote command
String remoteCommandJsonString = "";

#endif
#ifdef ENABLE_WIFI

const char* wifiSsid = SECRET_SSID;
const char* wifiPass = SECRET_PASS;
const char* domainName = DEVICE_NAME;
WiFiServer server(80);
WiFiUDP udp;
MDNS mdns(udp);

// exit RECEIVE remote state if client stops requesting it after a period of time
unsigned long remoteRecordRequestTime = 0;
// reset after receiving new assistant integration config, in order to apply it
bool resetAfterResponse = false;
// serialized json string containing the remote command to send
String recordedRemoteCommandJsonString = "";

#endif

IRsend remoteSender;
IRsendRaw remoteRawSender;
IRdecode remoteDecoder;
IRrecvPCI remoteReceiver(receiverPin);

enum RemoteStates {
  SEND,
  RECORD
};
RemoteStates remoteState = SEND;

/******************************************************************************************************************
* setup
******************************************************************************************************************/
void setup() {
  Serial.begin(9600);
  //while (!Serial);

  for (uint8_t t = 4; t > 0; t--)
  {
    Serial.println("[SETUP] BOOT WAIT " + String(t));
    Serial.flush();
    delay(1000);
  }

#ifdef ENABLE_WIFI
  setupWifi();
  setupCloudIoT();
#endif

#ifdef ENABLE_BLE
  setupBLE();
#endif
}

/******************************************************************************************************************
* loop
******************************************************************************************************************/
void loop() {
  
#ifdef ENABLE_BLE
  bleLoop();
#endif

#ifdef ENABLE_WIFI
  wifiLoop();
  mqttloop();
#endif

  if (remoteState == RECORD){
    getReceiverInput();
  }

  delay(1);
}

#ifdef ENABLE_WIFI

/******************************************************************************************************************
* setupWifi
******************************************************************************************************************/
void setupWifi() {
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to network named: ");
  Serial.println(wifiSsid);                   // print the network name (SSID);
  while (WiFi.status() != WL_CONNECTED) {
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.begin(wifiSsid, wifiPass);
    delay(3000);
    Serial.print(".");
  }
  Serial.println("connected");

  server.begin();                           // start the web server on port 80

  // set the hostname
  mdns.begin(WiFi.localIP(), domainName);

  printWifiStatus();                        // connected now, so print out the status
}

/******************************************************************************************************************
* wifiLoop
******************************************************************************************************************/
void wifiLoop(){

  mdns.run();

  // Check if WiFi needs to reconnect
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFI disconnected...");
    Serial.print("attempting to reconnect to network named: ");
    Serial.println(wifiSsid);                   // print the network name (SSID);
    while (WiFi.status() != WL_CONNECTED) {
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      WiFi.begin(wifiSsid, wifiPass);
      delay(3000);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected"); 
      // set the hostname
      mdns.begin(WiFi.localIP(), domainName); 
    }
  }

  else {

    WiFiClient client = server.available();   // listen for incoming clients

    if (client) {                             // if you get a client,
      String currentLine = "";                // make a String to hold incoming data from the client
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

              // the content of the HTTP response follows the header:
              // Send the recorded command to the client, if available
              // only if we are in remote RECORD mode (for recording commands)
              if (remoteState == RECORD) {
                if (recordedRemoteCommandJsonString != "") {
                  client.print(recordedRemoteCommandJsonString);
                  client.println("");
                  recordedRemoteCommandJsonString = "";
                }
              }

              // break out of the while loop:
              break;
            } 
            else {

              //
              // parse POST commands
              //
              if (currentLine.indexOf("POST /") != -1) {
                // remove the "POST /"
                String command = currentLine.substring(currentLine.indexOf("POST /") + 6);
                // remove anything after a space
                command = command.substring(0, command.indexOf(' '));

                //
                // Are we trying to send a remote command?
                //
                if (command.startsWith("sendRemoteCommand/")) {
                  command.replace("sendRemoteCommand/", "");
                  sendRemoteCommandFromWifi(command);
                }

                //
                // Are we trying to configure assistant integration?
                //
                if (command.startsWith("configureAssistantIntegration/")) {
                  command.replace("configureAssistantIntegration/", "");
                  configureAssistantIntegration(command);
                  resetAfterResponse = true;
                  Serial.println("Saved new Assistant integration config, resetting...");
                }
              }

              //
              // Parse GET commands
              //
              if (currentLine.indexOf("GET /") != -1) {
                // remove the "GET /"
                String command = currentLine.substring(currentLine.indexOf("GET /") + 5);
                // remove anything after a space
                command = command.substring(0, command.indexOf(' '));
                Serial.println(command);

                //
                // determine if a new recorded command is requested
                //
                if (command == "getRecordedCommand") {
                  if (remoteState != RECORD)
                    setRemoteState(RECORD);

                  // If we do not receive the next command within several seconds, we will go back to SEND state
                  remoteRecordRequestTime = millis();
                }
                //
                // determine if the client is done recording commands
                //
                else if (command == "getRecordedCommandDone") {
                  setRemoteState(SEND);
                }
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
  }

  if (resetAfterResponse == true)
    NVIC_SystemReset();

  // If we do not receive the next command within several seconds, we will go back to SEND state
  if (remoteRecordRequestTime != 0 && remoteRecordRequestTime + 5000 < millis()) {
    remoteRecordRequestTime = 0;
    if (remoteState != SEND)
      setRemoteState(SEND);
  }
}

/******************************************************************************************************************
* printWifiStatus
******************************************************************************************************************/
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print domain name
  Serial.print("Domain Name: ");
  Serial.print(domainName);
  Serial.println(".local");

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

/******************************************************************************************************************
* sendRemoteCommandFromWifi
******************************************************************************************************************/
void sendRemoteCommandFromWifi(String command) {

  // the command has been sent as a json string
  // this will parse it before sending the command
  RemoteCommand remoteCommand = RemoteCommand();
  DeserializationError error = deserializeJsonStringToRemoteCommand(command, &remoteCommand);

  if (!error) {
    if (remoteCommand.useCustomCode)
      parseCustomCommand(remoteCommand);
    else 
      sendRemoteCommand(remoteCommand);
  }
}

#endif

#ifdef ENABLE_BLE

/******************************************************************************************************************
* setupBLE
******************************************************************************************************************/
void setupBLE() {

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
}

/******************************************************************************************************************
* bleLoop
******************************************************************************************************************/
void bleLoop() {
  // poll for BLE events
  BLE.poll();
  // If we do not receive the rest of the remote command over BLE within 1 second, we assume it failed
  if (remoteSendTime != 0 && remoteSendTime + 1000 < millis()) {
    remoteSendTime = 0;
    remoteCommandJsonString = "";
    Serial.println("remote command cleared");
  }
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
  setRemoteState(SEND);

  // restart advertising
  BLE.stopAdvertise();
  BLE.advertise();
}

/******************************************************************************************************************
* remoteSendCharacteristicWritten
******************************************************************************************************************/
void remoteSendCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {

  Serial.println("remoteSendCharacteristic event");

  // If we do not receive the next value within 1 second, we will stop and assume it failed
  remoteSendTime = millis();

  for (uint16_t x = 0; x < remoteSendCharacteristic.valueLength(); x++) {
    remoteCommandJsonString += (char)remoteSendCharacteristic.value()[x];
  }

  // the command has been sent as a json string
  // this will parse it before sending the command
  // The command may not be fully complete until mutliple BLE events are received,
  // this will simply fail until that happens
  RemoteCommand remoteCommand = RemoteCommand();
  DeserializationError error = deserializeJsonStringToRemoteCommand(remoteCommandJsonString, &remoteCommand);
  
  if (!error) {
    if (remoteCommand.useCustomCode)
      parseCustomCommand(remoteCommand);
    else 
      sendRemoteCommand(remoteCommand);
    remoteSendTime = 0;
    remoteCommandJsonString = "";
  }
}

/******************************************************************************************************************
* remoteRecordEnableCharacteristicWritten
******************************************************************************************************************/
void remoteRecordEnableCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {

  Serial.print("remoteRecordEnableCharacteristic event, written: ");
  Serial.println(remoteRecordEnableCharacteristic.value());

  if (remoteRecordEnableCharacteristic.value()){
    setRemoteState(RECORD);
  }
  else {
    setRemoteState(SEND);
  }
}

/******************************************************************************************************************
* sendRecordedCommandOverBle
******************************************************************************************************************/
void sendRecordedCommandOverBle(RemoteCommand command) {

  //
  // serialize command to json string
  //
  String jsonString = "";
  serializeRemoteCommandToJsonString(command, jsonString);

  uint16_t index = 0;
  while(index < jsonString.length()) {

    uint8_t charactreristicLength = min(MIN_CHARACTERISTIC_LENGTH, jsonString.length()-index);
    byte characteristicArray[charactreristicLength] = {0};
    for (uint8_t x = 0; x < charactreristicLength; x++) 
      characteristicArray[x] = jsonString.charAt(index++);
    // write the array chunk over BLE
    remoteRecordCharacteristic.writeValue(characteristicArray, charactreristicLength);
    delay(3);
  }
}
#endif

/******************************************************************************************************************
* setRemoteState
******************************************************************************************************************/
void setRemoteState(RemoteStates state) {
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
* sendRemoteCommand
******************************************************************************************************************/
void sendRemoteCommand(RemoteCommand command) {
  
  Serial.print("Sending: ");
  if(command.codeProtocol == UNKNOWN) {
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
void getReceiverInput() {

  if (remoteReceiver.getResults()) {
    
    remoteDecoder.decode();

    RemoteCommand recordedRemoteCommand = RemoteCommand();
#ifdef ENABLE_WIFI
    recordedRemoteCommandJsonString = "";
#else
    String recordedRemoteCommandJsonString = "";
#endif

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

    serializeRemoteCommandToJsonString(recordedRemoteCommand, recordedRemoteCommandJsonString);

#ifdef ENABLE_BLE
    // write the recorded command to the ble characteristic
    // so listener devices can be notified of it
    sendRecordedCommandOverBle(recordedRemoteCommand);
#endif

    remoteReceiver.enableIRIn();
  }
}

/******************************************************************************************************************
* getSeperatedStringValue
******************************************************************************************************************/
String getSeperatedStringValue(String data, char separator, int index)
{
  // splits a string into an array, with a given delimeter, then returns the value of the specified index

  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

/******************************************************************************************************************
* deserializeJsonStringToRemoteCommand
******************************************************************************************************************/
DeserializationError deserializeJsonStringToRemoteCommand(String jsonString, RemoteCommand *command)
{
  // the WiFiClient seems to provide some characters as ascii.
  // we know which ones we will be sending with the json string.
  // its kind of annoying, but we can manually convert them.
  jsonString.replace("%5B", "[");
  jsonString.replace("%5D", "]");
  jsonString.replace("%7B", "{");
  jsonString.replace("%7D", "}");
  jsonString.replace("%22", "\"");

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
template <typename TDestination>
void serializeRemoteCommandToJsonString(RemoteCommand command, TDestination &destinationBuffer)
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
* printArray (for debugging)
******************************************************************************************************************/
void printArray(uint16_t* x, int length)
{
  for (int iCount = 0; iCount < length; iCount++)
  {
    Serial.print(x[iCount]);
    Serial.print(',');
  }
  Serial.println(' ');
}

/******************************************************************************************************************
* printByteArray (for debugging)
******************************************************************************************************************/
void printByteArray(byte* x, int length)
{
  for (int iCount = 0; iCount < length; iCount++)
  {
    Serial.print(x[iCount]);
    Serial.print(',');
  }
}

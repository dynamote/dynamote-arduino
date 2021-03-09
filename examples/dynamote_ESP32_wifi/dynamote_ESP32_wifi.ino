/******************************************************************************************************************
* includes
******************************************************************************************************************/
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DynamoteWiFi.h>

// change to the SSID and password of your WiFi network
#define SECRET_SSID "your_ssid"
#define SECRET_PASS "your_password"

// Change to your preffered remote hostname.
// Suggestions include "Living Room", "Basement", etc.
// You cannot have multiple devices on your WiFi network with the same name.
#define DEVICE_NAME "dynamote"

/******************************************************************************************************************
* global
******************************************************************************************************************/
DynamoteWiFi dynamote;

const char* wifiSsid = SECRET_SSID;
const char* wifiPass = SECRET_PASS;
const char* domainName = DEVICE_NAME;

/******************************************************************************************************************
* setup
******************************************************************************************************************/
void setup() {

  Serial.begin(9600);
  //while (!Serial);

  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to network named: ");
  Serial.println(wifiSsid);                   // print the network name (SSID);
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(wifiSsid, wifiPass);
    delay(3000);
    Serial.print(".");
  }
  Serial.println("connected");

  // Set up mDNS, this allows you to communicate to the device using the domain name, instead of its IP address
  if (!MDNS.begin(domainName)) {
    Serial.println("Error setting up MDNS responder!");
    while(1)
      delay(1000);
  }

  // connected now, so print out the status
  printWifiStatus();

  dynamote.begin();

  // This is optional.
  // You may provide your own function for responding to custom commands from Dynamote.
  dynamote.setCustomCommandHandlerFxn(&customCommandHandler);
}

/******************************************************************************************************************
* loop
******************************************************************************************************************/
void loop() {
  
  // Check if WiFi needs to reconnect
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFI disconnected...");
    Serial.print("attempting to reconnect to network named: ");
    Serial.println(wifiSsid);                   // print the network name (SSID);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(wifiSsid, wifiPass);
      delay(3000);
      Serial.print(".");
    }
    Serial.println("Connected"); 
    // set the hostname
    MDNS.begin(domainName);
  }
    
  dynamote.loop();
  delay(1);
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
  Serial.println(" dBm\n");
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

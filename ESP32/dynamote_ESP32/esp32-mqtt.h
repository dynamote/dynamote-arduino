#ifndef __ESP32_MQTT_H__
#define __ESP32_MQTT_H__

/******************************************************************************************************************
* includes
******************************************************************************************************************/
#include <MQTT.h>                             // https://github.com/256dpi/arduino-mqtt
#include <CloudIoTCore.h>                     // https://github.com/GoogleCloudPlatform/google-cloud-iot-arduino
#include <CloudIoTCoreMqtt.h>
#include <WiFiClientSecure.h>
#include "dynamote_ESP32.h"
#include <Preferences.h>

/******************************************************************************************************************
* globals
******************************************************************************************************************/
Client *netClient;
CloudIoTCoreDevice *iotDevice;
CloudIoTCoreMqtt *mqtt;
MQTTClient *mqttClient;
unsigned long iat = 0;
String jwt;

// Configuration for NTP
const char* ntp_primary = "pool.ntp.org";
const char* ntp_secondary = "time.nist.gov";

// Time (seconds) to expire token += 20 minutes for drift
const int jwt_exp_secs = 3600*22; // Maximum 24H (3600*24)

#define MAX_ID_LENGTH 255
typedef struct {
  char project_id[MAX_ID_LENGTH];
  uint8_t project_id_length;
  char location[MAX_ID_LENGTH];
  uint8_t location_length;
  char registry_id[MAX_ID_LENGTH];
  uint8_t registry_id_length;
  char device_id[MAX_ID_LENGTH];
  uint8_t device_id_length;
  char private_key_str[MAX_ID_LENGTH];
  uint8_t private_key_str_length;
} iotConfig_t;
iotConfig_t iotConfig;
char default_id[] = "default";
Preferences prefs;

char *project_id;
char *location;
char *registry_id;
char *device_id;
char *private_key_str;

/******************************************************************************************************************
* messageReceived
******************************************************************************************************************/
void messageReceived(String &topic, String &payload) {

  // MQTT messages will be received here

  // only react to commands
  String device_id_string = String(device_id);
  if (topic != "/devices/" + device_id_string + "/commands")
    return;

  String command = payload;
  Serial.println("Incoming MQTT command: - " + payload);
  sendRemoteCommandFromWifi(command);
}

/******************************************************************************************************************
* getJwt
******************************************************************************************************************/
String getJwt() {
  iat = time(nullptr);
  Serial.println("Refreshing JWT credential");
  jwt = iotDevice->createJWT(iat, jwt_exp_secs);
  return jwt;
}

/******************************************************************************************************************
* setupCloudIoT
******************************************************************************************************************/
void setupCloudIoT() {

  //
  // Get the iot config from EEPROM, if it exists
  //
  prefs.begin("iotConfig");
  size_t prefsLength = prefs.getBytesLength("iotConfig");
  if (prefsLength == sizeof(iotConfig_t)) {
    char buffer[prefsLength];
    prefs.getBytes("iotConfig", buffer, prefsLength);
    iotConfig_t *iotConfigTemp = (iotConfig_t *) buffer;
    // recall project id
    iotConfig.project_id_length = iotConfigTemp->project_id_length;
    for (int x = 0; x < iotConfig.project_id_length; x++)
      iotConfig.project_id[x] = iotConfigTemp->project_id[x];
    // recall location
    iotConfig.location_length = iotConfigTemp->location_length;
    for (int x = 0; x < iotConfig.location_length; x++)
      iotConfig.location[x] = iotConfigTemp->location[x];
    // recall registry_id
    iotConfig.registry_id_length = iotConfigTemp->registry_id_length;
    for (int x = 0; x < iotConfig.registry_id_length; x++)
      iotConfig.registry_id[x] = iotConfigTemp->registry_id[x];
    // recall device_id
    iotConfig.device_id_length = iotConfigTemp->device_id_length;
    for (int x = 0; x < iotConfig.device_id_length; x++)
      iotConfig.device_id[x] = iotConfigTemp->device_id[x];
    // recall private key str
    iotConfig.private_key_str_length = iotConfigTemp->private_key_str_length;
    for (int x = 0; x < iotConfig.private_key_str_length; x++)
      iotConfig.private_key_str[x] = iotConfigTemp->private_key_str[x];
  }
  else {
    // set project id
    iotConfig.project_id_length = sizeof(default_id[0])*sizeof(default_id);
    for (int x = 0; x < iotConfig.project_id_length; x++)
      iotConfig.project_id[x] = default_id[x];
    // set location
    iotConfig.location_length = sizeof(default_id[0])*sizeof(default_id);
    for (int x = 0; x < iotConfig.location_length; x++)
      iotConfig.location[x] = default_id[x];
    // set registry id
    iotConfig.registry_id_length = sizeof(default_id[0])*sizeof(default_id);
    for (int x = 0; x < iotConfig.registry_id_length; x++)
      iotConfig.registry_id[x] = default_id[x];
    // set device id
    iotConfig.device_id_length = sizeof(default_id[0])*sizeof(default_id);
    for (int x = 0; x < iotConfig.device_id_length; x++)
      iotConfig.device_id[x] = default_id[x];
    // set private key str
    iotConfig.private_key_str_length = sizeof(default_id[0])*sizeof(default_id);
    for (int x = 0; x < iotConfig.private_key_str_length; x++)
      iotConfig.private_key_str[x] = default_id[x];
    // save
    prefs.putBytes("iotConfig", &iotConfig, sizeof(iotConfig_t));
  }
  
  project_id = iotConfig.project_id;
  location = iotConfig.location;
  registry_id = iotConfig.registry_id;
  device_id = iotConfig.device_id;
  private_key_str = iotConfig.private_key_str;

  if (strcmp(project_id, default_id) == 0) {
    Serial.println("Assistant integration is not configured, skipping setup.");
    return;
  }

  configTime(0, 0, ntp_primary, ntp_secondary);
  Serial.println("Waiting on time sync...");
  while (time(nullptr) < 1510644967) {
    delay(10);
  }

  iotDevice = new CloudIoTCoreDevice(project_id, location, registry_id, device_id, private_key_str);

  netClient = new WiFiClientSecure();
  mqttClient = new MQTTClient(512);
  mqttClient->setOptions(180, true, 1000); // keepAlive, cleanSession, timeout
  mqtt = new CloudIoTCoreMqtt(mqttClient, netClient, iotDevice);
  mqtt->setUseLts(true);
  mqtt->startMQTT();
}

/******************************************************************************************************************
* mqttloop
******************************************************************************************************************/
void mqttloop() {

  if (strcmp(project_id, default_id) == 0)
    return;

  mqtt->loop();
  delay(10);

  if (!mqttClient->connected()) {
    static unsigned long mqttDisconnectTime = 0;
    if (mqttDisconnectTime == 0)
      mqttDisconnectTime = millis();
    if (WiFi.status() == WL_CONNECTED && mqttDisconnectTime + 5000 < millis()) {
      mqttClient->disconnect();
      //ESP.wdtDisable();
      mqtt->mqttConnectAsync();
      //ESP.wdtEnable(0);
      mqttDisconnectTime = 0;
    }
  }
}

/******************************************************************************************************************
* configureAssistantIntegration
******************************************************************************************************************/
void configureAssistantIntegration(String configurationString) {

  // first, seperate each piece of the configuration string
  String newProjectIdString = getSeperatedStringValue(configurationString, '/', 0);
  String newLocationString = getSeperatedStringValue(configurationString, '/', 1);
  String newRegistryIdString = getSeperatedStringValue(configurationString, '/', 2);
  String newDeviceIdString = getSeperatedStringValue(configurationString, '/', 3);
  String newPrivateKeyStrString = getSeperatedStringValue(configurationString, '/', 4);

  char newProjectId[newProjectIdString.length()+1];
  char newLocation[newLocationString.length()+1];
  char newRegistryId[newRegistryIdString.length()+1];
  char newDeviceId[newDeviceIdString.length()+1];
  char newPrivateKeyStr[newPrivateKeyStrString.length()+1];

  newProjectIdString.toCharArray(newProjectId, newProjectIdString.length()+1);
  newLocationString.toCharArray(newLocation, newLocationString.length()+1);
  newRegistryIdString.toCharArray(newRegistryId, newRegistryIdString.length()+1);
  newDeviceIdString.toCharArray(newDeviceId, newDeviceIdString.length()+1);
  newPrivateKeyStrString.toCharArray(newPrivateKeyStr, newPrivateKeyStrString.length()+1);

  iotConfig.project_id_length = newProjectIdString.length()+1;
  for(int x = 0; x < iotConfig.project_id_length; x++)
    iotConfig.project_id[x] = newProjectId[x];
  iotConfig.location_length = newLocationString.length()+1;
  for(int x = 0; x < iotConfig.location_length; x++)
    iotConfig.location[x] = newLocation[x];
  iotConfig.registry_id_length = newRegistryIdString.length()+1;
  for(int x = 0; x < iotConfig.registry_id_length; x++)
    iotConfig.registry_id[x] = newRegistryId[x];
  iotConfig.device_id_length = newDeviceIdString.length()+1;
  for(int x = 0; x < iotConfig.device_id_length; x++)
    iotConfig.device_id[x] = newDeviceId[x];
  iotConfig.private_key_str_length = newPrivateKeyStrString.length()+1;
  for(int x = 0; x < iotConfig.private_key_str_length; x++)
    iotConfig.private_key_str[x] = newPrivateKeyStr[x];

  prefs.putBytes("iotConfig", &iotConfig, sizeof(iotConfig_t));
}

#endif
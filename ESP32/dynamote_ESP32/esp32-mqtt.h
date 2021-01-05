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
    strcpy(iotConfig.project_id, iotConfigTemp->project_id);
    // recall location
    iotConfig.location_length = iotConfigTemp->location_length;
    strcpy(iotConfig.location, iotConfigTemp->location);
    // recall registry_id
    iotConfig.registry_id_length = iotConfigTemp->registry_id_length;
    strcpy(iotConfig.registry_id, iotConfigTemp->registry_id);
    // recall device_id
    iotConfig.device_id_length = iotConfigTemp->device_id_length;
    strcpy(iotConfig.device_id, iotConfigTemp->device_id);
    // recall private key str
    iotConfig.private_key_str_length = iotConfigTemp->private_key_str_length;
    strcpy(iotConfig.private_key_str, iotConfigTemp->private_key_str);
  }
  else {
    // set defaults
    iotConfig.project_id_length = sizeof(default_id[0])/sizeof(default_id);
    strcpy(iotConfig.project_id, default_id);
    iotConfig.location_length = sizeof(default_id[0])/sizeof(default_id);
    strcpy(iotConfig.location, default_id);
    iotConfig.registry_id_length = sizeof(default_id[0])/sizeof(default_id);
    strcpy(iotConfig.registry_id, default_id);
    iotConfig.device_id_length = sizeof(default_id[0])/sizeof(default_id);
    strcpy(iotConfig.device_id, default_id);
    iotConfig.private_key_str_length = sizeof(default_id[0])/sizeof(default_id);
    strcpy(iotConfig.private_key_str, default_id);
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

  iotConfig.project_id_length = sizeof(newProjectId)/sizeof(newProjectId[0]);
  strcpy(iotConfig.project_id, newProjectId);
  iotConfig.location_length = sizeof(newLocation)/sizeof(newLocation[0]);
  strcpy(iotConfig.location, newLocation);
  iotConfig.registry_id_length = sizeof(newRegistryId)/sizeof(newRegistryId[0]);
  strcpy(iotConfig.registry_id, newRegistryId);
  iotConfig.device_id_length = sizeof(newDeviceId)/sizeof(newDeviceId[0]);
  strcpy(iotConfig.device_id, newDeviceId);
  iotConfig.private_key_str_length = sizeof(newPrivateKeyStr)/sizeof(newPrivateKeyStr[0]);
  strcpy(iotConfig.private_key_str, newPrivateKeyStr);

  prefs.putBytes("iotConfig", &iotConfig, sizeof(iotConfig_t));
}

#endif
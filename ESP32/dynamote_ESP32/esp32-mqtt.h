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
#include <ArduinoJson.h>

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

#define CONNECT_MAX_BACKOFF_MS 60000

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

  //
  // determine if a mqtt reconnect is required
  //

  // Do nothing if already connected.
  if (mqttClient->connected()) {
      return;
  }

  // Init the backoff index.
  static int backoff_index = 0;
  // Init the last connect time.
  static unsigned long last_connect_t_ms = 0;
  // Init backoff delay.
  static uint32_t backoff_ms = 0;

  // Return if delay has not expired.
  if ((millis() - last_connect_t_ms) < backoff_ms) {
    return;
  }

  // return if wifi is not connected
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  // Set the connect ping.
  last_connect_t_ms = millis();

  // Attempt connect
  mqttClient->disconnect();
  mqtt->mqttConnectAsync();

  // If connection fails delay before trying again.
  if (mqttClient->connected()) {

    // Reset the backoff index.
    backoff_index = 0;
    // Reset the last ping time.
    last_connect_t_ms = 0;
    // Reset backoff delay.
    backoff_ms = 0;

  } else {
    // Compute the backoff delay.
    backoff_ms = min(int(pow(2, backoff_index++)) * 1000 + int(random(1000)), CONNECT_MAX_BACKOFF_MS);

    // Log the delay.
    Serial.print("Failed to connect. Trying again after ");
    Serial.print(backoff_ms);
    Serial.println("ms");
  }
}

/******************************************************************************************************************
* configureAssistantIntegration
******************************************************************************************************************/
void configureAssistantIntegration(String configurationJson) {

  char jsonStringCharArray[configurationJson.length()+1];
  configurationJson.toCharArray(jsonStringCharArray, configurationJson.length()+1);

  StaticJsonDocument<500> jsonDoc;                  // <- plenty of size for this
  DeserializationError deserializeStatus = deserializeJson(jsonDoc, jsonStringCharArray);

  if (deserializeStatus) {
    Serial.println("Error, could not parse new Assistnat configuration settings");
    return;
  }

  const char* newProjectId = jsonDoc["projectId"];
  const char* newLocation = jsonDoc["location"];
  const char* newRegistryId = jsonDoc["registryId"];
  const char* newDeviceId = jsonDoc["deviceId"];
  const char* newPrivateKeyStr = jsonDoc["pvtKeyString"];

  iotConfig.project_id_length = strlen(newProjectId);
  strcpy(iotConfig.project_id, newProjectId);
  iotConfig.location_length = strlen(newLocation);
  strcpy(iotConfig.location, newLocation);
  iotConfig.registry_id_length = strlen(newRegistryId);
  strcpy(iotConfig.registry_id, newRegistryId);
  iotConfig.device_id_length = strlen(newDeviceId);
  strcpy(iotConfig.device_id, newDeviceId);
  iotConfig.private_key_str_length = strlen(newPrivateKeyStr);
  strcpy(iotConfig.private_key_str, newPrivateKeyStr);

  prefs.putBytes("iotConfig", &iotConfig, sizeof(iotConfig_t));
}

#endif

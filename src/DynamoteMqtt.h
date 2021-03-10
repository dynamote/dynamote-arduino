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

#if !defined(DYNAMOTEMQTT_H) && !defined(DYNAMOTE_BLE)
#define DYNAMOTEMQTT_H

/******************************************************************************************************************
* includes
******************************************************************************************************************/
#include <DynamoteWiFi.h>
#include <WiFi.h>
#include <MQTT.h>                             // https://github.com/256dpi/arduino-mqtt
#include <CloudIoTCore.h>                     // https://github.com/GoogleCloudPlatform/google-cloud-iot-arduino
#include <CloudIoTCoreMqtt.h>
#if defined(__DYNAMOTE_ESP32__)
#include <WiFiClientSecure.h>
#include <Preferences.h>
#endif
#if defined(__DYNAMOTE_SAMD21__)
#include <FlashStorage.h>                     // FlashStorage https://github.com/cmaglie/FlashStorage
#endif
#include <ArduinoJson.h>                      // https://arduinojson.org/

#define CONNECT_MAX_BACKOFF_MS 			      60000

#if defined(__DYNAMOTE_ESP32__)
const char *root_cert =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIErjCCA5agAwIBAgIQW+5sTJWKajts11BgHkBRwjANBgkqhkiG9w0BAQsFADBU\n"
	"MQswCQYDVQQGEwJVUzEeMBwGA1UEChMVR29vZ2xlIFRydXN0IFNlcnZpY2VzMSUw\n"
	"IwYDVQQDExxHb29nbGUgSW50ZXJuZXQgQXV0aG9yaXR5IEczMB4XDTE5MDYxMTEy\n"
	"MzE1OVoXDTE5MDkwMzEyMjAwMFowbTELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNh\n"
	"bGlmb3JuaWExFjAUBgNVBAcMDU1vdW50YWluIFZpZXcxEzARBgNVBAoMCkdvb2ds\n"
	"ZSBMTEMxHDAaBgNVBAMME21xdHQuZ29vZ2xlYXBpcy5jb20wggEiMA0GCSqGSIb3\n"
	"DQEBAQUAA4IBDwAwggEKAoIBAQDHuQUoDZWl2155WvaQ9AmhTRNC+mHassokdQK7\n"
	"NxkZVZfrS8EhRkZop6SJGHdvozBP3Ko3g1MgGIZFzqb5fRohkRKB6mteHHi/W7Uo\n"
	"7d8+wuTTz3llUZ2gHF/hrXFJfztwnaZub/KB+fXwSqWgMyo1EBme4ULV0rQZGFu6\n"
	"7U38HK+mFRbeJkh1SDOureI2dxkC4ACGiqWfX/vSyzpZkWGRuxK2F5cnBHqRbcKs\n"
	"OfmYyUuxZjGah+fC5ePgDbAntLUuYNppkdgT8yt/13ae/V7+rRhKOZC4q76HBEaQ\n"
	"4Wn5UC+ShVaAGuo7BtfoIFSyZi8/DU2eTQcHWewIXU6V5InhAgMBAAGjggFhMIIB\n"
	"XTATBgNVHSUEDDAKBggrBgEFBQcDATA4BgNVHREEMTAvghNtcXR0Lmdvb2dsZWFw\n"
	"aXMuY29tghhtcXR0LW10bHMuZ29vZ2xlYXBpcy5jb20waAYIKwYBBQUHAQEEXDBa\n"
	"MC0GCCsGAQUFBzAChiFodHRwOi8vcGtpLmdvb2cvZ3NyMi9HVFNHSUFHMy5jcnQw\n"
	"KQYIKwYBBQUHMAGGHWh0dHA6Ly9vY3NwLnBraS5nb29nL0dUU0dJQUczMB0GA1Ud\n"
	"DgQWBBSKWpFfG/yH1dkkJT05y/ZnRm/M4DAMBgNVHRMBAf8EAjAAMB8GA1UdIwQY\n"
	"MBaAFHfCuFCaZ3Z2sS3ChtCDoH6mfrpLMCEGA1UdIAQaMBgwDAYKKwYBBAHWeQIF\n"
	"AzAIBgZngQwBAgIwMQYDVR0fBCowKDAmoCSgIoYgaHR0cDovL2NybC5wa2kuZ29v\n"
	"Zy9HVFNHSUFHMy5jcmwwDQYJKoZIhvcNAQELBQADggEBAKMoXHxmLI1oKnraV0tL\n"
	"NzznlVnle4ljS/pqNI8LUM4/5QqD3qGqnI4fBxX1l+WByCitbTiNvL2KRNi9xau5\n"
	"oqvsuSVkjRQxky2eesjkdrp+rrxTwFhQ6NAbUeZgUV0zfm5XZE76kInbcukwXxAx\n"
	"lneyQy2git0voUWTK4mipfCU946rcK3+ArcanV7EDSXbRxfjBSRBD6K+XGUhIPHW\n"
	"brk0v1wzED1RFEHTdzLAecU50Xwic6IniM3B9URfSOmjlBRebg2sEVQavMHbzURg\n"
	"94aDC+EkNlHh3pOmQ/V89MBiF1xDHbZZ1gB0GszYKPHec9omSwQ5HbIDV3uf3/DQ\n"
	"his=\n"
	"-----END CERTIFICATE-----\n";
#endif

/******************************************************************************************************************
* typedefs
******************************************************************************************************************/
#define MAX_ID_LENGTH 255
typedef struct {
  char project_id[MAX_ID_LENGTH] = {0};
  char location[MAX_ID_LENGTH] = {0};
  char registry_id[MAX_ID_LENGTH] = {0};
  char device_id[MAX_ID_LENGTH] = {0};
  char private_key_str[MAX_ID_LENGTH] = {0};
  boolean enabled = false;
#if defined(__DYNAMOTE_SAMD21__)
  boolean valid = true;
#endif
} mqttConfig_t;
mqttConfig_t mqttConfig;

/******************************************************************************************************************
* globals
******************************************************************************************************************/
// Configuration for NTP
const char* ntp_primary = "pool.ntp.org";
const char* ntp_secondary = "time.nist.gov";

// Time (seconds) to expire token += 20 minutes for drift
const int jwt_exp_secs = 3600*22; // Maximum 24H (3600*24)

Client *netClient;
CloudIoTCoreDevice *iotDevice;
CloudIoTCoreMqtt *mqtt;
MQTTClient *mqttClient;
unsigned long iat = 0;
String jwt;

#if defined(__DYNAMOTE_ESP32__)
Preferences prefs;
#endif
#if defined(__DYNAMOTE_SAMD21__)
FlashStorage(mqttConfig_flash_store, mqttConfig_t);
#endif

DynamoteWiFi *dynamotePtr = nullptr;

/******************************************************************************************************************
* messageReceived
******************************************************************************************************************/
void messageReceived(String &topic, String &payload) {

  // MQTT messages will be received here

  // only react to commands
  String device_id_string = String(&mqttConfig.device_id[0]);
  if (topic != "/devices/" + device_id_string + "/commands")
    return;

  String command = payload;
  Serial.println("Incoming MQTT command: - " + payload);
  dynamotePtr->sendJsonRemoteCommand(command);
}

/******************************************************************************************************************
* getJwt
******************************************************************************************************************/
String getJwt(void) {
#if defined(__DYNAMOTE_ESP32__)  
  iat = time(nullptr);
#elif defined(__DYNAMOTE_SAMD21__)
  iat = WiFi.getTime();
#endif
  Serial.println("Refreshing JWT credential");
  jwt = iotDevice->createJWT(iat, jwt_exp_secs);
  return jwt;
}

/******************************************************************************************************************
* setupMqtt
******************************************************************************************************************/
void setupMqtt(DynamoteWiFi *_dynamote) {

  dynamotePtr = _dynamote;

  //
  // Get the iot config from NVS, if it exists
  //
#if defined(__DYNAMOTE_ESP32__)
  prefs.begin("mqttConfig");
  size_t prefsLength = prefs.getBytesLength("mqttConfig");
  if (prefsLength == sizeof(mqttConfig_t)) {
    char buffer[prefsLength];
    prefs.getBytes("mqttConfig", buffer, prefsLength);
    mqttConfig_t *mqttConfigTemp = (mqttConfig_t *) buffer;
    memcpy(&mqttConfig, mqttConfigTemp, prefsLength);
  }
  else {
    // set default value in storage
    prefs.putBytes("mqttConfig", &mqttConfig, sizeof(mqttConfig_t));
  }
#endif
#if defined(__DYNAMOTE_SAMD21__)
  mqttConfig = mqttConfig_flash_store.read();
  if (mqttConfig.valid == false) {
    // set default values in storage
    mqttConfig_flash_store.write(mqttConfig);
  }
#endif

  //
  // skip if MQTT has not been configured
  //
  if (mqttConfig.enabled == false) {
    Serial.println("MQTT is not configured, skipping setup.");
    return;
  }

  Serial.println("Waiting on time sync...");
#if defined(__DYNAMOTE_ESP32__)
  configTime(0, 0, ntp_primary, ntp_secondary);
  while (time(nullptr) < 1510644967) {
#elif defined(__DYNAMOTE_SAMD21__)
  while (WiFi.getTime() < 1510644967) {
#endif
    delay(10);
  }

  iotDevice = new CloudIoTCoreDevice(&mqttConfig.project_id[0], 
                                      &mqttConfig.location[0], 
                                      &mqttConfig.registry_id[0], 
                                      &mqttConfig.device_id[0], 
                                      &mqttConfig.private_key_str[0]);

#if defined(__DYNAMOTE_ESP32__)
  netClient = new WiFiClientSecure();
#elif defined(__DYNAMOTE_SAMD21__)
  netClient = new WiFiSSLClient();
#endif
  mqttClient = new MQTTClient(512);
  mqttClient->setOptions(180, true, 1000); // keepAlive, cleanSession, timeout
  mqtt = new CloudIoTCoreMqtt(mqttClient, netClient, iotDevice);
  mqtt->setUseLts(true);
  mqtt->startMQTT();
}

/******************************************************************************************************************
* mqttloop
******************************************************************************************************************/
void mqttloop(void) {

  if (mqttConfig.enabled == false)
    return;
  
  //
  // determine if a mqtt reconnect is required
  //

  // Do nothing if already connected.
  if (mqttClient->connected()) {
		mqtt->loop();
		delay(10);
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
* configureMqtt
******************************************************************************************************************/
void configureMqtt(String configurationJson) {

  char jsonStringCharArray[configurationJson.length()+1];
  configurationJson.toCharArray(jsonStringCharArray, configurationJson.length()+1);

  StaticJsonDocument<500> jsonDoc;                  // <- plenty of size for this
  DeserializationError deserializeStatus = deserializeJson(jsonDoc, jsonStringCharArray);

  if (deserializeStatus) {
    Serial.println("Error, could not parse new MQTT configuration settings");
    return;
  }

	//
  // apply the new settings
  //

  const char* newProjectId = jsonDoc["projectId"];
  const char* newLocation = jsonDoc["location"];
  const char* newRegistryId = jsonDoc["registryId"];
  const char* newDeviceId = jsonDoc["deviceId"];
  const char* newPrivateKeyStr = jsonDoc["pvtKeyString"];
  strcpy(mqttConfig.project_id, newProjectId);
  strcpy(mqttConfig.location, newLocation);
  strcpy(mqttConfig.registry_id, newRegistryId);
  strcpy(mqttConfig.device_id, newDeviceId);
  strcpy(mqttConfig.private_key_str, newPrivateKeyStr);


  if (mqttClient->connected()) {
    mqttClient->disconnect();
    delay(10);
  }

  iotDevice->setProjectId(mqttConfig.project_id);
  iotDevice->setLocation(mqttConfig.location);
  iotDevice->setRegistryId(mqttConfig.registry_id);
  iotDevice->setDeviceId(mqttConfig.device_id);
  iotDevice->setPrivateKey(mqttConfig.private_key_str);

  //
  // set up MQTT if it was not previously done
  //
  if (mqttConfig.enabled == false) {

    mqttConfig.enabled = true;
    
    Serial.println("Waiting on time sync...");
#if defined(__DYNAMOTE_ESP32__)
    configTime(0, 0, ntp_primary, ntp_secondary);
    while (time(nullptr) < 1510644967) {
#elif defined(__DYNAMOTE_SAMD21__)
    while (WiFi.getTime() < 1510644967) {
#endif
      delay(10);
    }

    iotDevice = new CloudIoTCoreDevice(&mqttConfig.project_id[0], 
                                        &mqttConfig.location[0], 
                                        &mqttConfig.registry_id[0], 
                                        &mqttConfig.device_id[0], 
                                        &mqttConfig.private_key_str[0]);

#if defined(__DYNAMOTE_ESP32__)
    netClient = new WiFiClientSecure();
#elif defined(__DYNAMOTE_SAMD21__)
    netClient = new WiFiSSLClient();
#endif
    mqttClient = new MQTTClient(512);
    mqttClient->setOptions(180, true, 1000); // keepAlive, cleanSession, timeout
    mqtt = new CloudIoTCoreMqtt(mqttClient, netClient, iotDevice);
    mqtt->setUseLts(true);
    mqtt->startMQTT();
  }

	//
  // save the new settings
  //

	#if defined(__DYNAMOTE_ESP32__)
  prefs.putBytes("mqttConfig", &mqttConfig, sizeof(mqttConfig_t));
#elif defined(__DYNAMOTE_SAMD21__)
  mqttConfig_flash_store.write(mqttConfig);
#endif
}

#endif

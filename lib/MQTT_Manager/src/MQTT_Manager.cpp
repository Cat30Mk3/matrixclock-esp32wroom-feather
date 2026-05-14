#include "MQTT_Manager.h"
#include "globals.h"
#include "JSON_Handler.h"
#include "Ticker_Manager.h"

ICACHE_RAM_ATTR void callback(char* topic, byte* payload, unsigned int length) {
  char strPayload[1536];
  char compareMsg[100];

  int cmndIndex;
  boolean cmndValid = false;
  boolean payloadFound = false;

  mqttCallbackInprogress = true;

  Serial.print("MQTT Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    strPayload[i] = (char)payload[i];
  }
  parola.synchZoneStart();
  parola.displayAnimate();

  strPayload[length] = '\0';
  if (strlen(strPayload) > 0)payloadFound = true;

  for (int mqttDeviceIndex = 1; mqttDeviceIndex < 3; mqttDeviceIndex++) {
    sprintf(compareMsg, "%s/%s/%s", "tele", configDb.mqttDeviceName[mqttDeviceIndex], "Temp");

    if (strstr(topic, compareMsg) != NULL) {
      cmndValid = true;

      if (parseJSONPayloadVer3(mqttDeviceIndex, strPayload) == 0) {
        mqttCallbackInprogress = false;
        return;
      }
    }
  }
  mqttCallbackInprogress = false;
}

void publishLampState(void) {
  char topicBuild[50];
  lamp_1_State = digitalRead(LAMP_PIN);
  strlcpy(topicBuild, configDb.lampStatTopic, sizeof(topicBuild));
  strcat(topicBuild, "/POWER");
  Serial.print("responding with message:");
  Serial.print(topicBuild);
  Serial.print("  payload:");
  Serial.println(lamp_1_State ? "ON" : "OFF");
  mqttClient.publish(topicBuild, lamp_1_State ? "ON" : "OFF");
}

bool newMqttConnect(void) {
  char topicBuild[100];
  if (WiFi.status() != WL_CONNECTED)return false;
  if (!mqttClient.connected()) {
    digitalWrite(BLU_LED_PIN, LOW);
    int mqttState = mqttClient.state();
    Serial.print("mqtt disconnected status number:");
    Serial.println(mqttState);

    mqttClient.setServer(configDb.mqttServer, 1883);
    mqttClient.setCallback(callback);
    mqttClient.setBufferSize(1536);
    for (int i = 1; i <= 5; i++) {
      Serial.print("[newMqttConnect] mqtt not connected - attempt #");
      Serial.println(i);
      char clientId[40];
      if (strlen(configDb.mqttPassword) == 0) {
        sprintf(clientId, "%s-%04X", configDb.mqttClientId, random(0xffff));
      }
      else {
        strlcpy(clientId, configDb.mqttClientId, sizeof(clientId));
      }
      Serial.println("MQTT Credentials:");
      Serial.print("    clientId:"); Serial.println(clientId);
      Serial.print("  mqttUserId:"); Serial.println(configDb.mqttUserId);
      Serial.print("mqttPassword:"); Serial.println(configDb.mqttPassword);

      if (mqttClient.connect(clientId, configDb.mqttUserId, configDb.mqttPassword )) {
        strlcpy(topicBuild, "stat/", sizeof(topicBuild));
        strcat(topicBuild, "matrixClock/");
        strcat(topicBuild, "status");
        Serial.print("publishing ONLINE to:");
        Serial.println(topicBuild);
        mqttClient.publish(topicBuild, "ONLINE");

        for (int mqttDeviceIndex = 1; mqttDeviceIndex < 3; mqttDeviceIndex++) {
          strlcpy(topicBuild, "tele/", sizeof(topicBuild));
          strcat(topicBuild, configDb.mqttDeviceName[mqttDeviceIndex]);
          strcat(topicBuild, "/Temp");
          Serial.print("subscribing to:");
          Serial.println(topicBuild);
          mqttClient.subscribe(topicBuild);
        }

        Serial.println("[newMqttConnect] mqtt successfullt connected!!");
        tickerBlinkerInstance.detach();
        digitalWrite(BLU_LED_PIN, HIGH);
        mqttAlive = true;
        return true;
      }
      nonBlockingDelay(5000);
    }
    Serial.println("[newMqttConnect] mqtt failed to connect");
    tickerBlinkerInstance.attach_ms(100, tickerBlinkerISR, BLU_LED_PIN);
    return false;
  }
  return true;
}

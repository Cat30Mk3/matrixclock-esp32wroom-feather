#include "MQTT_Manager.h"
#include "globals.h"
#include "JSON_Handler.h"
#include "Ticker_Manager.h"

namespace {
const uint32_t kKeepAlivePublishMs = 25000;
uint32_t s_lastKeepAlivePublishMs = 0;

volatile bool s_queuedMqttPayloadReady = false;
volatile int s_queuedMqttDeviceIndex = -1;
volatile unsigned int s_queuedMqttPayloadLength = 0;
char s_queuedMqttTopic[100];
char s_queuedMqttPayload[1536];
}

ICACHE_RAM_ATTR void callback(char* topic, byte* payload, unsigned int length) {
  char compareMsg[100];

  mqttCallbackInprogress = true;

  for (int mqttDeviceIndex = 1; mqttDeviceIndex < 3; mqttDeviceIndex++) {
    snprintf(compareMsg, sizeof(compareMsg), "%s/%s/%s", "tele", configDb.mqttDeviceName[mqttDeviceIndex], "Temp");

    if (strstr(topic, compareMsg) != NULL) {
      const size_t copyLen = (length < (sizeof(s_queuedMqttPayload) - 1)) ? length : (sizeof(s_queuedMqttPayload) - 1);
      strlcpy(s_queuedMqttTopic, topic, sizeof(s_queuedMqttTopic));
      memcpy(s_queuedMqttPayload, payload, copyLen);
      s_queuedMqttPayload[copyLen] = '\0';
      s_queuedMqttPayloadLength = static_cast<unsigned int>(copyLen);
      s_queuedMqttDeviceIndex = mqttDeviceIndex;
      s_queuedMqttPayloadReady = true;

      Serial.print("MQTT Message queued [");
      Serial.print(s_queuedMqttTopic);
      Serial.print("] len=");
      Serial.println(s_queuedMqttPayloadLength);
      return;
    }
  }

  mqttCallbackInprogress = false;
}

void serviceQueuedMqttPayload(void) {
  if (!s_queuedMqttPayloadReady) {
    return;
  }

  const int deviceIndex = s_queuedMqttDeviceIndex;
  s_queuedMqttPayloadReady = false;

  if (deviceIndex >= 1 && deviceIndex < 3) {
    Serial.print("MQTT Message arrived [");
    Serial.print(s_queuedMqttTopic);
    Serial.println("] [parseJSONPayloadVer3]:start");
    parseJSONPayloadVer3(deviceIndex, s_queuedMqttPayload);
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
        s_lastKeepAlivePublishMs = 0;
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

void mqttServiceKeepAlive(void) {
  if (!mqttClient.connected()) {
    mqttAlive = false;
    return;
  }

  mqttAlive = mqttClient.loop();

  if ((millis() - s_lastKeepAlivePublishMs) < kKeepAlivePublishMs) {
    return;
  }

  s_lastKeepAlivePublishMs = millis();

  char topicBuild[64];
  snprintf(topicBuild, sizeof(topicBuild), "stat/matrixClock/keepalive");

  char payload[24];
  snprintf(payload, sizeof(payload), "%lu", static_cast<unsigned long>(millis() / 1000UL));

  if (mqttClient.publish(topicBuild, payload, false)) {
    Serial.print("[MQTT] keepalive published to ");
    Serial.println(topicBuild);
  } else {
    Serial.println("[MQTT] keepalive publish failed");
  }
}

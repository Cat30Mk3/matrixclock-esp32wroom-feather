#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>

void callback(char* topic, byte* payload, unsigned int length);
bool newMqttConnect(void);
void publishLampState(void);
void mqttServiceKeepAlive(void);

#endif // MQTT_MANAGER_H

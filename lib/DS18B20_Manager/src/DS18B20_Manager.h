#ifndef DS18B20_MANAGER_H
#define DS18B20_MANAGER_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

extern OneWire oneWire;
extern DallasTemperature sensors;
extern DeviceAddress ds18b20Address[2];

void intializeTemperatures(void);
void shortAddress(DeviceAddress deviceAddress, int index, char* shortAddress);
void printAddress(DeviceAddress deviceAddress);
int customGetAddresses(void);

#endif // DS18B20_MANAGER_H

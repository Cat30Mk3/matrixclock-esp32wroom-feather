#include "DS18B20_Manager.h"
#include "globals.h"
#include "Parola_Display.h"

OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress ds18b20Address[2];

void intializeTemperatures(void) {
  char msg[25];
  sensors.begin();
  sensors.setWaitForConversion(true);
  sensors.setResolution(12);
  ds18b20Count = customGetAddresses();
  sprintf(msg, "%d temp sensor%s found.", ds18b20Count, (ds18b20Count == 1) ? "" : "s");
  displayHorzMessage(msg);
}

void shortAddress(DeviceAddress deviceAddress, int index, char* shortAddress) {
  sprintf(shortAddress, "[%d]:%02X%02X",
    index,
    deviceAddress[6],
    deviceAddress[7]
  );
}

void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
  Serial.println();
}

int customGetAddresses(void) {
  int foundCount = 0;

  for (int index = 0; index < 2; index++) {
    sensors.getAddress(ds18b20Address[index], index);
    if (sensors.isConnected(ds18b20Address[index])) {
      foundCount++;
    }
  }

  if (SWAP_DS18B20) {
    Serial.println("SWAP");
    sensors.getAddress(ds18b20Address[1], 0);
    sensors.getAddress(ds18b20Address[0], 1);
  }
  else {
    Serial.println("NO SWAP");
    sensors.getAddress(ds18b20Address[0], 0);
    sensors.getAddress(ds18b20Address[1], 1);
  }

  Serial.print("ds18b20Address[0]:"); printAddress(ds18b20Address[0]);
  Serial.print("ds18b20Address[1]:"); printAddress(ds18b20Address[1]);

  if (foundCount == 2) {
    boolean duplicateFound = true;
    for (int bytePointer = 0; bytePointer < 8 ; bytePointer++) {
      if (ds18b20Address[0][bytePointer] != ds18b20Address[1][bytePointer])duplicateFound = false;
    }
    if (duplicateFound)foundCount = 1;
  }

  switch (foundCount) {
    case 0:
      dispParam[DISP_CURR_WIRED_TEMP_IN].enabled = false;
      dispParam[DISP_CURR_WIRED_TEMP_OUT].enabled = false;
      Serial.print("there are none");
      break;
    case 1:
      if (ONE_TEMP_IS_IN) {
        dispParam[DISP_CURR_WIRED_TEMP_IN].enabled = true;
        dispParam[DISP_CURR_WIRED_TEMP_OUT].enabled = true;
        Serial.print("IN  only 1 ds18b20Address[0]:"); printAddress(ds18b20Address[0]);
        Serial.print("IN  only 1 ds18b20Address[1]:"); printAddress(ds18b20Address[1]);
      }
      else {
        dispParam[DISP_CURR_WIRED_TEMP_IN].enabled = true;
        dispParam[DISP_CURR_WIRED_TEMP_OUT].enabled = true;
        Serial.print("OUT only 1 ds18b20Address[0]:"); printAddress(ds18b20Address[0]);
        Serial.print("OUT only 1 ds18b20Address[1]:"); printAddress(ds18b20Address[1]);
      }
      break;
    case 2:
      dispParam[DISP_CURR_WIRED_TEMP_IN].enabled = true;
      dispParam[DISP_CURR_WIRED_TEMP_OUT].enabled = true;
      Serial.print("there are 2 - ds18b20Address[0]:"); printAddress(ds18b20Address[0]);
      Serial.print("there are 2 - ds18b20Address[1]:"); printAddress(ds18b20Address[1]);
      break;
  }
  return foundCount;
}

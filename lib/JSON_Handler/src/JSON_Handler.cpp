#include "JSON_Handler.h"
#include "globals.h"
#include <ArduinoJson.h>

ICACHE_RAM_ATTR boolean parseJSONPayloadVer3(int deviceNameIndex, char *json) {
  char tempTemp[16];
  char objName[16];
  int objIndex, startIndex, endIndex;

  Serial.println("[parseJSONPayloadVer3]:start");

  JsonDocument root;
  DeserializationError error = deserializeJson(root, json);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return 1;
  }

  if(root.isNull()) return 1;

  JsonObject Temp = root["Temp"];
  if(Temp.isNull()) return 1;

  int errorCounter = 0;

  switch (deviceNameIndex) {
    case 1:
      startIndex = DISP_CURR_MQTT_COT_TEMP_IN;
      endIndex = DISP_CURR_MQTT_COT_TEMP_SMP;
      break;
    case 2:
      startIndex = DISP_CURR_MQTT_HOM_TEMP_IN;
      endIndex = DISP_CURR_MQTT_HOM_TEMP_CAV;
      break;
  }

  for (objIndex = startIndex; objIndex <= endIndex; objIndex++) {
    strcpy(objName, dispParam[objIndex].mqttTopic);
    strlcpy(tempTemp, Temp[objName]["value"] | "", 15);
    if (strlen(tempTemp) > 0) {
      sprintf(dispParam[objIndex].dispBuffer, "%s%c", tempTemp, dispParam[objIndex].symbolIndex);
      dispParam[objIndex].dispReady = true;
      dispParam[objIndex].lastReceivedUpdate = millis();
    }
    else {
      errorCounter++;
      dispParam[objIndex].dispReady = false;
    }
  }

  if (errorCounter > 3)  return 1;
  return 0;
}

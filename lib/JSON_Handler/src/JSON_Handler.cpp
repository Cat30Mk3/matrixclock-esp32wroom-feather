#include "JSON_Handler.h"
#include "globals.h"
#include <ArduinoJson.h>

namespace {
// Reuse a single document to avoid repeated alloc/free churn over long runtimes.
JsonDocument s_mqttJsonDoc;
}

ICACHE_RAM_ATTR boolean parseJSONPayloadVer3(int deviceNameIndex, char *json) {
  char tempTemp[16];
  char objName[16];
  int objIndex, startIndex, endIndex;

  Serial.println("[parseJSONPayloadVer3]:start");

  s_mqttJsonDoc.clear();
  DeserializationError error = deserializeJson(s_mqttJsonDoc, json);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return 1;
  }

  if (s_mqttJsonDoc.isNull()) return 1;

  JsonObject Temp = s_mqttJsonDoc["Temp"];
  if (Temp.isNull()) return 1;

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
    default:
      return 1;
  }

  for (objIndex = startIndex; objIndex <= endIndex; objIndex++) {
    strlcpy(objName, dispParam[objIndex].mqttTopic, sizeof(objName));
    strlcpy(tempTemp, Temp[objName]["value"] | "", sizeof(tempTemp));
    if (strlen(tempTemp) > 0) {
      snprintf(dispParam[objIndex].dispBuffer,
               sizeof(dispParam[objIndex].dispBuffer),
               "%s%c",
               tempTemp,
               dispParam[objIndex].symbolIndex);
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

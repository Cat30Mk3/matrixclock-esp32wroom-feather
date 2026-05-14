#include "Init_Manager.h"
#include "globals.h"

// ============================================================================
// INITIALIZATION FUNCTIONS
// ============================================================================

void fileNameParser(char *nameFromFileName, char *versionFromFileName, char *dateFromFileName, char *timeFromFileName)
{
  strlcpy(nameFromFileName, "matrixClock", sizeof(projectNameFromFileName));
  strlcpy(versionFromFileName, APP_VERSION, sizeof(projectVersionFromFileName));
  strlcpy(dateFromFileName, __DATE__, sizeof(compileDateFromFileName));
  strlcpy(timeFromFileName, __TIME__, sizeof(compileTimeFromFileName));
}

void initializeDispParam(void)
{
 
  dispParam[DISP_CURR_TIME].enabled = true;
  dispParam[DISP_CURR_DOW].enabled = true;
  dispParam[DISP_CURR_DATE].enabled = false;
  dispParam[DISP_CURR_WIRED_TEMP_IN].enabled = false;
  dispParam[DISP_CURR_WIRED_TEMP_OUT].enabled = false;
  dispParam[DISP_CURR_MQTT_COT_TEMP_IN].enabled = true;
  dispParam[DISP_CURR_MQTT_COT_TEMP_OUT].enabled = true;
  dispParam[DISP_CURR_MQTT_COT_TEMP_CRL].enabled = true;
  dispParam[DISP_CURR_MQTT_COT_TEMP_SMP].enabled = true;
  dispParam[DISP_CURR_MQTT_COT_MSG].enabled = true;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_IN].enabled = true;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_OUT].enabled = true;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_ATT].enabled = true;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_CAV].enabled = true;
  dispParam[DISP_CURR_MQTT_HOM_MSG].enabled = true;

  dispParam[DISP_CURR_TIME].source = 0;
  dispParam[DISP_CURR_DOW].source = 0;
  dispParam[DISP_CURR_DATE].source = 0;
  dispParam[DISP_CURR_WIRED_TEMP_IN].source = 0;
  dispParam[DISP_CURR_WIRED_TEMP_OUT].source = 0;
  dispParam[DISP_CURR_MQTT_COT_TEMP_IN].source = 1;
  dispParam[DISP_CURR_MQTT_COT_TEMP_OUT].source = 1;
  dispParam[DISP_CURR_MQTT_COT_TEMP_CRL].source = 1;
  dispParam[DISP_CURR_MQTT_COT_TEMP_SMP].source = 1;
  dispParam[DISP_CURR_MQTT_COT_MSG].source = 1;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_IN].source = 2;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_OUT].source = 2;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_ATT].source = 2;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_CAV].source = 2;
  dispParam[DISP_CURR_MQTT_HOM_MSG].source = 2;

#if DISPLAY_CONFIG == DISPLAY1X4
  dispParam[DISP_CURR_TIME].enabled = true;
  dispParam[DISP_CURR_DOW].enabled = true;
  dispParam[DISP_CURR_DATE].enabled = false;
  dispParam[DISP_CURR_WIRED_TEMP_IN].enabled = true;
  dispParam[DISP_CURR_WIRED_TEMP_OUT].enabled = true;

  dispParam[DISP_CURR_MQTT_COT_TEMP_IN].enabled = false;
  dispParam[DISP_CURR_MQTT_COT_TEMP_OUT].enabled = true;
  dispParam[DISP_CURR_MQTT_COT_TEMP_CRL].enabled = false;
  dispParam[DISP_CURR_MQTT_COT_TEMP_SMP].enabled = false;
  dispParam[DISP_CURR_MQTT_COT_MSG].enabled = true;

  dispParam[DISP_CURR_MQTT_HOM_TEMP_IN].enabled = false;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_OUT].enabled = true;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_ATT].enabled = false;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_CAV].enabled = false;
  dispParam[DISP_CURR_MQTT_HOM_MSG].enabled = true;

  dispParam[DISP_CURR_TIME].groupMode = 0;

#elif DISPLAY_CONFIG == DISPLAY2X8

  dispParam[DISP_CURR_TIME].enabled = true;
  dispParam[DISP_CURR_DOW].enabled = true;
  dispParam[DISP_CURR_DATE].enabled = false;
  dispParam[DISP_CURR_WIRED_TEMP_IN].enabled = true;
  dispParam[DISP_CURR_WIRED_TEMP_OUT].enabled = true;
  dispParam[DISP_CURR_MQTT_COT_TEMP_IN].enabled = true;
  dispParam[DISP_CURR_MQTT_COT_TEMP_OUT].enabled = true;
  dispParam[DISP_CURR_MQTT_COT_TEMP_CRL].enabled = true;
  dispParam[DISP_CURR_MQTT_COT_TEMP_SMP].enabled = true;
  dispParam[DISP_CURR_MQTT_COT_MSG].enabled = true;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_IN].enabled = true;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_OUT].enabled = true;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_ATT].enabled = true;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_CAV].enabled = true;
  dispParam[DISP_CURR_MQTT_HOM_MSG].enabled = true;

  dispParam[DISP_CURR_TIME].groupMode = 0;
  dispParam[DISP_CURR_DOW].groupMode = 0;
  dispParam[DISP_CURR_DATE].groupMode = 0;
  dispParam[DISP_CURR_WIRED_TEMP_IN].groupMode = 1;
  dispParam[DISP_CURR_WIRED_TEMP_OUT].groupMode = 1;
  dispParam[DISP_CURR_MQTT_COT_TEMP_IN].groupMode = 2;
  dispParam[DISP_CURR_MQTT_COT_TEMP_OUT].groupMode = 2;
  dispParam[DISP_CURR_MQTT_COT_TEMP_CRL].groupMode = 2;
  dispParam[DISP_CURR_MQTT_COT_TEMP_SMP].groupMode = 2;
  dispParam[DISP_CURR_MQTT_COT_MSG].groupMode = 0;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_IN].groupMode = 3;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_OUT].groupMode = 3;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_ATT].groupMode = 3;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_CAV].groupMode = 3;
  dispParam[DISP_CURR_MQTT_HOM_MSG].groupMode = 0;

#endif

  dispParam[DISP_CURR_TIME].remoteMqttDeviceIndex = 0;
  dispParam[DISP_CURR_DOW].remoteMqttDeviceIndex = 0;
  dispParam[DISP_CURR_DATE].remoteMqttDeviceIndex = 0;
  dispParam[DISP_CURR_WIRED_TEMP_IN].remoteMqttDeviceIndex = 0;
  dispParam[DISP_CURR_WIRED_TEMP_OUT].remoteMqttDeviceIndex = 0;
  dispParam[DISP_CURR_MQTT_COT_TEMP_IN].remoteMqttDeviceIndex = 1;
  dispParam[DISP_CURR_MQTT_COT_TEMP_OUT].remoteMqttDeviceIndex = 1;
  dispParam[DISP_CURR_MQTT_COT_TEMP_CRL].remoteMqttDeviceIndex = 1;
  dispParam[DISP_CURR_MQTT_COT_TEMP_SMP].remoteMqttDeviceIndex = 1;
  dispParam[DISP_CURR_MQTT_COT_MSG].remoteMqttDeviceIndex = 1;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_IN].remoteMqttDeviceIndex = 2;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_OUT].remoteMqttDeviceIndex = 2;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_ATT].remoteMqttDeviceIndex = 2;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_CAV].remoteMqttDeviceIndex = 2;
  dispParam[DISP_CURR_MQTT_HOM_MSG].remoteMqttDeviceIndex = 2;

  // caution: CASE sensitive! must match sent topic case
  strlcpy(dispParam[DISP_CURR_MQTT_COT_TEMP_IN].mqttTopic, "INSIDE", sizeof(dispParam[0].mqttTopic));
  strlcpy(dispParam[DISP_CURR_MQTT_COT_TEMP_OUT].mqttTopic, "OUTSIDE", sizeof(dispParam[0].mqttTopic));
  strlcpy(dispParam[DISP_CURR_MQTT_COT_TEMP_CRL].mqttTopic, "DRYPIT", sizeof(dispParam[0].mqttTopic));
  strlcpy(dispParam[DISP_CURR_MQTT_COT_TEMP_SMP].mqttTopic, "WETPIT", sizeof(dispParam[0].mqttTopic));
  strlcpy(dispParam[DISP_CURR_MQTT_COT_MSG].mqttTopic, "msg", sizeof(dispParam[0].mqttTopic));
  strlcpy(dispParam[DISP_CURR_MQTT_HOM_TEMP_IN].mqttTopic, "COMPUTER", sizeof(dispParam[0].mqttTopic));
  strlcpy(dispParam[DISP_CURR_MQTT_HOM_TEMP_OUT].mqttTopic, "OUTSIDE", sizeof(dispParam[0].mqttTopic));
  strlcpy(dispParam[DISP_CURR_MQTT_HOM_TEMP_ATT].mqttTopic, "ATTIC", sizeof(dispParam[0].mqttTopic));
  strlcpy(dispParam[DISP_CURR_MQTT_HOM_TEMP_CAV].mqttTopic, "CAVITY", sizeof(dispParam[0].mqttTopic));
  strlcpy(dispParam[DISP_CURR_MQTT_HOM_MSG].mqttTopic, "msg", sizeof(dispParam[0].mqttTopic));

  dispParam[DISP_CURR_WIRED_TEMP_IN].symbolIndex = 10;
  dispParam[DISP_CURR_WIRED_TEMP_OUT].symbolIndex = 11;
  dispParam[DISP_CURR_MQTT_COT_TEMP_IN].symbolIndex = 12;
  dispParam[DISP_CURR_MQTT_COT_TEMP_OUT].symbolIndex = 13;
  dispParam[DISP_CURR_MQTT_COT_TEMP_CRL].symbolIndex = 14;
  dispParam[DISP_CURR_MQTT_COT_TEMP_SMP].symbolIndex = 15;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_IN].symbolIndex = 16;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_OUT].symbolIndex = 17;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_ATT].symbolIndex = 18;
  dispParam[DISP_CURR_MQTT_HOM_TEMP_CAV].symbolIndex = 19;
}

void initializeGlobalVariables(void)
{
  sprintf(dispBuffer[0], "%s", "");
  sprintf(dispBuffer[1], "%s", "");
  sprintf(dispBuffer[2], "%s", "");
  sprintf(dispBuffer[3], "%s", "");
}

void initializeGPIOPins(void)
{
  pinMode(BLU_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YEL_LED_PIN, OUTPUT);
  pinMode(LAMP_PIN, OUTPUT);

  pinMode(PB_SEL_PIN, INPUT);
  pinMode(PB_DEC_PIN, INPUT);
  pinMode(PB_INC_PIN, INPUT);
  pinMode(PB_CNL_PIN, INPUT_PULLUP);
  pinMode(PB_MEN_PIN, INPUT_PULLUP);

  digitalWrite(BLU_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YEL_LED_PIN, LOW);
  digitalWrite(LAMP_PIN, LOW);
}

#include "Ticker_Manager.h"
#include "globals.h"
#include "DS18B20_Manager.h"
#include "Mode_Manager.h"
#include "MQTT_Manager.h"

ICACHE_RAM_ATTR void tickerBlinkerISR(int pinToBlink) {
  bool pinValue = digitalRead(pinToBlink);
  digitalWrite(pinToBlink, !pinValue);
}

ICACHE_RAM_ATTR void tickerDelayISR(void) {
  delayFlag = false;
}

ICACHE_RAM_ATTR void nonBlockingDelay(uint32_t msOfDelay) {
  delayFlag = true;
  tickerDelayInstance.once_ms(msOfDelay, tickerDelayISR);
  while (delayFlag) {
    if (modeManagerIsBackgroundPollingEnabled()) {
      modeManagerServiceButtonDiagnostics();
      modeManagerService();
    }
    mqttServiceKeepAlive();
    yield();
  }
}

ICACHE_RAM_ATTR void tickerKeepAliveMqttISR(void) {
  // Keep ISR lean; network stack work must run in foreground context.
}

ICACHE_RAM_ATTR void resetDisplayISR(void) {
  displayNormal = true;
}

ICACHE_RAM_ATTR void tickerNonBlockingTempStartISR(void) {
  sensors.requestTemperatures();
  tickerTempGetInstance.once_ms(2000, tickerNonBlockingTempGetISR);
}

ICACHE_RAM_ATTR void tickerNonBlockingTempGetISR(void) {
  char tempDegCStr[2][20];
  float tempDegCFlt;

  if (!mqttCallbackInprogress) {
    for (int index = 0; index < ds18b20Count; index++) {
      tempDegCFlt = sensors.getTempC(ds18b20Address[index]);
      tempDegCFlt = sensors.getTempC(ds18b20Address[index]);

      if (tempDegCFlt > -60) {
        dtostrf(tempDegCFlt, -4, 1, tempDegCStr[index]);
        sprintf(dispParam[DISP_CURR_WIRED_TEMP_IN + index].dispBuffer, "%s%c",
                tempDegCStr[index],
                dispParam[DISP_CURR_WIRED_TEMP_IN + index].symbolIndex);
        dispParam[DISP_CURR_WIRED_TEMP_IN + index].dispReady = true;
        dispParam[DISP_CURR_WIRED_TEMP_IN].lastReceivedUpdate = millis();
      }
    }
  }
}

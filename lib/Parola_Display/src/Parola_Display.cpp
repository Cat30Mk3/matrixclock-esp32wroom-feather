#include "Parola_Display.h"
#include "globals.h"
#include "Mode_Manager.h"

namespace {
void pollMqttDuringDisplayWait() {
  if (configDb.mqttEnabled && configDb.wifiEnabled && WiFi.status() == WL_CONNECTED) {
    if (mqttClient.connected()) {
      mqttAlive = mqttClient.loop();
    } else {
      mqttAlive = false;
    }
  }
}

void pollModeButtonsDuringDisplayWait() {
  // Keep AP entry/exit button handling responsive while long display waits are active.
  if (modeManagerIsBackgroundPollingEnabled()) {
    modeManagerServiceButtonDiagnostics();
    modeManagerService();
  }
}

bool waitForMainZonesReady(uint32_t timeoutMs, const char* phaseLabel, const char* pathTag) {
  const uint32_t startMs = millis();

#if DISPLAY_CONFIG == DISPLAY1X4
  while (!parola.getZoneStatus(ZONE_SINGLE))
#elif DISPLAY_CONFIG == DISPLAY2X8
  while (!(parola.getZoneStatus(ZONE_LOWER) && parola.getZoneStatus(ZONE_UPPER)))
#endif
  {
    if (modeManagerInApControlMode()) return false;

    if ((millis() - startMs) >= timeoutMs) {
      Serial.print(pathTag);
      Serial.print(" zone wait timeout at ");
      Serial.println(phaseLabel);
      return false;
    }

    // Keep wait loops non-reentrant: animate + targeted polling + yield only.
    // Do not call nonBlockingDelay() here because broad service calls can
    // mutate display/network state during an in-flight transition and starve
    // zone-ready completion checks.
    parola.displayAnimate();

    // Keep waits non-reentrant while still servicing MQTT keepalive.
    pollModeButtonsDuringDisplayWait();
    pollMqttDuringDisplayWait();

    yield();
  }

  return true;
}

#if DISPLAY_CONFIG == DISPLAY2X8
bool waitForZoneReady(uint8_t zone, uint32_t timeoutMs, const char* phaseLabel, const char* pathTag) {
  const uint32_t startMs = millis();
  while (!parola.getZoneStatus(zone)) {
    if (modeManagerInApControlMode()) return false;

    if ((millis() - startMs) >= timeoutMs) {
      Serial.print(pathTag);
      Serial.print(" zone wait timeout at ");
      Serial.println(phaseLabel);
      return false;
    }

    parola.displayAnimate();
    pollModeButtonsDuringDisplayWait();
    pollMqttDuringDisplayWait();
    yield();
  }

  return true;
}
#endif
}

void displayVertMessage(const char* msg) {
#if DEBUG_DISPLAY_TRACE
  Serial.print("[DISP][VERT] request: ");
  Serial.println(msg);
#endif
#if DEBUG_DISPLAY_TRACE
#if DISPLAY_CONFIG == DISPLAY1X4
  Serial.println("[DISP][VERT] waiting for ZONE_SINGLE ready");
#elif DISPLAY_CONFIG == DISPLAY2X8
  Serial.println("[DISP][VERT] waiting for ZONE_LOWER+ZONE_UPPER ready");
#endif
#endif
  if (!waitForMainZonesReady(15000, "pre-arm", "[DISP][VERT]")) return;
#if DEBUG_DISPLAY_TRACE
  Serial.println();
  Serial.println("[DISP][VERT] arming scroll message");
#endif

#if DISPLAY_CONFIG == DISPLAY1X4
  parola.displayZoneText(ZONE_SINGLE, msg, PA_CENTER, V_SCROLL_SPEED, V_PAUSE, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
#elif DISPLAY_CONFIG == DISPLAY2X8
  parola.setCharSpacing(doubleCharSpace);
  parola.displayZoneText(ZONE_LOWER, msg, PA_CENTER, V_SCROLL_SPEED, V_PAUSE, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
  parola.displayZoneText(ZONE_UPPER, msg, PA_CENTER, V_SCROLL_SPEED, V_PAUSE, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
#endif

#if DISPLAY_CONFIG == DISPLAY1X4
#if DEBUG_DISPLAY_VERT_SCROLL_ASYNC
  Serial.println("[DISP][VERT] async bypass enabled - returning after arm");
  return;
#endif
  if (!waitForMainZonesReady(15000, "post-arm", "[DISP][VERT]")) {
    parola.displayClear();
    return;
  }
#elif DISPLAY_CONFIG == DISPLAY2X8
  if (!waitForMainZonesReady(15000, "post-arm", "[DISP][VERT]")) {
    parola.displayClear();
    return;
  }
#endif
#if DEBUG_DISPLAY_TRACE
  Serial.println("[DISP][VERT] scroll complete");
#endif
}

boolean serviceQuadPage(int numbQuads, dispParamStruct UL, dispParamStruct UR, dispParamStruct LL, dispParamStruct LR) {
#if DISPLAY_CONFIG == DISPLAY2X8
  if (!waitForZoneReady(ZONE_UP_LFT, 15000, "pre-arm", "[DISP][QUAD]")) return false;

  if ((numbQuads < 1) || (numbQuads > 4))return false;
  if (!UL.dispReady && !UR.dispReady && !LL.dispReady && !LL.dispReady) return false;
  char msg[4][30] = {"", "", "", ""};
  switch (numbQuads) {
    case 4:
      if (LR.dispReady)strcpy(msg[3], LR.dispBuffer);
    case 3:
      if (LR.dispReady)strcpy(msg[2], LL.dispBuffer);
    case 2:
      if (LR.dispReady)strcpy(msg[1], UR.dispBuffer);
    case 1:
      if (LR.dispReady)strcpy(msg[0], UL.dispBuffer);
  }

  UL.dispReady = false;
  UR.dispReady = false;
  LL.dispReady = false;
  LR.dispReady = false;

  parola.setCharSpacing(singleCharSpace);

  parola.displayZoneText(ZONE_UP_LFT, msg[0], PA_RIGHT, V_SCROLL_SPEED, V_PAUSE * numbQuads, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
  parola.displayZoneText(ZONE_UP_RGT, msg[1], PA_RIGHT, V_SCROLL_SPEED, V_PAUSE * numbQuads, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
  parola.displayZoneText(ZONE_DN_LFT, msg[2], PA_RIGHT, V_SCROLL_SPEED, V_PAUSE * numbQuads, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
  parola.displayZoneText(ZONE_DN_RGT, msg[3], PA_RIGHT, V_SCROLL_SPEED, V_PAUSE * numbQuads, PA_SCROLL_DOWN, PA_SCROLL_DOWN);

  parola.synchZoneStart();
  if (!waitForZoneReady(ZONE_UP_LFT, 15000, "post-arm", "[DISP][QUAD]")) {
    parola.displayClear();
    return false;
  }
  return true;
#endif
  return false;
}

void displayHorzMessage(const char* msg) {
  if (!waitForMainZonesReady(15000, "pre-arm", "[DISP][HORZ]")) return;
  Serial.println();

#if DISPLAY_CONFIG == DISPLAY1X4
  parola.displayZoneText(ZONE_SINGLE, msg, PA_CENTER, H_SCROLL_SPEED, H_PAUSE, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
#elif DISPLAY_CONFIG == DISPLAY2X8
  parola.setFont(ZONE_LOWER, BigFontLower);
  parola.setFont(ZONE_UPPER, BigFontUpper);
  parola.displayZoneText(ZONE_LOWER, msg, PA_CENTER, H_SCROLL_SPEED, H_PAUSE, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  parola.displayZoneText(ZONE_UPPER, msg, PA_CENTER, H_SCROLL_SPEED, H_PAUSE, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
#endif

  parola.synchZoneStart();
  if (!waitForMainZonesReady(15000, "post-arm", "[DISP][HORZ]")) {
    parola.displayClear();
    return;
  }
  Serial.println();
}

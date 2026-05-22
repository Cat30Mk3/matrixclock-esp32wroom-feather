#include "Parola_Display.h"
#include "globals.h"
#include "Mode_Manager.h"

void displayVertMessage(const char* msg) {
#if DISPLAY_CONFIG == DISPLAY1X4
  while (!parola.getZoneStatus(ZONE_SINGLE))
#elif DISPLAY_CONFIG == DISPLAY2X8
  while (!(parola.getZoneStatus(ZONE_LOWER) && parola.getZoneStatus(ZONE_UPPER)))
#endif
  {
    if (modeManagerInApControlMode()) return;
    parola.displayAnimate();
    nonBlockingDelay(V_SCROLL_SPEED);
    Serial.print("^");
  }
  Serial.println();

#if DISPLAY_CONFIG == DISPLAY1X4
  parola.displayZoneText(ZONE_SINGLE, msg, PA_CENTER, V_SCROLL_SPEED, V_PAUSE, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
#elif DISPLAY_CONFIG == DISPLAY2X8
  parola.setCharSpacing(doubleCharSpace);
  parola.displayZoneText(ZONE_LOWER, msg, PA_CENTER, V_SCROLL_SPEED, V_PAUSE, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
  parola.displayZoneText(ZONE_UPPER, msg, PA_CENTER, V_SCROLL_SPEED, V_PAUSE, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
#endif

#if DISPLAY_CONFIG == DISPLAY1X4
  while (!parola.getZoneStatus(ZONE_SINGLE))
#elif DISPLAY_CONFIG == DISPLAY2X8
  while (!(parola.getZoneStatus(ZONE_LOWER) && parola.getZoneStatus(ZONE_UPPER)))
#endif
  {
    if (modeManagerInApControlMode()) return;
    parola.displayAnimate();
    nonBlockingDelay(V_SCROLL_SPEED);
  }
}

boolean serviceQuadPage(int numbQuads, dispParamStruct UL, dispParamStruct UR, dispParamStruct LL, dispParamStruct LR) {
#if DISPLAY_CONFIG == DISPLAY2X8
  while (!(parola.getZoneStatus(ZONE_UP_LFT))) {
    if (modeManagerInApControlMode()) return false;
    parola.displayAnimate();
    nonBlockingDelay(V_SCROLL_SPEED);
  }

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

  long animateTimer    = millis();
  long animateDuration = ((2 * VDOTS_PER_CHAR * V_SCROLL_SPEED) + (V_PAUSE * numbQuads));

  parola.synchZoneStart();
  while (!parola.getZoneStatus(ZONE_UP_LFT)) {
    if (modeManagerInApControlMode()) return false;
    parola.displayAnimate();
    nonBlockingDelay(V_SCROLL_SPEED);
  }
  return true;
#endif
  return false;
}

void displayHorzMessage(const char* msg) {
  long animateDuration;

#if DISPLAY_CONFIG == DISPLAY1X4
  while (!parola.getZoneStatus(ZONE_SINGLE))
#elif DISPLAY_CONFIG == DISPLAY2X8
  while (!(parola.getZoneStatus(ZONE_LOWER) && parola.getZoneStatus(ZONE_UPPER)))
#endif
  {
    if (modeManagerInApControlMode()) return;
    parola.displayAnimate();
    nonBlockingDelay(H_SCROLL_SPEED);
  }
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
#if DISPLAY_CONFIG == DISPLAY1X4
  while (!parola.getZoneStatus(ZONE_SINGLE))
#elif DISPLAY_CONFIG == DISPLAY2X8
  while (!(parola.getZoneStatus(ZONE_LOWER) && parola.getZoneStatus(ZONE_UPPER)))
#endif
  {
    if (modeManagerInApControlMode()) return;
    parola.displayAnimate();
    nonBlockingDelay(H_SCROLL_SPEED / 10);
  }
  Serial.println();
}

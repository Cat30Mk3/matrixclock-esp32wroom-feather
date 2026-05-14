#ifndef PAROLA_DISPLAY_H
#define PAROLA_DISPLAY_H

#include <Arduino.h>
#include "globals.h"
#include "Font_Data.h"
#include "Ticker_Manager.h"

void displayVertMessage(const char* msg);
boolean serviceQuadPage(int numbQuads, dispParamStruct UL, dispParamStruct UR, dispParamStruct LL, dispParamStruct LR);
void displayHorzMessage(const char* msg);

#endif // PAROLA_DISPLAY_H

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "globals.h"
#include "Parola_Display.h"
#include "Ticker_Manager.h"

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
//#define MAX_DEVICES 4

// GPIO Pin Definitions
#define CLK_PIN 5   // SPI BUS - CLK
#define DATA_PIN 18 // SPI BUS - MOSI
#define CS_PIN 21   // SPI BUS - CS for MD_MAX7219

// Hardware SPI connection
MD_Parola parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
MD_MAX72XX mx(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
// Arbitrary output pins
// MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);


void initializeGlobalVariables(void)
{
  sprintf(dispBuffer[0], "%s", "");
  sprintf(dispBuffer[1], "%s", "");
  sprintf(dispBuffer[2], "%s", "");
  sprintf(dispBuffer[3], "%s", "");
}

void setup(void)
{
  parola.begin(MAX_ZONES_FULL + MAX_ZONES_HALF);
  parola.setZone(ZONE_SINGLE, 0, ZONE_SIZE_FULL - 1);
  displayHorzMessage("Setup...");
  delay(5000);
}

void loop(void)
{

  if (parola.getZoneStatus(ZONE_SINGLE))
  {
    displayHorzMessage("Bing...");
  }
  delay(5000);
  if (parola.getZoneStatus(ZONE_SINGLE))
  {
    displayVertMessage("Bong...");
  }
  delay(5000);
}
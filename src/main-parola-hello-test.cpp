#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
#define MAX_DEVICES 4

// GPIO Pin Definitions
#define CLK_PIN       5    // SPI BUS - CLK
#define DATA_PIN     18    // SPI BUS - MOSI
#define CS_PIN       21    // SPI BUS - CS for MD_MAX7219

// Hardware SPI connection
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Arbitrary output pins
// MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

void setup(void)
{
  P.begin();
}

void loop(void)
{
  if (P.displayAnimate())
    P.displayText("Hello", PA_CENTER, P.getSpeed(), 1000, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
}
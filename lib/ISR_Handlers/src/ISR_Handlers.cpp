#include "ISR_Handlers.h"
#include "globals.h"

ICACHE_RAM_ATTR void pbSelFallIsr(void) {
  isrFlag = PB_SEL_PIN;
  delay(100);
}

ICACHE_RAM_ATTR void pbDecFallIsr(void) {
  isrFlag = PB_DEC_PIN;
  delay(100);
  lamp_1_State = false;
  digitalWrite(LAMP_PIN, lamp_1_State);
}

ICACHE_RAM_ATTR void pbIncFallIsr(void) {
  isrFlag = PB_INC_PIN;
  delay(100);
  lamp_1_State = true;
  digitalWrite(LAMP_PIN, lamp_1_State);
}

ICACHE_RAM_ATTR void pbCnlFallIsr(void) {
  isrFlag = PB_CNL_PIN;
  delay(100);
}

ICACHE_RAM_ATTR void pbMenFallIsr(void) {
  isrFlag = PB_MEN_PIN;
  delay(100);
}

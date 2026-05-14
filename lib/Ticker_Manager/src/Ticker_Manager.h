#ifndef TICKER_MANAGER_H
#define TICKER_MANAGER_H

#include <Arduino.h>

void tickerBlinkerISR(int pinToBlink);
void tickerDelayISR(void);
void nonBlockingDelay(uint32_t msOfDelay);
void tickerKeepAliveMqttISR(void);
void resetDisplayISR(void);
void tickerNonBlockingTempStartISR(void);
void tickerNonBlockingTempGetISR(void);

#endif // TICKER_MANAGER_H

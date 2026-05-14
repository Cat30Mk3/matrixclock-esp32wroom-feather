#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

#include <Arduino.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

extern WiFiUDP Udp;

void digitalClockDisplay();
time_t getNtpTime();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

#endif // NTP_MANAGER_H

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <TimeLib.h>

time_t compileTime(void);
void printDateTime(time_t t, const char *tz);
void getCurrentTime(char *timeStr, boolean hr24);
void getCurrentDate(char *dateStr);
void getCurrentDOW(char *dateStr);
void customSetTimeFromRTC(void);
void printRTCTime(void);

#endif // TIME_MANAGER_H

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <TimeLib.h>

// ─── Timezone table ───────────────────────────────────────────────────────────
struct TzTableEntry {
  const char *label;    // display label for portal select
  const char *posixStr; // POSIX TZ string for setenv("TZ", ...)
};

extern const TzTableEntry kTimezoneTable[];
extern const uint8_t      kTimezoneTableCount;

void applyTimezone(uint8_t idx); // sets TZ env var + calls tzset()

// ─── Time functions ───────────────────────────────────────────────────────────
time_t compileTime(void);
void printDateTime(time_t t, const char *tz);
void getCurrentTime(char *timeStr, boolean hr24);
void getCurrentDate(char *dateStr);
void getCurrentDOW(char *dateStr);
void customSetTimeFromRTC(void);
void printRTCTime(void);

#endif // TIME_MANAGER_H

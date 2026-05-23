#include "Time_Manager.h"
#include "globals.h"
#include <RTClib.h>
#include <time.h>

extern RTC_DS3231 rtc;

// ─── Timezone table (POSIX TZ strings) — Canadian zones per product spec ────────
const TzTableEntry kTimezoneTable[] = {
  {"AST/ADT (Atlantic)",  "AST4ADT,M3.2.0,M11.1.0"},   // idx 0 — UTC-4/UTC-3
  {"EST/EDT (Eastern)",   "EST5EDT,M3.2.0,M11.1.0"},    // idx 1 — UTC-5/UTC-4
  {"CST/CDT (Central)",   "CST6CDT,M3.2.0,M11.1.0"},    // idx 2 — UTC-6/UTC-5
  {"MST/MDT (Mountain)",  "MST7MDT,M3.2.0,M11.1.0"},    // idx 3 — UTC-7/UTC-6
  {"PST/PDT (Pacific)",   "PST8PDT,M3.2.0,M11.1.0"},    // idx 4 — UTC-8/UTC-7
  {"UTC",                 "UTC0"},                        // idx 5
};
const uint8_t kTimezoneTableCount = sizeof(kTimezoneTable) / sizeof(kTimezoneTable[0]);

void applyTimezone(uint8_t idx) {
  if (idx >= kTimezoneTableCount) idx = 0;
  setenv("TZ", kTimezoneTable[idx].posixStr, 1);
  tzset();
}

time_t compileTime(void) {
  const time_t FUDGE(10);
  const char *compDate = __DATE__, *compTime = __TIME__, *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char chMon[3], *m;
  tmElements_t tm;

  strncpy(chMon, compDate, 3);
  chMon[3] = '\0';
  m = strstr(months, chMon);
  tm.Month = ((m - months) / 3 + 1);

  tm.Day = atoi(compDate + 4);
  tm.Year = atoi(compDate + 7) - 1970;
  tm.Hour = atoi(compTime);
  tm.Minute = atoi(compTime + 3);
  tm.Second = atoi(compTime + 6);
  time_t t = makeTime(tm);
  return t + FUDGE;
}

void printDateTime(time_t t, const char *tz) {
  char buf[32];
  char m[4];
  strlcpy(m, monthShortStr(month(t)), sizeof(m));
  sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s",
          hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), tz);
  Serial.println(buf);
}

void getCurrentTime(char *timeStr, boolean hr24) {
  time_t utc = now();
  struct tm tmLocal;
  localtime_r(&utc, &tmLocal);
  int currentHour   = tmLocal.tm_hour;
  int currentMinute = tmLocal.tm_min;
  boolean PM = false;
  if (currentHour > 12) PM = true;
  if (hr24)
    sprintf(timeStr, "%02d:%02d", currentHour, currentMinute);
  else {
    if (PM) currentHour = currentHour - 12;
    sprintf(timeStr, "%2d:%02d", currentHour, currentMinute);
  }
}

void getCurrentDate(char *dateStr) {
  time_t utc = now();
  struct tm tmLocal;
  localtime_r(&utc, &tmLocal);
  char m[4];
  strlcpy(m, monthShortStr(tmLocal.tm_mon + 1), sizeof(m));
  sprintf(dateStr, "%s %d", m, tmLocal.tm_mday);
}

void getCurrentDOW(char *dateStr) {
  time_t utc = now();
  struct tm tmLocal;
  localtime_r(&utc, &tmLocal);
  // tm_wday: 0=Sun..6=Sat; TimeLib dayShortStr: 1=Sun..7=Sat
  sprintf(dateStr, "%s", dayShortStr(tmLocal.tm_wday + 1));
}

void customSetTimeFromRTC(void) {
  DateTime rtcNow = rtc.now();
  int rtcHour = rtcNow.hour();
  int rtcMinute = rtcNow.minute();
  int rtcSecond = rtcNow.second();
  int rtcMonth = rtcNow.month();
  int rtcDay = rtcNow.day();
  int rtcYear = rtcNow.year();
  setTime(rtcHour, rtcMinute, rtcSecond, rtcDay, rtcMonth, rtcYear);
}

void printRTCTime(void) {
  DateTime rtcNow = rtc.now();
  int rtcHour = rtcNow.hour();
  int rtcMinute = rtcNow.minute();
  int rtcSecond = rtcNow.second();
  int rtcMonth = rtcNow.month();
  int rtcDow = rtcNow.dayOfTheWeek();
  int rtcDay = rtcNow.day();
  int rtcYear = rtcNow.year();
  char buf[50];
  char m[4];
    strlcpy(m, monthShortStr(rtcMonth), sizeof(m));
    const int timeLibWeekday = (rtcDow % 7) + 1;
  sprintf(buf, "RTC: %.2d:%.2d:%.2d %s %.2d %s %d %s",
      rtcHour, rtcMinute, rtcSecond, dayShortStr(timeLibWeekday), rtcDay, m, rtcYear, "UTC");
  Serial.println(buf);
}

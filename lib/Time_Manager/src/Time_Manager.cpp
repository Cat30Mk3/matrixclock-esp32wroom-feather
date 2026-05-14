#include "Time_Manager.h"
#include "globals.h"
#include <RTClib.h>

extern RTC_DS3231 rtc;

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
   time_t local = myTZ.toLocal(utc);
  int currentHour = hour(local);
  int currentMinute = minute(local);
  boolean PM = false;
  if (currentHour > 12) PM = true;
  if (hr24)
    sprintf(timeStr, "%02d:%02d", currentHour, currentMinute);
  else {
    if (PM)currentHour = currentHour - 12;
    sprintf(timeStr, "%2d:%02d", currentHour, currentMinute);
  }
}

void getCurrentDate(char *dateStr) {
  time_t utc = now();
  time_t local = myTZ.toLocal(utc);
  char m[4];
  strlcpy(m, monthShortStr(month(local)), sizeof(m));
  int currentDay = day(local);
  sprintf(dateStr, "%s %d", m, currentDay);
}

void getCurrentDOW(char *dateStr) {
  time_t utc = now();
  time_t local = myTZ.toLocal(utc);
  sprintf(dateStr, "%s", dayShortStr(weekday(local)));
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
  strcpy(m, monthShortStr(rtcMonth));
  sprintf(buf, "RTC: %.2d:%.2d:%.2d %s %.2d %s %d %s",
          rtcHour, rtcMinute, rtcSecond, dayShortStr(rtcDow), rtcDay, m, rtcYear, "UTC");
  Serial.println(buf);
}

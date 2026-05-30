#include "WiFi_Manager.h"
#include "globals.h"
#include "Parola_Display.h"
#include "Ticker_Manager.h"

namespace {
const uint32_t kInitialStaSettleDelayMs = 1500;
const int      kPerAttemptWaitSteps     = 16;
const uint32_t kPerStepDelayMs          = 750;
const uint32_t kInterAttemptBackoffMs   = 500;

bool wifiStatusZonesReady() {
#if DISPLAY_CONFIG == DISPLAY1X4
  return parola.getZoneStatus(ZONE_SINGLE);
#elif DISPLAY_CONFIG == DISPLAY2X8
  return parola.getZoneStatus(ZONE_LOWER) && parola.getZoneStatus(ZONE_UPPER);
#endif
}

void wifiDisplayStatus(const char *text) {
#if DISPLAY_CONFIG == DISPLAY1X4
  parola.displayZoneText(ZONE_SINGLE, text, PA_CENTER, H_SCROLL_SPEED, H_PAUSE, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
#elif DISPLAY_CONFIG == DISPLAY2X8
  parola.setFont(ZONE_LOWER, BigFontLower);
  parola.setFont(ZONE_UPPER, BigFontUpper);
  parola.displayZoneText(ZONE_LOWER, text, PA_CENTER, H_SCROLL_SPEED, H_PAUSE, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  parola.displayZoneText(ZONE_UPPER, text, PA_CENTER, H_SCROLL_SPEED, H_PAUSE, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
#endif
  parola.synchZoneStart();
}

// Wait for 'ms' milliseconds while keeping the Parola display animated.
// Calls parola.displayAnimate() every 50 ms so scrolling messages stay alive.
void wifiWaitWithDisplay(uint32_t ms) {
  const uint32_t kChunkMs = 50;
  uint32_t remaining = ms;
  while (remaining > 0) {
    uint32_t chunk = (remaining < kChunkMs) ? remaining : kChunkMs;
    nonBlockingDelay(chunk);
    parola.displayAnimate();
    remaining -= chunk;
  }
}

// Set a short non-blocking "WiFi X" dot-cycle label on the display.
// Restarts the scroll whenever the zone reports done so the screen is never blank.
void wifiShowConnecting(int attemptNum) {
  static char buf[12];
  const char *dots = (attemptNum % 3 == 1) ? "." : (attemptNum % 3 == 2) ? ".." : "...";
  snprintf(buf, sizeof(buf), "WiFi%s", dots);
  if (wifiStatusZonesReady()) {
    wifiDisplayStatus(buf);
  }
}

const char *wifiStatusToString(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return "IDLE";
    case WL_NO_SSID_AVAIL:
      return "NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
      return "SCAN_COMPLETED";
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_CONNECT_FAILED:
      return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
  }
}
}

bool newWiFiConnect(boolean force) {
  if (WiFi.status() == WL_CONNECTED) return true;

  if (!configDb.wifiEnabled) {
    Serial.println("[newWiFiConnect] WiFi disabled in config - skipping");
    return false;
  }

  digitalWrite(BLU_LED_PIN, LOW);
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.mode(WIFI_STA);
  // Disconnect without cycling the radio power — a full power-off/on (wifioff=true)
  // inside the retry loop only gives 250ms to reinitialize, which is insufficient.
  // Disconnect once here, let the stack settle for the full kInitialStaSettleDelayMs,
  // then retry with only a soft disconnect between attempts.
  WiFi.disconnect(false, false);
  // Show initial "WiFi." while the radio settles — starts the animation before the
  // long settle delay so the display is never blank.
  wifiDisplayStatus("WiFi.");
  wifiWaitWithDisplay(kInitialStaSettleDelayMs);

  // Resolve hostname from config; fall back to firmware default if blank.
  const char *hostname = (configDb.wifiHostname[0] != '\0') ? configDb.wifiHostname : "matrixClock";

  // Build AP list from all non-empty SSID slots. Round-robin across attempts.
  struct APEntry { const char *ssid; const char *psk; };
  APEntry apList[3];
  int apCount = 0;
  if (force) {
    if (configDb.ssid[0]  != '\0') apList[apCount++] = {configDb.ssid,  configDb.password};
    if (configDb.ssid2[0] != '\0') apList[apCount++] = {configDb.ssid2, configDb.password2};
    if (configDb.ssid3[0] != '\0') apList[apCount++] = {configDb.ssid3, configDb.password3};
    Serial.println("[newWiFiConnect] AP list:");
    for (int j = 0; j < apCount; j++) {
      Serial.print("  ["); Serial.print(j + 1); Serial.print("] ssid:"); Serial.println(apList[j].ssid);
    }
  }
  // Always need at least one entry for the loop below.
  if (apCount == 0) apList[apCount++] = {configDb.ssid, configDb.password};

  for (int i = 1; i <= 6; i++) {
    const int apIdx = (i - 1) % apCount;
    Serial.print("[newWiFiConnect] wifi not connected - attempt #");
    Serial.println(i);
    WiFi.setHostname(hostname);

    if (force) {
      WiFi.begin(apList[apIdx].ssid, apList[apIdx].psk);
    } else {
      WiFi.begin();
    }

    for (int step = 0; step < kPerAttemptWaitSteps; step++) {
      wifiWaitWithDisplay(kPerStepDelayMs);
      wifiShowConnecting(i);
      if (WiFi.status() == WL_CONNECTED) break;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[newWiFiConnect] wifi now connected");
      Serial.print("SSID: ");
      Serial.println(WiFi.SSID());
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      Serial.print("MAC address: ");
      Serial.println(WiFi.macAddress());
      tickerBlinkerInstance.detach();
      digitalWrite(BLU_LED_PIN, HIGH);
      return true;
    }

    Serial.print("[newWiFiConnect] attempt #");
    Serial.print(i);
    Serial.print(" ended with status=");
    Serial.print((int)WiFi.status());
    Serial.print(" (");
    Serial.print(wifiStatusToString(WiFi.status()));
    Serial.println(")");

    // Soft disconnect between retries — no radio power cycle.
    WiFi.disconnect(false, false);
    wifiWaitWithDisplay(kInterAttemptBackoffMs);
  }

  Serial.println("[newWiFiConnect] wifi failed to connect");
  tickerBlinkerInstance.attach_ms(100, tickerBlinkerISR, BLU_LED_PIN);
  return false;
}

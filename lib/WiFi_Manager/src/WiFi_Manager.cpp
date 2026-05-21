#include "WiFi_Manager.h"
#include "globals.h"
#include "Parola_Display.h"
#include "Ticker_Manager.h"

namespace {
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
  if ((WiFi.status() != WL_CONNECTED )) {
    digitalWrite(BLU_LED_PIN, LOW);
    WiFi.persistent(false);
    WiFi.setSleep(false);

    for (int i = 1; i <= 6; i++) {
      Serial.print("[newWiFiConnect] wifi not connected - attempt #");
      Serial.println(i);
      WiFi.disconnect(true, true);
      nonBlockingDelay(250);
      WiFi.mode(WIFI_STA);
      WiFi.setHostname("matrixClock");

      if (force) {
        Serial.println("[newWiFiConnect] forced saved ssid and psk");
        Serial.print("[newWiFiConnect] trying ssid:");
        Serial.println(configDb.ssid);
        Serial.println("[newWiFiConnect] trying psk:<hidden>");
        WiFi.begin(configDb.ssid, configDb.password);
      }
      else {
        WiFi.begin();
      }

      int lastStatus = -999;
      for (int waitStep = 0; waitStep < 24; waitStep++) {
        nonBlockingDelay(500);
        wl_status_t status = WiFi.status();

        if ((int)status != lastStatus) {
          Serial.print("[newWiFiConnect] status=");
          Serial.print((int)status);
          Serial.print(" (");
          Serial.print(wifiStatusToString(status));
          Serial.println(")");
          lastStatus = (int)status;
        }

        if (status == WL_CONNECTED) {
          break;
        }
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

      Serial.print("[newWiFiConnect] attempt ended with status=");
      Serial.print((int)WiFi.status());
      Serial.print(" (");
      Serial.print(wifiStatusToString(WiFi.status()));
      Serial.println(")");
    }

    Serial.println("[newWiFiMqttConnect] wifi failed to connect");
    tickerBlinkerInstance.attach_ms(100, tickerBlinkerISR, BLU_LED_PIN);
    return false;
  }
  return true;
}

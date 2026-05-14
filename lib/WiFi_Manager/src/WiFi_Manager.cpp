#include "WiFi_Manager.h"
#include "globals.h"
#include "Parola_Display.h"
#include "Ticker_Manager.h"

bool newWiFiConnect(boolean force) {
  if ((WiFi.status() != WL_CONNECTED )) {
    digitalWrite(BLU_LED_PIN, LOW);
    for (int i = 1; i <= 5; i++) {
      Serial.print("[newWiFiConnect] wifi not connected - attempt #");
      Serial.println(i);
      WiFi.disconnect();
      WiFi.mode(WIFI_STA);
      WiFi.setHostname("matrixClock");
      if (force) {
        Serial.println("[newWiFiConnect]forced saved ssid and psk");
        Serial.print("[newWiFiConnect] trying ssid:");
        Serial.println(configDb.ssid);
        Serial.print("[newWiFiConnect] trying psk:");
        Serial.println(configDb.password);
        WiFi.begin(configDb.ssid, configDb.password);
      }
      else WiFi.begin();

      for (int i = 0; i < 4; i++) {
        nonBlockingDelay(1000);
        if (WiFi.status() == WL_CONNECTED) break;
        displayHorzMessage("...");
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
    }
    Serial.println("[newWiFiMqttConnect] wifi failed to connect");
    tickerBlinkerInstance.attach_ms(100, tickerBlinkerISR, BLU_LED_PIN);
    return false;
  }
  return true;
}

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include "globals.h"
#include "Font_Data.h"
#include "Parola_Display.h"
#include "WiFi_Manager.h"
#include "MQTT_Manager.h"
#include "JSON_Handler.h"
#include "Time_Manager.h"
#include "DS18B20_Manager.h"
#include "NTP_Manager.h"
#include "ISR_Handlers.h"
#include "Ticker_Manager.h"
#include "Init_Manager.h"
#include "MatrixClock_Config.h"

// ============================================================================
// RTC INSTANCE
// ============================================================================
RTC_DS3231 rtc;

// ============================================================================
// DISPLAY & MQTT CLIENT INSTANCES
// ============================================================================
MD_Parola parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
MD_MAX72XX mx(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ============================================================================
// TICKER INSTANCES
// ============================================================================
// Ticker tickerBlinkerInstance;
// Ticker tickerDelayInstance;
// Ticker tickerMqttKeepAliveInstance;
// Ticker tickerTempStartInstance;
// Ticker tickerTempGetInstance;



// ============================================================================
// SETUP
// ============================================================================
void setup()
{
  Serial.begin(115200);
  nonBlockingDelay(2000);

  fileNameParser(projectNameFromFileName, projectVersionFromFileName, compileDateFromFileName, compileTimeFromFileName);
  Serial.println("\n##########################\nS T A R T I N G   S E T U P ");
  Serial.print("      program name: ");
  Serial.println(projectNameFromFileName);
  Serial.print("   program version: ");
  Serial.println(projectVersionFromFileName);
  Serial.print("     last compiled: ");
  Serial.print(compileDateFromFileName);
  Serial.print("  ");
  Serial.println(compileTimeFromFileName);
  Serial.println();

  parola.begin(MAX_ZONES_FULL + MAX_ZONES_HALF);

#if DISPLAY_CONFIG == DISPLAY1X4
  parola.setZone(ZONE_SINGLE, 0, ZONE_SIZE_FULL - 1);
  parola.setFont(ZONE_SINGLE, Special);
#elif DISPLAY_CONFIG == DISPLAY2X8
  parola.setFont(ZONE_LOWER, BigFontLower);
  parola.setFont(ZONE_UPPER, BigFontUpper);
  parola.setFont(ZONE_UP_LFT, Special);
  parola.setFont(ZONE_UP_RGT, Special);
  parola.setFont(ZONE_DN_LFT, Special);
  parola.setFont(ZONE_DN_RGT, Special);

  parola.setZone(ZONE_UPPER, ZONE_SIZE_FULL, (2 * ZONE_SIZE_FULL) - 1);
  parola.setZone(ZONE_LOWER, 0, ZONE_SIZE_FULL - 1);
  parola.setZone(ZONE_UP_LFT, (3 * ZONE_SIZE_HALF), (4 * ZONE_SIZE_HALF) - 1);
  parola.setZone(ZONE_UP_RGT, (2 * ZONE_SIZE_HALF), (3 * ZONE_SIZE_HALF) - 1);
  parola.setZone(ZONE_DN_LFT, ZONE_SIZE_HALF, (2 * ZONE_SIZE_HALF) - 1);
  parola.setZone(ZONE_DN_RGT, 0, ZONE_SIZE_HALF - 1);

  singleCharSpace = parola.getCharSpacing();
  doubleCharSpace = singleCharSpace * 2;
  Serial.print("single char spacing:");
  Serial.println(singleCharSpace);
  Serial.print("double char spacing:");
  Serial.println(doubleCharSpace);
  parola.setCharSpacing(doubleCharSpace);

  if (invertUpperZone)
  {
    parola.setZoneEffect(ZONE_UPPER, true, PA_FLIP_UD);
    parola.setZoneEffect(ZONE_UPPER, true, PA_FLIP_LR);
  }
#endif

  displayHorzMessage("Setup...");
  displayHorzMessage(projectVersionFromFileName);

  initializeDispParam();
  intializeTemperatures();
  initializeGPIOPins();
  initializeGlobalVariables();

  MatrixClockConfigInitResult configInitResult = {false, false};
  if (matrixClockConfigInitializeRuntimeConfig(configInitResult))
  {
    if (configInitResult.loadedFromNvs)
    {
      Serial.println("[CONFIG] Loaded runtime configuration from NVS");
    }
    else
    {
      Serial.println("[CONFIG] Loaded bootstrap defaults (secrets) into runtime config");
      if (configInitResult.seededNvsFromBootstrap)
      {
        Serial.println("[CONFIG] Seeded NVS from bootstrap defaults");
      }
      else
      {
        Serial.println("[CONFIG] WARNING: Bootstrap defaults loaded but NVS seed write failed");
      }
    }
  }
  else
  {
    Serial.println("[CONFIG] WARNING: Runtime config initialization reported failure");
  }

  if (matrixClockConfigRegisterPortalContracts())
  {
    Serial.println("[CONFIG] AP portal schema registration OK");
  }
  else
  {
    Serial.println("[CONFIG] WARNING: AP portal schema registration failed");
  }

  // Scan I2C bus for devices
  Serial.println("\nScanning I2C bus for devices...");
  Wire.begin();
  Wire.setClock(100000);  // Set I2C clock to 100kHz (standard for DS3231)
  delay(100);  // Give I2C time to stabilize
  
  int devicesFound = 0;
  for (uint8_t addr = 1; addr < 127; addr++)
  {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    if (error == 0)
    {
      Serial.print("I2C device found at 0x");
      Serial.println(addr, HEX);
      devicesFound++;
    }
  }
  Serial.print("Total I2C devices found: ");
  Serial.println(devicesFound);
  Serial.println("(DS3231 RTC should be at 0x68)");

  delay(100);  // Additional delay before RTC initialization
  boolean rtcPresent = false;
  if (rtc.begin())
  {
    displayHorzMessage("RTC Up");
    Serial.println("RTC initialized successfully");
    rtcPresent = true;
  }
  else
  {
    displayHorzMessage("RTC Dn");
    Serial.println("ERROR: rtc.begin() failed - device at 0x68 found but not responding to RTClib commands");
    Serial.println("Check: battery voltage, crystal oscillator, pullup resistors on SDA/SCL");
    Serial.println("System will continue using NTP for time synchronization");
    rtcPresent = false;
  }

  if (rtcPresent)
  {
    customSetTimeFromRTC();
  }
  else
  {
    // Keep a sane baseline until NTP sync when RTC is unavailable.
    setTime(compileTime());
  }

  Serial.print("Time after RTC stage:");
  digitalClockDisplay();

  displayHorzMessage("Starting WiFi..");
  if (newWiFiConnect(true))
    displayHorzMessage("WiFi Up");
  else
    displayHorzMessage("WiFi Dn");

  if (newMqttConnect())
    displayHorzMessage("MQTT Up");
  else
    displayHorzMessage("MQTT Dn");

  displayHorzMessage("Starting NTP..");
  Udp.begin(localPort);
  // Serial.println("waiting for sync");
  delay(2000);
  setSyncProvider(getNtpTime);
  setSyncInterval(24 * 60 * 60);

  if (rtcPresent)
  {
    printRTCTime();
    rtc.adjust(now());
    printRTCTime();
  }
  else
  {
    Serial.println("RTC adjust skipped (RTC not available)");
  }

  Serial.print("Time after NTP sync:");
  digitalClockDisplay();

  tickerBlinkerInstance.attach_ms(500, tickerBlinkerISR, BLU_LED_PIN);
  tickerMqttKeepAliveInstance.attach_ms(3000, tickerKeepAliveMqttISR);
  tickerTempStartInstance.attach_ms(30000, tickerNonBlockingTempStartISR);

  Serial.println("S E T U P   C O M P L E T E");
}

// ============================================================================
// LOOP
// ============================================================================
void loop()
{
  char displayString[10];
  parola.synchZoneStart();
  parola.displayAnimate();

  boolean groupMode1Done = false;
  boolean groupMode2Done = false;
  boolean groupMode3Done = false;

#if DISPLAY_CONFIG == DISPLAY1X4
  if (parola.getZoneStatus(ZONE_SINGLE))
#elif DISPLAY_CONFIG == DISPLAY2X8
  if (parola.getZoneStatus(ZONE_LOWER) && parola.getZoneStatus(ZONE_UPPER))
#endif
  {
    time_t utc = now();
    time_t local = myTZ.toLocal(utc);

    getCurrentDOW(dispParam[DISP_CURR_DOW].dispBuffer);
    getCurrentDate(dispParam[DISP_CURR_DATE].dispBuffer);
    dispParam[DISP_CURR_DOW].dispReady = true;
    dispParam[DISP_CURR_DATE].dispReady = true;

    if (!mqttAlive)
    {
      Serial.println("ALERT - mqtt not connected");
      if (newMqttConnect())
        Serial.println("RESOLVED - mqtt successfully reconnected");
    }

    for (int paramIndex = 1; paramIndex < DISP_PARAM_TOTAL_COUNT; paramIndex++)
    {
      if (!dispParam[paramIndex].enabled || !dispParam[paramIndex].dispReady || dispParam[paramIndex].groupMode != 0)
        continue;

      if (paramIndex == DISP_CURR_MQTT_COT_MSG || paramIndex == DISP_CURR_MQTT_HOM_MSG)
        displayHorzMessage(dispParam[paramIndex].dispBuffer);
      else
      {
        displayVertMessage(dispParam[paramIndex].dispBuffer);
      }

      getCurrentTime(displayString, false);
      displayVertMessage(displayString);

      Serial.print("verticle parameter display index:");
      Serial.println(paramIndex);
      Serial.print("parameter display dispBuffer:");
      Serial.println(dispParam[paramIndex].dispBuffer);

      if (dispParam[paramIndex].lastReceivedUpdate + MQTT_TELE_TOPIC_TIMEOUT_MS < millis())
      {
        dispParam[paramIndex].dispReady = false;
      }
    }

#if DISPLAY_CONFIG == DISPLAY2X8
    if (serviceQuadPage(2,
                        dispParam[DISP_CURR_WIRED_TEMP_IN],
                        dispParam[DISP_CURR_WIRED_TEMP_OUT],
                        dispParam[DISP_CURR_WIRED_TEMP_IN],
                        dispParam[DISP_CURR_WIRED_TEMP_IN]))
    {
      getCurrentTime(displayString, false);
      displayVertMessage(displayString);
    }

    if (serviceQuadPage(4,
                        dispParam[DISP_CURR_MQTT_COT_TEMP_IN],
                        dispParam[DISP_CURR_MQTT_COT_TEMP_OUT],
                        dispParam[DISP_CURR_MQTT_COT_TEMP_CRL],
                        dispParam[DISP_CURR_MQTT_COT_TEMP_SMP]))
    {
      getCurrentTime(displayString, false);
      displayVertMessage(displayString);
    }

    if (serviceQuadPage(4,
                        dispParam[DISP_CURR_MQTT_HOM_TEMP_IN],
                        dispParam[DISP_CURR_MQTT_HOM_TEMP_OUT],
                        dispParam[DISP_CURR_MQTT_HOM_TEMP_ATT],
                        dispParam[DISP_CURR_MQTT_HOM_TEMP_CAV]))
    {
      getCurrentTime(displayString, false);
      displayVertMessage(displayString);
    }
#endif
  }
}



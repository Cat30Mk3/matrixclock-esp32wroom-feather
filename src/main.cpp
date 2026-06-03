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
#include <SPI.h>
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
#include "Mode_Manager.h"
#include "AP_Config_Portal.h"

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
namespace {
const ModeManagerConfig kPassiveModeManagerConfig = {
  3000,
  1200,
  4000,
  5000
};

bool s_apRuntimeActive = false;
char s_apSsid[40] = "";           // stored for stage-1 display
uint8_t s_lastApStationCount = 0; // tracks first client connection

void serviceStagedWiredTemps()
{
  for (int index = 0; index < 2; index++)
  {
    bool ready = false;
    uint32_t updateStamp = 0;
    char stagedValue[30] = {0};

    noInterrupts();
    ready = g_wiredTempStageReady[index];
    if (ready)
    {
      strncpy(stagedValue, g_wiredTempStageBuffer[index], sizeof(stagedValue) - 1);
      stagedValue[sizeof(stagedValue) - 1] = '\0';
      updateStamp = g_wiredTempStageMillis[index];
      g_wiredTempStageReady[index] = false;
    }
    interrupts();

    if (!ready)
    {
      continue;
    }

    const int paramIndex = DISP_CURR_WIRED_TEMP_IN + index;
    strncpy(dispParam[paramIndex].dispBuffer, stagedValue, sizeof(dispParam[paramIndex].dispBuffer) - 1);
    dispParam[paramIndex].dispBuffer[sizeof(dispParam[paramIndex].dispBuffer) - 1] = '\0';
    dispParam[paramIndex].dispReady = true;
    dispParam[paramIndex].lastReceivedUpdate = updateStamp;
  }
}

void startApSetupRuntime()
{
  if (s_apRuntimeActive)
  {
    return;
  }

  const char *baseName = (projectNameFromFileName[0] != '\0') ? projectNameFromFileName : "matrixClock";
  snprintf(s_apSsid, sizeof(s_apSsid), "%s-AP", baseName);
  s_lastApStationCount = 0;

  WiFi.mode(WIFI_AP_STA);
  const bool apStarted = WiFi.softAP(s_apSsid, nullptr, 1, false, 1); // max 1 client

  apPortalBegin();
  const bool portalRegistered = matrixClockConfigRegisterPortalContracts();
  const bool portalServerStarted = portalRegistered && apPortalStartServer(80);

  Serial.print("[AP] start ssid=");
  Serial.println(s_apSsid);
  Serial.print("[AP] softAP=");
  Serial.println(apStarted ? "up" : "failed");
  Serial.print("[AP] portalContracts=");
  Serial.println(portalRegistered ? "ok" : "failed");
  Serial.print("[AP] portalServer=");
  Serial.println(portalServerStarted ? "up" : "failed");
  if (portalServerStarted)
  {
    Serial.print("[AP] portal pin=");
    Serial.println(apPortalGetLoginPin());
    Serial.println("[AP] browse http://192.168.4.1/");
  }
  if (apStarted)
  {
    Serial.print("[AP] ip=");
    Serial.println(WiFi.softAPIP());
  }

  s_apRuntimeActive = apStarted;
}

void stopApSetupRuntime()
{
  if (!s_apRuntimeActive)
  {
    return;
  }

  apPortalStopServer();
  apPortalEnd();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  s_apRuntimeActive = false;
  // Reload from NVS to discard any unsaved portal edits (e.g. inactivity timeout mid-edit)
  MatrixClockConfigInitResult reloadResult = {false, false};
  matrixClockConfigInitializeRuntimeConfig(reloadResult);
  Serial.println("[AP] stopped; config reloaded from NVS");
}

void serviceModeManagerPassiveDiagnostics()
{
  static bool s_lastConfirmPromptActive = false;
  static uint32_t s_lastButtonSnapshotMs = 0;

  modeManagerServiceButtonDiagnostics();
  modeManagerService();

  if ((millis() - s_lastButtonSnapshotMs) >= 2000)
  {
    s_lastButtonSnapshotMs = millis();
    Serial.print("[BTN-SNAPSHOT] SEL=");
    Serial.print(digitalRead(PB_SEL_PIN));
    Serial.print(" DEC=");
    Serial.print(digitalRead(PB_DEC_PIN));
    Serial.print(" INC=");
    Serial.print(digitalRead(PB_INC_PIN));
    Serial.print(" CNL=");
    Serial.print(digitalRead(PB_CNL_PIN));
    Serial.print(" MEN=");
    Serial.println(digitalRead(PB_MEN_PIN));
  }

  bool confirmActive = modeManagerIsConfirmPromptActive();
  if (confirmActive != s_lastConfirmPromptActive)
  {
    s_lastConfirmPromptActive = confirmActive;
    Serial.print("[MODE] confirmPrompt=");
    Serial.println(confirmActive ? 1 : 0);
  }
}

void handleModeEntryEvents()
{
  MatrixClockMode enteredMode;
  if (!modeManagerConsumeModeEntryEvent(enteredMode))
  {
    return;
  }

  Serial.print("[MODE] entry mode=");
  Serial.println(modeManagerGetModeName(enteredMode));

  if (enteredMode == MATRIXCLOCK_MODE_AP_SETUP)
  {
    startApSetupRuntime();
  }
  else if (enteredMode == MATRIXCLOCK_MODE_RECOVERY)
  {
    Serial.println("[MODE] recovery mode entered - starting AP portal");
    startApSetupRuntime();
  }
  else if (enteredMode == MATRIXCLOCK_MODE_NORMAL)
  {
    stopApSetupRuntime();
    displayHorzMessage("Normal");
  }
}

void serviceApModeDisplay()
{
  static bool           s_apDisplayInitialized = false;
  static APDisplayStage s_lastRenderedStage     = AP_STAGE_SSID;
  static char           s_currentBanner[32]     = "";
  const uint16_t kApScrollSpeed = 45;
  const uint16_t kApPauseMs     = 700;

  auto startApBanner = [&](const char *text) {
#if DISPLAY_CONFIG == DISPLAY1X4
    parola.displayZoneText(ZONE_SINGLE, text, PA_CENTER, kApScrollSpeed, kApPauseMs, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
#elif DISPLAY_CONFIG == DISPLAY2X8
    parola.setFont(ZONE_LOWER, BigFontLower);
    parola.setFont(ZONE_UPPER, BigFontUpper);
    parola.displayZoneText(ZONE_LOWER, text, PA_CENTER, kApScrollSpeed, kApPauseMs, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    parola.displayZoneText(ZONE_UPPER, text, PA_CENTER, kApScrollSpeed, kApPauseMs, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
#endif
    parola.synchZoneStart();
  };

  if (!modeManagerInApControlMode())
  {
    s_apDisplayInitialized = false;
    s_lastRenderedStage    = AP_STAGE_SSID;
    return;
  }

  APDisplayStage stage = apPortalGetDisplayStage();

  // Rebuild banner text whenever the stage changes or on first call
  if (stage != s_lastRenderedStage || !s_apDisplayInitialized)
  {
    if (stage == AP_STAGE_SSID)
    {
      strlcpy(s_currentBanner, s_apSsid[0] ? s_apSsid : "matrixClock-AP", sizeof(s_currentBanner));
    }
    else if (stage == AP_STAGE_IP)
    {
      strlcpy(s_currentBanner, "192.168.4.1", sizeof(s_currentBanner));
    }
    else if (stage == AP_STAGE_PIN)
    {
      const char *pin = apPortalGetLoginPin();
      snprintf(s_currentBanner, sizeof(s_currentBanner), "PIN %s", pin ? pin : "----");
    }
    else // AP_STAGE_ACTIVE
    {
      strlcpy(s_currentBanner, "AP Active", sizeof(s_currentBanner));
    }

    parola.displayClear();
    startApBanner(s_currentBanner);
    s_apDisplayInitialized = true;
    s_lastRenderedStage    = stage;
    return; // let the first animation frame settle
  }

  // Re-arm scroll when animation completes (message repeats per spec)
#if DISPLAY_CONFIG == DISPLAY1X4
  if (parola.getZoneStatus(ZONE_SINGLE))
#elif DISPLAY_CONFIG == DISPLAY2X8
  if (parola.getZoneStatus(ZONE_LOWER) && parola.getZoneStatus(ZONE_UPPER))
#endif
  {
    startApBanner(s_currentBanner);
  }

  parola.displayAnimate();
}

void initializeDisplayBusState()
{
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  pinMode(CLK_PIN, OUTPUT);
  digitalWrite(CLK_PIN, LOW);
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, LOW);
  SPI.begin(CLK_PIN, -1, DATA_PIN, CS_PIN);
  delay(5);
}

void normalizeDisplayState()
{
  MD_MAX72XX *gfx = parola.getGraphicObject();
  if (gfx == nullptr)
  {
    return;
  }

  gfx->control(MD_MAX72XX::TEST, MD_MAX72XX::OFF);
  gfx->control(MD_MAX72XX::SHUTDOWN, MD_MAX72XX::OFF);
}
}

void setup()
{
  Serial.begin(115200);
  nonBlockingDelay(2000);

  initializeDisplayBusState();

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
  normalizeDisplayState();
  parola.setIntensity(3);
  parola.displayClear();

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
  modeManagerBegin(kPassiveModeManagerConfig);
  modeManagerLogButtonPinMapping();
  Serial.print("[MODE] initialized in mode=");
  Serial.println(modeManagerGetModeName(modeManagerGetMode()));
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
    if (matrixClockConfigValidatePortalFieldMappings())
    {
      Serial.println("[CONFIG] AP portal field mappings validated");
    }
    else
    {
      Serial.println("[CONFIG] WARNING: AP portal field mappings validation failed");
    }
  }
  else
  {
    Serial.println("[CONFIG] WARNING: AP portal schema registration failed");
  }

  // Boot recovery: Menu+Select held through Reset forces AP mode, skipping network startup.
  if (modeManagerCheckBootRecoveryRequest()) {
    Serial.println("[BOOT] Recovery combo detected - skipping network startup");
    displayHorzMessage("Recovery..");
    modeManagerSetBackgroundPollingEnabled(true);
    Serial.println("S E T U P   C O M P L E T E  (recovery)");
    return;
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

  if (configDb.wifiEnabled) {
    displayHorzMessage("Starting WiFi..");
    if (newWiFiConnect(true))
      displayHorzMessage("WiFi Up");
    else
      displayHorzMessage("WiFi Dn");
  } else {
    Serial.println("[setup] WiFi disabled in config - skipping connect");
    displayHorzMessage("WiFi Off");
  }

  if (configDb.mqttEnabled) {
    if (newMqttConnect())
      displayHorzMessage("MQTT Up");
    else
      displayHorzMessage("MQTT Dn");
  } else {
    Serial.println("[setup] MQTT disabled in config - skipping connect");
    displayHorzMessage("MQTT Off");
  }

  if (configDb.wifiEnabled && g_matrixClockRuntimeConfig.configDb.wifiEnabled) {
    displayHorzMessage("Starting NTP..");
    Udp.begin(localPort);
    delay(2000);
    setSyncProvider(getNtpTime);
    setSyncInterval(24 * 60 * 60);
  } else {
    Serial.println("[setup] NTP skipped - WiFi disabled, RTC is sole time source");
  }

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
  tickerTempStartInstance.attach_ms(30000, tickerNonBlockingTempStartISR);

  modeManagerSetBackgroundPollingEnabled(true);

  Serial.println("S E T U P   C O M P L E T E");
}

// ============================================================================
// LOOP
// ============================================================================
void loop()
{
  static uint32_t lastDisplayNormalizeMs = 0;
  static uint32_t lastApModeHeartbeatMs = 0;
  static uint32_t lastHealthLogMs = 0;

#if DEBUG_LOOP_BREADCRUMBS
  Serial.println("[LOOP] enter");
#endif

  serviceModeManagerPassiveDiagnostics();
  handleModeEntryEvents();

  if (!modeManagerInApControlMode() && s_apRuntimeActive)
  {
    stopApSetupRuntime();
    // Safety net for Cancel & Exit: ESP32 WIFI_AP_STA normally maintains the
    // existing STA connection, so these are usually no-ops. If STA did drop
    // (edge case), restore WiFi and MQTT without requiring a reboot.
    if (configDb.wifiEnabled && WiFi.status() != WL_CONNECTED)
    {
      newWiFiConnect(true);
      if (configDb.mqttEnabled && WiFi.status() == WL_CONNECTED && !mqttAlive)
        newMqttConnect();
    }
  }

  if (configDb.mqttEnabled && configDb.wifiEnabled) {
    mqttServiceKeepAlive();
  }

  if (millis() - lastHealthLogMs >= 60000)
  {
    lastHealthLogMs = millis();
    Serial.print("[HEALTH] up_s=");
    Serial.print(static_cast<unsigned long>(millis() / 1000UL));
    Serial.print(" freeHeap=");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" minFreeHeap=");
    Serial.print(ESP.getMinFreeHeap());
    Serial.print(" maxAllocHeap=");
    Serial.print(ESP.getMaxAllocHeap());
    Serial.print(" wifi=");
    Serial.print((WiFi.status() == WL_CONNECTED) ? "UP" : "DOWN");
    Serial.print(" mqtt=");
    Serial.println(mqttAlive ? "UP" : "DOWN");
  }

  if (millis() - lastDisplayNormalizeMs >= 1000)
  {
    lastDisplayNormalizeMs = millis();
#if DEBUG_LOOP_BREADCRUMBS
    Serial.println("[LOOP] display normalize tick");
#endif
#if !DEBUG_DISABLE_DISPLAY_NORMALIZE
    normalizeDisplayState();
#else
#if DEBUG_LOOP_BREADCRUMBS
    Serial.println("[LOOP] display normalize skipped");
#endif
#endif
  }

  if (modeManagerInApControlMode())
  {
    // Track the first STA connection to advance the display from SSID → IP stage
    uint8_t stationCount = WiFi.softAPgetStationNum();
    if (stationCount > 0 && s_lastApStationCount == 0 && apPortalGetDisplayStage() == AP_STAGE_SSID)
    {
      apPortalSignalClientConnected();
      Serial.println("[AP] First client connected — advancing display to IP stage");
    }
    s_lastApStationCount = stationCount;

    apPortalService();

    // Portal-initiated exit (Save & Exit, Cancel & Exit, inactivity timeout)
    if (apPortalShouldExit())
    {
      bool wasSave = apPortalExitWasSave();
      apPortalClearExitRequest();
      if (wasSave) {
        // Reboot so all saved settings (WiFi creds, toggles, hostname, etc.) take effect cleanly.
        displayHorzMessage("Rebooting..");
        nonBlockingDelay(500);
        ESP.restart();
      }
      modeManagerRequestNormalMode();
      nonBlockingDelay(20);
      return;
    }

    serviceApModeDisplay();

    if (millis() - lastApModeHeartbeatMs >= 5000)
    {
      lastApModeHeartbeatMs = millis();
      Serial.print("[MODE] active=");
      Serial.println(modeManagerGetModeName(modeManagerGetMode()));
    }

    nonBlockingDelay(20);
    return;
  }

  serviceStagedWiredTemps();

  char displayString[10];
#if DEBUG_LOOP_BREADCRUMBS
  Serial.println("[LOOP] before synch/displayAnimate");
#endif
#if !DEBUG_DISABLE_LOOP_SYNCHZONESTART
  parola.synchZoneStart();
#else
#if DEBUG_LOOP_BREADCRUMBS
  Serial.println("[LOOP] synchZoneStart skipped");
#endif
#endif
#if !DEBUG_DISABLE_LOOP_DISPLAYANIMATE
#if DISPLAY_CONFIG == DISPLAY1X4
  if (!parola.getZoneStatus(ZONE_SINGLE))
  {
    parola.displayAnimate();
  }
#elif DISPLAY_CONFIG == DISPLAY2X8
  if (!(parola.getZoneStatus(ZONE_LOWER) && parola.getZoneStatus(ZONE_UPPER)))
  {
    parola.displayAnimate();
  }
#endif
#else
#if DEBUG_LOOP_BREADCRUMBS
  Serial.println("[LOOP] displayAnimate skipped");
#endif
#endif

#if DEBUG_LOOP_BREADCRUMBS
  Serial.println("[LOOP] after synch/displayAnimate");
#endif

  boolean groupMode1Done = false;
  boolean groupMode2Done = false;
  boolean groupMode3Done = false;

#if DISPLAY_CONFIG == DISPLAY1X4
  if (parola.getZoneStatus(ZONE_SINGLE))
#elif DISPLAY_CONFIG == DISPLAY2X8
  if (parola.getZoneStatus(ZONE_LOWER) && parola.getZoneStatus(ZONE_UPPER))
#endif
  {
    getCurrentDOW(dispParam[DISP_CURR_DOW].dispBuffer);
    getCurrentDate(dispParam[DISP_CURR_DATE].dispBuffer);
    dispParam[DISP_CURR_DOW].dispReady = true;
    dispParam[DISP_CURR_DATE].dispReady = true;

    if (configDb.mqttEnabled && configDb.wifiEnabled && WiFi.status() == WL_CONNECTED)
    {
      if (!mqttAlive)
      {
        Serial.println("ALERT - mqtt not connected");
        if (newMqttConnect())
          Serial.println("RESOLVED - mqtt successfully reconnected");
      }
    }

    for (int paramIndex = 1; paramIndex < DISP_PARAM_TOTAL_COUNT; paramIndex++)
    {
      if (modeManagerInApControlMode())
      {
        break;
      }

      if (!dispParam[paramIndex].enabled || !dispParam[paramIndex].dispReady || dispParam[paramIndex].groupMode != 0)
        continue;

      if (paramIndex == DISP_CURR_MQTT_COT_MSG || paramIndex == DISP_CURR_MQTT_HOM_MSG)
        displayHorzMessage(dispParam[paramIndex].dispBuffer);
      else
      {
      #if DISPLAY_CONFIG == DISPLAY1X4
        if ((paramIndex == DISP_CURR_WIRED_TEMP_IN || paramIndex == DISP_CURR_WIRED_TEMP_OUT) && DEBUG_DISABLE_DISPLAY1X4_WIRED_TEMP_VERTICAL)
        {
      #if DEBUG_DISPLAY_TRACE
          Serial.print("[DISP][MAIN] wired temp vertical skipped index=");
          Serial.println(paramIndex);
      #endif
          continue;
        }
      #endif
#if DEBUG_DISPLAY_TRACE
        Serial.print("[DISP][MAIN] vertical payload index=");
        Serial.println(paramIndex);
#endif
        displayVertMessage(dispParam[paramIndex].dispBuffer);
      }

#if !DEBUG_DISABLE_TIME_INSERT_AFTER_VERTICAL
      getCurrentTime(displayString, false);
      displayVertMessage(displayString);
#else
#if DEBUG_DISPLAY_TRACE
      Serial.println("[DISP][MAIN] time insert skipped");
#endif
#endif

      Serial.print("verticle parameter display index:");
      Serial.println(paramIndex);
      Serial.print("parameter display dispBuffer:");
      Serial.println(dispParam[paramIndex].dispBuffer);

      if (dispParam[paramIndex].lastReceivedUpdate + MQTT_TELE_TOPIC_TIMEOUT_MS < millis())
      {
        dispParam[paramIndex].dispReady = false;
      }

      if (modeManagerInApControlMode())
      {
        break;
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

#if DEBUG_LOOP_BREADCRUMBS
  Serial.println("[LOOP] exit");
#endif
}



#include "MatrixClock_Config.h"
#include "AP_Config_Portal.h"
#include <Preferences.h>

namespace {
const char *kConfigNamespace = "mcfg";
const char *kSchemaKey = "schema";
const char *kConfigBlobKey = "cfgblob";

void applyRuntimeConfigToLegacyGlobals(const MatrixClockRuntimeConfig &runtimeConfig) {
  configDb = runtimeConfig.configDb;
}

bool portalLoadConfig(void *context) {
  (void)context;
  MatrixClockConfigInitResult result = {false, false};
  return matrixClockConfigInitializeRuntimeConfig(result);
}

bool portalSaveConfig(void *context) {
  (void)context;
  return matrixClockConfigPersistActiveRuntimeConfig();
}

bool portalApplyConfig(void *context) {
  (void)context;
  applyRuntimeConfigToLegacyGlobals(g_matrixClockRuntimeConfig);
  return true;
}

bool portalGetStatus(void *context, APPortalStatus &status) {
  (void)context;
  status.apModeActive = false;
  status.stationConnected = (WiFi.status() == WL_CONNECTED);
  status.mqttConnected = mqttClient.connected();
  status.timeValid = (timeStatus() != timeNotSet);
  status.timeSourceMode = "RTC_OR_NTP";
  return true;
}

const APFieldDefinition kPortalFields[] = {
  {"wifi", "wifi_enabled", "WiFi Enabled", AP_FIELD_TOGGLE, 1, true, nullptr, 0},
  {"wifi", "wifi_ssid_1", "WiFi SSID #1", AP_FIELD_TEXT, 29, false, nullptr, 0},
  {"wifi", "wifi_password_1", "WiFi Password #1", AP_FIELD_PASSWORD, 29, false, nullptr, 0},
  {"wifi", "wifi_ssid_2", "WiFi SSID #2", AP_FIELD_TEXT, 29, false, nullptr, 0},
  {"wifi", "wifi_password_2", "WiFi Password #2", AP_FIELD_PASSWORD, 29, false, nullptr, 0},
  {"wifi", "wifi_ssid_3", "WiFi SSID #3", AP_FIELD_TEXT, 29, false, nullptr, 0},
  {"wifi", "wifi_password_3", "WiFi Password #3", AP_FIELD_PASSWORD, 29, false, nullptr, 0},

  {"mqtt", "mqtt_enabled", "MQTT Enabled", AP_FIELD_TOGGLE, 1, true, nullptr, 0},
  {"mqtt", "mqtt_server", "MQTT Server", AP_FIELD_TEXT, 34, false, nullptr, 0},
  {"mqtt", "mqtt_user", "MQTT User", AP_FIELD_TEXT, 99, false, nullptr, 0},
  {"mqtt", "mqtt_password", "MQTT Password", AP_FIELD_PASSWORD, 34, false, nullptr, 0},
  {"mqtt", "mqtt_client_id", "MQTT Client ID", AP_FIELD_TEXT, 34, false, nullptr, 0},
  {"mqtt", "mqtt_device_name_1", "MQTT Device Name #1", AP_FIELD_TEXT, 39, false, nullptr, 0},
  {"mqtt", "mqtt_device_name_2", "MQTT Device Name #2", AP_FIELD_TEXT, 39, false, nullptr, 0},
  {"mqtt", "mqtt_device_name_3", "MQTT Device Name #3", AP_FIELD_TEXT, 39, false, nullptr, 0},
  {"mqtt", "mqtt_topic_cmd", "Lamp Command Topic", AP_FIELD_TEXT, 19, false, nullptr, 0},
  {"mqtt", "mqtt_topic_stat", "Lamp Status Topic", AP_FIELD_TEXT, 19, false, nullptr, 0}
};
}

MatrixClockRuntimeConfig g_matrixClockRuntimeConfig = {
  MATRIXCLOCK_CONFIG_SCHEMA_VERSION,
  {}
};

bool matrixClockConfigRegisterPortalContracts() {
  bool ok = true;

  ok = apPortalRegisterPage("wifi", "WiFi Settings") && ok;
  ok = apPortalRegisterPage("mqtt", "MQTT Settings") && ok;

  for (size_t i = 0; i < (sizeof(kPortalFields) / sizeof(kPortalFields[0])); ++i) {
    ok = apPortalRegisterField(kPortalFields[i]) && ok;
  }

  APPortalCallbacks callbacks = {
    nullptr,
    portalLoadConfig,
    portalSaveConfig,
    portalApplyConfig,
    portalGetStatus
  };
  apPortalSetCallbacks(callbacks);

  return ok;
}

bool matrixClockConfigLoadFromNvs(MatrixClockRuntimeConfig &outConfig) {
  Preferences prefs;
  if (!prefs.begin(kConfigNamespace, true)) {
    return false;
  }

  const uint16_t storedVersion = prefs.getUShort(kSchemaKey, 0);
  if (!matrixClockConfigIsSchemaCompatible(storedVersion)) {
    prefs.end();
    return false;
  }

  const size_t storedLength = prefs.getBytesLength(kConfigBlobKey);
  if (storedLength != sizeof(configDb_t)) {
    prefs.end();
    return false;
  }

  size_t readLength = prefs.getBytes(kConfigBlobKey, &outConfig.configDb, sizeof(configDb_t));
  prefs.end();

  if (readLength != sizeof(configDb_t)) {
    return false;
  }

  outConfig.schemaVersion = storedVersion;
  return true;
}

bool matrixClockConfigSaveToNvs(const MatrixClockRuntimeConfig &config) {
  Preferences prefs;
  if (!prefs.begin(kConfigNamespace, false)) {
    return false;
  }

  bool ok = true;
  const size_t versionWritten = prefs.putUShort(kSchemaKey, config.schemaVersion);
  const size_t blobWritten = prefs.putBytes(kConfigBlobKey, &config.configDb, sizeof(configDb_t));

  if (versionWritten != sizeof(uint16_t)) {
    ok = false;
  }

  if (blobWritten != sizeof(configDb_t)) {
    ok = false;
  }

  prefs.end();
  return ok;
}

bool matrixClockConfigIsSchemaCompatible(uint16_t storedVersion) {
  return storedVersion == MATRIXCLOCK_CONFIG_SCHEMA_VERSION;
}

void matrixClockConfigLoadBootstrapDefaults(MatrixClockRuntimeConfig &outConfig) {
  outConfig.schemaVersion = MATRIXCLOCK_CONFIG_SCHEMA_VERSION;
  outConfig.configDb = configDb;
}

bool matrixClockConfigInitializeRuntimeConfig(MatrixClockConfigInitResult &outResult) {
  outResult.loadedFromNvs = false;
  outResult.seededNvsFromBootstrap = false;

  if (matrixClockConfigLoadFromNvs(g_matrixClockRuntimeConfig)) {
    outResult.loadedFromNvs = true;
    applyRuntimeConfigToLegacyGlobals(g_matrixClockRuntimeConfig);
    return true;
  }

  matrixClockConfigLoadBootstrapDefaults(g_matrixClockRuntimeConfig);
  applyRuntimeConfigToLegacyGlobals(g_matrixClockRuntimeConfig);
  outResult.seededNvsFromBootstrap = matrixClockConfigSaveToNvs(g_matrixClockRuntimeConfig);
  return true;
}

const MatrixClockRuntimeConfig &matrixClockConfigGetActiveRuntimeConfig() {
  return g_matrixClockRuntimeConfig;
}

void matrixClockConfigSetActiveRuntimeConfig(const MatrixClockRuntimeConfig &runtimeConfig) {
  g_matrixClockRuntimeConfig = runtimeConfig;
  applyRuntimeConfigToLegacyGlobals(g_matrixClockRuntimeConfig);
}

bool matrixClockConfigPersistActiveRuntimeConfig() {
  return matrixClockConfigSaveToNvs(g_matrixClockRuntimeConfig);
}

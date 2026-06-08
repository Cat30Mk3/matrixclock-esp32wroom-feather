#include "MatrixClock_Config.h"
#include "AP_Config_Portal.h"
#include "Mode_Manager.h"
#include "Time_Manager.h"
#include <Preferences.h>
#include <RTClib.h>
#include <time.h>

extern RTC_DS3231 rtc;

namespace {
// Size of v1 configDb_t blob — used for v1->v2 migration detection.
static const size_t kV1ConfigDbSize = 570;
// Size of v2 configDb_t blob — used for v2->v3 migration detection.
static const size_t kV2ConfigDbSize = 724;
// Build-time guard: all-char + bool + uint8 fields → no padding expected on ESP32.
static_assert(sizeof(configDb_t) == 725, "configDb_t size mismatch — check struct padding");

const char *kConfigNamespace = "mcfg";
const char *kSchemaKey = "schema";
const char *kConfigBlobKey = "cfgblob";
const char *kMappedFieldIds[] = {
  "wifi_enabled",
  "wifi_ssid_1",
  "wifi_password_1",
  "wifi_hostname",
  "wifi_ssid_2",
  "wifi_password_2",
  "wifi_ssid_3",
  "wifi_password_3",
  "mqtt_enabled",
  "mqtt_server",
  "mqtt_user",
  "mqtt_password",
  "mqtt_client_id",
  "mqtt_device_name_1",
  "mqtt_device_name_2",
  "mqtt_device_name_3",
  "mqtt_topic_cmd",
  "mqtt_topic_stat"
};

// Ephemeral buffer for datetime-local input; set by setFieldValue, consumed by applyConfig.
static char s_dtLocalBuf[17] = {0}; // "YYYY-MM-DDTHH:MM"

// Timezone options for the portal datetime page select (values are string indices into kTimezoneTable).
static const APFieldOption kTzOptions[] = {
  {"0", "AST/ADT (Atlantic)"},
  {"1", "EST/EDT (Eastern)"},
  {"2", "CST/CDT (Central)"},
  {"3", "MST/MDT (Mountain)"},
  {"4", "PST/PDT (Pacific)"},
  {"5", "UTC"},
};

bool copyToOutputBuffer(const char *source, char *outValue, size_t outValueLen) {
  if (outValue == nullptr || outValueLen == 0) {
    return false;
  }
  strlcpy(outValue, source == nullptr ? "" : source, outValueLen);
  return true;
}

bool writeConfigField(char *destination, size_t destinationLen, const char *value) {
  if (destination == nullptr || destinationLen == 0 || value == nullptr) {
    return false;
  }
  strlcpy(destination, value, destinationLen);
  return true;
}

void applyRuntimeConfigToLegacyGlobals(const MatrixClockRuntimeConfig &runtimeConfig) {
  configDb = runtimeConfig.configDb;
  applyTimezone(configDb.tzIndex);
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

  // If a dt_local value was staged by setFieldValue, perform the time set now.
  if (s_dtLocalBuf[0] != '\0') {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0;
    sscanf(s_dtLocalBuf, "%d-%d-%dT%d:%d", &y, &mo, &d, &h, &mi);
    if (y > 2000) { // basic sanity
      struct tm tmLocal = {};
      tmLocal.tm_year  = y - 1900;
      tmLocal.tm_mon   = mo - 1;
      tmLocal.tm_mday  = d;
      tmLocal.tm_hour  = h;
      tmLocal.tm_min   = mi;
      tmLocal.tm_isdst = -1; // let mktime resolve DST using the current TZ
      time_t utcTime = mktime(&tmLocal);
      if (utcTime != (time_t)-1) {
        setTime((time_t)utcTime);
        rtc.adjust(DateTime((uint32_t)utcTime));
        matrixClockConfigPersistActiveRuntimeConfig(); // tzIndex already in configDb
        Serial.printf("[CONFIG] Time set via portal: UTC epoch %lu\n", (unsigned long)utcTime);
      }
    }
    s_dtLocalBuf[0] = '\0'; // consume
  }
  return true;
}

bool portalGetFieldValue(void *context, const char *fieldId, char *outValue, size_t outValueLen) {
  (void)context;
  return matrixClockConfigGetFieldValue(fieldId, outValue, outValueLen);
}

bool portalSetFieldValue(void *context, const char *fieldId, const char *value) {
  (void)context;
  return matrixClockConfigSetFieldValue(fieldId, value);
}

bool portalLoadDefaults(void *context) {
  (void)context;
  matrixClockConfigLoadFactoryDefaults();
  return true;
}

bool portalGetStatus(void *context, APPortalStatus &status) {
  (void)context;
  static char sApSsid[44] = {0};
  static char sApIp[20]   = {0};

  status.apModeActive     = modeManagerInApControlMode();
  status.stationConnected = (WiFi.status() == WL_CONNECTED);
  status.mqttConnected    = mqttClient.connected();
  status.timeValid        = (timeStatus() != timeNotSet);
  status.timeSourceMode   = "RTC_OR_NTP";

  // Device info for the portal Info page
  status.projectName      = projectNameFromFileName;
  status.firmwareVersion  = projectVersionFromFileName;
  status.compileDate      = compileDateFromFileName;
  status.compileTime      = compileTimeFromFileName;
  status.uptimeSeconds    = millis() / 1000UL;

  // AP SSID — mirrors the name constructed in startApSetupRuntime()
  if (projectNameFromFileName[0] != '\0') {
    snprintf(sApSsid, sizeof(sApSsid), "%s-AP", projectNameFromFileName);
  } else {
    strlcpy(sApSsid, "matrixClock-AP", sizeof(sApSsid));
  }
  status.apSsid = sApSsid;

  // AP IP — use named variable to avoid calling toCharArray() on a temporary
  { String apIpStr = WiFi.softAPIP().toString(); apIpStr.toCharArray(sApIp, sizeof(sApIp)); }
  status.apIp = sApIp;

  return true;
}

const APFieldDefinition kPortalFields[] = {
  {"wifi", "wifi_enabled", "WiFi Enabled", AP_FIELD_TOGGLE, 1, true, nullptr, 0},
  {"wifi", "wifi_ssid_1", "WiFi SSID #1", AP_FIELD_TEXT, 29, false, nullptr, 0},
  {"wifi", "wifi_password_1", "WiFi Password #1", AP_FIELD_PASSWORD, 29, false, nullptr, 0},
  {"wifi", "wifi_hostname",   "WiFi Hostname",    AP_FIELD_TEXT,     31, false, nullptr, 0},
  {"wifi", "wifi_ssid_2",    "WiFi SSID #2",     AP_FIELD_TEXT,     29, false, nullptr, 0},
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
  {"mqtt", "mqtt_topic_cmd",  "Lamp Command Topic", AP_FIELD_TEXT,   19, false, nullptr, 0},
  {"mqtt", "mqtt_topic_stat", "Lamp Status Topic",  AP_FIELD_TEXT,   19, false, nullptr, 0},

  // Date & Time page
  {"datetime", "tz_index",  "Time Zone",        AP_FIELD_SELECT,   1, false, kTzOptions, sizeof(kTzOptions)/sizeof(kTzOptions[0])},
  {"datetime", "dt_local",  "Date / Time (local)", AP_FIELD_DATETIME, 16, false, nullptr, 0},
};
}

MatrixClockRuntimeConfig g_matrixClockRuntimeConfig = {
  MATRIXCLOCK_CONFIG_SCHEMA_VERSION,
  {}
};

bool matrixClockConfigRegisterPortalContracts() {
  bool ok = true;

  ok = apPortalRegisterPage("wifi",     "WiFi Settings") && ok;
  ok = apPortalRegisterPage("mqtt",     "MQTT Settings") && ok;
  ok = apPortalRegisterPage("datetime", "Date & Time", "Set Time") && ok;

  for (size_t i = 0; i < (sizeof(kPortalFields) / sizeof(kPortalFields[0])); ++i) {
    ok = apPortalRegisterField(kPortalFields[i]) && ok;
  }

  APPortalCallbacks callbacks = {
    nullptr,
    portalLoadConfig,
    portalSaveConfig,
    portalApplyConfig,
    portalGetFieldValue,
    portalSetFieldValue,
    portalGetStatus,
    portalLoadDefaults
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
  const size_t   storedLength  = prefs.getBytesLength(kConfigBlobKey);

  // V1 → V2 migration: new fields are appended at the end of the struct so
  // all v1 fields sit at identical byte offsets in v2.  Load the v1 blob into
  // a zero-initialised v2 struct, then patch in sensible defaults for the new
  // fields and auto-persist as v2 so the next boot loads cleanly.
  if (storedVersion == 1 && storedLength == kV1ConfigDbSize) {
    memset(&outConfig.configDb, 0, sizeof(configDb_t));
    size_t readLen = prefs.getBytes(kConfigBlobKey, &outConfig.configDb, kV1ConfigDbSize);
    prefs.end();
    if (readLen != kV1ConfigDbSize) return false;
    outConfig.configDb.wifiEnabled = true;
    outConfig.configDb.mqttEnabled = true;
    outConfig.configDb.tzIndex     = 0; // default US Eastern
    strlcpy(outConfig.configDb.wifiHostname, "matrixClock", sizeof(outConfig.configDb.wifiHostname));
    outConfig.schemaVersion = MATRIXCLOCK_CONFIG_SCHEMA_VERSION;
    matrixClockConfigSaveToNvs(outConfig);
    Serial.println("[CONFIG] NVS schema migrated v1 -> v3");
    return true;
  }

  // V2 → V3 migration: tzIndex appended; load v2 blob, patch default, persist as v3.
  if (storedVersion == 2 && storedLength == kV2ConfigDbSize) {
    memset(&outConfig.configDb, 0, sizeof(configDb_t));
    size_t readLen = prefs.getBytes(kConfigBlobKey, &outConfig.configDb, kV2ConfigDbSize);
    prefs.end();
    if (readLen != kV2ConfigDbSize) return false;
    outConfig.configDb.tzIndex = 0; // default US Eastern
    outConfig.schemaVersion = MATRIXCLOCK_CONFIG_SCHEMA_VERSION;
    matrixClockConfigSaveToNvs(outConfig);
    Serial.println("[CONFIG] NVS schema migrated v2 -> v3");
    return true;
  }

  if (!matrixClockConfigIsSchemaCompatible(storedVersion) || storedLength != sizeof(configDb_t)) {
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
  outConfig.configDb = g_factoryDefaults;
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

void matrixClockConfigLoadFactoryDefaults() {
  g_matrixClockRuntimeConfig.configDb = g_factoryDefaults;
  g_matrixClockRuntimeConfig.schemaVersion = MATRIXCLOCK_CONFIG_SCHEMA_VERSION;
  applyRuntimeConfigToLegacyGlobals(g_matrixClockRuntimeConfig);
  Serial.println("[CONFIG] Factory defaults loaded into active config (not yet saved to NVS)");
}

void matrixClockConfigSetActiveRuntimeConfig(const MatrixClockRuntimeConfig &runtimeConfig) {
  g_matrixClockRuntimeConfig = runtimeConfig;
  applyRuntimeConfigToLegacyGlobals(g_matrixClockRuntimeConfig);
}

bool matrixClockConfigPersistActiveRuntimeConfig() {
  return matrixClockConfigSaveToNvs(g_matrixClockRuntimeConfig);
}

bool matrixClockConfigGetFieldValue(const char *fieldId, char *outValue, size_t outValueLen) {
  if (fieldId == nullptr) {
    return false;
  }

  if (strcmp(fieldId, "wifi_enabled") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.wifiEnabled ? "1" : "0", outValue, outValueLen);
  }
  if (strcmp(fieldId, "mqtt_enabled") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.mqttEnabled ? "1" : "0", outValue, outValueLen);
  }

  if (strcmp(fieldId, "wifi_ssid_1") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.ssid, outValue, outValueLen);
  }
  if (strcmp(fieldId, "wifi_password_1") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.password, outValue, outValueLen);
  }
  if (strcmp(fieldId, "wifi_hostname") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.wifiHostname, outValue, outValueLen);
  }
  if (strcmp(fieldId, "wifi_ssid_2") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.ssid2, outValue, outValueLen);
  }
  if (strcmp(fieldId, "wifi_password_2") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.password2, outValue, outValueLen);
  }
  if (strcmp(fieldId, "wifi_ssid_3") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.ssid3, outValue, outValueLen);
  }
  if (strcmp(fieldId, "wifi_password_3") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.password3, outValue, outValueLen);
  }

  if (strcmp(fieldId, "mqtt_server") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.mqttServer, outValue, outValueLen);
  }
  if (strcmp(fieldId, "mqtt_user") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.mqttUserId, outValue, outValueLen);
  }
  if (strcmp(fieldId, "mqtt_password") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.mqttPassword, outValue, outValueLen);
  }
  if (strcmp(fieldId, "mqtt_client_id") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.mqttClientId, outValue, outValueLen);
  }

  if (strcmp(fieldId, "mqtt_device_name_1") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.mqttDeviceName[0], outValue, outValueLen);
  }
  if (strcmp(fieldId, "mqtt_device_name_2") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.mqttDeviceName[1], outValue, outValueLen);
  }
  if (strcmp(fieldId, "mqtt_device_name_3") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.mqttDeviceName[2], outValue, outValueLen);
  }

  if (strcmp(fieldId, "mqtt_topic_cmd") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.lampCmndTopic, outValue, outValueLen);
  }
  if (strcmp(fieldId, "mqtt_topic_stat") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.lampStatTopic, outValue, outValueLen);
  }
  if (strcmp(fieldId, "tz_index") == 0) {
    char buf[4];
    snprintf(buf, sizeof(buf), "%u", g_matrixClockRuntimeConfig.configDb.tzIndex);
    return copyToOutputBuffer(buf, outValue, outValueLen);
  }
  if (strcmp(fieldId, "dt_local") == 0) {
    // Return current local time pre-formatted for datetime-local input
    time_t utc = now();
    struct tm tmLocal;
    localtime_r(&utc, &tmLocal);
    strftime(outValue, outValueLen, "%Y-%m-%dT%H:%M", &tmLocal);
    return true;
  }

  return false;
}

bool matrixClockConfigSetFieldValue(const char *fieldId, const char *value) {
  if (fieldId == nullptr || value == nullptr) {
    return false;
  }

  if (strcmp(fieldId, "wifi_enabled") == 0) {
    g_matrixClockRuntimeConfig.configDb.wifiEnabled = (strcmp(value, "1") == 0);
    applyRuntimeConfigToLegacyGlobals(g_matrixClockRuntimeConfig);
    return true;
  }
  if (strcmp(fieldId, "mqtt_enabled") == 0) {
    g_matrixClockRuntimeConfig.configDb.mqttEnabled = (strcmp(value, "1") == 0);
    applyRuntimeConfigToLegacyGlobals(g_matrixClockRuntimeConfig);
    return true;
  }

  bool updated = false;

  if (strcmp(fieldId, "wifi_ssid_1") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.ssid, sizeof(g_matrixClockRuntimeConfig.configDb.ssid), value);
  } else if (strcmp(fieldId, "wifi_password_1") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.password, sizeof(g_matrixClockRuntimeConfig.configDb.password), value);
  } else if (strcmp(fieldId, "wifi_hostname") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.wifiHostname, sizeof(g_matrixClockRuntimeConfig.configDb.wifiHostname), value);
  } else if (strcmp(fieldId, "wifi_ssid_2") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.ssid2, sizeof(g_matrixClockRuntimeConfig.configDb.ssid2), value);
  } else if (strcmp(fieldId, "wifi_password_2") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.password2, sizeof(g_matrixClockRuntimeConfig.configDb.password2), value);
  } else if (strcmp(fieldId, "wifi_ssid_3") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.ssid3, sizeof(g_matrixClockRuntimeConfig.configDb.ssid3), value);
  } else if (strcmp(fieldId, "wifi_password_3") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.password3, sizeof(g_matrixClockRuntimeConfig.configDb.password3), value);
  } else if (strcmp(fieldId, "mqtt_server") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.mqttServer, sizeof(g_matrixClockRuntimeConfig.configDb.mqttServer), value);
  } else if (strcmp(fieldId, "mqtt_user") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.mqttUserId, sizeof(g_matrixClockRuntimeConfig.configDb.mqttUserId), value);
  } else if (strcmp(fieldId, "mqtt_password") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.mqttPassword, sizeof(g_matrixClockRuntimeConfig.configDb.mqttPassword), value);
  } else if (strcmp(fieldId, "mqtt_client_id") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.mqttClientId, sizeof(g_matrixClockRuntimeConfig.configDb.mqttClientId), value);
  } else if (strcmp(fieldId, "mqtt_device_name_1") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.mqttDeviceName[0], sizeof(g_matrixClockRuntimeConfig.configDb.mqttDeviceName[0]), value);
  } else if (strcmp(fieldId, "mqtt_device_name_2") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.mqttDeviceName[1], sizeof(g_matrixClockRuntimeConfig.configDb.mqttDeviceName[1]), value);
  } else if (strcmp(fieldId, "mqtt_device_name_3") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.mqttDeviceName[2], sizeof(g_matrixClockRuntimeConfig.configDb.mqttDeviceName[2]), value);
  } else if (strcmp(fieldId, "mqtt_topic_cmd") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.lampCmndTopic, sizeof(g_matrixClockRuntimeConfig.configDb.lampCmndTopic), value);
  } else if (strcmp(fieldId, "mqtt_topic_stat") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.lampStatTopic, sizeof(g_matrixClockRuntimeConfig.configDb.lampStatTopic), value);
  } else if (strcmp(fieldId, "tz_index") == 0) {
    uint8_t idx = (uint8_t)atoi(value);
    if (idx < kTimezoneTableCount) {
      g_matrixClockRuntimeConfig.configDb.tzIndex = idx;
      updated = true;
    }
  } else if (strcmp(fieldId, "dt_local") == 0) {
    // Stage the datetime string; actual time-set happens in portalApplyConfig()
    strlcpy(s_dtLocalBuf, value, sizeof(s_dtLocalBuf));
    updated = true;
  }

  if (!updated) {
    return false;
  }

  applyRuntimeConfigToLegacyGlobals(g_matrixClockRuntimeConfig);
  return true;
}

bool matrixClockConfigValidatePortalFieldMappings() {
  char valueBuffer[128];

  for (size_t i = 0; i < (sizeof(kMappedFieldIds) / sizeof(kMappedFieldIds[0])); ++i) {
    const char *fieldId = kMappedFieldIds[i];
    if (!matrixClockConfigGetFieldValue(fieldId, valueBuffer, sizeof(valueBuffer))) {
      return false;
    }

    if (!matrixClockConfigSetFieldValue(fieldId, valueBuffer)) {
      return false;
    }
  }

  return true;
}

#include "MatrixClock_Config.h"
#include "AP_Config_Portal.h"
#include "Mode_Manager.h"
#include <Preferences.h>

namespace {
const char *kConfigNamespace = "mcfg";
const char *kSchemaKey = "schema";
const char *kConfigBlobKey = "cfgblob";
const char *kMappedFieldIds[] = {
  "wifi_enabled",
  "wifi_ssid_1",
  "wifi_password_1",
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
  status.apModeActive = modeManagerInApControlMode();
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

bool matrixClockConfigGetFieldValue(const char *fieldId, char *outValue, size_t outValueLen) {
  if (fieldId == nullptr) {
    return false;
  }

  if (strcmp(fieldId, "wifi_enabled") == 0 || strcmp(fieldId, "mqtt_enabled") == 0) {
    return copyToOutputBuffer("1", outValue, outValueLen);
  }

  if (strcmp(fieldId, "wifi_ssid_1") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.ssid, outValue, outValueLen);
  }
  if (strcmp(fieldId, "wifi_password_1") == 0) {
    return copyToOutputBuffer(g_matrixClockRuntimeConfig.configDb.password, outValue, outValueLen);
  }

  // WiFiMulti not yet implemented in config schema. Keep fields readable as empty placeholders.
  if (strcmp(fieldId, "wifi_ssid_2") == 0 || strcmp(fieldId, "wifi_password_2") == 0 ||
      strcmp(fieldId, "wifi_ssid_3") == 0 || strcmp(fieldId, "wifi_password_3") == 0) {
    return copyToOutputBuffer("", outValue, outValueLen);
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

  return false;
}

bool matrixClockConfigSetFieldValue(const char *fieldId, const char *value) {
  if (fieldId == nullptr || value == nullptr) {
    return false;
  }

  // Enable toggles are placeholders until runtime feature flags are added to schema.
  if (strcmp(fieldId, "wifi_enabled") == 0 || strcmp(fieldId, "mqtt_enabled") == 0) {
    return true;
  }

  bool updated = false;

  if (strcmp(fieldId, "wifi_ssid_1") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.ssid, sizeof(g_matrixClockRuntimeConfig.configDb.ssid), value);
  } else if (strcmp(fieldId, "wifi_password_1") == 0) {
    updated = writeConfigField(g_matrixClockRuntimeConfig.configDb.password, sizeof(g_matrixClockRuntimeConfig.configDb.password), value);
  } else if (strcmp(fieldId, "wifi_ssid_2") == 0 || strcmp(fieldId, "wifi_password_2") == 0 ||
             strcmp(fieldId, "wifi_ssid_3") == 0 || strcmp(fieldId, "wifi_password_3") == 0) {
    // WiFiMulti fields are accepted as no-op until schema expansion lands.
    updated = true;
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

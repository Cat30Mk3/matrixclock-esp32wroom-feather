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
}

MatrixClockRuntimeConfig g_matrixClockRuntimeConfig = {
  MATRIXCLOCK_CONFIG_SCHEMA_VERSION,
  {}
};

void matrixClockConfigRegisterPortalContracts() {
  // Phase 0 contract stub only: page/field registration and callbacks are defined later.
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

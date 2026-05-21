#include "MatrixClock_Config.h"
#include "AP_Config_Portal.h"

MatrixClockRuntimeConfig g_matrixClockRuntimeConfig = {
  MATRIXCLOCK_CONFIG_SCHEMA_VERSION,
  {}
};

void matrixClockConfigRegisterPortalContracts() {
  // Phase 0 contract stub only: page/field registration and callbacks are defined later.
}

bool matrixClockConfigLoadFromNvs(MatrixClockRuntimeConfig &outConfig) {
  (void)outConfig;
  return false;
}

bool matrixClockConfigSaveToNvs(const MatrixClockRuntimeConfig &config) {
  (void)config;
  return false;
}

bool matrixClockConfigIsSchemaCompatible(uint16_t storedVersion) {
  return storedVersion == MATRIXCLOCK_CONFIG_SCHEMA_VERSION;
}

void matrixClockConfigLoadBootstrapDefaults(MatrixClockRuntimeConfig &outConfig) {
  outConfig.schemaVersion = MATRIXCLOCK_CONFIG_SCHEMA_VERSION;
  outConfig.configDb = configDb;
}

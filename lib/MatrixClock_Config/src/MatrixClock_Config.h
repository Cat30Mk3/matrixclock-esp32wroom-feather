#ifndef MATRIXCLOCK_CONFIG_H
#define MATRIXCLOCK_CONFIG_H

#include <Arduino.h>
#include "globals.h"

static const uint16_t MATRIXCLOCK_CONFIG_SCHEMA_VERSION = 1;

struct MatrixClockRuntimeConfig {
	uint16_t schemaVersion;
	configDb_t configDb;
};

struct MatrixClockConfigInitResult {
	bool loadedFromNvs;
	bool seededNvsFromBootstrap;
};

extern MatrixClockRuntimeConfig g_matrixClockRuntimeConfig;

bool matrixClockConfigRegisterPortalContracts();

bool matrixClockConfigLoadFromNvs(MatrixClockRuntimeConfig &outConfig);
bool matrixClockConfigSaveToNvs(const MatrixClockRuntimeConfig &config);
bool matrixClockConfigIsSchemaCompatible(uint16_t storedVersion);
void matrixClockConfigLoadBootstrapDefaults(MatrixClockRuntimeConfig &outConfig);
bool matrixClockConfigInitializeRuntimeConfig(MatrixClockConfigInitResult &outResult);

const MatrixClockRuntimeConfig &matrixClockConfigGetActiveRuntimeConfig();
void matrixClockConfigSetActiveRuntimeConfig(const MatrixClockRuntimeConfig &runtimeConfig);
bool matrixClockConfigPersistActiveRuntimeConfig();

bool matrixClockConfigGetFieldValue(const char *fieldId, char *outValue, size_t outValueLen);
bool matrixClockConfigSetFieldValue(const char *fieldId, const char *value);
bool matrixClockConfigValidatePortalFieldMappings();

#endif // MATRIXCLOCK_CONFIG_H

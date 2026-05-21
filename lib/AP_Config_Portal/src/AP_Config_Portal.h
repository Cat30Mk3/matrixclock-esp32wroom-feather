#ifndef AP_CONFIG_PORTAL_H
#define AP_CONFIG_PORTAL_H

#include <Arduino.h>

enum APFieldType {
  AP_FIELD_TEXT,
  AP_FIELD_PASSWORD,
  AP_FIELD_NUMBER,
  AP_FIELD_TOGGLE,
  AP_FIELD_SELECT
};

struct APFieldOption {
  const char *value;
  const char *label;
};

struct APFieldDefinition {
  const char *pageId;
  const char *fieldId;
  const char *label;
  APFieldType type;
  uint16_t maxLength;
  bool required;
  const APFieldOption *options;
  size_t optionCount;
};

struct APPortalStatus {
  bool apModeActive;
  bool stationConnected;
  bool mqttConnected;
  bool timeValid;
  const char *timeSourceMode;
};

typedef bool (*APLoadConfigCallback)(void *context);
typedef bool (*APSaveConfigCallback)(void *context);
typedef bool (*APApplyConfigCallback)(void *context);
typedef bool (*APGetStatusCallback)(void *context, APPortalStatus &status);

struct APPortalCallbacks {
  void *context;
  APLoadConfigCallback loadConfig;
  APSaveConfigCallback saveConfig;
  APApplyConfigCallback applyConfig;
  APGetStatusCallback getStatus;
};

void apPortalBegin();
void apPortalEnd();

bool apPortalRegisterPage(const char *pageId, const char *title);
bool apPortalRegisterField(const APFieldDefinition &field);
void apPortalSetCallbacks(const APPortalCallbacks &callbacks);

#endif // AP_CONFIG_PORTAL_H

#ifndef AP_CONFIG_PORTAL_H
#define AP_CONFIG_PORTAL_H

#include <Arduino.h>

enum APFieldType {
  AP_FIELD_TEXT,
  AP_FIELD_PASSWORD,
  AP_FIELD_NUMBER,
  AP_FIELD_TOGGLE,
  AP_FIELD_SELECT,
  AP_FIELD_DATETIME  // renders as <input type="datetime-local">, value format: YYYY-MM-DDTHH:MM
};

// Staged display state — advances as user progresses through setup flow
enum APDisplayStage {
  AP_STAGE_SSID   = 1, // Stage 1: No client yet — scroll AP SSID
  AP_STAGE_IP     = 2, // Stage 2: Client connected — scroll AP IP
  AP_STAGE_PIN    = 3, // Stage 3: PIN page opened — scroll PIN
  AP_STAGE_ACTIVE = 4  // Stage 4: Authenticated — show "AP Active"
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
  // Device info — filled by host for the Info page
  const char *projectName;
  const char *firmwareVersion;
  const char *compileDate;
  const char *compileTime;
  const char *apSsid;
  const char *apIp;
  unsigned long uptimeSeconds;
};

typedef bool (*APLoadConfigCallback)(void *context);
typedef bool (*APSaveConfigCallback)(void *context);
typedef bool (*APApplyConfigCallback)(void *context);
typedef bool (*APGetFieldValueCallback)(void *context, const char *fieldId, char *outValue, size_t outValueLen);
typedef bool (*APSetFieldValueCallback)(void *context, const char *fieldId, const char *value);
typedef bool (*APGetStatusCallback)(void *context, APPortalStatus &status);

struct APPortalCallbacks {
  void *context;
  APLoadConfigCallback loadConfig;
  APSaveConfigCallback saveConfig;
  APApplyConfigCallback applyConfig;
  APGetFieldValueCallback getFieldValue;
  APSetFieldValueCallback setFieldValue;
  APGetStatusCallback getStatus;
};

void apPortalBegin();
void apPortalEnd();
bool apPortalStartServer(uint16_t port = 80, bool enableDns = true);
void apPortalStopServer();
void apPortalService();
bool apPortalIsServerRunning();
const char *apPortalGetLoginPin();

// Display stage — read by main.cpp to drive staged scrolling messages
APDisplayStage apPortalGetDisplayStage();
void apPortalSignalClientConnected(); // called when first STA joins AP

// Exit control — poll after apPortalService(); clear before calling modeManagerRequestNormalMode()
bool apPortalShouldExit();
bool apPortalExitWasSave();
void apPortalClearExitRequest();

bool apPortalRegisterPage(const char *pageId, const char *title, const char *submitLabel = nullptr);
bool apPortalRegisterField(const APFieldDefinition &field);
void apPortalSetCallbacks(const APPortalCallbacks &callbacks);

#endif // AP_CONFIG_PORTAL_H

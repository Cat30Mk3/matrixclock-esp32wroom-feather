#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include <Arduino.h>

enum MatrixClockMode {
  MATRIXCLOCK_MODE_NORMAL = 0,
  MATRIXCLOCK_MODE_AP_SETUP = 1,
  MATRIXCLOCK_MODE_RECOVERY = 2
};

struct ModeManagerConfig {
  uint32_t bootRecoveryWindowMs;
  uint32_t bootComboHoldMs;
  uint32_t menuHoldDurationMs;
  uint32_t selectConfirmWindowMs;
};

void modeManagerBegin(const ModeManagerConfig &config);
void modeManagerSetBackgroundPollingEnabled(bool enabled);
bool modeManagerIsBackgroundPollingEnabled();
bool modeManagerCheckBootRecoveryRequest();
void modeManagerService();

MatrixClockMode modeManagerGetMode();
const char *modeManagerGetModeName(MatrixClockMode mode);
bool modeManagerInApControlMode();

bool modeManagerIsConfirmPromptActive();
bool modeManagerConsumeModeEntryEvent(MatrixClockMode &outMode);
void modeManagerLogButtonPinMapping();
void modeManagerServiceButtonDiagnostics();

#endif // MODE_MANAGER_H

#include "Mode_Manager.h"
#include "globals.h"

namespace {
ModeManagerConfig s_config = {
  3000,
  1200,
  4000,
  5000
};

MatrixClockMode s_mode = MATRIXCLOCK_MODE_NORMAL;
bool s_entryEventPending = false;
bool s_confirmPromptActive = false;
uint32_t s_menuHoldStartMs = 0;
uint32_t s_confirmWindowStartMs = 0;
bool s_selectLatched = false;
bool s_backgroundPollingEnabled = false;
const uint32_t kButtonDiagDebounceMs = 40;
const uint32_t kMenuReleaseToleranceMs = 150;

struct ButtonDiag {
  const char *name;
  uint8_t pin;
  bool expectsInternalPullup;
};

const ButtonDiag kButtonDiag[] = {
  {"SEL", PB_SEL_PIN, false},
  {"DEC", PB_DEC_PIN, false},
  {"INC", PB_INC_PIN, false},
  {"CNL", PB_CNL_PIN, true},
  {"MEN", PB_MEN_PIN, true}
};

int s_lastButtonRaw[sizeof(kButtonDiag) / sizeof(kButtonDiag[0])] = {-1, -1, -1, -1, -1};
int s_idleButtonRaw[sizeof(kButtonDiag) / sizeof(kButtonDiag[0])] = {-1, -1, -1, -1, -1};
uint32_t s_lastButtonChangeMs[sizeof(kButtonDiag) / sizeof(kButtonDiag[0])] = {0, 0, 0, 0, 0};
uint32_t s_modeManagerStartMs = 0;
bool s_externalButtonStuckWarningShown = false;
uint32_t s_menuReleaseStartMs = 0;
uint32_t s_lastMenuHoldProgressLogMs = 0;

bool isPressedByIndex(size_t index) {
  int raw = digitalRead(kButtonDiag[index].pin);
  if (s_idleButtonRaw[index] == -1) {
    return raw == LOW;
  }
  return raw != s_idleButtonRaw[index];
}

bool isPressed(uint8_t pin) {
  for (size_t i = 0; i < (sizeof(kButtonDiag) / sizeof(kButtonDiag[0])); ++i) {
    if (kButtonDiag[i].pin == pin) {
      return isPressedByIndex(i);
    }
  }
  return digitalRead(pin) == LOW;
}

void setMode(MatrixClockMode mode) {
  s_mode = mode;
  s_entryEventPending = true;
  s_confirmPromptActive = false;
  s_menuHoldStartMs = 0;
  s_menuReleaseStartMs = 0;
  s_lastMenuHoldProgressLogMs = 0;
  s_confirmWindowStartMs = 0;
  s_selectLatched = false;
}

bool elapsedSince(uint32_t startMs, uint32_t durationMs) {
  return startMs != 0 && (millis() - startMs) >= durationMs;
}
}

void modeManagerSetBackgroundPollingEnabled(bool enabled) {
  s_backgroundPollingEnabled = enabled;
}

bool modeManagerIsBackgroundPollingEnabled() {
  return s_backgroundPollingEnabled;
}

bool modeManagerCheckBootRecoveryRequest() {
  const uint32_t startedAtMs = millis();
  uint32_t comboHoldStartMs = 0;

  while ((millis() - startedAtMs) < s_config.bootRecoveryWindowMs) {
    const bool menuPressed = isPressed(PB_MEN_PIN);
    const bool recoveryConfirmPressed = isPressed(PB_SEL_PIN) || isPressed(PB_CNL_PIN);
    if (menuPressed && recoveryConfirmPressed) {
      if (comboHoldStartMs == 0) {
        comboHoldStartMs = millis();
      }

      if ((millis() - comboHoldStartMs) >= s_config.bootComboHoldMs) {
        setMode(MATRIXCLOCK_MODE_RECOVERY);
        return true;
      }
    } else {
      comboHoldStartMs = 0;
    }

    yield();
  }

  return false;
}

void modeManagerService() {
  if (s_mode != MATRIXCLOCK_MODE_NORMAL) {
    return;
  }

  const bool menuPressed = isPressed(PB_MEN_PIN);
  const bool selectPressed = isPressed(PB_SEL_PIN);
  const bool cancelPressed = isPressed(PB_CNL_PIN);
  const bool confirmPressed = selectPressed || cancelPressed;

  if (!s_confirmPromptActive) {
    if (menuPressed) {
      s_menuReleaseStartMs = 0;

      if (s_menuHoldStartMs == 0) {
        s_menuHoldStartMs = millis();
        s_lastMenuHoldProgressLogMs = s_menuHoldStartMs;
        Serial.println("[MODE] menu hold started");
      }

      if ((millis() - s_lastMenuHoldProgressLogMs) >= 1000) {
        s_lastMenuHoldProgressLogMs = millis();
        Serial.print("[MODE] menu hold ms=");
        Serial.print(millis() - s_menuHoldStartMs);
        Serial.print("/");
        Serial.println(s_config.menuHoldDurationMs);
      }

      if (elapsedSince(s_menuHoldStartMs, s_config.menuHoldDurationMs)) {
        s_confirmPromptActive = true;
        s_confirmWindowStartMs = millis();
        s_selectLatched = confirmPressed;
        Serial.println("[MODE] confirm prompt active (press CNL or SEL)");
      }
    } else {
      if (s_menuHoldStartMs != 0) {
        if (s_menuReleaseStartMs == 0) {
          s_menuReleaseStartMs = millis();
        }

        if (elapsedSince(s_menuReleaseStartMs, kMenuReleaseToleranceMs)) {
          Serial.println("[MODE] menu hold canceled");
          s_menuHoldStartMs = 0;
          s_menuReleaseStartMs = 0;
          s_lastMenuHoldProgressLogMs = 0;
        }
      }
    }

    return;
  }

  // Keep prompt armed until a confirm edge is seen; this avoids missing entry
  // during long display cycles or when the user releases MENU before pressing CNL/SEL.
  if (elapsedSince(s_confirmWindowStartMs, s_config.selectConfirmWindowMs)) {
    s_confirmWindowStartMs = millis();
    Serial.println("[MODE] confirm prompt still active");
  }

  if (confirmPressed && !s_selectLatched) {
    setMode(MATRIXCLOCK_MODE_AP_SETUP);
    return;
  }

  s_selectLatched = confirmPressed;
}

MatrixClockMode modeManagerGetMode() {
  return s_mode;
}

const char *modeManagerGetModeName(MatrixClockMode mode) {
  switch (mode) {
    case MATRIXCLOCK_MODE_NORMAL:
      return "NORMAL";
    case MATRIXCLOCK_MODE_AP_SETUP:
      return "AP_SETUP";
    case MATRIXCLOCK_MODE_RECOVERY:
      return "RECOVERY";
    default:
      return "UNKNOWN";
  }
}

bool modeManagerInApControlMode() {
  return s_mode == MATRIXCLOCK_MODE_AP_SETUP || s_mode == MATRIXCLOCK_MODE_RECOVERY;
}

bool modeManagerIsConfirmPromptActive() {
  return s_confirmPromptActive;
}

bool modeManagerConsumeModeEntryEvent(MatrixClockMode &outMode) {
  if (!s_entryEventPending) {
    return false;
  }

  s_entryEventPending = false;
  outMode = s_mode;
  return true;
}

void modeManagerLogButtonPinMapping() {
  Serial.println("[BTN] Pin mapping and assumptions:");
  for (size_t i = 0; i < (sizeof(kButtonDiag) / sizeof(kButtonDiag[0])); ++i) {
    Serial.print("[BTN] ");
    Serial.print(kButtonDiag[i].name);
    Serial.print(" pin=");
    Serial.print(kButtonDiag[i].pin);
    Serial.print(" idleRaw=");
    Serial.print(s_idleButtonRaw[i]);
    Serial.print(" input=");
    Serial.println(kButtonDiag[i].expectsInternalPullup ? "INPUT_PULLUP" : "INPUT (external pull resistor expected)");
  }
}

void modeManagerServiceButtonDiagnostics() {
  for (size_t i = 0; i < (sizeof(kButtonDiag) / sizeof(kButtonDiag[0])); ++i) {
    int raw = digitalRead(kButtonDiag[i].pin);
    if (s_idleButtonRaw[i] == -1) {
      s_idleButtonRaw[i] = raw;
    }

    if (raw != s_lastButtonRaw[i]) {
      uint32_t nowMs = millis();
      if ((nowMs - s_lastButtonChangeMs[i]) < kButtonDiagDebounceMs) {
        continue;
      }

      s_lastButtonChangeMs[i] = nowMs;
      s_lastButtonRaw[i] = raw;

      Serial.print("[BTN] ");
      Serial.print(kButtonDiag[i].name);
      Serial.print(" raw=");
      Serial.print(raw);
      Serial.print(" idle=");
      Serial.print(s_idleButtonRaw[i]);
      Serial.print(" pressed=");
      bool pressed = isPressedByIndex(i);
      Serial.println(pressed ? 1 : 0);

      if (s_confirmPromptActive && pressed &&
          (kButtonDiag[i].pin == PB_SEL_PIN || kButtonDiag[i].pin == PB_CNL_PIN)) {
        Serial.println("[MODE] confirmEdge accepted");
        setMode(MATRIXCLOCK_MODE_AP_SETUP);
        return;
      }
    }
  }

  if (!s_externalButtonStuckWarningShown && (millis() - s_modeManagerStartMs) >= 10000) {
    bool selStuck = (s_idleButtonRaw[0] == 0) && (digitalRead(kButtonDiag[0].pin) == s_idleButtonRaw[0]);
    bool decStuck = (s_idleButtonRaw[1] == 0) && (digitalRead(kButtonDiag[1].pin) == s_idleButtonRaw[1]);
    bool incStuck = (s_idleButtonRaw[2] == 0) && (digitalRead(kButtonDiag[2].pin) == s_idleButtonRaw[2]);

    if (selStuck && decStuck && incStuck) {
      s_externalButtonStuckWarningShown = true;
      Serial.println("[BTN-WARN] SEL/DEC/INC stayed low for 10s with no edges; check wiring/pin mapping/pull-up path.");
    }
  }
}

void modeManagerBegin(const ModeManagerConfig &config) {
  s_config = config;
  s_mode = MATRIXCLOCK_MODE_NORMAL;
  s_entryEventPending = false;
  s_confirmPromptActive = false;
  s_menuHoldStartMs = 0;
  s_menuReleaseStartMs = 0;
  s_lastMenuHoldProgressLogMs = 0;
  s_confirmWindowStartMs = 0;
  s_selectLatched = false;

  for (size_t i = 0; i < (sizeof(kButtonDiag) / sizeof(kButtonDiag[0])); ++i) {
    int raw = digitalRead(kButtonDiag[i].pin);
    s_idleButtonRaw[i] = raw;
    s_lastButtonRaw[i] = raw;
    s_lastButtonChangeMs[i] = 0;
  }

  s_modeManagerStartMs = millis();
  s_externalButtonStuckWarningShown = false;
  s_backgroundPollingEnabled = false;
}

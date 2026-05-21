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
const uint32_t kButtonDiagDebounceMs = 40;

struct ButtonDiag {
  const char *name;
  uint8_t pin;
  bool activeLow;
  bool expectsInternalPullup;
};

const ButtonDiag kButtonDiag[] = {
  {"SEL", PB_SEL_PIN, true, false},
  {"DEC", PB_DEC_PIN, true, false},
  {"INC", PB_INC_PIN, true, false},
  {"CNL", PB_CNL_PIN, true, true},
  {"MEN", PB_MEN_PIN, true, true}
};

int s_lastButtonRaw[sizeof(kButtonDiag) / sizeof(kButtonDiag[0])] = {-1, -1, -1, -1, -1};
uint32_t s_lastButtonChangeMs[sizeof(kButtonDiag) / sizeof(kButtonDiag[0])] = {0, 0, 0, 0, 0};

bool isPressed(uint8_t pin) {
  return digitalRead(pin) == LOW;
}

bool isPressed(const ButtonDiag &button) {
  int raw = digitalRead(button.pin);
  return button.activeLow ? (raw == LOW) : (raw == HIGH);
}

void setMode(MatrixClockMode mode) {
  s_mode = mode;
  s_entryEventPending = true;
  s_confirmPromptActive = false;
  s_menuHoldStartMs = 0;
  s_confirmWindowStartMs = 0;
  s_selectLatched = false;
}

bool elapsedSince(uint32_t startMs, uint32_t durationMs) {
  return startMs != 0 && (millis() - startMs) >= durationMs;
}
}

void modeManagerBegin(const ModeManagerConfig &config) {
  s_config = config;
  s_mode = MATRIXCLOCK_MODE_NORMAL;
  s_entryEventPending = false;
  s_confirmPromptActive = false;
  s_menuHoldStartMs = 0;
  s_confirmWindowStartMs = 0;
  s_selectLatched = false;
}

bool modeManagerCheckBootRecoveryRequest() {
  const uint32_t startedAtMs = millis();
  uint32_t comboHoldStartMs = 0;

  while ((millis() - startedAtMs) < s_config.bootRecoveryWindowMs) {
    if (isPressed(PB_MEN_PIN) && isPressed(PB_SEL_PIN)) {
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

  if (!s_confirmPromptActive) {
    if (menuPressed) {
      if (s_menuHoldStartMs == 0) {
        s_menuHoldStartMs = millis();
      }

      if (elapsedSince(s_menuHoldStartMs, s_config.menuHoldDurationMs)) {
        s_confirmPromptActive = true;
        s_confirmWindowStartMs = millis();
        s_selectLatched = selectPressed;
      }
    } else {
      s_menuHoldStartMs = 0;
    }

    return;
  }

  if (elapsedSince(s_confirmWindowStartMs, s_config.selectConfirmWindowMs)) {
    s_confirmPromptActive = false;
    s_menuHoldStartMs = 0;
    s_confirmWindowStartMs = 0;
    s_selectLatched = false;
    return;
  }

  if (selectPressed && !s_selectLatched) {
    setMode(MATRIXCLOCK_MODE_AP_SETUP);
    return;
  }

  s_selectLatched = selectPressed;
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
    Serial.print(" active=");
    Serial.print(kButtonDiag[i].activeLow ? "LOW" : "HIGH");
    Serial.print(" input=");
    Serial.println(kButtonDiag[i].expectsInternalPullup ? "INPUT_PULLUP" : "INPUT (external pull resistor expected)");
  }
}

void modeManagerServiceButtonDiagnostics() {
  for (size_t i = 0; i < (sizeof(kButtonDiag) / sizeof(kButtonDiag[0])); ++i) {
    int raw = digitalRead(kButtonDiag[i].pin);
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
      Serial.print(" pressed=");
      Serial.println(isPressed(kButtonDiag[i]) ? 1 : 0);
    }
  }
}

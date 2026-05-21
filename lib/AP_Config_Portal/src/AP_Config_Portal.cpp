#include "AP_Config_Portal.h"

static APPortalCallbacks s_callbacks = {nullptr, nullptr, nullptr, nullptr, nullptr};

void apPortalBegin() {
}

void apPortalEnd() {
}

bool apPortalRegisterPage(const char *pageId, const char *title) {
  if (pageId == nullptr || title == nullptr) {
    return false;
  }
  return true;
}

bool apPortalRegisterField(const APFieldDefinition &field) {
  if (field.pageId == nullptr || field.fieldId == nullptr || field.label == nullptr) {
    return false;
  }
  return true;
}

void apPortalSetCallbacks(const APPortalCallbacks &callbacks) {
  s_callbacks = callbacks;
}

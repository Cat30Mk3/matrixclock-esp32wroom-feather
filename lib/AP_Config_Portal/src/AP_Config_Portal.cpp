#include "AP_Config_Portal.h"

namespace {
static const size_t kMaxPages = 8;
static const size_t kMaxFields = 64;

struct RegisteredPage {
  const char *pageId;
  const char *title;
};

struct RegisteredField {
  APFieldDefinition field;
};

static RegisteredPage s_pages[kMaxPages];
static size_t s_pageCount = 0;
static RegisteredField s_fields[kMaxFields];
static size_t s_fieldCount = 0;

bool hasText(const char *value) {
  return value != nullptr && value[0] != '\0';
}

bool pageExists(const char *pageId) {
  for (size_t i = 0; i < s_pageCount; ++i) {
    if (strcmp(s_pages[i].pageId, pageId) == 0) {
      return true;
    }
  }
  return false;
}

bool fieldExists(const char *fieldId) {
  for (size_t i = 0; i < s_fieldCount; ++i) {
    if (strcmp(s_fields[i].field.fieldId, fieldId) == 0) {
      return true;
    }
  }
  return false;
}
}

static APPortalCallbacks s_callbacks = {nullptr, nullptr, nullptr, nullptr, nullptr};

void apPortalBegin() {
  s_pageCount = 0;
  s_fieldCount = 0;
}

void apPortalEnd() {
}

bool apPortalRegisterPage(const char *pageId, const char *title) {
  if (!hasText(pageId) || !hasText(title)) {
    return false;
  }

  if (s_pageCount >= kMaxPages) {
    return false;
  }

  if (pageExists(pageId)) {
    return false;
  }

  s_pages[s_pageCount].pageId = pageId;
  s_pages[s_pageCount].title = title;
  ++s_pageCount;

  return true;
}

bool apPortalRegisterField(const APFieldDefinition &field) {
  if (!hasText(field.pageId) || !hasText(field.fieldId) || !hasText(field.label)) {
    return false;
  }

  if (!pageExists(field.pageId)) {
    return false;
  }

  if (field.maxLength == 0) {
    return false;
  }

  if (field.optionCount > 0 && field.options == nullptr) {
    return false;
  }

  if (s_fieldCount >= kMaxFields) {
    return false;
  }

  if (fieldExists(field.fieldId)) {
    return false;
  }

  s_fields[s_fieldCount].field = field;
  ++s_fieldCount;

  return true;
}

void apPortalSetCallbacks(const APPortalCallbacks &callbacks) {
  s_callbacks = callbacks;
}

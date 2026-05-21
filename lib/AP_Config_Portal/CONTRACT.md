# AP_Config_Portal Contract (Phase 0)

## Scope

AP_Config_Portal is a reusable engine for AP setup UX and form handling.

## Must Not Depend On

- matrixClock global variables
- secrets.h or secrets_template.h
- matrixClock-specific display implementation details

## Adapter Responsibilities (Project-Specific)

MatrixClock_Config provides:

- page and field registration
- load callback implementation (project storage)
- save callback implementation (project storage)
- apply callback implementation (project runtime side effects)
- status provider implementation (WiFi, MQTT, and time source state)

## Interface Surface Defined in Phase 0

See AP_Config_Portal.h:

- APFieldDefinition and APFieldOption
- APPortalStatus
- APPortalCallbacks
- apPortalRegisterPage(...)
- apPortalRegisterField(...)
- apPortalSetCallbacks(...)

No runtime behavior is implemented in Phase 0.

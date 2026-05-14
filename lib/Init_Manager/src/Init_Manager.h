#ifndef INIT_MANAGER_H
#define INIT_MANAGER_H

#include <Arduino.h>

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * Parse project metadata from compile time information
 * @param nameFromFileName - pointer to store project name
 * @param versionFromFileName - pointer to store version string
 * @param dateFromFileName - pointer to store compile date
 * @param timeFromFileName - pointer to store compile time
 */
void fileNameParser(char *nameFromFileName, char *versionFromFileName, char *dateFromFileName, char *timeFromFileName);

/**
 * Initialize display parameters with groupMode, MQTT topics, and symbol indices
 * Configures display behavior differently for DISPLAY1X4 and DISPLAY2X8 modes
 */
void initializeDispParam(void);

/**
 * Initialize global display buffer variables
 */
void initializeGlobalVariables(void);

/**
 * Initialize GPIO pins for LEDs and buttons
 */
void initializeGPIOPins(void);

#endif // INIT_MANAGER_H

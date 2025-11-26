/*
 * Logger Implementation with Log Levels
 * Provides DEBUG, INFO, WARNING, ERROR log levels
 */

#ifndef LOGGER_H
#define LOGGER_H

#include "config.h"
#include <Arduino.h>

// External log level variable (defined in calibration_data.cpp)
extern int g_logLevel;

// Log level checking macros
#define LOG_DEBUG(msg)   if (g_logLevel <= LOG_LEVEL_DEBUG) { Serial.println("[DEBUG] " + String(msg)); }
#define LOG_INFO(msg)    if (g_logLevel <= LOG_LEVEL_INFO) { Serial.println("[INFO] " + String(msg)); }
#define LOG_WARNING(msg) if (g_logLevel <= LOG_LEVEL_WARNING) { Serial.println("[WARNING] " + String(msg)); }
#define LOG_ERROR(msg)   if (g_logLevel <= LOG_LEVEL_ERROR) { Serial.println("[ERROR] " + String(msg)); }

// Helper function to get log level name
String getLogLevelName(int level);

#endif


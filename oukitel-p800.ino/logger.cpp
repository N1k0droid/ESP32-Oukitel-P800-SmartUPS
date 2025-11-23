/*
 * Logger Implementation
 */

#include "logger.h"

String getLogLevelName(int level) {
  switch (level) {
    case LOG_LEVEL_DEBUG:   return "DEBUG";
    case LOG_LEVEL_INFO:     return "INFO";
    case LOG_LEVEL_WARNING:  return "WARNING";
    case LOG_LEVEL_ERROR:    return "ERROR";
    case LOG_LEVEL_NONE:     return "NONE";
    default:                 return "INFO";
  }
}


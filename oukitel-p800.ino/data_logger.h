/*
 * Data Logger - Handles persistent data storage and energy calculations
 */

#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

struct EnergyRecord {
  unsigned long timestamp;
  float consumption;
  float generation;
  float batteryLevel;
};

class DataLogger {
private:
  bool initialized;
  String currentLogFile;
  unsigned long lastLogTime;
  
  // Energy tracking
  float totalDailyConsumption;
  float totalMonthlyConsumption;
  unsigned long lastEnergyCalc;
  int currentDay;
  int currentMonth;
  
  // File management
  bool createLogFile();
  bool rotateLogFiles();
  String getLogFileName(int year, int month, int day);
  void cleanOldLogs();
  
  // Energy calculations
  void calculateEnergyConsumption(const SensorData& data);
  void updateDailyTotals();
  void updateMonthlyTotals();

public:
  DataLogger();
  bool begin();
  
  // Logging methods
  bool logData(const SensorData& sensorData, const EnergyData& energyData);
  bool logEvent(const String& event, const String& details = "");
  
  // Data retrieval
  String getLogData(int days = 1);
  String getEnergyStats();
  EnergyData getEnergyData();
  
  // Maintenance
  void clearLogs();
  size_t getLogSize();
  String getStorageInfo();
};

#endif // DATA_LOGGER_H
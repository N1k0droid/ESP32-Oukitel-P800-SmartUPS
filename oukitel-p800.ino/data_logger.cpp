/*
 * Data Logger Implementation
 */

#include "data_logger.h"
#include "logger.h"
#include <time.h>

DataLogger::DataLogger() {
  initialized = false;
  lastLogTime = 0;
  totalDailyConsumption = 0.0;
  totalMonthlyConsumption = 0.0;
  lastEnergyCalc = 0;
  currentDay = 0;
  currentMonth = 0;
}

bool DataLogger::begin() {
  Serial.println("[LOG] Initializing data logger...");
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("[LOG] SPIFFS initialization failed");
    return false;
  }
  
  // Check available space
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  
  Serial.println("[LOG] SPIFFS initialized:");
  Serial.println("  Total: " + String(totalBytes) + " bytes");
  Serial.println("  Used: " + String(usedBytes) + " bytes");
  Serial.println("  Free: " + String(totalBytes - usedBytes) + " bytes");
  
  // Clean old logs if needed
  cleanOldLogs();
  
  // Load existing energy data
  updateDailyTotals();
  updateMonthlyTotals();
  
  initialized = true;
  Serial.println("[LOG] Data logger initialized");
  return true;
}

bool DataLogger::logData(const SensorData& sensorData, const EnergyData& energyData) {
  if (!initialized) return false;
  
  // Check if enough time has passed since last log
  if (millis() - lastLogTime < LOG_INTERVAL) {
    return true;
  }
  
  // Calculate energy consumption
  calculateEnergyConsumption(sensorData);
  
  // Create log entry
  DynamicJsonDocument doc(512);
  doc["timestamp"] = sensorData.timestamp;
  doc["main_current"] = sensorData.mainCurrent;
  doc["output_current"] = sensorData.outputCurrent;
  doc["battery_voltage"] = sensorData.batteryVoltage;
  doc["battery_percentage"] = sensorData.batteryPercentage;
  doc["main_power"] = sensorData.mainPower;
  doc["output_power"] = sensorData.outputPower;
  doc["on_battery"] = sensorData.onBattery;
  doc["daily_consumption"] = totalDailyConsumption;
  doc["monthly_consumption"] = totalMonthlyConsumption;
  
  // Get current date for log file - verify NTP sync first
  time_t now = time(nullptr);
  if (now < 1000000000) {
    // NTP not synchronized, use millis() as fallback
    LOG_WARNING("Data logger: NTP not synchronized, using system uptime for timestamp");
    now = 0; // Will be handled by log entry timestamp
  }
  struct tm* timeinfo = localtime(&now);
  
  String logFile = getLogFileName(
    timeinfo->tm_year + 1900,
    timeinfo->tm_mon + 1,
    timeinfo->tm_mday
  );
  
  // Append to log file
  File file = SPIFFS.open(logFile, "a");
  if (!file) {
    LOG_ERROR("Data logger: Failed to open log file: " + logFile);
    return false;
  }
  
  String logEntry;
  serializeJson(doc, logEntry);
  file.println(logEntry);
  file.close();
  
  lastLogTime = millis();
  
  // Debug output every 10 logs
  static int logCount = 0;
  if (++logCount >= 10) {
    LOG_DEBUG("Data logger: Logged 10 entries to: " + logFile);
    LOG_DEBUG("  Daily consumption: " + String(totalDailyConsumption, 3) + " kWh");
    LOG_DEBUG("  Monthly consumption: " + String(totalMonthlyConsumption, 3) + " kWh");
    logCount = 0;
  }
  
  return true;
}

bool DataLogger::logEvent(const String& event, const String& details) {
  if (!initialized) return false;
  
  DynamicJsonDocument doc(256);
  doc["timestamp"] = millis();
  doc["type"] = "event";
  doc["event"] = event;
  doc["details"] = details;
  
  File file = SPIFFS.open("/events.log", "a");
  if (!file) {
    LOG_ERROR("Data logger: Failed to open events log");
    return false;
  }
  
  String logEntry;
  serializeJson(doc, logEntry);
  file.println(logEntry);
  file.close();
  
  Serial.println("[LOG] Event logged: " + event + " - " + details);
  return true;
}

void DataLogger::calculateEnergyConsumption(const SensorData& data) {
  unsigned long currentTime = millis();
  
  if (lastEnergyCalc == 0) {
    lastEnergyCalc = currentTime;
    return;
  }
  
  // Calculate time difference in hours
  float timeDiff = (currentTime - lastEnergyCalc) / 3600000.0; // Convert to hours
  
  // Calculate energy consumption (kWh)
  // We track consumption from the main power input, not output
  float energyConsumed = (data.mainPower / 1000.0) * timeDiff;
  
  // Add to daily total
  totalDailyConsumption += energyConsumed;
  
  // Add to monthly total
  totalMonthlyConsumption += energyConsumed;
  
  // Check if day has changed - verify NTP sync first
  time_t now = time(nullptr);
  int today = 0;
  int thisMonth = 0;
  
  if (now >= 1000000000) {
    // NTP synchronized, use real time
    struct tm* timeinfo = localtime(&now);
    today = timeinfo->tm_mday;
    thisMonth = timeinfo->tm_mon + 1;
  } else {
    // NTP not synchronized, skip day/month change detection
    return;
  }
  
  if (currentDay != today) {
    if (currentDay != 0) {
      // Day changed, reset daily total
      Serial.println("[LOG] Day changed, daily consumption was: " + String(totalDailyConsumption, 3) + " kWh");
      totalDailyConsumption = 0.0;
    }
    currentDay = today;
  }
  
  if (currentMonth != thisMonth) {
    if (currentMonth != 0) {
      // Month changed, reset monthly total
      Serial.println("[LOG] Month changed, monthly consumption was: " + String(totalMonthlyConsumption, 3) + " kWh");
      totalMonthlyConsumption = 0.0;
    }
    currentMonth = thisMonth;
  }
  
  lastEnergyCalc = currentTime;
}

void DataLogger::updateDailyTotals() {
  time_t now = time(nullptr);
  if (now < 1000000000) {
    // NTP not synchronized
    totalDailyConsumption = 0.0;
    return;
  }
  struct tm* timeinfo = localtime(&now);
  
  String todayFile = getLogFileName(
    timeinfo->tm_year + 1900,
    timeinfo->tm_mon + 1,
    timeinfo->tm_mday
  );
  
  if (!SPIFFS.exists(todayFile)) {
    totalDailyConsumption = 0.0;
    return;
  }
  
  File file = SPIFFS.open(todayFile, "r");
  if (!file) return;
  
  float maxConsumption = 0.0;
  String line;
  
  while (file.available()) {
    line = file.readStringUntil('\n');
    line.trim();
    
    if (line.length() > 0) {
      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, line);
      
      if (!error && doc.containsKey("daily_consumption")) {
        float consumption = doc["daily_consumption"];
        if (consumption > maxConsumption) {
          maxConsumption = consumption;
        }
      }
    }
  }
  
  file.close();
  totalDailyConsumption = maxConsumption;
  currentDay = timeinfo->tm_mday;
}

void DataLogger::updateMonthlyTotals() {
  time_t now = time(nullptr);
  if (now < 1000000000) {
    // NTP not synchronized
    totalMonthlyConsumption = 0.0;
    return;
  }
  struct tm* timeinfo = localtime(&now);
  
  // Sum all daily files for current month
  float monthlyTotal = 0.0;
  int year = timeinfo->tm_year + 1900;
  int month = timeinfo->tm_mon + 1;
  
  for (int day = 1; day <= 31; day++) {
    String dayFile = getLogFileName(year, month, day);
    
    if (SPIFFS.exists(dayFile)) {
      File file = SPIFFS.open(dayFile, "r");
      if (file) {
        float dayMax = 0.0;
        String line;
        
        while (file.available()) {
          line = file.readStringUntil('\n');
          line.trim();
          
          if (line.length() > 0) {
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, line);
            
            if (!error && doc.containsKey("daily_consumption")) {
              float consumption = doc["daily_consumption"];
              if (consumption > dayMax) {
                dayMax = consumption;
              }
            }
          }
        }
        
        file.close();
        monthlyTotal += dayMax;
      }
    }
  }
  
  totalMonthlyConsumption = monthlyTotal;
  currentMonth = month;
}

String DataLogger::getLogFileName(int year, int month, int day) {
  return "/log_" + String(year) + "_" + 
         String(month < 10 ? "0" : "") + String(month) + "_" +
         String(day < 10 ? "0" : "") + String(day) + ".json";
}

String DataLogger::getLogData(int days) {
  if (!initialized) return "{}";
  
  DynamicJsonDocument doc(4096);
  JsonArray dataArray = doc.createNestedArray("data");
  
  time_t now = time(nullptr);
  if (now < 1000000000) {
    // NTP not synchronized, return empty data
    String result;
    serializeJson(doc, result);
    return result;
  }
  
  for (int i = 0; i < days; i++) {
    time_t dayTime = now - (i * 24 * 60 * 60);
    struct tm* timeinfo = localtime(&dayTime);
    
    String logFile = getLogFileName(
      timeinfo->tm_year + 1900,
      timeinfo->tm_mon + 1,
      timeinfo->tm_mday
    );
    
    if (SPIFFS.exists(logFile)) {
      File file = SPIFFS.open(logFile, "r");
      if (file) {
        String line;
        while (file.available() && dataArray.size() < 100) { // Limit to 100 entries
          line = file.readStringUntil('\n');
          line.trim();
          
          if (line.length() > 0) {
            DynamicJsonDocument entryDoc(512);
            DeserializationError error = deserializeJson(entryDoc, line);
            
            if (!error) {
              dataArray.add(entryDoc);
            }
          }
        }
        file.close();
      }
    }
  }
  
  String result;
  serializeJson(doc, result);
  return result;
}

String DataLogger::getEnergyStats() {
  DynamicJsonDocument doc(512);
  doc["daily_consumption"] = totalDailyConsumption;
  doc["monthly_consumption"] = totalMonthlyConsumption;
  doc["current_day"] = currentDay;
  doc["current_month"] = currentMonth;
  doc["log_entries"] = getLogSize();
  
  String result;
  serializeJson(doc, result);
  return result;
}

EnergyData DataLogger::getEnergyData() {
  EnergyData data;
  data.dailyConsumption = totalDailyConsumption;
  data.monthlyConsumption = totalMonthlyConsumption;
  data.instantPower = 0.0; // Will be filled by energy monitor
  data.peakPower = 0.0;    // Will be filled by energy monitor
  data.operatingTime = millis() / 1000; // System uptime in seconds
  
  return data;
}

void DataLogger::cleanOldLogs() {
  Serial.println("[LOG] Cleaning old log files...");
  
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  time_t now = time(nullptr);
  if (now < 1000000000) {
    // NTP not synchronized, skip cleanup
    Serial.println("[LOG] NTP not synchronized, skipping log cleanup");
    return;
  }
  time_t cutoffTime = now - (LOG_RETENTION_DAYS * 24 * 60 * 60);
  
  int deletedFiles = 0;
  
  while (file) {
    String fileName = file.name();
    
    if (fileName.startsWith("/log_") && fileName.endsWith(".json")) {
      // Extract date from filename: log_YYYY_MM_DD.json
      int year = fileName.substring(5, 9).toInt();
      int month = fileName.substring(10, 12).toInt();
      int day = fileName.substring(13, 15).toInt();
      
      struct tm fileTime = {0};
      fileTime.tm_year = year - 1900;
      fileTime.tm_mon = month - 1;
      fileTime.tm_mday = day;
      
      time_t fileTimestamp = mktime(&fileTime);
      
      if (fileTimestamp < cutoffTime) {
        file.close();
        SPIFFS.remove(fileName);
        deletedFiles++;
        Serial.println("[LOG] Deleted old log file: " + fileName);
      }
    }
    
    file = root.openNextFile();
  }
  
  if (deletedFiles > 0) {
    Serial.println("[LOG] Cleaned " + String(deletedFiles) + " old log files");
  }
}

size_t DataLogger::getLogSize() {
  size_t totalSize = 0;
  
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  while (file) {
    String fileName = file.name();
    if (fileName.startsWith("/log_") && fileName.endsWith(".json")) {
      totalSize += file.size();
    }
    file = root.openNextFile();
  }
  
  return totalSize;
}

String DataLogger::getStorageInfo() {
  DynamicJsonDocument doc(512);
  
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  size_t logSize = getLogSize();
  
  doc["total_bytes"] = totalBytes;
  doc["used_bytes"] = usedBytes;
  doc["free_bytes"] = totalBytes - usedBytes;
  doc["log_size"] = logSize;
  doc["log_percentage"] = (float)logSize / totalBytes * 100.0;
  
  String result;
  serializeJson(doc, result);
  return result;
}

void DataLogger::clearLogs() {
  Serial.println("[LOG] Clearing all log files...");
  
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  int deletedFiles = 0;
  
  while (file) {
    String fileName = file.name();
    if (fileName.startsWith("/log_") && fileName.endsWith(".json")) {
      file.close();
      SPIFFS.remove(fileName);
      deletedFiles++;
    }
    file = root.openNextFile();
  }
  
  // Reset energy totals
  totalDailyConsumption = 0.0;
  totalMonthlyConsumption = 0.0;
  
  Serial.println("[LOG] Cleared " + String(deletedFiles) + " log files");
}
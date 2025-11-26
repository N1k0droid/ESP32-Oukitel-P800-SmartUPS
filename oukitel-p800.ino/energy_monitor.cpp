/*
 * Energy Monitor Implementation with NTP Time Sync
 */


#include "energy_monitor.h"
#include "config.h"
#include "logger.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <time.h>


PowerStationMonitor::PowerStationMonitor() {
  initialized = false;
  peakPower = 0.0;
  averagePower = 0.0;
  startTime = 0;
  lastUpdate = 0;
  totalEnergyConsumed = 0.0;
  dailyEnergyReset = 0.0;
  monthlyEnergyReset = 0.0;
  powerFactor = 1.0;
  efficiency = 0.0;
  bufferIndex = 0;
  bufferFull = false;
  
  lastStableMonthly = 0.0;
  lastStableDaily = 0.0;
  lastStableUpdate = 0;
  lastDailyCheck = 0;
  lastMonthCheck = 0;
  lastDay = 0;
  
  // Initialize current month/year - DON'T use time() here as NTP may not be synced yet
  // Will be set correctly in syncTimeAfterNTP() or checkMonthRollover()
  currentMonth = 0;  // 0 = not initialized (will be set after NTP sync)
  currentYear = 0;   // 0 = not initialized (will be set after NTP sync)
  
  // Initialize power buffer
  for (int i = 0; i < POWER_BUFFER_SIZE; i++) {
    powerBuffer[i] = 0.0;
  }
  
  // Initialize energy data structure
  currentData.dailyConsumption = 0.0;
  currentData.monthlyConsumption = 0.0;
  currentData.instantPower = 0.0;
  currentData.peakPower = 0.0;
  currentData.operatingTime = 0;
}


bool PowerStationMonitor::begin() {
  LOG_INFO("Initializing energy monitor...");
  
  startTime = millis();
  lastUpdate = startTime;
  lastDailyCheck = startTime;
  lastMonthCheck = startTime;
  
  // Load state from SPIFFS
  loadEnergyState();
  
  // Load monthly history from SPIFFS
  loadMonthlyHistory();
  
  initialized = true;
  LOG_DEBUG("Energy monitor initialized");
  return true;
}


void PowerStationMonitor::update(const SensorData& sensorData) {
  if (!initialized) return;
  
  unsigned long currentTime = millis();
  float timeDelta = (currentTime - lastUpdate) / 1000.0; // Convert to seconds
  
  if (timeDelta < 0.1) return; // Minimum 100ms between updates
  
  // Check for daily rollover (every 60 seconds)
  if (currentTime - lastDailyCheck >= 60000) {
    checkDailyRollover();
    lastDailyCheck = currentTime;
  }
  
  // Check for month rollover (every 60 seconds)
  if (currentTime - lastMonthCheck >= 60000) {
    checkMonthRollover();
    lastMonthCheck = currentTime;
  }
  
  // Use mainPower (total consumption of PowerStation)
  currentData.instantPower = sensorData.mainPower;
  
  // Update power buffer for averaging
  updatePowerBuffer(currentData.instantPower);
  
  // Calculate average power
  averagePower = calculateAveragePower();
  
  // Update peak power
  if (currentData.instantPower > peakPower) {
    peakPower = currentData.instantPower;
    currentData.peakPower = peakPower;
  }
  
  // Calculate energy consumption (kWh) - Accumulate without resetting
  float energyDelta = (currentData.instantPower / 1000.0) * (timeDelta / 3600.0);
  totalEnergyConsumed += energyDelta;
  
  // Update daily and monthly consumption
  currentData.dailyConsumption = totalEnergyConsumed - dailyEnergyReset;
  currentData.monthlyConsumption = totalEnergyConsumed - monthlyEnergyReset;
  
  // Calculate efficiency
  calculateEfficiency(sensorData);
  
  // Update operating time
  currentData.operatingTime = (currentTime - startTime) / 1000;
  
  lastUpdate = currentTime;
  
  // Save state periodically (every 5 minutes)
  static unsigned long lastSave = 0;
  if (currentTime - lastSave > 300000) {
    saveEnergyState();
    lastSave = currentTime;
  }
  
  // Debug output every 60 seconds
  static unsigned long lastDebug = 0;
  if (currentTime - lastDebug > 60000) {
    LOG_DEBUG("Energy Statistics:");
    LOG_DEBUG("  Instant: " + String(currentData.instantPower, 1) + "W | Avg: " + 
              String(averagePower, 1) + "W | Peak: " + String(peakPower, 1) + "W");
    LOG_DEBUG("  Daily: " + String(currentData.dailyConsumption, 3) + "kWh | " +
              "Monthly: " + String(currentData.monthlyConsumption, 3) + "kWh");
    LOG_DEBUG("  Total: " + String(totalEnergyConsumed, 3) + "kWh | " +
              "Eff: " + String(efficiency, 1) + "% | " +
              "PF: " + String(powerFactor, 2));
    lastDebug = currentTime;
  }
}


void PowerStationMonitor::checkDailyRollover() {
  time_t now = time(nullptr);
  if (now < 1000000000) return; // NTP not synced yet
  
  struct tm* timeinfo = localtime(&now);
  int newDay = timeinfo->tm_mday;
  
  if (newDay != lastDay) {
    LOG_INFO("Energy monitor: Daily rollover detected: " + String(lastDay) + " -> " + String(newDay));
    
    // Reset daily consumption at midnight
    resetDailyStats();
    
    lastDay = newDay;
    saveEnergyState();
  }
}


void PowerStationMonitor::checkMonthRollover() {
  time_t now = time(nullptr);
  if (now < 1000000000) return; // NTP not synced yet
  
  struct tm* timeinfo = localtime(&now);
  int newMonth = timeinfo->tm_mon + 1;
  int newYear = timeinfo->tm_year + 1900;
  
  if (newMonth != currentMonth || newYear != currentYear) {
    LOG_INFO("Energy monitor: Month rollover detected: " + String(currentYear) + "-" + 
             String(currentMonth) + " -> " + String(newYear) + "-" + String(newMonth));
    
    // Save current month's data to history
    MonthlyEnergyRecord record;
    record.year = currentYear;
    record.month = currentMonth;
    record.consumption = currentData.monthlyConsumption;
    
    // Add to history (keep only last 12 months)
    monthlyHistory.push_back(record);
    if (monthlyHistory.size() > 12) {
      monthlyHistory.erase(monthlyHistory.begin());
    }
    
    // Save to SPIFFS
    saveMonthlyHistory();
    
    // Reset monthly consumption
    monthlyEnergyReset = totalEnergyConsumed;
    currentData.monthlyConsumption = 0.0;
    lastStableMonthly = 0.0;
    
    currentMonth = newMonth;
    currentYear = newYear;
    
    saveEnergyState();
    
    LOG_INFO("Energy monitor: Month rollover complete. History saved.");
  }
}


void PowerStationMonitor::loadEnergyState() {
  if (!SPIFFS.exists("/energy_state.json")) {
    LOG_DEBUG("Energy monitor: No saved energy state found");
    return;
  }
  
  File file = SPIFFS.open("/energy_state.json", "r");
  if (!file) {
    LOG_ERROR("Energy monitor: Failed to open energy state file");
    return;
  }
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    LOG_ERROR("Energy monitor: Failed to parse energy state: " + String(error.c_str()));
    return;
  }
  
  totalEnergyConsumed = doc["totalEnergy"] | 0.0;
  dailyEnergyReset = doc["dailyReset"] | 0.0;
  monthlyEnergyReset = doc["monthlyReset"] | 0.0;
  peakPower = doc["peakPower"] | 0.0;
  lastDay = doc["lastDay"] | 0;
  currentMonth = doc["currentMonth"] | 1;
  currentYear = doc["currentYear"] | 2024;
  
  currentData.dailyConsumption = totalEnergyConsumed - dailyEnergyReset;
  currentData.monthlyConsumption = totalEnergyConsumed - monthlyEnergyReset;
  currentData.peakPower = peakPower;
  
  LOG_DEBUG("Energy monitor: Loaded energy state:");
  Serial.println("  Total: " + String(totalEnergyConsumed, 3) + " kWh");
  Serial.println("  Daily: " + String(currentData.dailyConsumption, 3) + " kWh");
  Serial.println("  Monthly: " + String(currentData.monthlyConsumption, 3) + " kWh");
}


void PowerStationMonitor::saveEnergyState() {
  DynamicJsonDocument doc(1024);
  doc["totalEnergy"] = totalEnergyConsumed;
  doc["dailyReset"] = dailyEnergyReset;
  doc["monthlyReset"] = monthlyEnergyReset;
  doc["peakPower"] = peakPower;
  doc["lastDay"] = lastDay;
  doc["currentMonth"] = currentMonth;
  doc["currentYear"] = currentYear;
  doc["timestamp"] = time(nullptr);
  
  File file = SPIFFS.open("/energy_state.json", "w");
  if (!file) {
    LOG_ERROR("Energy monitor: Failed to save energy state");
    return;
  }
  
  serializeJson(doc, file);
  file.close();
}


void PowerStationMonitor::loadMonthlyHistory() {
  if (!SPIFFS.exists(ENERGY_HISTORY_FILE)) {
    LOG_DEBUG("Energy monitor: No monthly history file found");
    return;
  }
  
  File file = SPIFFS.open(ENERGY_HISTORY_FILE, "r");
  if (!file) {
    LOG_ERROR("Energy monitor: Failed to open history file");
    return;
  }
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    LOG_ERROR("Energy monitor: Failed to parse history JSON: " + String(error.c_str()));
    return;
  }
  
  monthlyHistory.clear();
  JsonArray history = doc["history"].as<JsonArray>();
  int validRecords = 0;
  int invalidRecords = 0;
  
  for (JsonObject record : history) {
    MonthlyEnergyRecord r;
    r.year = record["year"] | 2024;
    r.month = record["month"] | 1;
    r.consumption = record["consumption"] | 0.0;
    
    // Filter out invalid dates (1970 = NTP not synced when saved)
    if (r.year == 1970 || r.year < 2020 || r.year > 2100 || r.month < 1 || r.month > 12) {
      invalidRecords++;
      LOG_DEBUG("Energy monitor: Skipping invalid history record: " + String(r.year) + "-" + String(r.month));
      continue;  // Skip invalid records
    }
    
    monthlyHistory.push_back(r);
    validRecords++;
  }
  
  if (invalidRecords > 0) {
    LOG_INFO("Energy monitor: Filtered out " + String(invalidRecords) + " invalid history records (1970 dates)");
    // Save cleaned history back to SPIFFS
    saveMonthlyHistory();
  }
  
  LOG_DEBUG("Energy monitor: Loaded " + String(validRecords) + " valid monthly records");
}


void PowerStationMonitor::saveMonthlyHistory() {
  DynamicJsonDocument doc(2048);
  JsonArray history = doc.createNestedArray("history");
  
  for (const auto& record : monthlyHistory) {
    JsonObject obj = history.createNestedObject();
    obj["year"] = record.year;
    obj["month"] = record.month;
    obj["consumption"] = record.consumption;
  }
  
  File file = SPIFFS.open(ENERGY_HISTORY_FILE, "w");
  if (!file) {
    LOG_ERROR("Energy monitor: Failed to save history file");
    return;
  }
  
  serializeJson(doc, file);
  file.close();
  
  LOG_DEBUG("Energy monitor: Monthly history saved");
}

// ===================================================================
// NTP Time Synchronization Method
// ===================================================================
void PowerStationMonitor::syncTimeAfterNTP() {
  time_t now = time(nullptr);
  
  // Verify NTP is truly synchronized
  if (now < 1000000000) {
    LOG_WARNING("Energy monitor: syncTimeAfterNTP() called but NTP not synchronized");
    return;
  }
  
  struct tm* timeinfo = localtime(&now);
  int newMonth = timeinfo->tm_mon + 1;
  int newYear = timeinfo->tm_year + 1900;
  
  // If month/year was 0 (not initialized) or 1970 (not synchronized), update it
  if (currentYear == 0 || currentYear == 1970 || currentMonth == 0) {
    if (currentYear == 1970 || currentYear == 0) {
      LOG_INFO("Energy monitor: Time correction: Month/Year updated from " + 
                     String(currentYear) + "-" + String(currentMonth) + " to " + 
                     String(newYear) + "-" + String(newMonth));
    }
    
    currentMonth = newMonth;
    currentYear = newYear;
    lastDay = timeinfo->tm_mday;
    
    // Reset daily and month check timers to trigger immediate rollover check
    lastDailyCheck = 0;
    lastMonthCheck = 0;
    
    // Save corrected state to SPIFFS
    saveEnergyState();
    
    LOG_INFO("Energy monitor: Time synchronized after NTP");
  } else {
    // Already synchronized, nothing to do
    LOG_DEBUG("Energy monitor: syncTimeAfterNTP() called - Already synchronized (" + 
                   String(currentYear) + "-" + String(currentMonth) + ")");
  }
}


std::vector<MonthlyEnergyRecord> PowerStationMonitor::getMonthlyHistory() {
  return monthlyHistory;
}


void PowerStationMonitor::updatePowerBuffer(float power) {
  powerBuffer[bufferIndex] = power;
  bufferIndex++;
  
  if (bufferIndex >= POWER_BUFFER_SIZE) {
    bufferIndex = 0;
    bufferFull = true;
  }
}


float PowerStationMonitor::calculateAveragePower() {
  float sum = 0.0;
  int count = bufferFull ? POWER_BUFFER_SIZE : bufferIndex;
  
  if (count == 0) return 0.0;
  
  for (int i = 0; i < count; i++) {
    sum += powerBuffer[i];
  }
  
  return sum / count;
}


void PowerStationMonitor::calculateEfficiency(const SensorData& data) {
  if (data.mainPower > 10.0) {
    efficiency = (data.outputPower / data.mainPower) * 100.0;
    
    if (efficiency > 100.0) efficiency = 100.0;
    if (efficiency < 0.0) efficiency = 0.0;
  } else {
    efficiency = 0.0;
  }
  
  if (data.mainCurrent > 0.1) {
    float apparentPower = data.mainCurrent * g_mainsVoltage; // Use configurable mains voltage
    powerFactor = data.mainPower / apparentPower;
    
    if (powerFactor > 1.0) powerFactor = 1.0;
    if (powerFactor < 0.0) powerFactor = 0.0;
  } else {
    powerFactor = 1.0;
  }
}


EnergyData PowerStationMonitor::getEnergyData() {
  return currentData;
}


EnergyData PowerStationMonitor::getStableEnergyData() {
  unsigned long currentTime = millis();
  EnergyData stable = currentData;
  
  if (lastStableMonthly > 0.001) {
    if (stable.monthlyConsumption < lastStableMonthly * 0.7) {
      LOG_WARNING("Energy monitor: Anomaly detected!");
      Serial.println("  Monthly: " + String(stable.monthlyConsumption, 3) + "kWh");
      Serial.println("  Expected min: " + String(lastStableMonthly * 0.7, 3) + "kWh");
      Serial.println("  Keeping last stable value: " + String(lastStableMonthly, 3) + "kWh");
      
      stable.monthlyConsumption = lastStableMonthly;
      stable.dailyConsumption = lastStableDaily;
      return stable;
    }
  }
  
  if (stable.monthlyConsumption >= lastStableMonthly) {
    lastStableMonthly = stable.monthlyConsumption;
    lastStableDaily = stable.dailyConsumption;
    lastStableUpdate = currentTime;
  }
  
  return stable;
}


float PowerStationMonitor::getPeakPower() {
  return peakPower;
}


float PowerStationMonitor::getAveragePower() {
  return averagePower;
}


float PowerStationMonitor::getEfficiency() {
  return efficiency;
}


float PowerStationMonitor::getPowerFactor() {
  return powerFactor;
}


unsigned long PowerStationMonitor::getOperatingTime() {
  return currentData.operatingTime;
}


void PowerStationMonitor::resetDailyStats() {
  dailyEnergyReset = totalEnergyConsumed;
  currentData.dailyConsumption = 0.0;
  lastStableDaily = 0.0;
  
  saveEnergyState();
  
    LOG_INFO("Energy monitor: Daily statistics reset. Total: " + String(totalEnergyConsumed, 3) + "kWh");
}


void PowerStationMonitor::resetMonthlyStats() {
  monthlyEnergyReset = totalEnergyConsumed;
  currentData.monthlyConsumption = 0.0;
  lastStableMonthly = 0.0;
  
  saveEnergyState();
  
    LOG_INFO("Energy monitor: Monthly statistics reset. Total: " + String(totalEnergyConsumed, 3) + "kWh");
}


void PowerStationMonitor::resetAllStats() {
  peakPower = 0.0;
  averagePower = 0.0;
  totalEnergyConsumed = 0.0;
  dailyEnergyReset = 0.0;
  monthlyEnergyReset = 0.0;
  efficiency = 0.0;
  powerFactor = 1.0;
  
  lastStableMonthly = 0.0;
  lastStableDaily = 0.0;
  lastStableUpdate = 0;
  
  currentData.dailyConsumption = 0.0;
  currentData.monthlyConsumption = 0.0;
  currentData.instantPower = 0.0;
  currentData.peakPower = 0.0;
  currentData.operatingTime = 0;
  
  for (int i = 0; i < POWER_BUFFER_SIZE; i++) {
    powerBuffer[i] = 0.0;
  }
  bufferIndex = 0;
  bufferFull = false;
  
  startTime = millis();
  lastUpdate = startTime;
  
  saveEnergyState();
  
    LOG_INFO("Energy monitor: All statistics reset");
}
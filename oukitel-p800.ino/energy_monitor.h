/*
 * Energy Monitor - Handles energy calculations and power management
 */


#ifndef ENERGY_MONITOR_H
#define ENERGY_MONITOR_H


#include "config.h"
#include <vector>


class PowerStationMonitor {
private:
  EnergyData currentData;
  bool initialized;
  
  // Power tracking
  float peakPower;
  float averagePower;
  unsigned long startTime;
  unsigned long lastUpdate;
  
  // Energy calculations
  float totalEnergyConsumed;
  float dailyEnergyReset;
  float monthlyEnergyReset;
  
  // Power quality metrics
  float powerFactor;
  float efficiency;
  
  // Moving averages
  static const int POWER_BUFFER_SIZE = 60; // 1 minute of data
  float powerBuffer[POWER_BUFFER_SIZE];
  int bufferIndex;
  bool bufferFull;
  
  // Data stabilization
  float lastStableMonthly;
  float lastStableDaily;
  unsigned long lastStableUpdate;
  
  // Time tracking for rollover detection
  unsigned long lastDailyCheck;
  unsigned long lastMonthCheck;
  int lastDay;
  
  // Monthly history tracking
  std::vector<MonthlyEnergyRecord> monthlyHistory;
  int currentMonth;
  int currentYear;
  
  // Helper methods
  void updatePowerBuffer(float power);
  float calculateAveragePower();
  void calculateEfficiency(const SensorData& data);
  void checkDailyRollover();
  void checkMonthRollover();
  void loadMonthlyHistory();
  void saveMonthlyHistory();
  void loadEnergyState();
  void saveEnergyState();


public:
  PowerStationMonitor();
  bool begin();
  void update(const SensorData& sensorData);
  EnergyData getEnergyData();
  
  // Stable data without fluctuations
  EnergyData getStableEnergyData();
  
  // ===================================================================
  // NTP Time Synchronization
  // ===================================================================
  void syncTimeAfterNTP();    // Ricalibbra mese/anno dopo NTP sync
  
  // Monthly history methods
  std::vector<MonthlyEnergyRecord> getMonthlyHistory();
  
  // Statistics methods
  float getPeakPower();
  float getAveragePower();
  float getEfficiency();
  float getPowerFactor();
  unsigned long getOperatingTime();
  
  // Reset methods
  void resetDailyStats();
  void resetMonthlyStats();
  void resetAllStats();
};


#endif // ENERGY_MONITOR_H

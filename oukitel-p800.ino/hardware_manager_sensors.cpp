/*
 * Hardware Manager Implementation - SENSORS AND CONVERSIONS
 */

#include "hardware_manager.h"
#include "logger.h"

float HardwareManager::readBatteryVoltageRaw() {
  // If a fixed/override voltage is configured (> 0), use it directly for testing
  if (g_fixedVoltage > 0.0f) {
    return g_fixedVoltage;
  }

  uint32_t adcSum = 0;
  for(int i = 0; i < BATTERY_ADC_SAMPLES; i++) {
    adcSum += analogRead(PIN_BATTERY_VOLTAGE);
    delayMicroseconds(100);
  }
  float adcValue = adcSum / (float)BATTERY_ADC_SAMPLES;
  float adcVoltage = (adcValue / 4095.0) * 3.3;
  
  float batteryVoltage = adcVoltage * g_batteryAdcCalibration * g_batteryDividerRatio;
  
  if(currentState == STATE_DISCHARGING) {
    batteryVoltage = compensateVoltageDischarge(batteryVoltage);
    batteryVoltage += g_voltageOffsetDischarge;
  }
  else if(currentState == STATE_CHARGING) {
    batteryVoltage = compensateVoltageCharge(batteryVoltage);
    batteryVoltage += g_voltageOffsetCharge;
  }
  else if(currentState == STATE_BYPASS) {
    batteryVoltage += VOLTAGE_OFFSET_BYPASS;
  }
  else {
    batteryVoltage += g_voltageOffsetRest;
  }
  
  return batteryVoltage;
}

float HardwareManager::compensateVoltageDischarge(float measuredV) {
  if(measuredV >= 28.5) return measuredV - 1.00;
  else if(measuredV >= 27.0) return measuredV - 0; 
  else if(measuredV >= 26.5) return measuredV - 0;
  else if(measuredV >= 26.0) return measuredV - 0;   
  else if(measuredV >= 25.5) return measuredV + 0.10;
  else if(measuredV >= 25.2) return measuredV + 0.20;
  else if(measuredV >= 25.0) return measuredV + 0.30;
  else if(measuredV >= 24.8) return measuredV + 0.40;
  else if(measuredV >= 24.5) return measuredV + 0.50;
  else if(measuredV >= 24.0) return measuredV + 0.50;
  else if(measuredV >= 23.5) return measuredV + 0;
  else if(measuredV >= 23.0) return measuredV + 0;
  else if(measuredV >= 22.5) return measuredV + 0;
  else if(measuredV >= 22.0) return measuredV + 0;
  else if(measuredV >= 21.5) return measuredV + 1.50;
  else return measuredV;
}

float HardwareManager::compensateVoltageCharge(float measuredV) {
  if(measuredV >= 28.5) return measuredV - 1.00;
  else if(measuredV >= 28.4) return measuredV - 0.95;
  else if(measuredV >= 28.3) return measuredV - 0.90;
  else if(measuredV >= 28.2) return measuredV - 0.85;
  else if(measuredV >= 28.1) return measuredV - 0.80;
  else if(measuredV >= 28.0) return measuredV - 0.75;
  else if(measuredV >= 27.8) return measuredV - 0.70;
  else if(measuredV >= 27.6) return measuredV - 0.60;
  else if(measuredV >= 27.4) return measuredV - 0.50;
  else if(measuredV >= 27.2) return measuredV - 0.40;
  else if(measuredV >= 27.0) return measuredV - 0.30;
  else if(measuredV >= 26.5) return measuredV - 0.25;
  else if(measuredV >= 26.0) return measuredV + 0;
  else if(measuredV >= 25.5) return measuredV + 0.40;
  else if(measuredV >= 25.0) return measuredV + 0.50;
  else if(measuredV >= 24.5) return measuredV + 0.60;
  else if(measuredV >= 24.0) return measuredV + 0.70;
  else if(measuredV >= 23.5) return measuredV + 0.80;
  else if(measuredV >= 23.0) return measuredV + 1.00;
  else if(measuredV >= 22.5) return measuredV + 1.00;
  else if(measuredV >= 22.0) return measuredV + 1.00;
  else if(measuredV >= 21.5) return measuredV + 1.00;
  else if(measuredV >= 21.0) return measuredV + 1.00;
  else return measuredV;
}

float HardwareManager::voltageToBatteryPercent(float voltage) {
  const float (*curve)[2];
  int points;
  
  if(currentState == STATE_CHARGING) {
    curve = BATTERY_CURVE_CHARGE;
    points = CURVE_CHARGE_POINTS;
  } else {
    curve = BATTERY_CURVE_NORMAL;
    points = CURVE_NORMAL_POINTS;
  }
  
  if(voltage <= curve[points-1][1]) return 0.0;
  if(voltage >= curve[0][1]) return 100.0;
  
  for(int i = 0; i < points - 1; i++) {
    float percentHigh = curve[i][0];
    float voltageHigh = curve[i][1];
    float percentLow = curve[i+1][0];
    float voltageLow = curve[i+1][1];
    
    if(voltage >= voltageLow && voltage <= voltageHigh) {
    return percentLow + (voltage - voltageLow) * 
      (percentHigh - percentLow) / (voltageHigh - voltageLow);
    }
  }
  return 0.0;
}

BatteryState HardwareManager::detectState(float powerIN, float powerOUT) {
  // Usa g_powerThreshold dinamico al posto della costante
  if(powerIN > g_powerThreshold && powerOUT > g_powerThreshold) {
    float diff = abs(powerIN - powerOUT);
    float minPower = min(powerIN, powerOUT);
    if(diff < minPower * 0.3) {
      return STATE_BYPASS;
    }
  }
  
  float netPower = powerIN - powerOUT;
  
  if(netPower > g_powerThreshold) {
    return STATE_CHARGING;
  }
  
  if(netPower < -g_powerThreshold) {
    return STATE_DISCHARGING;
  }
  
  return STATE_REST;
}

void HardwareManager::sortArray(float arr[], int n) {
  for(int i = 0; i < n-1; i++) {
    for(int j = i+1; j < n; j++) {
      if(arr[i] > arr[j]) {
        float temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
      }
    }
  }
}

float HardwareManager::getMedianSOC() {
  int count = socBufferFull ? g_socBufferSize : socBufferIndex;
  if(count == 0) return 0;
  
  float tempArray[SOC_BUFFER_SIZE];
  for(int i = 0; i < count; i++) {
    tempArray[i] = socBuffer[i];
  }
  
  sortArray(tempArray, count);
  
  if(count % 2 == 0) {
    return (tempArray[count/2 - 1] + tempArray[count/2]) / 2.0;
  } else {
    return tempArray[count/2];
  }
}

void HardwareManager::updateSOCBuffer(float newSOC) {
  socBuffer[socBufferIndex] = newSOC;
  socBufferIndex++;
  if(socBufferIndex >= g_socBufferSize) {
    socBufferIndex = 0;
    socBufferFull = true;
  }
}

float HardwareManager::getStableSOC(float currentSOC) {
  updateSOCBuffer(currentSOC);
  
  if(displayedSOC == 0 && socBufferIndex == 1) {
    displayedSOC = currentSOC;
    return displayedSOC;
  }
  
  float medianSOC = getMedianSOC();
  
  if(abs(medianSOC - displayedSOC) > 3.0) {
    int agreeCount = 0;
    int count = socBufferFull ? g_socBufferSize : socBufferIndex;
    
    for(int i = 0; i < count; i++) {
      if(medianSOC > displayedSOC) {
        if(socBuffer[i] > displayedSOC + 1.0) agreeCount++;
      } else {
        if(socBuffer[i] < displayedSOC - 1.0) agreeCount++;
      }
    }
    
    if(agreeCount >= g_socChangeThreshold) {
      displayedSOC = medianSOC;
    }
  } else {
    displayedSOC = medianSOC;
  }
  
  return displayedSOC;
}

void HardwareManager::initializePowerFilters(float powerIN, float powerOUT) {
  if(!powerFilterInitialized) {
    filteredPowerIN = powerIN;
    filteredPowerOUT = powerOUT;
    lastValidPowerIN = powerIN;
    lastValidPowerOUT = powerOUT;
    powerFilterInitialized = true;
  }
}

float HardwareManager::filterPowerIN(float rawPowerIN) {
  if(!powerFilterInitialized) {
    filteredPowerIN = rawPowerIN;
    return filteredPowerIN;
  }
  
  // Usa g_powerFilterAlpha dinamico al posto della costante
  filteredPowerIN = (g_powerFilterAlpha * rawPowerIN) + 
                    ((1.0 - g_powerFilterAlpha) * filteredPowerIN);
  
  return filteredPowerIN;
}

float HardwareManager::filterPowerOUT(float rawPowerOUT) {
  if(!powerFilterInitialized) {
    filteredPowerOUT = rawPowerOUT;
    return filteredPowerOUT;
  }
  
  // Usa g_powerFilterAlpha dinamico al posto della costante
  filteredPowerOUT = (g_powerFilterAlpha * rawPowerOUT) + 
                     ((1.0 - g_powerFilterAlpha) * filteredPowerOUT);
  
  return filteredPowerOUT;
}

bool HardwareManager::validatePowerReadings(float powerIN, float powerOUT) {
  // Check if readings exceed maximum valid power
  if(powerIN > g_maxPowerReading || powerOUT > g_maxPowerReading) {
    Serial.println("[HW] Power reading exceeds maximum (" + String(g_maxPowerReading, 0) + "W): IN=" + String(powerIN, 1) + "W, OUT=" + String(powerOUT, 1) + "W - DISCARDED");
    return false;
  }
  
  // Usa g_powerThreshold dinamico al posto della costante
  if(powerIN > g_powerThreshold) {
    if(powerOUT > powerIN + 5.0) {
      return false;
    }
  }
  return true;
}

// ===================================================================
// ADVANCED SETTINGS (FIXED!)
// ===================================================================
AdvancedSettings HardwareManager::getAdvancedSettings() {
  AdvancedSettings settings;
  settings.powerStationOffVoltage = g_powerStationOffVoltage;
  settings.powerThreshold = g_powerThreshold;
  settings.powerFilterAlpha = g_powerFilterAlpha;
  settings.voltageMinSafe = g_voltageMinSafe;
  settings.batteryLowWarning = g_batteryLowWarning;
  settings.batteryCritical = g_batteryCritical;
  settings.autoPowerOnDelay = g_autoPowerOnDelay;
  settings.socBufferSize = g_socBufferSize;
  settings.socChangeThreshold = g_socChangeThreshold;
  settings.warmupDelay = g_warmupDelay;
  settings.maxPowerReading = g_maxPowerReading;
  settings.valid = true;
  return settings;
}

void HardwareManager::applyAdvancedSettings(const AdvancedSettings& settings) {
  Serial.println("[HW] Applying advanced settings...");
  
  g_powerStationOffVoltage = settings.powerStationOffVoltage;
  g_powerThreshold = settings.powerThreshold;
  g_powerFilterAlpha = settings.powerFilterAlpha;
  g_voltageMinSafe = settings.voltageMinSafe;
  g_batteryLowWarning = settings.batteryLowWarning;
  g_batteryCritical = settings.batteryCritical;
  g_autoPowerOnDelay = settings.autoPowerOnDelay;
  g_socBufferSize = settings.socBufferSize;
  g_socChangeThreshold = settings.socChangeThreshold;
  g_warmupDelay = settings.warmupDelay;
  g_maxPowerReading = settings.maxPowerReading;
  
  Serial.println("[HW] Advanced settings applied successfully");
  Serial.println("     Power Station OFF Voltage: " + String(g_powerStationOffVoltage, 1) + "V");
  Serial.println("     Power Threshold: " + String(g_powerThreshold, 2) + "W");
  Serial.println("     Power Filter Alpha: " + String(g_powerFilterAlpha, 2));
  Serial.println("     Voltage Min Safe: " + String(g_voltageMinSafe, 2) + "V");
  Serial.println("     Battery Low Warning: " + String(g_batteryLowWarning, 1) + "%");
  Serial.println("     Auto Power On Delay: " + String(g_autoPowerOnDelay) + "ms");
  Serial.println("     Warmup Delay: " + String(g_warmupDelay) + "ms");
  Serial.println("     Max Power Reading: " + String(g_maxPowerReading, 1) + "W");
}

void HardwareManager::saveAdvancedSettings() {
  Serial.println("[HW] Saving advanced settings to SPIFFS...");
  
  AdvancedSettings settings = getAdvancedSettings();
  saveAdvancedSettingsToSPIFFS(settings);
}

// ===================================================================
// CALIBRATION
// ===================================================================
CalibrationData HardwareManager::getCalibrationData() {
  CalibrationData cal;
  cal.sct013CalIn = g_sct013CalIn;
  cal.sct013OffsetIn = g_sct013OffsetIn;
  cal.sct013CalOut = g_sct013CalOut;
  cal.sct013OffsetOut = g_sct013OffsetOut;
  cal.batteryDividerRatio = g_batteryDividerRatio;
  cal.batteryAdcCalibration = g_batteryAdcCalibration;
  cal.voltageOffsetCharge = g_voltageOffsetCharge;
  cal.voltageOffsetDischarge = g_voltageOffsetDischarge;
  cal.voltageOffsetRest = g_voltageOffsetRest;
  cal.fixedVoltage = g_fixedVoltage;     // NEW
  cal.mainsVoltage = g_mainsVoltage;     // NEW
  cal.valid = true;
  return cal;
}

void HardwareManager::applyCalibration(const CalibrationData& cal) {
  Serial.println("[HW] Applying calibration...");
  
  g_sct013CalIn = cal.sct013CalIn;
  g_sct013OffsetIn = cal.sct013OffsetIn;
  g_sct013CalOut = cal.sct013CalOut;
  g_sct013OffsetOut = cal.sct013OffsetOut;
  g_batteryDividerRatio = cal.batteryDividerRatio;
  g_batteryAdcCalibration = cal.batteryAdcCalibration;
  g_voltageOffsetCharge = cal.voltageOffsetCharge;
  g_voltageOffsetDischarge = cal.voltageOffsetDischarge;
  g_voltageOffsetRest = cal.voltageOffsetRest;
  g_fixedVoltage = cal.fixedVoltage;     // NEW
  g_mainsVoltage = cal.mainsVoltage;     // NEW
  
  sctMain.current(PIN_SCT013_MAIN, g_sct013CalIn);
  sctOutput.current(PIN_SCT013_OUTPUT, g_sct013CalOut);
  
  Serial.println("[HW] Calibration applied successfully");
  Serial.println("     SCT013 Cal In: " + String(g_sct013CalIn, 2));
  Serial.println("     Voltage Offset Rest: " + String(g_voltageOffsetRest, 2));
  Serial.println("     Fixed Voltage: " + String(g_fixedVoltage, 1) + "V");
  Serial.println("     Mains Voltage: " + String(g_mainsVoltage, 1) + "V");
}

void HardwareManager::saveCalibration() {
  Serial.println("[HW] Saving calibration to SPIFFS...");
  
  CalibrationData cal = getCalibrationData();
  saveCalibrationToSPIFFS(cal);
}
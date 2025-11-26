/*
 * Hardware Manager Implementation - DIAGNOSTICS AND DEBUG
 */

#include "hardware_manager.h"
#include "logger.h"

// Helper function to safely check if time has elapsed (handles millis() overflow)
static bool timeElapsed(unsigned long startTime, unsigned long interval) {
  return (millis() - startTime) >= interval;
}

void HardwareManager::printStatusHeader() {
  Serial.println("\n╔═══════════════════════════════════════════════════════════════════════════════╗");
  Serial.println("║ Tensione │  SOC   │ Ah rem │ Stato    │ P-IN │ P-OUT │ Diff │ Grafico       ║");
  Serial.println("╠═══════════════════════════════════════════════════════════════════════════════╣");
}

void HardwareManager::printBatteryBar(float percent) {
  Serial.print("[");
  int bars = (int)(percent / 5);
  for(int i = 0; i < 20; i++) {
    if(i < bars) {
      if(percent >= 60) Serial.print("█");
      else if(percent >= 20) Serial.print("▓");
      else Serial.print("░");
    } else {
      Serial.print("░");
    }
  }
  Serial.print("]");
}

void HardwareManager::printStatusLine() {
  Serial.print("║  ");
  Serial.print(currentData.batteryVoltage, 2);
  Serial.print("V │ ");
  
  if(currentState == STATE_CHARGING) {
    Serial.print("~");
    if(lastValidSOC < 100) Serial.print(" ");
    if(lastValidSOC < 10) Serial.print(" ");
    Serial.print(lastValidSOC, 1);
  } else {
    Serial.print(" ");
    if(currentData.batteryPercentage < 100) Serial.print(" ");
    if(currentData.batteryPercentage < 10) Serial.print(" ");
    Serial.print(currentData.batteryPercentage, 1);
  }
  Serial.print("% │ ");
  
  float ahRemaining = getEstimatedAh(currentData.batteryPercentage);
  if(ahRemaining < 100) Serial.print(" ");
  if(ahRemaining < 10) Serial.print(" ");
  Serial.print(ahRemaining, 1);
  Serial.print("Ah │ ");
  
  String stateStr = getStateString(currentState);
  Serial.print(stateStr);
  for(int i = stateStr.length(); i < 9; i++) Serial.print(" ");
  Serial.print("│ ");
  
  if(currentData.mainPower < 1000) Serial.print(" ");
  if(currentData.mainPower < 100) Serial.print(" ");
  Serial.print(currentData.mainPower, 0);
  Serial.print("W │ ");
  
  if(currentData.outputPower < 1000) Serial.print(" ");
  if(currentData.outputPower < 100) Serial.print(" ");
  Serial.print(currentData.outputPower, 0);
  Serial.print("W │ ");
  
  float powerDiff = currentData.mainPower - currentData.outputPower;
  if(powerDiff >= 0) Serial.print("+");
  else Serial.print("-");
  if(abs(powerDiff) < 100) Serial.print(" ");
  if(abs(powerDiff) < 10) Serial.print(" ");
  Serial.print(abs(powerDiff), 0);
  Serial.print("W │ ");
  
  printBatteryBar(currentData.batteryPercentage);
  Serial.println(" ║");
  
  // ===================================================================
  // EMERGENCY CONDITIONS ALERTS
  // ===================================================================
  if(isPowerStationOn) {
    if(currentData.batteryVoltage <= g_voltageMinSafe && currentState != STATE_CHARGING) {
      Serial.println("║  [!] CRITICAL VOLTAGE! BMS intervention imminent                             ║");
    } else if(currentData.batteryPercentage <= g_batteryCritical && currentState != STATE_CHARGING) {
      Serial.println("║  [!] CRITICAL BATTERY! Shutdown non-essential loads                          ║");
    } else if(currentData.batteryPercentage <= g_batteryLowWarning && currentState != STATE_CHARGING) {
      Serial.println("║  [!] Low battery - recharge soon                                             ║");
    }
  } else {
    Serial.println("║  [!] Power Station OFF - V < " + String(g_powerStationOffVoltage, 1) + "V                              ║");
  }
}

// ===================================================================
// EMERGENCY CONDITIONS MANAGEMENT (UPDATED WITH TIMERS!)
// ===================================================================

bool HardwareManager::getPowerStationOn() {
  return isPowerStationOn;
}

void HardwareManager::checkEmergencyConditions() {
  // Don't check emergency conditions during warmup
  if(!isWarmedUp) {
    return;
  }
  
  // Track power station state changes and trigger warmup
  bool wasPowerStationOn = isPowerStationOn;
  
  // Se Power Station è spenta (V < 20V), NON fare niente
  if(currentData.batteryVoltage < g_powerStationOffVoltage) {
    isPowerStationOn = false;
    
    // If power station just turned OFF, trigger warmup
    if(wasPowerStationOn && !isPowerStationOn) {
      Serial.println("[HW] Power Station turned OFF - triggering warmup period");
      isWarmedUp = false;
      warmupStartTime = millis();
    }
    
    // Reset all counters and flags
    voltageMinSafeCounter = 0;
    batteryLowWarningCounter = 0;
    batteryCriticalCounter = 0;
    powerStationOffVoltageCounter = 0;
    lowBatteryAlertActive = false;
    criticalBatteryAlertActive = false;
    lastLowBatteryAlertTime = 0;
    lastCriticalBatteryAlertTime = 0;
    
    return;  // EXIT: Non processiamo allarmi se PS è OFF
  }
  
  // If power station just turned ON, trigger warmup
  if(!wasPowerStationOn && currentData.batteryVoltage >= g_powerStationOffVoltage) {
    Serial.println("[HW] Power Station turned ON - triggering warmup period");
    isWarmedUp = false;
    warmupStartTime = millis();
  }
  
  isPowerStationOn = true;
  
  // ===================================================================
  // Se in carica, resetta tutti gli allarmi
  // ===================================================================
  if(currentState == STATE_CHARGING) {
    if(lowBatteryAlertActive || criticalBatteryAlertActive) {
      Serial.println("[HW] Battery charging - resetting all alerts");
      lowBatteryAlertActive = false;
      criticalBatteryAlertActive = false;
      lastLowBatteryAlertTime = 0;
      lastCriticalBatteryAlertTime = 0;
      voltageMinSafeCounter = 0;
      batteryLowWarningCounter = 0;
      batteryCriticalCounter = 0;
    }
    return;
  }
  
  // ===================================================================
  // Se in bypass, ignora tutti gli eventi basati sulla percentuale
  // (le variazioni sono solo rumore, la batteria è carica)
  // ===================================================================
  if(currentState == STATE_BYPASS) {
    // Reset counters ma non resettare gli allarmi attivi (potrebbero essere validi)
    // Solo impedire che nuovi eventi vengano triggerati
    voltageMinSafeCounter = 0;
    batteryLowWarningCounter = 0;
    batteryCriticalCounter = 0;
    return;
  }
  
  unsigned long currentTime = millis();
  
  // ===================================================================
  // LIVELLO 1: VOLTAGE_MIN_SAFE (< 23.5V per 5 cicli)
  // Azione: UPS Shutdown + 5 beep (IMMEDIATO, non periodico)
  // ===================================================================
  if(currentData.batteryVoltage < g_voltageMinSafe) {
    voltageMinSafeCounter++;
    
    if(voltageMinSafeCounter >= 5) {
      LOG_ERROR("Battery voltage critically low: " + String(currentData.batteryVoltage, 2) + "V");
      LOG_ERROR("Sending UPS shutdown signal...");
      emergencyShutdownUPS();
      triggerBeepAlert(5);
      voltageMinSafeCounter = 0;  // Reset dopo azione
    }
  } else {
    voltageMinSafeCounter = 0;
  }
  
  // ===================================================================
  // LIVELLO 2: BATTERY_LOW_WARNING (< 20% per 5 cicli)
  // Azione: 5 beep ogni 5 minuti + UPS Shutdown ad ogni ciclo
  // ===================================================================
  if(currentData.batteryPercentage < g_batteryLowWarning) {
    batteryLowWarningCounter++;
    
    // Attiva allarme Low Battery dopo 5 cicli consecutivi
    if(batteryLowWarningCounter >= 5 && !lowBatteryAlertActive) {
      LOG_WARNING("Low Battery Warning activated: " + String(currentData.batteryPercentage, 1) + "%");
      lowBatteryAlertActive = true;
      lastLowBatteryAlertTime = currentTime;
      
      // Primo allarme: 5 beep + shutdown
      Serial.println("[WARNING] Sending UPS shutdown signal...");
      emergencyShutdownUPS();
      triggerBeepAlert(5);
    }
    
    // Se allarme attivo, ripeti ogni 5 minuti (300000 ms)
    if(lowBatteryAlertActive && timeElapsed(lastLowBatteryAlertTime, 300000)) {
      Serial.println("[WARNING] Low Battery periodic alert: " + String(currentData.batteryPercentage, 1) + "%");
      Serial.println("[WARNING] Sending UPS shutdown signal...");
      emergencyShutdownUPS();
      triggerBeepAlert(5);
      lastLowBatteryAlertTime = currentTime;
    }
    
  } else {
    // Batteria sopra 20% - disattiva allarme Low Battery
    if(lowBatteryAlertActive) {
      Serial.println("[WARNING] Battery recovered above " + String(g_batteryLowWarning, 1) + "% - Low Battery alert deactivated");
      lowBatteryAlertActive = false;
      lastLowBatteryAlertTime = 0;
    }
    batteryLowWarningCounter = 0;
  }
  
  // ===================================================================
  // LIVELLO 3: BATTERY_CRITICAL (< 10% per 3 cicli)
  // Azione: 10 beep ogni 1 minuto fino a scarica totale o ripristino
  // ===================================================================
  if(currentData.batteryPercentage < g_batteryCritical) {
    batteryCriticalCounter++;
    
    // Attiva allarme Critical Battery dopo 3 cicli consecutivi
    if(batteryCriticalCounter >= 3 && !criticalBatteryAlertActive) {
      Serial.println("[CRITICAL] Critical Battery Level activated: " + String(currentData.batteryPercentage, 1) + "%");
      Serial.println("[CRITICAL] BMS intervention imminent!");
      criticalBatteryAlertActive = true;
      lastCriticalBatteryAlertTime = currentTime;
      
      // Disattiva Low Battery alert (Critical ha priorità)
      lowBatteryAlertActive = false;
      
      // Primo allarme: 10 beep
      triggerBeepAlert(10);
    }
    
    // Se allarme attivo, ripeti ogni 1 minuto (60000 ms)
    if(criticalBatteryAlertActive && timeElapsed(lastCriticalBatteryAlertTime, 60000)) {
      Serial.println("[CRITICAL] Critical Battery periodic alert: " + String(currentData.batteryPercentage, 1) + "%");
      triggerBeepAlert(10);
      lastCriticalBatteryAlertTime = currentTime;
    }
    
  } else {
    // Batteria sopra 10% - disattiva allarme Critical Battery
    if(criticalBatteryAlertActive) {
      Serial.println("[CRITICAL] Battery recovered above " + String(g_batteryCritical, 1) + "% - Critical Battery alert deactivated");
      criticalBatteryAlertActive = false;
      lastCriticalBatteryAlertTime = 0;
      
      // Riattiva Low Battery alert se ancora sotto 20%
      if(currentData.batteryPercentage < g_batteryLowWarning) {
        lowBatteryAlertActive = true;
        lastLowBatteryAlertTime = currentTime;
        Serial.println("[WARNING] Low Battery alert reactivated");
      }
    }
    batteryCriticalCounter = 0;
  }
}

void HardwareManager::triggerBeepAlert(int pulses) {
  // Respect global beepsEnabled setting
  if (!g_beepsEnabled) {
    Serial.println("[BEEP] Beep alerts disabled - skipping (" + String(pulses) + ")");
    return;
  }

  Serial.println("[BEEP] Triggering alert with " + String(pulses) + " beeps");
  
  isBeeping = true;
  beepCount = 0;
  totalBeepsNeeded = pulses;
  lastBeepTime = millis();
}

void HardwareManager::updateBeepState() {
  if(!isBeeping) return;
  
  unsigned long now = millis();
  
  // Beep pattern: 0.5s ON (press), 0.5s OFF (release)
  const unsigned long beepDuration = 500;  // 0.5s
  
  if(timeElapsed(lastBeepTime, beepDuration)) {
    beepCount++;
    lastBeepTime = now;
    
    // Simula pressione breve del pulsante Power (0.5s per beep)
    if(beepCount % 2 == 1) {
      // Beepo ON
      pressBeepButton();
    } else {
      // Beepo OFF (implicit - pulsante rilasciato)
    }
    
    if(beepCount >= (totalBeepsNeeded * 2)) {
      isBeeping = false;
      beepCount = 0;
      totalBeepsNeeded = 0;
      Serial.println("[BEEP] Alert sequence complete");
    }
  }
}

void HardwareManager::emergencyShutdownUPS() {
  // Emergency shutdown: UPS protocol handles shutdown via updateStatus()
  // The shutdown is triggered automatically when battery reaches critical level
  if(webServerRef != nullptr) {
    Serial.println("[UPS] Emergency shutdown command queued");
  }
}

bool HardwareManager::runSelfTest() {
  Serial.println("[HW] Running self-test...");
  
  float voltage = readBatteryVoltageRaw();
  if(voltage < BATTERY_VMIN || voltage > BATTERY_VMAX) {
    Serial.println("[HW] Self-test FAILED: Battery voltage out of range");
    return false;
  }
  
  double testIN = sctMain.calcIrms(SCT013_SAMPLES);
  double testOUT = sctOutput.calcIrms(SCT013_SAMPLES);
  
  Serial.println("[HW] Self-test results:");
  Serial.println("  Battery voltage: " + String(voltage, 2) + "V");
  Serial.println("  Current IN: " + String(testIN, 2) + "A");
  Serial.println("  Current OUT: " + String(testOUT, 2) + "A");
  Serial.println("[HW] Self-test passed");
  
  return true;
}

void HardwareManager::printDiagnostics() {
  Serial.println("\n=== HARDWARE DIAGNOSTICS ===");
  Serial.println("Battery:");
  Serial.println("  Voltage: " + String(currentData.batteryVoltage, 2) + "V");
  Serial.println("  SOC: " + String(currentData.batteryPercentage, 1) + "%");
  Serial.println("  Ah Remaining: " + String(getEstimatedAh(currentData.batteryPercentage), 1) + "Ah");
  Serial.println("  State: " + getStateString(currentState));
  Serial.println("\nPower:");
  Serial.println("  Main IN: " + String(currentData.mainPower, 0) + "W (" + String(currentData.mainCurrent, 2) + "A)");
  Serial.println("  Output: " + String(currentData.outputPower, 0) + "W (" + String(currentData.outputCurrent, 2) + "A)");
  Serial.println("  Net: " + String(currentData.mainPower - currentData.outputPower, 0) + "W");
  Serial.println("\nStatus:");
  Serial.println("  Power Station: " + String(isPowerStationOn ? "ON" : "OFF"));
  Serial.println("  On Battery: " + String(currentData.onBattery ? "YES" : "NO"));
  Serial.println("  Warm-up: " + String(isWarmedUp ? "Complete" : "In Progress"));
  Serial.println("  Auto Power On: " + String(autoPowerOnEnabled ? "ENABLED" : "DISABLED"));
  Serial.println("\nEmergency Alerts:");
  Serial.println("  Low Battery Alert (20%): " + String(lowBatteryAlertActive ? "ACTIVE" : "INACTIVE"));
  Serial.println("  Critical Battery Alert (10%): " + String(criticalBatteryAlertActive ? "ACTIVE" : "INACTIVE"));
  if(lowBatteryAlertActive) {
    unsigned long timeSinceLastAlert = (millis() - lastLowBatteryAlertTime) / 1000;
    Serial.println("  Time since last Low Battery alert: " + String(timeSinceLastAlert) + "s (next in " + String(300 - timeSinceLastAlert) + "s)");
  }
  if(criticalBatteryAlertActive) {
    unsigned long timeSinceLastAlert = (millis() - lastCriticalBatteryAlertTime) / 1000;
    Serial.println("  Time since last Critical alert: " + String(timeSinceLastAlert) + "s (next in " + String(60 - timeSinceLastAlert) + "s)");
  }
  Serial.println("\nEmergency Counters:");
  Serial.println("  Voltage Min Safe Counter: " + String(voltageMinSafeCounter) + "/5");
  Serial.println("  Battery Low Warning Counter: " + String(batteryLowWarningCounter) + "/5");
  Serial.println("  Battery Critical Counter: " + String(batteryCriticalCounter) + "/3");
  Serial.println("\nCalibration:");
  Serial.println("  SCT013 Cal In: " + String(g_sct013CalIn, 2));
  Serial.println("  SCT013 Cal Out: " + String(g_sct013CalOut, 2));
  Serial.println("  Battery Divider Ratio: " + String(g_batteryDividerRatio, 3));
  Serial.println("  Voltage Offset Rest: " + String(g_voltageOffsetRest, 2));
  Serial.println("\nAdvanced Settings:");
  Serial.println("  Power Station OFF Voltage: " + String(g_powerStationOffVoltage, 1) + "V");
  Serial.println("  Voltage Min Safe: " + String(g_voltageMinSafe, 1) + "V");
  Serial.println("  Battery Low Warning: " + String(g_batteryLowWarning, 1) + "%");
  Serial.println("  Battery Critical: " + String(g_batteryCritical, 1) + "%");
  Serial.println("============================\n");
}
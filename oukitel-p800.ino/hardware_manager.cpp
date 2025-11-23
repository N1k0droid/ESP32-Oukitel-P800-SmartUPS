/*
 * Hardware Manager Implementation - CORE
 */


#include "hardware_manager.h"
#include "web_server.h"
#include "logger.h"
#include <SPIFFS.h>


HardwareManager::HardwareManager() {
  buttonPins[BTN_POWER] = PIN_BUTTON_POWER;
  buttonPins[BTN_USB] = PIN_BUTTON_USB;
  buttonPins[BTN_DC] = PIN_BUTTON_DC;
  buttonPins[BTN_FLASHLIGHT] = PIN_BUTTON_FLASHLIGHT;
  buttonPins[BTN_AC] = PIN_BUTTON_AC;
  
  currentData.mainCurrent = 0;
  currentData.outputCurrent = 0;
  currentData.batteryVoltage = 0;
  currentData.batteryPercentage = 0;
  currentData.mainPower = 0;
  currentData.outputPower = 0;
  currentData.onBattery = false;
  currentData.batteryState = STATE_REST;
  currentData.timestamp = 0;
  
  isWarmedUp = false;
  warmupCounter = 0;
  warmupStartTime = 0;
  
  socBufferIndex = 0;
  socBufferFull = false;
  displayedSOC = 0;
  for(int i = 0; i < SOC_BUFFER_SIZE; i++) {
    socBuffer[i] = 0;
  }
  
  filteredPowerIN = 0;
  filteredPowerOUT = 0;
  powerFilterInitialized = false;
  
  lastValidPowerIN = 0;
  lastValidPowerOUT = 0;
  invalidReadingsCount = 0;
  
  currentState = STATE_REST;
  previousState = STATE_REST;
  lastValidSOC = 0;
  
  autoPowerOnEnabled = false;
  powerStationWasOff = true;
  powerOnTime = 0;
  acAlreadyActivated = false;
  
  webServerRef = nullptr;
  
  // Initialize emergency counters
  voltageMinSafeCounter = 0;
  batteryLowWarningCounter = 0;
  batteryCriticalCounter = 0;
  powerStationOffVoltageCounter = 0;
  
  // Initialize timer tracking (NEW!)
  lastLowBatteryAlertTime = 0;
  lastCriticalBatteryAlertTime = 0;
  lowBatteryAlertActive = false;
  criticalBatteryAlertActive = false;
  
  // Initialize beep state
  isBeeping = false;
  beepCount = 0;
  totalBeepsNeeded = 0;
  lastBeepTime = 0;
  
  isPowerStationOn = false;
  
  // Initialize non-blocking button state
  buttonActive = false;
  activeButtonIndex = -1;
  buttonPressStartTime = 0;
  buttonPressDuration = 0;
  flashlightAlertActive = false;
  flashlightPulseCount = 0;
  lastFlashlightToggle = 0;
}


bool HardwareManager::begin() {
  Serial.println("[HW] Initializing hardware manager...");
  
  analogSetPinAttenuation(PIN_BATTERY_VOLTAGE, ADC_11db);
  analogSetPinAttenuation(PIN_SCT013_MAIN, ADC_11db);
  analogSetPinAttenuation(PIN_SCT013_OUTPUT, ADC_11db);
  
  sctMain.current(PIN_SCT013_MAIN, g_sct013CalIn);
  sctOutput.current(PIN_SCT013_OUTPUT, g_sct013CalOut);
  
  for(int i = 0; i < 5; i++) {
    pinMode(buttonPins[i], OUTPUT);
    digitalWrite(buttonPins[i], LOW);
  }
  
  Serial.println("[HW] Button mapping:");
  Serial.println("  POWER (3s):     GPIO" + String(PIN_BUTTON_POWER));
  Serial.println("  USB Output:     GPIO" + String(PIN_BUTTON_USB));
  Serial.println("  DC Output:      GPIO" + String(PIN_BUTTON_DC));
  Serial.println("  Flashlight:     GPIO" + String(PIN_BUTTON_FLASHLIGHT));
  Serial.println("  AC Output:      GPIO" + String(PIN_BUTTON_AC));
  
  Serial.println("[HW] Current sensors initialized:");
  Serial.println("  SCT013 Main   (PIN " + String(PIN_SCT013_MAIN) + "): " + String(g_sct013CalIn, 2));
  Serial.println("  SCT013 Output (PIN " + String(PIN_SCT013_OUTPUT) + "): " + String(g_sct013CalOut, 2));
  Serial.println("[HW] Battery voltage divider configured (PIN " + String(PIN_BATTERY_VOLTAGE) + ")");
  Serial.println("[HW] Starting sensor warm-up phase (" + String(g_warmupDelay) + "ms)...");
  
  warmupStartTime = millis();
  
  loadAutoPowerOnState();
  
  return true;
}


void HardwareManager::setWebServerReference(WebServerManager* webServer) {
  webServerRef = webServer;
  Serial.println("[HW] WebServer reference set");
}


SensorData HardwareManager::getSensorData() {
  return currentData;
}


void HardwareManager::readSensors() {
  // Check if warmup period has elapsed
  if(!isWarmedUp) {
    unsigned long elapsedTime = millis() - warmupStartTime;
    if(elapsedTime >= g_warmupDelay) {
      isWarmedUp = true;
      Serial.println("[HW] Sensor warm-up complete (" + String(elapsedTime) + "ms) - readings now valid");
    } else {
      // During warmup, read sensors but don't use the data
      double dummyIN = sctMain.calcIrms(500);
      yield();
      double dummyOUT = sctOutput.calcIrms(500);
      yield();
      return;
    }
  }
  
  yield();
  double IrmsIN = sctMain.calcIrms(SCT013_SAMPLES) - g_sct013OffsetIn;
  yield();
  double IrmsOUT = sctOutput.calcIrms(SCT013_SAMPLES) - g_sct013OffsetOut;
  yield();
  
  if(IrmsIN < 0.05) IrmsIN = 0;
  if(IrmsOUT < 0.05) IrmsOUT = 0;
  
  // Use configurable mains voltage instead of constant
  float rawPowerIN = IrmsIN * g_mainsVoltage;
  float rawPowerOUT = IrmsOUT * g_mainsVoltage;
  
  initializePowerFilters(rawPowerIN, rawPowerOUT);
  
  float powerIN = filterPowerIN(rawPowerIN) + POWER_IN_OFFSET;
  float powerOUT = filterPowerOUT(rawPowerOUT) + POWER_OUT_OFFSET;
  
  if(!validatePowerReadings(powerIN, powerOUT)) {
    powerIN = lastValidPowerIN;
    powerOUT = lastValidPowerOUT;
    invalidReadingsCount++;
    
    if(invalidReadingsCount % 10 == 0) {
      Serial.println("[HW] Warning: Invalid power readings detected");
    }
  } else {
    lastValidPowerIN = powerIN;
    lastValidPowerOUT = powerOUT;
  }
  
  currentState = detectState(powerIN, powerOUT);
  
  float voltage = readBatteryVoltageRaw();
  float rawSOC = voltageToBatteryPercent(voltage);
  float soc = getStableSOC(rawSOC);
  

  lastValidSOC = soc;
  
  currentData.mainCurrent = IrmsIN;
  currentData.outputCurrent = IrmsOUT;
  currentData.batteryVoltage = voltage;
  currentData.batteryPercentage = soc;
  currentData.mainPower = powerIN;
  currentData.outputPower = powerOUT;
  currentData.onBattery = (currentState == STATE_DISCHARGING);
  currentData.batteryState = currentState;
  currentData.timestamp = millis();
}


void HardwareManager::checkStateTransition() {
  // Don't process state transitions during warmup
  if(!isWarmedUp) {
    return;
  }
  
  if(currentState != previousState) {
    Serial.println("[HW] State transition: " + getStateString(previousState) + " -> " + getStateString(currentState));
    
    // ===================================================================
    // STATE TRANSITION BEEP ALERTS
    // ===================================================================
    
    // 2 beeps: When switching to battery power (charge/rest/bypass → discharge)
    if((previousState == STATE_CHARGING || previousState == STATE_REST || previousState == STATE_BYPASS) && currentState == STATE_DISCHARGING) {
      Serial.println("[HW] Power lost - switching to battery power (2 beeps)");
      triggerBeepAlert(2);
    }
    
    // 1 beep: When returning to electric power (discharge → charge/bypass)
    if(previousState == STATE_DISCHARGING && (currentState == STATE_CHARGING || currentState == STATE_BYPASS)) {
      Serial.println("[HW] Power restored - returning to electric power (2 beep)");
      triggerBeepAlert(2);
    }
    
    previousState = currentState;
  }
}


String HardwareManager::getStateString(BatteryState state) {
  switch(state) {
    case STATE_CHARGING: return "CHARGE";
    case STATE_DISCHARGING: return "DISCHARGE";
    case STATE_BYPASS: return "BYPASS";
    case STATE_REST: return "REST";
    default: return "UNKNOWN";
  }
}


float HardwareManager::getEstimatedAh(float percent) {
  return (percent / 100.0) * BATTERY_CAPACITY_AH;
}


// ===================================================================
// Getter pubblico per stato warm-up
// ===================================================================
bool HardwareManager::getIsWarmedUp() {
  return isWarmedUp;
}
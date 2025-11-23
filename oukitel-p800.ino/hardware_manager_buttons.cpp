/*
 * Hardware Manager Implementation - BUTTONS AND AUTO POWER ON
 */

#include "hardware_manager.h"
#include "web_server.h"
#include "logger.h"
#include <SPIFFS.h>

bool HardwareManager::pressButton(int buttonIndex, int duration) {
  if(buttonIndex < 0 || buttonIndex >= 5) {
    Serial.println("[HW] Invalid button index: " + String(buttonIndex));
    return false;
  }
  
  // If a button is already active, reject new request
  if(buttonActive) {
    Serial.println("[HW] Button press already in progress, ignoring request");
    return false;
  }
  
  Serial.println("[HW] Starting non-blocking button press: " + String(buttonIndex) + " for " + String(duration) + "ms");
  
  // Start non-blocking button press
  buttonActive = true;
  activeButtonIndex = buttonIndex;
  buttonPressStartTime = millis();
  buttonPressDuration = duration;
  
  // Set button HIGH immediately
  digitalWrite(buttonPins[buttonIndex], HIGH);
  
  return true;
}

bool HardwareManager::pressPowerButton() {
  Serial.println("[HW] Pressing POWER button (3 seconds)");
  return pressButton(BTN_POWER, BUTTON_POWER_DURATION);
}

bool HardwareManager::pressBeepButton() {
  // Short press for beep - 0.5 seconds (500ms)
  // Use non-blocking pressButton instead of delay
  return pressButton(BTN_POWER, 500);
}

bool HardwareManager::pressUSBButton() {
  Serial.println("[HW] Pressing USB button");
  return pressButton(BTN_USB, BUTTON_STANDARD_DURATION);
}

bool HardwareManager::pressDCButton() {
  Serial.println("[HW] Pressing DC button");
  return pressButton(BTN_DC, BUTTON_STANDARD_DURATION);
}

bool HardwareManager::pressFlashlightButton() {
  return pressButton(BTN_FLASHLIGHT, BUTTON_STANDARD_DURATION);
}

bool HardwareManager::pressACButton() {
  Serial.println("[HW] Pressing AC button");
  return pressButton(BTN_AC, BUTTON_STANDARD_DURATION);
}

void HardwareManager::flashlightAlert() {
  Serial.println("[HW] Starting non-blocking flashlight alert: " + String(FLASHLIGHT_ALERT_PULSES) + " pulses");
  
  // Start non-blocking flashlight alert
  flashlightAlertActive = true;
  flashlightPulseCount = 0;
  lastFlashlightToggle = millis();
  
  // Start first pulse
  digitalWrite(buttonPins[BTN_FLASHLIGHT], HIGH);
}

void HardwareManager::loadAutoPowerOnState() {
  if(!SPIFFS.exists(AUTO_POWER_ON_FILE)) {
    autoPowerOnEnabled = false;
    Serial.println("[HW] Auto Power On: DISABLED (default)");
    return;
  }
  
  File file = SPIFFS.open(AUTO_POWER_ON_FILE, "r");
  if(!file) {
    LOG_ERROR("Hardware: Failed to open Auto Power On file");
    LOG_DEBUG("Hardware: Using default: DISABLED");
    autoPowerOnEnabled = false;
    return;
  }
  
  String state = file.readStringUntil('\n');
  file.close();
  state.trim();
  autoPowerOnEnabled = (state == "1");
  LOG_DEBUG("Hardware: Auto Power On loaded: " + String(autoPowerOnEnabled ? "ENABLED" : "DISABLED"));
}

void HardwareManager::saveAutoPowerOnState() {
  // Check SPIFFS space
  size_t freeBytes = SPIFFS.totalBytes() - SPIFFS.usedBytes();
  if (freeBytes < 64) {
    LOG_ERROR("Hardware: Insufficient SPIFFS space to save Auto Power On state");
    return;
  }
  
  File file = SPIFFS.open(AUTO_POWER_ON_FILE, "w");
  if(!file) {
    LOG_ERROR("Hardware: Failed to save Auto Power On state to SPIFFS");
    return;
  }
  
  file.println(autoPowerOnEnabled ? "1" : "0");
  file.close();
  Serial.println("[HW] Auto Power On saved: " + String(autoPowerOnEnabled ? "ENABLED" : "DISABLED"));
}

void HardwareManager::setAutoPowerOn(bool enabled) {
  autoPowerOnEnabled = enabled;
  saveAutoPowerOnState();
}

bool HardwareManager::getAutoPowerOn() {
  return autoPowerOnEnabled;
}

// Helper function to safely check if time has elapsed (handles millis() overflow)
static bool timeElapsed(unsigned long startTime, unsigned long interval) {
  return (millis() - startTime) >= interval;
}

void HardwareManager::updateButtonState() {
  unsigned long now = millis();
  
  // Handle active button press
  if(buttonActive) {
    if(timeElapsed(buttonPressStartTime, buttonPressDuration)) {
      // Button press duration completed, release button
      digitalWrite(buttonPins[activeButtonIndex], LOW);
      buttonActive = false;
      activeButtonIndex = -1;
      buttonPressStartTime = 0;
      buttonPressDuration = 0;
      Serial.println("[HW] Button press completed");
    }
  }
  
  // Handle flashlight alert
  if(flashlightAlertActive) {
    if(timeElapsed(lastFlashlightToggle, FLASHLIGHT_ALERT_INTERVAL)) {
      // Toggle flashlight
      bool currentState = digitalRead(buttonPins[BTN_FLASHLIGHT]);
      digitalWrite(buttonPins[BTN_FLASHLIGHT], !currentState);
      lastFlashlightToggle = now;
      
      if(!currentState) {
        // Button was LOW, now HIGH - increment pulse count
        flashlightPulseCount++;
        
        if(flashlightPulseCount >= FLASHLIGHT_ALERT_PULSES * 2) {
          // All pulses completed (HIGH and LOW for each pulse)
          flashlightAlertActive = false;
          flashlightPulseCount = 0;
          lastFlashlightToggle = 0;
          digitalWrite(buttonPins[BTN_FLASHLIGHT], LOW);
          Serial.println("[HW] Flashlight alert completed");
        }
      }
    }
  }
}

void HardwareManager::checkAutoPowerOn() {
  if(!autoPowerOnEnabled) {
    return;
  }
  
  bool isPowerOn = currentData.batteryVoltage >= 20.0;
  
  // Detect power station turning ON
  if(isPowerOn && powerStationWasOff) {
    powerOnTime = millis();
    powerStationWasOff = false;
    acAlreadyActivated = false;
    Serial.println("[HW] Power station turned ON - AC auto-activation will trigger in 5 seconds");
  }
  
  // Detect power station turning OFF
  if(!isPowerOn && !powerStationWasOff) {
    powerStationWasOff = true;
    Serial.println("[HW] Power station turned OFF");
  }
  
  // Auto-activate AC if conditions are met
  if(isPowerOn && !acAlreadyActivated && !powerStationWasOff) {
    if(timeElapsed(powerOnTime, 5000)) {
      Serial.println("[HW] Auto Power On: Activating AC output now!");
      pressACButton();
      acAlreadyActivated = true;
      
      // Notify WebServer to update UI
      if(webServerRef != nullptr) {
        webServerRef->notifyACActivated();
      }
    }
  }
}
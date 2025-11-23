/*
 * Hardware Manager - Handles all sensor readings and button controls
 */


#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H


#include "config.h"
#include "EmonLib.h"


// Forward declaration
class WebServerManager;


class HardwareManager {
private:
  EnergyMonitor sctMain;
  EnergyMonitor sctOutput;


  int buttonPins[5];


  SensorData currentData;


  // Warm-up state
  bool isWarmedUp;
  int warmupCounter;
  unsigned long warmupStartTime;


  // SOC filtering
  float socBuffer[SOC_BUFFER_SIZE];
  int socBufferIndex;
  bool socBufferFull;
  float displayedSOC;


  // Power filtering
  float filteredPowerIN;
  float filteredPowerOUT;
  bool powerFilterInitialized;


  // Validation
  float lastValidPowerIN;
  float lastValidPowerOUT;
  int invalidReadingsCount;


  // Battery state tracking
  BatteryState currentState;
  BatteryState previousState;
  float lastValidSOC;


  // Auto Power On
  bool autoPowerOnEnabled;
  bool powerStationWasOff;
  unsigned long powerOnTime;
  bool acAlreadyActivated;


  // Emergency conditions tracking (UPDATED!)
  int voltageMinSafeCounter;          // Conta cicli < voltageMinSafe
  int batteryLowWarningCounter;       // Conta cicli < batteryLowWarning%
  int batteryCriticalCounter;         // Conta cicli < batteryCritical%
  int powerStationOffVoltageCounter;  // Conta cicli < powerStationOffVoltage
  
  // Timer tracking for periodic alerts
  unsigned long lastLowBatteryAlertTime;     // Ultimo allarme Low Battery (20%)
  unsigned long lastCriticalBatteryAlertTime; // Ultimo allarme Critical Battery (10%)
  bool lowBatteryAlertActive;                 // Flag per Low Battery attivo
  bool criticalBatteryAlertActive;            // Flag per Critical Battery attivo
  
  // Beep alert state
  bool isBeeping;
  int beepCount;
  int totalBeepsNeeded;
  unsigned long lastBeepTime;
  
  // Power station state
  bool isPowerStationOn;              // True se V >= powerStationOffVoltage
  
  // Non-blocking button state machine
  bool buttonActive;                  // True if a button press is in progress
  int activeButtonIndex;              // Index of button being pressed
  unsigned long buttonPressStartTime; // When button press started
  unsigned long buttonPressDuration;  // Total duration needed
  bool flashlightAlertActive;          // True if flashlight alert is active
  int flashlightPulseCount;            // Current pulse count for flashlight alert
  unsigned long lastFlashlightToggle; // Last time flashlight was toggled


  WebServerManager* webServerRef;


  // Private calibration storage
  CalibrationData calibration;
  AdvancedSettings advancedSettings;


  // Private helper methods for sensors, voltage & power filtering, validation etc.
  float readBatteryVoltageRaw();
  float compensateVoltageDischarge(float measuredV);
  float compensateVoltageCharge(float measuredV);
  float voltageToBatteryPercent(float voltage);


  BatteryState detectState(float powerIN, float powerOUT);


  void updateSOCBuffer(float newSOC);
  float getMedianSOC();
  float getStableSOC(float currentSOC);
  void sortArray(float arr[], int n);


  void initializePowerFilters(float powerIN, float powerOUT);
  float filterPowerIN(float rawPowerIN);
  float filterPowerOUT(float rawPowerOUT);


  bool validatePowerReadings(float powerIN, float powerOUT);


  void printBatteryBar(float percent);


  void loadAutoPowerOnState();
  void saveAutoPowerOnState();


  // Emergency conditions (PRIVATE HELPERS)
  void triggerBeepAlert(int pulses);  // Emette N beep (0.5s on/off)
  void emergencyShutdownUPS();        // Invia shutdown signal agli UPS
  
  // Beep button control
  bool pressBeepButton();             // Preme il pulsante POWER per 0.5s (per beep)


public:
  HardwareManager();
  bool begin();
  void readSensors();
  SensorData getSensorData();


  // Button control
  bool pressButton(int buttonIndex, int duration);
  bool pressPowerButton();
  bool pressUSBButton();
  bool pressDCButton();
  bool pressFlashlightButton();
  bool pressACButton();


  void flashlightAlert();


  void checkAutoPowerOn();
  void setAutoPowerOn(bool enabled);
  bool getAutoPowerOn();


  void setWebServerReference(WebServerManager* webServer);


  void checkStateTransition();


  bool runSelfTest();
  void printDiagnostics();


  void printStatusHeader();
  void printStatusLine();


  String getStateString(BatteryState state);
  float getEstimatedAh(float percent);


  // ===================================================================
  // WARM-UP STATUS
  // ===================================================================
  bool getIsWarmedUp();              // Ritorna true se warm-up completato


  // ===================================================================
  // POWER STATION STATE
  // ===================================================================
  bool getPowerStationOn();           // Ritorna true se V >= powerStationOffVoltage


  // ===================================================================
  // EMERGENCY CONDITIONS (Called from main loop)
  // ===================================================================
  void checkEmergencyConditions();    // Controlla le soglie critiche con timer
  void updateBeepState();             // Aggiorna stato beep
  void updateButtonState();           // Aggiorna stato pulsanti (non-blocking)


  // ===================================================================
  // CALIBRATION RELATED
  // ===================================================================
  void saveCalibration();
  CalibrationData getCalibrationData();
  void applyCalibration(const CalibrationData& calData);


  // ===================================================================
  // ADVANCED SETTINGS RELATED
  // ===================================================================
  AdvancedSettings getAdvancedSettings();
  void applyAdvancedSettings(const AdvancedSettings& settings);
  void saveAdvancedSettings();
};


#endif // HARDWARE_MANAGER_H
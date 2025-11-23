/*
 * WiFi Manager - Handles WiFi connection with fallback AP mode
 * Non-blocking implementation
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h"
#include <WiFi.h>
#include <DNSServer.h>

enum WiFiState {
  WIFI_STATE_IDLE,
  WIFI_STATE_CONNECTING,
  WIFI_STATE_CONNECTED,
  WIFI_STATE_AP_MODE,
  WIFI_STATE_WAITING_RETRY
};

class WiFiManager {
private:
  WiFiCredentials credentials;
  DNSServer dnsServer;
  
  WiFiState currentState;
  int retryCount;
  unsigned long lastStateChange;
  unsigned long apStartTime;           // When AP mode was started
  unsigned long lastApClientCheck;     // Last time we checked for AP clients
  bool hasDefaultCredentials;          // Flag indicating if using default credentials
  
  void startAccessPoint();
  void stopAccessPoint();
  void loadCredentials();

public:
  WiFiManager();
  bool begin();
  void handleConnection();
  bool isConnected();
  bool isAPMode();

  // Credential management
  bool setCredentials(const String& ssid, const String& password);
  WiFiCredentials getCredentials();
  void clearCredentials();

  // Make this public to allow persistence saving
  void saveCredentials();

  // Network info
  String getLocalIP();
  String getAPIP();
  String getSSID();
  int getRSSI();
  String getMACAddress();

  // Status methods
  String getConnectionStatus();
  void printNetworkInfo();
};

#endif // WIFI_MANAGER_H
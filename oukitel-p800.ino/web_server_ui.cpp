/*
 * Web Server Manager - UI HTML/JavaScript Generation
 */

#include "web_server.h"
#include "hardware_manager.h"
#include <ArduinoJson.h>

// Dichiarazioni extern per le variabili globali
extern HardwareManager hardware;

String WebServerManager::generateHTML() {
  // Pre-allocate String to reduce memory fragmentation
  // Estimated HTML size: ~15-20KB
  String html;
  html.reserve(20000);  // Pre-allocate 20KB to reduce fragmentation
  
  html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>";
  html += "<title>Oukitel P800E</title><style>";

  // CSS styles
  html += "*{margin:0;padding:0;box-sizing:border-box}";
  html += "body{font:14px Arial,sans-serif;background:#f0f0f0;color:#333;-webkit-tap-highlight-color:transparent}";
  html += ".container{max-width:900px;margin:0 auto;background:#fff;min-height:100vh}";
  html += "header{background:#07d;color:#fff;padding:15px 20px;display:flex;justify-content:space-between;align-items:center}";
  html += "header h1{font-size:18px;font-weight:normal}";
  html += ".status{font-size:11px;opacity:0.9}";
  html += "nav{background:#eee;border-bottom:2px solid #ddd;overflow-x:auto;white-space:nowrap;-webkit-overflow-scrolling:touch}";
  html += "nav button{background:none;border:none;padding:12px 15px;cursor:pointer;font-size:13px;color:#666;border-bottom:3px solid transparent;display:inline-block}";
  html += "nav button:hover,nav button:active{background:#e0e0e0}";
  html += "nav button.active{color:#07d;border-bottom-color:#07d;font-weight:bold}";
  html += ".tab-content{display:none;padding:15px}";
  html += ".tab-content.active{display:block}";
  html += ".section{background:#fafafa;border:1px solid #ddd;padding:15px;margin-bottom:15px;border-radius:3px}";
  html += ".section h3{font-size:15px;margin-bottom:10px;color:#07d;border-bottom:1px solid #ddd;padding-bottom:5px}";
  html += ".config-status{padding:10px;margin-bottom:10px;border-radius:3px;background:#f8f8f8;border:1px solid #ddd;text-align:center;font-weight:bold;color:#999}";
  html += ".config-status.configured{background:#d4edda;color:#155724;border-color:#c3e6cb}";
  html += "table{width:100%;border-collapse:collapse;margin:10px 0}";
  html += "table td{padding:8px;border-bottom:1px solid #eee}";
  html += "table td:first-child{font-weight:bold;width:45%;color:#666}";
  html += "input[type='text'],input[type='password'],input[type='number'],select{width:100%;padding:10px;border:1px solid #ddd;border-radius:3px;font-size:14px;-webkit-appearance:none}";
  html += "label{display:block;margin:10px 0 5px;font-weight:bold;color:#666;font-size:13px}";
  html += ".checkbox-label{display:flex;align-items:center;margin:10px 0;cursor:pointer;user-select:none}";
  html += ".checkbox-label input[type='checkbox']{width:auto;margin-right:8px;cursor:pointer}";
  html += ".btn{background:#07d;color:#fff;border:none;padding:12px 15px;border-radius:3px;cursor:pointer;font-size:14px;margin:5px 5px 5px 0;touch-action:manipulation;user-select:none;transition:opacity 0.2s}";
  html += ".btn:hover,.btn:active{background:#069}";
  html += ".btn:disabled{background:#ccc;color:#999;cursor:not-allowed;opacity:0.6}";
  html += ".btn:disabled:hover{background:#ccc}";

  // POWER button: dark green ON, dark red OFF
  html += ".btn-power-on{background:#2d7d2d;color:#fff}";
  html += ".btn-power-on:hover,.btn-power-on:active{background:#236623}";
  html += ".btn-power-off{background:#8b3a3a;color:#fff}";
  html += ".btn-power-off:hover,.btn-power-off:active{background:#6b2828}";

  // Other buttons: light green ON, light red OFF
  html += ".btn-output-on{background:#4CAF50;color:#fff}";
  html += ".btn-output-on:hover,.btn-output-on:active{background:#45a049}";
  html += ".btn-output-off{background:#e57373;color:#fff}";
  html += ".btn-output-off:hover,.btn-output-off:active{background:#d32f2f}";

  html += ".btn-group{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:10px;margin:10px 0}";
  html += ".value{color:#07d;font-weight:bold;font-size:16px}";
  html += ".subsection{margin-top:15px;padding-top:10px;border-top:1px dashed #ddd}";
  html += ".subsection-title{font-size:13px;font-weight:bold;color:#666;margin-bottom:8px}";
  html += ".alert{padding:10px;margin:10px 0;border-radius:3px;border-left:4px solid;font-size:13px}";
  html += ".alert-warning{background:#fff3cd;color:#856404;border-color:#ffc107}";
  html += ".alert-success{background:#d4edda;color:#155724;border-color:#28a745}";
  html += ".alert-info{background:#d1ecf1;color:#0c5460;border-color:#17a2b8}";
  html += ".alert-danger{background:#f8d7da;color:#721c24;border-color:#f5c6cb}";
  html += ".code-block{background:#f4f4f4;border:1px solid #ddd;border-radius:3px;padding:10px;margin:10px 0;font-family:monospace;font-size:12px;overflow-x:auto;white-space:pre-wrap;word-wrap:break-word}";
  html += ".info-section{margin-bottom:20px}";
  html += ".info-section h4{color:#07d;margin-bottom:8px;font-size:14px}";
  html += ".info-section p{margin-bottom:8px;line-height:1.5}";
  html += "@media (max-width:600px){";
  html += "header h1{font-size:16px}";
  html += ".btn-group{grid-template-columns:1fr}";
  html += ".tab-content{padding:10px}";
  html += ".section{padding:10px}";
  html += "}";

  html += "</style></head><body>";

  html += "<div class='container'>";
  html += "<header><h1>Oukitel P800E</h1><div class='status' id='wifiStatus'>Connecting...</div></header>";

  html += "<nav>";
  html += "<button class='active' onclick='showTab(0)'>Main</button>";
  html += "<button onclick='showTab(1)'>WiFi</button>";
  html += "<button onclick='showTab(2)'>Calibration</button>";
  html += "<button onclick='showTab(3)'>Advanced</button>";
  html += "<button onclick='showTab(4)'>Energy</button>";
  html += "<button onclick='showTab(5)'>MQTT</button>";
  html += "<button onclick='showTab(6)'>HTTP/HA</button>";
  html += "<button onclick='showTab(7)'>UPS</button>";
  html += "<button onclick='showTab(8)'>System</button>";
  html += "<button onclick='showTab(9)'>API Info</button>";
  html += "</nav>";

  // Tab 0 - Main Status
  html += "<div class='tab-content active' id='tab0'>";
  html += "<div class='section'>";
  html += "<h3>Battery Status</h3>";
  html += "<table><tr><td>Voltage</td><td><span class='value' id='voltage'>--</span> V</td></tr>";
  html += "<tr><td>Charge Level</td><td><span class='value' id='soc'>--</span> %</td></tr>";
  html += "<tr><td>State</td><td><span class='value' id='state'>--</span></td></tr></table>";
  html += "<div id='powerOffWarning' class='alert alert-warning' style='display:none'>Power Station is OFF. Press POWER button to turn on.</div>";
  html += "</div>";

  html += "<div class='section'>";
  html += "<h3>Power</h3>";
  html += "<table><tr><td>Input</td><td><span class='value' id='powerIn'>--</span> W</td></tr>";
  html += "<tr><td>Output</td><td><span class='value' id='powerOut'>--</span> W</td></tr>";
//  html += "<tr><td>Net</td><td><span class='value' id='powerNet'>--</span> W</td>";
  html += "<tr style='display:none'><td>Net</td><td><span class='value' id='powerNet'>--</span> W</td></tr>";
  html += "</tr></table>";
  html += "</div>";

  html += "<div class='section'>";
  html += "<h3>Output Controls</h3>";
  html += "<button class='btn btn-power-off' id='btnPower' onclick='confirmPower()' style='width:100%;margin-bottom:15px'>POWER (OFF)</button>";
  html += "<div class='btn-group'>";
  html += "<button class='btn btn-output-off' id='btnUsb' onclick='pressBtn(1)'>USB</button>";
  html += "<button class='btn btn-output-off' id='btnDc' onclick='pressBtn(2)'>DC</button>";
  html += "<button class='btn btn-output-off' id='btnFlash' onclick='pressBtn(3)'>Flash</button>";
  html += "<button class='btn btn-output-off' id='btnAc' onclick='pressBtn(4)'>AC</button>";
  html += "</div>";

  html += "<div class='subsection'>";
  html += "<div class='subsection-title'>Automatic AC Activation</div>";
  html += "<button class='btn btn-output-off' id='btnAutoPower' onclick='toggleAutoPower()' style='width:100%'>Auto Power On (OFF)</button>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>When enabled, AC output will activate automatically at power station startup.</p>";
  html += "</div>";
  html += "</div>";
  html += "</div>";

  // Tab 1 - WiFi Configuration
  html += "<div class='tab-content' id='tab1'>";
  html += "<div class='section'>";
  html += "<h3>WiFi Configuration</h3>";
  html += "<label>Network SSID</label><input type='text' id='wifiSsid' placeholder='Enter WiFi SSID'>";
  html += "<label>Password</label><input type='password' id='wifiPass' placeholder='Enter WiFi Password'>";
  html += "<button class='btn' style='width:100%;margin-top:10px' onclick='saveWifi()'>Save and Connect</button>";
  html += "<button class='btn' style='width:100%;margin-top:5px' onclick='scanWifi()'>Scan Networks</button>";
  html += "<div id='wifiScanResults' style='margin-top:15px;'></div>";
  html += "</div>";
  html += "</div>";

  // Tab 2 - Calibration
  html += "<div class='tab-content' id='tab2'>";
  html += "<div id='calStatus'></div>";
  html += "<div class='section'>";
  html += "<h3>SCT013 Current Sensors</h3>";
  html += "<label>Input Calibration (SCT013 Main)</label><input type='number' step='0.01' id='calMainCal' placeholder='Calibration value'>";
  html += "<label>Input Offset</label><input type='number' step='0.01' id='calMainOff' placeholder='Offset value'>";
  html += "<label>Output Calibration (SCT013 Output)</label><input type='number' step='0.01' id='calOutCal' placeholder='Calibration value'>";
  html += "<label>Output Offset</label><input type='number' step='0.01' id='calOutOff' placeholder='Offset value'>";
  html += "</div>";

  html += "<div class='section'>";
  html += "<h3>Battery Voltage Calibration</h3>";
  html += "<label>Divider Ratio</label><input type='number' step='0.001' id='calBattRatio' placeholder='Ratio value'>";
  html += "<label>ADC Calibration</label><input type='number' step='0.0001' id='calBattAdc' placeholder='ADC calibration value'>";
  html += "<label>Voltage Offset - Charging</label><input type='number' step='0.01' id='calOffCharge' placeholder='Charge offset'>";
  html += "<label>Voltage Offset - Discharging</label><input type='number' step='0.01' id='calOffDischarge' placeholder='Discharge offset'>";
  html += "<label>Voltage Offset - Rest</label><input type='number' step='0.01' id='calOffRest' placeholder='Rest offset'>";
  html += "</div>";

  html += "<div class='section'>";
  html += "<h3>TEST</h3>";
  html += "<label>Battery Voltage Override (0 = disabled)</label><input type='number' step='0.1' id='calFixedVoltage' value='0' placeholder='Leave 0 to disable'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Set a fixed voltage for testing. Leave 0 for normal calculation.</p>";
  // Mains voltage configuration used for IN/OUT power calculations
  html += "<label>Mains Voltage (V)</label><input type='number' step='0.1' id='calMainsVoltage' value='230' placeholder='Grid voltage for power calculations'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Used to convert current (A) to power (W) for IN and OUT. Default: 230V.</p>";
  html += "</div>";

  html += "<div class='section'>";
  html += "<div class='btn-group'>";
  html += "<button class='btn' onclick='resetCalToDefaults()' style='background:#17a2b8'>Reset to Defaults</button>";
  html += "<button class='btn' style='background:#28a745;flex:1' onclick='saveCal()'>Save to Device</button>";
  html += "</div>";
  html += "</div>";
  html += "</div>";

  // Tab 3 - Advanced Settings
  html += "<div class='tab-content' id='tab3'>";
  html += "<div id='advStatus'></div>";

  html += "<div class='section'>";
  html += "<h3>🔌 Power Station State</h3>";
  html += "<label>Power Station OFF Voltage (V)</label><input type='number' step='0.1' id='advPowerStationOffVoltage' placeholder='Voltage to consider PS OFF'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>When battery voltage drops below this value, the Power Station is considered OFF. No alarms will be sent below this threshold.</p>";
  html += "</div>";

  html += "<div class='section'>";
  html += "<h3>⚡ Power Management</h3>";
  html += "<label>Power Threshold (W)</label><input type='number' step='0.1' id='advPowerThreshold' placeholder='Minimum power to detect state'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Minimum power (Watts) to consider the system as actively charging/discharging.</p>";
  html += "<label>Power Filter Alpha</label><input type='number' step='0.01' min='0.1' max='0.9' id='advPowerFilterAlpha' placeholder='Filter responsiveness'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Higher = smoother readings but slower response (0.1-0.9).</p>";
  html += "</div>";

  html += "<div class='section'>";
  html += "<h3>🔋 Battery Thresholds</h3>";
  html += "<label>Critical Voltage (V)</label><input type='number' step='0.1' id='advVoltageMinSafe' placeholder='Critical voltage threshold'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>When voltage drops below this value for 5 cycles, system sends UPS shutdown signal (5 beeps alert).</p>";
  html += "<label>Low Battery Warning (%)</label><input type='number' step='1' min='0' max='100' id='advBatteryLowWarning' placeholder='Low battery percentage'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>When SOC drops below this value for 5 cycles, system sends UPS shutdown signal (5 beeps alert).</p>";
  html += "<label>Critical Battery Level (%)</label><input type='number' step='1' min='0' max='100' id='advBatteryCritical' placeholder='Critical percentage'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>When SOC drops below this value for 3 cycles, BMS intervention alarm is triggered (10 beeps alert).</p>";
  html += "</div>";

  html += "<div class='section'>";
  html += "<h3>⏱️ Timing Settings</h3>";
  html += "<label>Boot Delay (ms)</label><input type='number' step='100' min='0' id='advWarmupDelay' placeholder='Boot delay in milliseconds'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Wait time after power station boot before sensor readings affect system logic. Ignores all sensor data during this period. Default: 20000ms (20 seconds).</p>";
  html += "<label>Auto Power On Delay (ms)</label><input type='number' step='100' min='0' id='advAutoPowerOnDelay' placeholder='Delay in milliseconds'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Wait time after power station boot before automatically activating AC output.</p>";
  html += "</div>";

  html += "<div class='section'>";
  html += "<h3>🔄 SOC Smoothing</h3>";
  html += "<label>Buffer Size</label><input type='number' step='1' min='1' max='50' id='advSocBufferSize' placeholder='Number of samples'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Number of samples to smooth SOC readings. Higher = smoother but slower updates.</p>";
  html += "<label>Change Threshold</label><input type='number' step='1' min='1' max='10' id='advSocChangeThreshold' placeholder='Agreement count required'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Number of samples that must agree to trigger SOC change.</p>";
  html += "</div>";

  html += "<div class='section'>";
  html += "<div class='btn-group'>";
  html += "<button class='btn' onclick='resetAdvToDefaults()' style='background:#17a2b8'>Reset to Defaults</button>";
  html += "<button class='btn' style='background:#28a745;flex:1' onclick='saveAdv()'>Save to Device</button>";
  html += "</div>";
  html += "</div>";
  html += "</div>";

  // Tab 4 - Energy
  html += "<div class='tab-content' id='tab4'>";
  html += "<div class='section'>";
  html += "<h3>Current Power</h3>";
  html += "<table><tr><td>Instantaneous</td><td><span class='value' id='instantPower'>--</span> W</td></tr></table>";
  html += "</div>";
  html += "<div class='section'>";
  html += "<h3>Energy Consumption</h3>";
  html += "<table><tr><td>Daily</td><td><span class='value' id='dailyConsumption'>--</span> kWh</td></tr>";
  html += "<tr><td>Current Month</td><td><span class='value' id='monthCurrent'>--</span> kWh</td></tr>";
  html += "<tr><td>Annual Estimate</td><td><span class='value' id='yearEstimate'>--</span> kWh</td></tr></table>";
  html += "<button class='btn' style='width:100%;background:#dc3545' onclick='resetMonth()'>Reset Current Month</button>";
  html += "</div>";
  html += "<div class='section'>";
  html += "<h3>12-Month History</h3>";
  html += "<table id='monthHistory'><tr><td colspan='2'>Loading...</td></tr></table>";
  html += "</div>";
  html += "</div>";

  // Tab 5 - MQTT
  html += "<div class='tab-content' id='tab5'>";
  html += "<div class='config-status' id='mqttConfigStatus'>NOT CONFIGURED</div>";
  html += "<div id='mqttStatus'></div>";
  html += "<div class='section'>";
  html += "<h3>MQTT Configuration</h3>";
  html += "<label class='checkbox-label'><input type='checkbox' id='mqttEnabled'> Enable MQTT</label>";
  html += "<label>MQTT Server</label><input type='text' id='mqttServer' placeholder='mqtt.example.com'>";
  html += "<label>Port</label><input type='number' id='mqttPort' placeholder='1883' value='1883'>";
  html += "<label>Username</label><input type='text' id='mqttUsername' placeholder='Username (optional)'>";
  html += "<label>Password</label><input type='password' id='mqttPassword' placeholder='Password (optional)'>";
  html += "<label>Client ID</label><input type='text' id='mqttClientId' placeholder='Auto-generated'>";
  html += "<button class='btn' style='width:100%;margin-top:10px' onclick='saveMqttConfig()'>Save MQTT Configuration</button>";
  html += "<p style='font-size:11px;color:#666;margin-top:10px'>MQTT publishes all sensor data and supports Home Assistant auto-discovery. Device will reboot after saving.</p>";
  html += "</div>";
  html += "</div>";

  // Tab 6 - Home Assistant
  html += "<div class='tab-content' id='tab6'>";
  html += "<div class='config-status' id='haConfigStatus'>NOT CONFIGURED</div>";
  html += "<div id='haStatus'></div>";
  html += "<div class='section'>";
  html += "<h3>Home Assistant HTTP API</h3>";
  html += "<label class='checkbox-label'><input type='checkbox' id='haEnabled'> Enable Home Assistant</label>";
  html += "<label>Server URL</label><input type='text' id='haServer' placeholder='192.168.1.100'>";
  html += "<label>Port</label><input type='number' id='haPort' placeholder='8123' value='8123'>";
  html += "<label>API Token</label><input type='password' id='haToken' placeholder='Long-lived access token'>";
  html += "<label>Endpoint</label><input type='text' id='haEndpoint' placeholder='/api/states/sensor.oukitel_p800e' value='/api/states/sensor.oukitel_p800e'>";
  html += "<button class='btn' style='width:100%;margin-top:10px' onclick='saveHaConfig()'>Save HA Configuration</button>";
  html += "<p style='font-size:11px;color:#666;margin-top:10px'>Sends sensor data to Home Assistant via HTTP POST every 30 seconds. Device will reboot after saving.</p>";
  html += "</div>";
  
  // HTTP Shutdown Notification Section (NEW!)
  html += "<div class='section'>";
  html += "<h3>🔋 HTTP Shutdown Notification</h3>";
  html += "<div class='config-status' id='shutdownConfigStatus'>NOT CONFIGURED</div>";
  html += "<div id='shutdownStatus'></div>";
  html += "<label class='checkbox-label'><input type='checkbox' id='shutdownEnabled'> Enable Shutdown Notification</label>";
  html += "<label>Battery Threshold (%)</label><input type='number' step='1' min='0' max='100' id='shutdownThreshold' placeholder='15' value='15'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Send shutdown notification when battery drops below this percentage.</p>";
  html += "<label>Server Address</label><input type='text' id='shutdownServer' placeholder='192.168.1.100'>";
  html += "<label>Server Port</label><input type='number' id='shutdownPort' placeholder='8080' value='8080'>";
  html += "<label>Password</label><input type='password' id='shutdownPassword' placeholder='shutdown123'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Password included in shutdown notification payload for authentication.</p>";
  html += "<button class='btn' style='width:100%;margin-top:10px' onclick='saveShutdownConfig()'>Save Shutdown Configuration</button>";
  html += "<p style='font-size:11px;color:#666;margin-top:10px'>Sends HTTP POST to http://[server]:[port]/shutdown with battery data and password when threshold is reached.</p>";
  html += "</div>";
  html += "</div>";

  // Tab 7 - UPS
  html += "<div class='tab-content' id='tab7'>";
  html += "<div class='config-status' id='upsConfigStatus'>NOT CONFIGURED</div>";
  html += "<div id='upsStatus'></div>";
  html += "<div class='section'>";
  html += "<h3>UPS Protocol (NUT Compatible)</h3>";
  html += "<label class='checkbox-label'><input type='checkbox' id='upsEnabled'> Enable UPS Protocol</label>";
  html += "<label>Port</label><input type='number' id='upsPort' placeholder='3493' value='3493'>";
  html += "<label>Shutdown Threshold (%)</label><input type='number' id='upsThreshold' placeholder='10' value='10'>";
  html += "<button class='btn' style='width:100%;margin-top:10px' onclick='saveUpsConfig()'>Save UPS Configuration</button>";
  html += "<p style='font-size:11px;color:#666;margin-top:10px'>Compatible with Network UPS Tools (NUT) for Proxmox and Linux systems. Device will reboot after saving.</p>";
  html += "</div>";
  html += "</div>";

  // Tab 8 - System
  html += "<div class='tab-content' id='tab8'>";
  html += "<div class='section'>";
  html += "<h3>System Information</h3>";
  html += "<table><tr><td>Firmware</td><td id='fwVersion'>v1.1.0</td></tr>";
  html += "<tr><td>IP Address</td><td id='ipAddr'>--</td></tr>";
  html += "<tr><td>MAC Address</td><td id='macAddr'>--</td></tr>";
  html += "<tr><td>SSID</td><td id='ssid'>--</td></tr>";
  html += "<tr><td>Signal Strength</td><td id='rssi'>--</td></tr>";
  html += "<tr><td>Free Heap</td><td><span id='heap'>--</span> bytes</td></tr>";
  html += "<tr><td>Uptime</td><td><span id='uptime'>--</span></td></tr>";
  html += "<tr><td>Date/Time</td><td id='datetime'>--</td></tr></table>";
  html += "</div>";
  
  // Sound Alerts (Beep) toggle
  html += "<div class='section'>";
  html += "<h3>🔔 Sound Alerts</h3>";
  html += "<button class='btn btn-output-on' id='btnBeeps' onclick='toggleBeeps()' style='width:100%'>Beep Alerts (ON)</button>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Disable all beeps when alerts occur. Default: enabled.</p>";
  html += "</div>";
  
  // NTP Configuration Section (NEW!)
  html += "<div class='section'>";
  html += "<h3>🕐 NTP Time Configuration</h3>";
  html += "<div id='ntpStatus'></div>";
  html += "<label>NTP Server</label><input type='text' id='ntpServer' placeholder='pool.ntp.org' value='pool.ntp.org'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>NTP server address for time synchronization.</p>";
  html += "<label>GMT Offset (seconds)</label><input type='number' step='3600' id='gmtOffset' placeholder='3600' value='3600'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Time zone offset in seconds (e.g., 3600 for GMT+1, -18000 for GMT-5).</p>";
  html += "<label>Daylight Offset (seconds)</label><input type='number' step='3600' id='daylightOffset' placeholder='3600' value='3600'>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Daylight saving time offset in seconds (usually 0 or 3600).</p>";
  html += "</div>";
  
  html += "<div class='section'>";
  html += "<h3>📊 Log Level Configuration</h3>";
  html += "<label>Log Level</label>";
  html += "<select id='logLevel' style='width:100%;padding:8px;border:1px solid #ddd;border-radius:4px;margin-bottom:10px'>";
  html += "<option value='0'>DEBUG - All messages</option>";
  html += "<option value='1' selected>INFO - Normal operation</option>";
  html += "<option value='2'>WARNING - Warnings and errors</option>";
  html += "<option value='3'>ERROR - Errors only</option>";
  html += "<option value='4'>NONE - No logging</option>";
  html += "</select>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Control verbosity of serial output. DEBUG shows all messages, NONE disables logging.</p>";
  html += "<button class='btn' style='width:100%;margin-top:10px' onclick='saveSystemSettings()'>Save System Settings</button>";
  html += "<p style='font-size:11px;color:#666;margin-top:10px'>Device will reboot after saving to apply new settings.</p>";
  html += "</div>";
  
  html += "<div class='section'>";
  html += "<h3>HTTP API Security</h3>";
  html += "<label>HTTP API Password</label><input type='password' id='apiPassword' placeholder='Password for HTTP commands'>";
  html += "<button class='btn' style='width:100%;margin-top:10px' onclick='saveApiPassword()'>Save API Password</button>";
  html += "<p style='font-size:11px;color:#666;margin-top:5px'>Set a password to protect HTTP commands. Use header: X-API-Password</p>";
  html += "</div>";
  
  html += "<div class='section'>";
  html += "<h3>Actions</h3>";
  html += "<button class='btn' style='width:100%;background:#dc3545;margin-bottom:10px' onclick='factoryReset()'>Factory Reset</button>";
  html += "<button class='btn' style='width:100%;background:#dc3545' onclick='rebootDevice()'>Reboot Device</button>";
  html += "</div>";
  html += "</div>";

  // Tab 9 - API Info
  html += "<div class='tab-content' id='tab9'>";
  html += "<div class='section'>";
  html += "<h3>HTTP API Documentation</h3>";
  html += "<div class='info-section'>";
  html += "<h4>🔐 Authentication</h4>";
  html += "<p>All API commands require password authentication. Set your password in the System tab.</p>";
  html += "<p><strong>Default password:</strong> oukitel2024</p>";
  html += "</div>";
  
  html += "<div class='info-section'>";
  html += "<h4>📡 API Endpoint</h4>";
  html += "<div class='code-block'>POST http://[DEVICE_IP]/api/command\\nHeader: X-API-Password: your_password\\nContent-Type: application/json</div>";
  html += "</div>";
  
  html += "<div class='info-section'>";
  html += "<h4>🎮 Available Commands</h4>";
  html += "<p><strong>1. Press Button</strong></p>";
  html += "<div class='code-block'>curl -X POST http://[DEVICE_IP]/api/command \\\\\n  -H \\\"X-API-Password: your_password\\\" \\\\\n  -H \\\"Content-Type: application/json\\\" \\\\\n  -d '{\\\"command\\\":\\\"pressButton\\\",\\\"button\\\":0}'</div>";
  html += "<p style='margin-top:5px'><strong>Button values:</strong> 0=POWER, 1=USB, 2=DC, 3=FLASH, 4=AC</p>";
  
  html += "<p style='margin-top:15px'><strong>2. Get Sensor Data</strong></p>";
  html += "<div class='code-block'>curl -X POST http://[DEVICE_IP]/api/command \\\\\n  -H \\\"X-API-Password: your_password\\\" \\\\\n  -H \\\"Content-Type: application/json\\\" \\\\\n  -d '{\\\"command\\\":\\\"getData\\\"}'</div>";
  
  html += "<p style='margin-top:15px'><strong>3. Set Auto Power On</strong></p>";
  html += "<div class='code-block'>curl -X POST http://[DEVICE_IP]/api/command \\\\\n  -H \\\"X-API-Password: your_password\\\" \\\\\n  -H \\\"Content-Type: application/json\\\" \\\\\n  -d '{\\\"command\\\":\\\"setAutoPowerOn\\\",\\\"enabled\\\":true}'</div>";
  html += "</div>";
  
  html += "<div class='info-section'>";
  html += "<h4>📋 Response Format</h4>";
  html += "<p><strong>Success:</strong></p>";
  html += "<div class='code-block'>{\\\"success\\\":true,\\\"message\\\":\\\"Button pressed\\\"}</div>";
  html += "<p style='margin-top:10px'><strong>Error:</strong></p>";
  html += "<div class='code-block'>{\\\"error\\\":\\\"Unauthorized - Invalid password\\\"}</div>";
  html += "</div>";
  
  html += "<div class='info-section'>";
  html += "<h4>💡 Quick Examples</h4>";
  html += "<p><strong>Turn on AC output:</strong></p>";
  html += "<div class='code-block'>curl -X POST http://192.168.1.100/api/command \\\\\n  -H \\\"X-API-Password: oukitel2024\\\" \\\\\n  -H \\\"Content-Type: application/json\\\" \\\\\n  -d '{\\\"command\\\":\\\"pressButton\\\",\\\"button\\\":4}'</div>";
  
  html += "<p style='margin-top:10px'><strong>Enable Auto Power On:</strong></p>";
  html += "<div class='code-block'>curl -X POST http://192.168.1.100/api/command \\\\\n  -H \\\"X-API-Password: oukitel2024\\\" \\\\\n  -H \\\"Content-Type: application/json\\\" \\\\\n  -d '{\\\"command\\\":\\\"setAutoPowerOn\\\",\\\"enabled\\\":true}'</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // container

  html += "<script>";
  html += "var ws, currentTab=0, isPowerOn=false, autoPowerState=false, usbState=false, dcState=false, flashState=false, acState=false, beepsEnabled=true;";

  html += "function init(){";
  html += "ws=new WebSocket('ws://'+location.hostname+':81');";
  html += "ws.onopen=function(){console.log('Connected');document.getElementById('wifiStatus').textContent='Connected';loadAutoPower();};";
  html += "ws.onclose=function(){document.getElementById('wifiStatus').textContent='Disconnected';setTimeout(init,5000);};";
  html += "ws.onmessage=function(e){updateData(JSON.parse(e.data));};";
  html += "ws.onerror=function(e){console.error('WebSocket error:',e);};";
  html += "}";

  html += "function updatePowerState(voltage){";
  html += "var wasPowerOn=isPowerOn;";
  html += "isPowerOn=voltage>=20.0;";
  html += "var warning=document.getElementById('powerOffWarning');";
  html += "var btnPower=document.getElementById('btnPower');";
  html += "var btnUsb=document.getElementById('btnUsb');";
  html += "var btnDc=document.getElementById('btnDc');";
  html += "var btnFlash=document.getElementById('btnFlash');";
  html += "var btnAc=document.getElementById('btnAc');";

  html += "if(isPowerOn){";
  html += "btnPower.className='btn btn-power-on';";
  html += "btnPower.textContent='POWER (ON)';";
  html += "}else{";
  html += "btnPower.className='btn btn-power-off';";
  html += "btnPower.textContent='POWER (OFF)';";
  html += "}";

  html += "if(!isPowerOn){";
  html += "warning.style.display='block';";
  html += "btnUsb.disabled=true;";
  html += "btnDc.disabled=true;";
  html += "btnFlash.disabled=true;";
  html += "btnAc.disabled=true;";
  html += "usbState=false;dcState=false;flashState=false;acState=false;";
  html += "btnUsb.className='btn btn-output-off';btnUsb.textContent='USB';";
  html += "btnDc.className='btn btn-output-off';btnDc.textContent='DC';";
  html += "btnFlash.className='btn btn-output-off';btnFlash.textContent='Flash';";
  html += "btnAc.className='btn btn-output-off';btnAc.textContent='AC';";
  html += "}else{";
  html += "warning.style.display='none';";
  html += "btnUsb.disabled=false;";
  html += "btnDc.disabled=false;";
  html += "btnFlash.disabled=false;";
  html += "btnAc.disabled=false;";
  html += "}";
  html += "}";

  html += "function showTab(n){";
  html += "currentTab=n;";
  html += "var tabs=document.querySelectorAll('.tab-content');";
  html += "var btns=document.querySelectorAll('nav button');";
  html += "tabs.forEach(function(t,i){t.className='tab-content'+(i===n?' active':'');});";
  html += "btns.forEach(function(b,i){b.className=i===n?'active':'';});";
  html += "}";

  html += "function pressBtn(i){";
  html += "if(!isPowerOn && i!==0)return;";
  html += "ws.send(JSON.stringify({command:'pressButton',button:i}));";
  html += "if(i===1){usbState=!usbState;var btn=document.getElementById('btnUsb');btn.className=usbState?'btn btn-output-on':'btn btn-output-off';}";
  html += "if(i===2){dcState=!dcState;var btn=document.getElementById('btnDc');btn.className=dcState?'btn btn-output-on':'btn btn-output-off';}";
  html += "if(i===3){flashState=!flashState;var btn=document.getElementById('btnFlash');btn.className=flashState?'btn btn-output-on':'btn btn-output-off';}";
  html += "if(i===4){acState=!acState;var btn=document.getElementById('btnAc');btn.className=acState?'btn btn-output-on':'btn btn-output-off';}";
  html += "}";

  html += "function confirmPower(){";
  html += "if(confirm('Activate POWER (3 seconds)?'))pressBtn(0);";
  html += "}";

  html += "function toggleAutoPower(){";
  html += "autoPowerState=!autoPowerState;";
  html += "var btn=document.getElementById('btnAutoPower');";
  html += "if(autoPowerState){btn.className='btn btn-output-on';btn.textContent='Auto Power On (ON)';}";
  html += "else{btn.className='btn btn-output-off';btn.textContent='Auto Power On (OFF)';}";
  html += "ws.send(JSON.stringify({command:'setAutoPowerOn',enabled:autoPowerState}));";
  html += "}";

  html += "function loadAutoPower(){";
  html += "ws.send(JSON.stringify({command:'getAutoPowerOn'}));";
  html += "}";

  html += "function saveWifi(){";
  html += "var ssid=document.getElementById('wifiSsid').value;";
  html += "var pass=document.getElementById('wifiPass').value;";
  html += "if(!ssid){alert('SSID required');return;}";
  html += "ws.send(JSON.stringify({command:'setWifi',ssid:ssid,password:pass}));";
  html += "alert('WiFi configured. Rebooting...');";
  html += "}";

  html += "function scanWifi(){";
  html += "ws.send(JSON.stringify({command:'scanWifi'}));";
  html += "}";

  html += "function showStatus(divId, msg, type){";
  html += "var div=document.getElementById(divId);";
  html += "var classes='alert alert-'+type;";
  html += "div.innerHTML='<div class=\"'+classes+'\">'+msg+'</div>';";
  html += "setTimeout(function(){div.innerHTML='';},5000);";
  html += "}";

  html += "function resetCalToDefaults(){";
  html += "ws.send(JSON.stringify({command:'getCalibration',defaults:true}));";
  html += "showStatus('calStatus','Loading default values...', 'info');";
  html += "}";

  html += "function resetAdvToDefaults(){";
  html += "ws.send(JSON.stringify({command:'getAdvancedSettings',defaults:true}));";
  html += "showStatus('advStatus','Loading default values...', 'info');";
  html += "}";

  html += "function saveCal(){";
  html += "var cmd={";
  html += "command:'saveCalibration',";
  html += "sct013CalIn:parseFloat(document.getElementById('calMainCal').value),";
  html += "sct013OffsetIn:parseFloat(document.getElementById('calMainOff').value),";
  html += "sct013CalOut:parseFloat(document.getElementById('calOutCal').value),";
  html += "sct013OffsetOut:parseFloat(document.getElementById('calOutOff').value),";
  html += "batteryDividerRatio:parseFloat(document.getElementById('calBattRatio').value),";
  html += "batteryAdcCalibration:parseFloat(document.getElementById('calBattAdc').value),";
  html += "voltageOffsetCharge:parseFloat(document.getElementById('calOffCharge').value),";
  html += "voltageOffsetDischarge:parseFloat(document.getElementById('calOffDischarge').value),";
  html += "voltageOffsetRest:parseFloat(document.getElementById('calOffRest').value),";
  html += "mainsVoltage:parseFloat(document.getElementById('calMainsVoltage').value||230),";
  html += "fixedVoltage:parseFloat(document.getElementById('calFixedVoltage').value||0)";
  html += "};";
  html += "ws.send(JSON.stringify(cmd));";
  html += "showStatus('calStatus','Calibration sent to device...', 'info');";
  html += "}";

  html += "function saveAdv(){";
  html += "var cmd={";
  html += "command:'saveAdvancedSettings',";
  html += "powerStationOffVoltage:parseFloat(document.getElementById('advPowerStationOffVoltage').value),";
  html += "powerThreshold:parseFloat(document.getElementById('advPowerThreshold').value),";
  html += "powerFilterAlpha:parseFloat(document.getElementById('advPowerFilterAlpha').value),";
  html += "voltageMinSafe:parseFloat(document.getElementById('advVoltageMinSafe').value),";
  html += "batteryLowWarning:parseFloat(document.getElementById('advBatteryLowWarning').value),";
  html += "batteryCritical:parseFloat(document.getElementById('advBatteryCritical').value),";
  html += "autoPowerOnDelay:parseInt(document.getElementById('advAutoPowerOnDelay').value),";
  html += "socBufferSize:parseInt(document.getElementById('advSocBufferSize').value),";
  html += "socChangeThreshold:parseInt(document.getElementById('advSocChangeThreshold').value),";
  html += "warmupDelay:parseInt(document.getElementById('advWarmupDelay').value)";
  html += "};";
  html += "ws.send(JSON.stringify(cmd));";
  html += "showStatus('advStatus','Advanced settings sent to device...', 'info');";
  html += "}";

  html += "function saveMqttConfig(){";
  html += "var cmd={";
  html += "command:'saveMqttConfig',";
  html += "enabled:document.getElementById('mqttEnabled').checked,";
  html += "server:document.getElementById('mqttServer').value,";
  html += "port:parseInt(document.getElementById('mqttPort').value),";
  html += "username:document.getElementById('mqttUsername').value,";
  html += "password:document.getElementById('mqttPassword').value,";
  html += "clientId:document.getElementById('mqttClientId').value";
  html += "};";
  html += "ws.send(JSON.stringify(cmd));";
  html += "showStatus('mqttStatus','MQTT configuration saved. Rebooting...', 'success');";
  html += "}";

  html += "function saveHaConfig(){";
  html += "var cmd={";
  html += "command:'saveHttpConfig',";
  html += "enabled:document.getElementById('haEnabled').checked,";
  html += "server:document.getElementById('haServer').value,";
  html += "port:parseInt(document.getElementById('haPort').value),";
  html += "endpoint:document.getElementById('haEndpoint').value,";
  html += "apiKey:document.getElementById('haToken').value";
  html += "};";
  html += "ws.send(JSON.stringify(cmd));";
  html += "showStatus('haStatus','Home Assistant configuration saved. Rebooting...', 'success');";
  html += "}";

  // Save HTTP Shutdown Configuration
  html += "function saveShutdownConfig(){";
  html += "var cmd={";
  html += "command:'saveHttpShutdownConfig',";
  html += "enabled:document.getElementById('shutdownEnabled').checked,";
  html += "batteryThreshold:parseFloat(document.getElementById('shutdownThreshold').value),";
  html += "server:document.getElementById('shutdownServer').value,";
  html += "port:parseInt(document.getElementById('shutdownPort').value),";
  html += "password:document.getElementById('shutdownPassword').value";
  html += "};";
  html += "ws.send(JSON.stringify(cmd));";
  html += "showStatus('shutdownStatus','Shutdown notification configuration saved.', 'success');";
  html += "}";

  html += "function saveUpsConfig(){";
  html += "var cmd={";
  html += "command:'saveUpsConfig',";
  html += "enabled:document.getElementById('upsEnabled').checked,";
  html += "port:parseInt(document.getElementById('upsPort').value),";
  html += "shutdownThreshold:parseInt(document.getElementById('upsThreshold').value)";
  html += "};";
  html += "ws.send(JSON.stringify(cmd));";
  html += "showStatus('upsStatus','UPS configuration saved. Rebooting...', 'success');";
  html += "}";

  // Save System Settings
  html += "function saveSystemSettings(){";
  html += "var cmd={";
  html += "command:'saveSystemSettings',";
  html += "ntpServer:document.getElementById('ntpServer').value,";
  html += "gmtOffset:parseInt(document.getElementById('gmtOffset').value),";
  html += "daylightOffset:parseInt(document.getElementById('daylightOffset').value),";
  html += "beepsEnabled:beepsEnabled,";
  html += "logLevel:parseInt(document.getElementById('logLevel').value)||1";
  html += "};";
  html += "ws.send(JSON.stringify(cmd));";
  html += "showStatus('ntpStatus','Settings saved. Rebooting...', 'success');";
  html += "}";

  html += "function toggleBeeps(){";
  html += "beepsEnabled=!beepsEnabled;";
  html += "var btn=document.getElementById('btnBeeps');";
  html += "if(beepsEnabled){btn.className='btn btn-output-on';btn.textContent='Beep Alerts (ON)';}";
  html += "else{btn.className='btn btn-output-off';btn.textContent='Beep Alerts (OFF)';}";
  html += "saveSystemSettings();";
  html += "}";

  html += "function saveApiPassword(){";
  html += "var password=document.getElementById('apiPassword').value;";
  html += "if(!password){alert('Password required');return;}";
  html += "ws.send(JSON.stringify({command:'saveApiPassword',password:password}));";
  html += "}";

  html += "function factoryReset(){";
  html += "if(confirm('Reset to factory settings? All configurations will be lost!')){";
  html += "ws.send(JSON.stringify({command:'factoryReset'}));";
  html += "alert('Factory reset initiated. Device will reboot...');";
  html += "}";
  html += "}";

  html += "function rebootDevice(){";
  html += "if(confirm('Reboot device?')){";
  html += "ws.send(JSON.stringify({command:'reboot'}));";
  html += "alert('Device is rebooting...');";
  html += "}";
  html += "}";

  html += "function updateConfigStatus(divId, enabled, hasServer){";
  html += "var div=document.getElementById(divId);";
  html += "if(enabled && hasServer){";
  html += "div.textContent='CONFIGURED AND ACTIVE';";
  html += "div.className='config-status configured';";
  html += "}else{";
  html += "div.textContent='NOT CONFIGURED';";
  html += "div.className='config-status';";
  html += "}";
  html += "}";

  html += "function updateData(d){";
  html += "if(d.type==='wifiScanResult'){";
  html += "var results=document.getElementById('wifiScanResults');";
  html += "results.innerHTML='<h4>Available Networks:</h4>';";
  html += "d.networks.forEach(function(net){";
  html += "var div=document.createElement('div');";
  html += "div.textContent=net.ssid+' ('+net.rssi+' dBm)';";
  html += "div.style.cursor='pointer';";
  html += "div.style.padding='8px';";
  html += "div.style.borderBottom='1px solid #ddd';";
  html += "div.onclick=function(){document.getElementById('wifiSsid').value=net.ssid;};";
  html += "results.appendChild(div);";
  html += "});";
  html += "return;";
  html += "}";

  html += "if(d.type==='wifiStatus'){alert(d.message);return;}";

  html += "if(d.type==='calibrationData'){";
  html += "document.getElementById('calMainCal').value=(d.sct013CalIn).toFixed(2);";
  html += "document.getElementById('calMainOff').value=(d.sct013OffsetIn).toFixed(2);";
  html += "document.getElementById('calOutCal').value=(d.sct013CalOut).toFixed(2);";
  html += "document.getElementById('calOutOff').value=(d.sct013OffsetOut).toFixed(2);";
  html += "document.getElementById('calBattRatio').value=(d.batteryDividerRatio).toFixed(3);";
  html += "document.getElementById('calBattAdc').value=(d.batteryAdcCalibration).toFixed(4);";
  html += "document.getElementById('calOffCharge').value=(d.voltageOffsetCharge).toFixed(2);";
  html += "document.getElementById('calOffDischarge').value=(d.voltageOffsetDischarge).toFixed(2);";
  html += "document.getElementById('calOffRest').value=(d.voltageOffsetRest).toFixed(2);";
  html += "document.getElementById('calMainsVoltage').value=(d.mainsVoltage||230).toFixed(1);";
  html += "document.getElementById('calFixedVoltage').value=(d.fixedVoltage||0).toFixed(1);";
  html += "showStatus('calStatus','Calibration loaded successfully', 'success');";
  html += "return;";
  html += "}";

  html += "if(d.type==='advancedSettings'){";
  html += "document.getElementById('advPowerStationOffVoltage').value=(d.powerStationOffVoltage).toFixed(1);";
  html += "document.getElementById('advPowerThreshold').value=(d.powerThreshold).toFixed(2);";
  html += "document.getElementById('advPowerFilterAlpha').value=(d.powerFilterAlpha).toFixed(2);";
  html += "document.getElementById('advVoltageMinSafe').value=(d.voltageMinSafe).toFixed(2);";
  html += "document.getElementById('advBatteryLowWarning').value=(d.batteryLowWarning).toFixed(1);";
  html += "document.getElementById('advBatteryCritical').value=(d.batteryCritical).toFixed(1);";
  html += "document.getElementById('advAutoPowerOnDelay').value=d.autoPowerOnDelay;";
  html += "document.getElementById('advSocBufferSize').value=d.socBufferSize;";
  html += "document.getElementById('advSocChangeThreshold').value=d.socChangeThreshold;";
  html += "document.getElementById('advWarmupDelay').value=d.warmupDelay;";
  html += "showStatus('advStatus','Advanced settings loaded successfully', 'success');";
  html += "return;";
  html += "}";

  html += "if(d.type==='mqttConfig'){";
  html += "document.getElementById('mqttEnabled').checked=d.enabled;";
  html += "document.getElementById('mqttServer').value=d.server||'';";
  html += "document.getElementById('mqttPort').value=d.port||1883;";
  html += "document.getElementById('mqttUsername').value=d.username||'';";
  html += "document.getElementById('mqttPassword').value=d.password||'';";
  html += "document.getElementById('mqttClientId').value=d.clientId||'';";
  html += "updateConfigStatus('mqttConfigStatus', d.enabled, d.server && d.server.length>0);";
  html += "return;";
  html += "}";

  html += "if(d.type==='httpConfig'){";
  html += "document.getElementById('haEnabled').checked=d.enabled;";
  html += "document.getElementById('haServer').value=d.server||'';";
  html += "document.getElementById('haPort').value=d.port||8123;";
  html += "document.getElementById('haEndpoint').value=d.endpoint||'/api/states/sensor.oukitel_p800e';";
  html += "document.getElementById('haToken').value=d.apiKey||'';";
  html += "updateConfigStatus('haConfigStatus', d.enabled, d.server && d.server.length>0);";
  html += "return;";
  html += "}";

  // Handle HTTP Shutdown Config
  html += "if(d.type==='httpShutdownConfig'){";
  html += "document.getElementById('shutdownEnabled').checked=d.enabled;";
  html += "document.getElementById('shutdownThreshold').value=d.batteryThreshold||15;";
  html += "document.getElementById('shutdownServer').value=d.server||'';";
  html += "document.getElementById('shutdownPort').value=d.port||8080;";
  html += "document.getElementById('shutdownPassword').value=d.password||'';";
  html += "updateConfigStatus('shutdownConfigStatus', d.enabled, d.server && d.server.length>0);";
  html += "return;";
  html += "}";

  // Handle System Settings
  html += "if(d.type==='systemSettings'){";
  html += "document.getElementById('ntpServer').value=d.ntpServer||'pool.ntp.org';";
  html += "document.getElementById('gmtOffset').value=d.gmtOffset||3600;";
  html += "document.getElementById('daylightOffset').value=d.daylightOffset||3600;";
  html += "beepsEnabled=(d.beepsEnabled!==undefined?d.beepsEnabled:true);";
  html += "var b=document.getElementById('btnBeeps');";
  html += "if(beepsEnabled){b.className='btn btn-output-on';b.textContent='Beep Alerts (ON)';}else{b.className='btn btn-output-off';b.textContent='Beep Alerts (OFF)';}";
  html += "document.getElementById('logLevel').value=d.logLevel||1;";
  html += "return;";
  html += "}";

  html += "if(d.type==='upsConfig'){";
  html += "document.getElementById('upsEnabled').checked=d.enabled;";
  html += "document.getElementById('upsPort').value=d.port||3493;";
  html += "document.getElementById('upsThreshold').value=d.shutdownThreshold||10;";
  html += "updateConfigStatus('upsConfigStatus', d.enabled, true);";
  html += "return;";
  html += "}";

  html += "if(d.type==='apiPasswordStatus'){";
  html += "alert(d.message);";
  html += "return;";
  html += "}";

  html += "if(d.type==='calibrationStatus'){";
  html += "showStatus('calStatus',d.message, d.success?'success':'danger');";
  html += "return;";
  html += "}";

  html += "if(d.type==='advancedSettingsStatus'){";
  html += "showStatus('advStatus',d.message, d.success?'success':'danger');";
  html += "return;";
  html += "}";

  // Handle HTTP Shutdown Status
  html += "if(d.type==='httpShutdownStatus'){";
  html += "showStatus('shutdownStatus',d.message, d.success?'success':'danger');";
  html += "return;";
  html += "}";

  // Handle System Settings Status
  html += "if(d.type==='systemSettingsStatus'){";
  html += "showStatus('ntpStatus',d.message, d.success?'success':'danger');";
  html += "return;";
  html += "}";

  html += "if(d.type==='monthlyHistory'){";
  html += "var table=document.getElementById('monthHistory');";
  html += "if(d.history && d.history.length>0){";
  html += "table.innerHTML='';";
  html += "d.history.forEach(function(r){";
  html += "var row=table.insertRow();";
  html += "var c1=row.insertCell(0);";
  html += "var c2=row.insertCell(1);";
  html += "c1.textContent=r.year+'-'+String(r.month).padStart(2,'0');";
  html += "c2.innerHTML='<span class=\"value\">'+r.consumption.toFixed(2)+'</span> kWh';";
  html += "});";
  html += "}else{";
  html += "table.innerHTML='<tr><td colspan=\"2\">No history available</td></tr>';"; 
  html += "}";
  html += "return;";
  html += "}";

  html += "if(d.type==='acActivated'){";
  html += "acState=true;";
  html += "var btnAc=document.getElementById('btnAc');";
  html += "btnAc.className='btn btn-output-on';";
  html += "return;";
  html += "}";

  html += "if(d.autoPowerOn!==undefined){";
  html += "autoPowerState=d.autoPowerOn;";
  html += "var btn=document.getElementById('btnAutoPower');";
  html += "if(autoPowerState){btn.className='btn btn-output-on';btn.textContent='Auto Power On (ON)';}";
  html += "else{btn.className='btn btn-output-off';btn.textContent='Auto Power On (OFF)';}";
  html += "}";

  html += "if(d.voltage!==undefined){";
  html += "document.getElementById('voltage').textContent=d.voltage.toFixed(2);";
  html += "document.getElementById('soc').textContent=d.soc.toFixed(1);";
  html += "document.getElementById('powerIn').textContent=Math.round(d.powerIn);";
  html += "document.getElementById('powerOut').textContent=Math.round(d.powerOut);";
  html += "var net=d.powerIn-d.powerOut;";
  html += "document.getElementById('powerNet').textContent=(net>=0?'+':'')+Math.round(net);";
  html += "document.getElementById('state').textContent=d.state||'--';";
  html += "updatePowerState(d.voltage);";
  html += "}";

  html += "if(d.instantPower!==undefined){";
  html += "document.getElementById('instantPower').textContent=Math.round(d.instantPower);";
  html += "}";

  html += "if(d.dailyConsumption!==undefined){";
  html += "document.getElementById('dailyConsumption').textContent=d.dailyConsumption.toFixed(3);";
  html += "}";

  html += "if(d.monthCurrent!==undefined){";
  html += "document.getElementById('monthCurrent').textContent=d.monthCurrent.toFixed(3);";
  html += "}";

  html += "if(d.yearEstimate!==undefined){";
  html += "document.getElementById('yearEstimate').textContent=d.yearEstimate.toFixed(2);";
  html += "}";

  html += "if(d.heap)document.getElementById('heap').textContent=d.heap;";
  html += "if(d.uptime){";
  html += "var h=Math.floor(d.uptime/3600);";
  html += "var m=Math.floor((d.uptime%3600)/60);";
  html += "var s=d.uptime%60;";
  html += "document.getElementById('uptime').textContent=h+'h '+m+'m '+s+'s';";
  html += "}";

  html += "if(d.ipAddress)document.getElementById('ipAddr').textContent=d.ipAddress;";
  html += "if(d.macAddress)document.getElementById('macAddr').textContent=d.macAddress;";
  html += "if(d.ssid)document.getElementById('ssid').textContent=d.ssid;";
  html += "if(d.rssi)document.getElementById('rssi').textContent=d.rssi+' dBm';";

  html += "}";

  html += "function resetMonth(){";
  html += "if(confirm('Reset current month energy data?')){";
  html += "ws.send(JSON.stringify({command:'resetMonthlyEnergy'}));";
  html += "alert('Monthly energy data reset');";
  html += "}";
  html += "}";

  html += "function updateDateTime(){";
  html += "var now=new Date();";
  html += "var str=now.toLocaleString('en-US');";
  html += "document.getElementById('datetime').textContent=str;";
  html += "}";

  html += "init();";
  html += "setInterval(function(){if(ws && ws.readyState===1)ws.send(JSON.stringify({command:'getData'}));},5000);";
  html += "setInterval(updateDateTime,1000);";
  html += "</script></body></html>";

  return html;
}
## 🔌 Integration Examples

### Home Assistant via MQTT Discovery

Home Assistant will automatically discover the device when MQTT is configured.

#### Setup Steps

1. **Configure MQTT in ESP32**:
   - Go to **Settings → MQTT**
   - Enable MQTT
   - Enter Home Assistant MQTT broker IP (usually same as HA)
   - Port: 1883 (default)
   - Enter MQTT username/password (if configured in HA)
   - Save settings

2. **Verify in Home Assistant**:
   - Go to **Settings → Devices & Services**
   - Under **MQTT**, you should see "Oukitel-P800E" device
   - All sensors and switches will be automatically created

3. **Available Entities**:
   - `sensor.oukitel_p800e_battery_voltage` - Battery voltage (V)
   - `sensor.oukitel_p800e_battery_percentage` - SoC (%)
   - `sensor.oukitel_p800e_power_in` - Input power (W)
   - `sensor.oukitel_p800e_power_out` - Output power (W)
   - `sensor.oukitel_p800e_current_in` - Input current (A)
   - `sensor.oukitel_p800e_current_out` - Output current (A)
   - `switch.oukitel_p800e_usb_output` - USB control
   - `switch.oukitel_p800e_dc_output` - DC control
   - `switch.oukitel_p800e_ac_output` - AC control
   - `switch.oukitel_p800e_flashlight` - Flashlight control

4. **Create Automations**:
   ```yaml
   automation:
     - alias: "Low Battery Alert"
       trigger:
         - platform: numeric_state
           entity_id: sensor.oukitel_p800e_battery_percentage
           below: 20
       action:
         - service: notify.mobile_app
           data:
             message: "Oukitel P800E battery is low!"
   ```

### Synology NAS via NUT Protocol

#### Setup Steps

1. **Enable UPS Server on ESP32**:
   - Go to **Settings → UPS (NUT)**
   - Enable UPS Server
   - Port: 3493 (default)
   - Set Shutdown Threshold (e.g., 20%)
   - Save settings

2. **Configure Synology NAS**:
   - Open **Control Panel → Hardware & Power → UPS**
   - Enable UPS support
   - Select **"Synology UPS Server"** or **"Network UPS Tools"**
   - Enter ESP32 IP address as UPS IP
   - Test connection

3. **Verify Connection**:
   - Synology should show battery status
   - Test shutdown by lowering threshold temporarily

4. **Automatic Shutdown**:
   - When battery reaches threshold, ESP32 sends shutdown command
   - NAS will safely shut down before battery depletion

### QNAP NAS via NUT Protocol

Similar to Synology:

1. **Enable UPS Server on ESP32** (same as above)
2. **Configure QNAP**:
   - Open **Control Panel → System → UPS**
   - Enable UPS support
   - Select **"Network UPS Tools (NUT)"**
   - Enter ESP32 IP address
   - Test connection

### Custom HTTP Integration

Use HTTP shutdown notifications for custom scripts or integrations.

#### Example: Node-RED Integration

1. **Configure HTTP Shutdown on ESP32**:
   - Go to **Settings → HTTP Shutdown**
   - Enable HTTP Shutdown
   - Server: Node-RED IP
   - Port: 1880 (Node-RED default)
   - Set threshold (e.g., 15%)

2. **Create Node-RED Flow**:
   ```json
   [
     {
       "id": "http-in",
       "type": "http in",
       "path": "/shutdown",
       "method": "post"
     },
     {
       "id": "function",
       "type": "function",
       "func": "msg.payload = {battery: msg.payload.battery, action: 'shutdown'};\nreturn msg;"
     },
     {
       "id": "notification",
       "type": "notification",
       "title": "Power Station Shutdown"
     }
   ]
   ```

3. **ESP32 will POST** to `/shutdown` when threshold is reached

### MQTT Integration with Node-RED

1. **Configure MQTT on ESP32** (see Home Assistant section)
2. **Subscribe to Topics in Node-RED**:
   - `oukitel_p800e_<MAC>/sensor/#` - All sensor data
   - `oukitel_p800e_<MAC>/switch/#` - Switch states
3. **Publish to Control Topics**:
   - `oukitel_p800e_<MAC>/switch/usb_output/set` - Control USB
   - `oukitel_p800e_<MAC>/switch/ac_output/set` - Control AC

---

## ❓ Troubleshooting

### Device Won't Connect to WiFi

**Symptoms**: Device creates Access Point instead of connecting

**Solutions**:
1. **Verify SSID and Password**: Check for typos, case sensitivity
2. **Check WiFi Signal**: Ensure device is within range
3. **Check Router Settings**: Some routers block new devices
4. **Use Access Point Mode**: Connect to `Oukitel-P800A` AP and configure via web interface
5. **Reset WiFi Settings**: Clear saved credentials and reconfigure

### Incorrect Power Readings

**Symptoms**: Power readings don't match actual consumption

**Solutions**:
1. **Calibrate Sensors**: Follow [Calibration Guide](#-calibration-guide)
2. **Check Mains Voltage**: Verify correct mains voltage setting
3. **Check Sensor Placement**: Ensure SCT013 clamps are properly positioned
4. **Verify Offset**: Zero current offset should be calibrated with no load
5. **Check Wiring**: Verify all connections are secure

### Web Interface Slow or Frozen

**Symptoms**: Web page loads slowly or becomes unresponsive

**Solutions**:
1. **Reload Page**: Simple refresh often resolves temporary issues
2. **Check Memory**: System may skip non-essential operations if memory is low
3. **Reduce Log Level**: Set to WARNING or ERROR to reduce overhead
4. **Restart Device**: Power cycle ESP32
5. **Check Network**: Verify stable WiFi connection

### MQTT Not Connecting

**Symptoms**: MQTT status shows disconnected

**Solutions**:
1. **Verify Broker IP**: Check MQTT broker address is correct
2. **Check Port**: Default is 1883, verify broker port
3. **Verify Credentials**: Check username/password
4. **Test Broker**: Use MQTT client tool to verify broker is accessible
5. **Check Firewall**: Ensure port 1883 is not blocked

### UPS Protocol Not Working

**Symptoms**: NAS doesn't recognize UPS

**Solutions**:
1. **Verify UPS Server Enabled**: Check Settings → UPS (NUT)
2. **Check Port**: Default is 3493, verify NAS is using same port
3. **Test Connection**: Use NUT client tools to test connection
4. **Check Firewall**: Ensure port 3493 is open
5. **Verify IP Address**: Ensure NAS is connecting to correct ESP32 IP

### Battery Voltage Incorrect

**Symptoms**: Displayed voltage doesn't match multimeter reading

**Solutions**:
1. **Calibrate Voltage**: Follow voltage calibration steps
2. **Check Divider Ratio**: Verify resistor values (220kΩ / 27kΩ)
3. **Check ADC Calibration**: Adjust ADC calibration factor
4. **State-Specific Offsets**: Calibrate for each battery state
5. **Verify Wiring**: Check voltage divider connections

### Serial Monitor Shows Errors

**Symptoms**: Error messages in Serial Monitor

**Common Errors**:
- **SPIFFS errors**: File system may be corrupted, format SPIFFS
- **WiFi errors**: Connection issues, check credentials
- **MQTT errors**: Broker connection problems
- **Sensor errors**: Check wiring and calibration

**Solutions**:
1. **Check Log Level**: Set to DEBUG for detailed error messages
2. **Review Error Messages**: Error messages indicate specific issues
3. **Check Wiring**: Verify all connections
4. **Restart Device**: Power cycle may resolve temporary issues

### Device Keeps Restarting

**Symptoms**: ESP32 restarts repeatedly

**Solutions**:
1. **Check Power Supply**: Ensure stable 5V power (3A minimum)
2. **Check Wiring**: Short circuits can cause resets
3. **Review Serial Monitor**: Look for crash messages
4. **Reduce Logging**: High log verbosity can cause memory issues
5. **Check Memory**: Low memory can cause watchdog resets

### Calibration Values Reset

**Symptoms**: Calibration settings revert to defaults

**Solutions**:
1. **Check SPIFFS**: File system may be corrupted
2. **Save Settings**: Ensure "Save Calibration" is clicked after changes
3. **Verify SPIFFS Space**: Low space can prevent saving
4. **Format SPIFFS**: As last resort, format and reconfigure

---

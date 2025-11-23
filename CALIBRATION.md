
## 🔧 Calibration Guide

Accurate calibration is essential for reliable readings. Follow these steps carefully.

### Prerequisites

- Multimeter (for voltage measurement)
- Known load (e.g., 60W incandescent bulb, space heater with known wattage)
- Wattmeter or power meter (optional but recommended)
- Stable power source

### Current Sensor Calibration (SCT013)

#### Step 1: Zero Current Calibration (Offset)

1. **Disconnect all loads** from the power station
2. **Turn off all outputs** (USB, DC, AC)
3. **Ensure no input power** (unplug AC adapter)
4. In web interface, go to **Settings → Calibration**
5. Note the current readings for Input and Output
6. **Set Offset values** to match the measured current:
   - If Input shows 0.65A, set `SCT013 Input Offset` to `0.65`
   - If Output shows 0.55A, set `SCT013 Output Offset` to `0.55`
7. Click **Save Calibration**
8. Verify readings are now near zero (within ±0.05A)

#### Step 2: Load Calibration (Calibration Factor)

1. **Connect a known load** (e.g., 60W incandescent bulb)
2. **Turn on AC output** and connect the load
3. **Measure actual current** with a clamp meter or calculate from power:
   - For 60W at 230V: Current = 60W / 230V ≈ 0.26A
4. **Note the displayed current** in the web interface
5. **Calculate calibration factor**:
   ```
   New Factor = (Old Factor × Actual Current) / Displayed Current
   ```
   Example:
   - Old Factor: 32.20
   - Displayed Current: 0.30A
   - Actual Current: 0.26A
   - New Factor = (32.20 × 0.26) / 0.30 = 27.91
6. **Update calibration factor** in web interface
7. **Save and verify** the reading matches actual current

#### Step 3: Multiple Point Calibration (Recommended)

For best accuracy, calibrate at multiple load points:

1. **Low Load** (10-20W): Small device, LED light
2. **Medium Load** (50-100W): Incandescent bulb, small heater
3. **High Load** (200-500W): Space heater, power tool

Repeat calibration factor adjustment for each load point and use an average value.

### Battery Voltage Calibration

#### Step 1: Measure Actual Voltage

1. **Use a multimeter** to measure battery voltage directly at terminals
2. **Note the exact voltage** (e.g., 26.45V)

#### Step 2: Compare with Displayed Voltage

1. Check displayed voltage in web interface
2. **Calculate difference**: Actual - Displayed
3. **Adjust voltage offset**:
   - If displayed is 0.2V lower, increase `Voltage Offset Rest` by 0.2
   - If displayed is 0.2V higher, decrease offset by 0.2

#### Step 3: State-Specific Calibration

Calibrate for different battery states:

- **Charging**: Measure voltage while charging, adjust `Voltage Offset Charge`
- **Discharging**: Measure voltage under load, adjust `Voltage Offset Discharge`
- **Rest**: Measure voltage with no load/charge, adjust `Voltage Offset Rest`

#### Step 4: Divider Ratio Calibration (If Needed)

If voltage readings are consistently off by a percentage:

1. Measure actual battery voltage: `V_actual`
2. Measure voltage at GPIO36: `V_adc` (requires oscilloscope or precision ADC)
3. Calculate divider ratio: `Ratio = V_actual / V_adc`
4. Update `Battery Divider Ratio` in calibration settings

### Mains Voltage Calibration

1. **Check your local mains voltage**:
   - North America: 110V or 120V
   - Europe: 220V or 230V
   - Asia: 220V or 240V
2. **Update Mains Voltage** in calibration settings
3. This affects power calculations: `Power = Current × Voltage`

### Calibration Tips

- **Stable Conditions**: Calibrate when battery and loads are stable
- **Multiple Measurements**: Take several readings and average
- **Document Values**: Write down calibration values for future reference
- **Re-calibrate Periodically**: Sensors may drift over time
- **Test After Calibration**: Verify readings with known loads

---

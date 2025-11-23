
## 📡 API Documentation

### REST API Endpoints

All API calls require authentication via `X-API-Key` header or `api_password` parameter.

#### Authentication

```http
GET /api/status?api_password=your_password
```

Or via header:
```http
GET /api/status
X-API-Key: your_password
```

#### Get System Status

```http
GET /api/status?api_password=your_password
```

**Response**:
```json
{
  "batteryVoltage": 26.45,
  "batteryPercentage": 85.2,
  "mainCurrent": 2.5,
  "outputCurrent": 1.8,
  "mainPower": 575.0,
  "outputPower": 414.0,
  "onBattery": false,
  "batteryState": "charging"
}
```

#### Control Outputs

```http
POST /api/control?api_password=your_password
Content-Type: application/json

{
  "usb": true,
  "dc": false,
  "ac": true,
  "flashlight": false
}
```

#### Get Energy Data

```http
GET /api/energy?api_password=your_password
```

**Response**:
```json
{
  "dailyConsumption": 2.45,
  "monthlyConsumption": 45.8,
  "instantPower": 414.0,
  "peakPower": 850.0,
  "operatingTime": 86400
}
```

### WebSocket API

Real-time data streaming via WebSocket.

#### Connection

```javascript
const ws = new WebSocket('ws://esp32-ip:81');
```

#### Message Format

**Client → Server** (Commands):
```json
{
  "command": "getData",
  "type": "sensorData"
}
```

**Server → Client** (Updates):
```json
{
  "type": "sensorData",
  "data": {
    "batteryVoltage": 26.45,
    "batteryPercentage": 85.2,
    "mainPower": 575.0,
    "outputPower": 414.0
  }
}
```

#### Available Commands

- `getData` - Request current sensor data
- `control` - Control outputs (same format as REST API)
- `saveCalibration` - Save calibration data
- `saveAdvancedSettings` - Save advanced settings

---

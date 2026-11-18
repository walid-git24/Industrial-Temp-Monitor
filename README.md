# Industrial-Temp-Monitor

## Project Overview
An ESP32-based temperature and humidity monitoring system that publishes sensor data through MQTT and provides local emergency fan control using a pushbutton.

The project is implemented and demonstrated in the Wokwi ESP32 simulation environment.

## Circuit Overview
**Simulation:**[Open Wokwy Project](https://wokwi.com/projects/481018658309225473)

## Features

- Reads temperature and humidity from a DHT22 sensor every 2 seconds.
- Connects to Wi-Fi using the `Wokwi-GUEST` network.
- Publishes sensor readings to an MQTT broker in JSON format.
- Uses a pushbutton to toggle the emergency fan indicator LED.
- Uses a non-blocking `millis()`-based main control loop.
- Separates Wi-Fi and MQTT management into a FreeRTOS network task.
- Automatically attempts Wi-Fi and MQTT reconnection.

## Hardware Components

| Component | Quantity |
|---|---:|
| ESP32 DevKit | 1 |
| DHT22 temperature and humidity sensor | 1 |
| Pushbutton | 1 |
| LED (fan indicator) | 1 |
| 220 Ω resistor | 1 |
| 10 kΩ resistor | 1 |

## Pin Configuration

| Component | ESP32 Pin | Purpose |
|---|---|---|
| DHT22 DATA | GPIO 4 | Sensor data |
| Pushbutton | GPIO 27 | Emergency fan toggle input |
| Fan indicator LED | GPIO 26 | Fan ON/OFF output |

### Wiring Notes

- DHT22 VCC connects to 3V3 and GND connects to GND.
- DHT22 DATA connects to GPIO 4.
- A 10 kΩ pull-up resistor connects the DHT22 DATA line to 3V3.
- The pushbutton connects GPIO 27 to GND and uses the ESP32 internal pull-up.
- The LED connects to GPIO 26 through a 220 Ω current-limiting resistor, with its cathode connected to GND.

## Software and Libraries

- Arduino framework for ESP32
- `dhtESP32-rmt`
- `PubSubClient`

## Wi-Fi and MQTT Configuration

| Parameter | Value |
|---|---|
| Wi-Fi SSID | `Wokwi-GUEST` |
| MQTT Broker | `broker.hivemq.com` |
| MQTT Port | `1883` |

### Published MQTT Payload

Sensor readings are published as JSON:

```json
{
  "temperature": 24.00,
  "humidity": 40.00
}
```

## System Operation

1. The ESP32 starts the firmware and initializes the input and output pins.
2. The main loop checks the emergency button and updates the fan indicator LED when a debounced press is detected.
3. The DHT22 is read at 2-second intervals.
4. The latest successful sensor reading is shared with the network task.
5. The FreeRTOS network task manages Wi-Fi and MQTT connections.
6. When MQTT is connected, the latest available temperature and humidity values are published to the configured topic.

### Task Separation

- **Main loop:** Emergency button handling, fan LED control, and periodic DHT22 readings.
- **Network task:** Wi-Fi connection management, MQTT connection management, reconnection attempts, and sensor-data publishing.

## Simulation

The project was tested using the Wokwi ESP32 simulation environment.

Verified operations include:
- DHT22 temperature and humidity readings.
- Wi-Fi connection.
- MQTT connection and sensor-data publishing.
- Emergency fan indicator LED ON/OFF control through the pushbutton.


## Repository Structure

```text
MSTI_Arena-ASL-05/
├── sketch.ino
├── diagram.json
├── wokwi.toml
├── libraries.txt
├── README.md
├── LICENSE

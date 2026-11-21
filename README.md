# Soil-Moisture-RF-Attenuation
ESP32-based soil moisture sensing using RF attenuation and RSSI measurement.
# Sub-Terrestrial Soil Moisture Sensing via RF Attenuation

## Project Overview
This project demonstrates soil moisture sensing using RF signal attenuation and RSSI measurements with three ESP32 boards.

## Hardware Components
- 3 × ESP32 development boards
- 0.96-inch OLED display
- DS18B20 temperature sensor
- 4.7kΩ resistor
- Jumper wires
- Soil testing container

## Working Principle
1. ESP32 #1 transmits RF signals.
2. ESP32 #2 measures the reference signal strength through the air.
3. ESP32 #3 measures the signal strength through the soil path.
4. Both receivers send RSSI readings to ESP32 #1.
5. ESP32 #1 calculates the difference between reference RSSI and soil RSSI.
6. The readings are displayed on the OLED.

## ESP32 Code Files
- `ESP32_1_TX_OLED.ino` – Transmitter, OLED and temperature monitoring.
- `ESP32_2_Reference.ino` – Reference RF receiver.
- `ESP32_3_Soil_Receiver.ino` – Soil-path RF receiver.

## Technologies
- Arduino IDE
- ESP32
- ESP-NOW
- RSSI measurement
- OLED display
- DS18B20 temperature sensor

## Project Status
Prototype developed and tested. Further calibration is required to establish the relationship between RF attenuation and soil moisture percentage.

# Temperature Monitoring and Control System

## Overview
This project is a temperature monitoring and control system built with an ESP32 microcontroller. It uses an LM35 temperature sensor to continuously monitor ambient temperature and automatically controls a relay to manage connected equipment (like cooling or heating systems). The system includes an OLED display for real-time information, LED indicators, alarm functionality, and Blynk IoT integration for remote monitoring and control.

## Features
- **Real-time Temperature Monitoring**: Uses LM35 temperature sensor for accurate readings
- **Automatic Control**: Activates/deactivates a relay based on configurable temperature thresholds
- **Visual Feedback**:
  - OLED display showing current temperature, system status, and relay state
  - LED indicators for relay status (ON/OFF)
- **Audio Alerts**: Different buzzer patterns for various states (overheat, recovery, system toggle)
- **Manual Control**: Physical button to toggle system operation
- **Remote Monitoring and Control**: Blynk IoT platform integration allows monitoring and control from anywhere
- **Fail-safe Operation**: System can work offline if WiFi/Blynk connection is unavailable

## Hardware Requirements
- ESP32 development board
- LM35 temperature sensor
- 0.96" OLED display (SSD1306, I2C interface)
- 5V relay module
- Piezo buzzer
- 2 LEDs (for relay status indication)
- Push button
- Resistors and connecting wires
- 5V power supply

## Software Dependencies
- Arduino IDE
- Libraries:
  - `Wire.h` - For I2C communication
  - `Adafruit_GFX.h` - Graphics library for the OLED display
  - `Adafruit_SSD1306.h` - Display driver for the OLED
  - `WiFi.h` - For WiFi connectivity
  - `BlynkSimpleEsp32.h` - For Blynk IoT platform integration
  - `pitches.h` - For buzzer tones

## 🛠️ Hardware Setup
### Components
| Component       | ESP32 Pin |
|-----------------|-----------|
| LM35 Sensor     | GPIO34    |
| Relay           | GPIO5     |
| Button          | GPIO13    |
| Buzzer          | GPIO18    |
| Relay ON LED    | GPIO14    |
| Relay OFF LED   | GPIO12    |
| OLED Display    | I2C (0x3C)|

## Installation and Setup
1. Clone this repository or download the source code
2. Install required libraries through Arduino Library Manager
3. Create a Blynk account and set up a new project
4. Replace the following credentials in the code:
   ```cpp
   #define BLYNK_TEMPLATE_ID "TMPL3MCE-W-US"
   #define BLYNK_TEMPLATE_NAME "TempControl"
   #define BLYNK_AUTH_TOKEN "rpbJiYaLbjx23ra69y84uos5Xge_dRlS"
   
   char ssid[] = "your-wifi-ssid";
   char pass[] = "your-wifi-password";



### Wiring Diagram
```plaintext
[Insert ASCII art or link to wiring diagram image]

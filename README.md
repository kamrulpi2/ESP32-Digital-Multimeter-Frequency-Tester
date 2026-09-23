# ESP32 DigiMeter Pro  
### Digital Multimeter & Frequency Tester with OLED Display and WiFi Web Interface
<img width="1204" height="1600" alt="cfa0cde2-dd7e-414e-8813-1b733a121459" src="https://github.com/user-attachments/assets/73384720-baa4-4dd3-a822-ea049479bd00" />


![ESP32](https://img.shields.io/badge/Platform-ESP32-blue)
![Arduino](https://img.shields.io/badge/Framework-Arduino-green)
![OLED](https://img.shields.io/badge/Display-SSD1306-orange)


---

## 📌 Overview

ESP32 DigiMeter Pro is a multifunction electronic measurement device built using ESP32.

The project combines digital frequency measurement, component testing, voltage measurement, OLED visualization, and WiFi monitoring into a compact embedded system.

The device uses ESP32 hardware features including:

- Digital interrupts
- ADC measurement
- FreeRTOS task management
- I2C OLED display
- WiFi Web Server

---

# ✨ Features

## ⚡ Frequency Meter

High-speed digital frequency measurement using interrupt-based counting.

Features:

- Fast frequency update
- Noise filtering
- Automatic unit conversion
- Minimum / Maximum tracking
- Period calculation

Supported units:
100.00 Hz

1.000 kHz

5.000 MHz


---

## OLED Display

Display information:

- Current measurement value
- Minimum value
- Maximum value
- Signal status
- Active measurement mode

Supported display:
SSD1306 OLED
128x64
I2C
Address: 0x3C


---

# Measurement Modes

| Mode | Function |
|---|---|
| Frequency | Digital signal frequency measurement |
| Resistor | Resistance measurement |
| Capacitor | Capacitance measurement |
| Diode | Forward voltage test |
| LED | LED forward voltage test |
| Voltage | DC voltage measurement |

---

# 🔧 Hardware

## Controller

- ESP32 DevKit Board

---

## OLED Connection

| OLED | ESP32 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |

---

# Frequency Tester Connection

## Frequency Input


Recommended circuit:


---

## Internal Frequency Test

ESP32 generates a test signal:
Default:


Frequency: 1kHz
Duty Cycle: 50%


---

# Pin Configuration

| Function | GPIO |
|---|---|
| Frequency Input | GPIO27 |
| Frequency Test Output | GPIO18 |
| Resistor Test | GPIO35 |
| Capacitor Test | GPIO32 |
| Capacitor Charge | GPIO33 |
| Diode Test | GPIO25 |
| LED Test | GPIO26 |
| OLED SDA | GPIO21 |
| OLED SCL | GPIO22 |

---

# 📚 Required Libraries

Install from Arduino Library Manager:

ESP32 Built-in:


WiFi
WebServer
FreeRTOS

---

# 🚀 Installation

1. Install ESP32 board support in Arduino IDE.

2. Install required libraries.

frequency_tester.ino


4. Select board:


ESP32 Dev Module


5. Upload the firmware.

---

# 🖥 Serial Monitor

Baud Rate:


115200


Example:


Frequency Tester Ready

Frequency: 1.000 kHz
Frequency: 1.000 kHz
Frequency: 1.000 kHz


---

# 🌐 WiFi Feature

The device creates a WiFi Access Point:


SSID:
Tester

Password:
12341234


Open:


192.168.4.1


to view measurement data from a browser.

---

# 🧪 Testing Procedure

Recommended test order:

1. OLED display test
2. Frequency test (GPIO18 → GPIO27)
3. Voltage measurement
4. Resistor measurement
5. Capacitor measurement
6. Diode test
7. LED test

---

# ⚠️ Safety Notes

- Do not apply more than 3.3V directly to ESP32 GPIO pins.
- Use proper voltage divider for voltage measurement.
- Connect electrolytic capacitors with correct polarity.
- Use clean digital signals for frequency measurement.

---

# Future Improvements

- Higher frequency measurement range
- Auto ranging resistor measurement
- Improved capacitor accuracy
- Rotary encoder menu control
- Custom PCB design
- Battery powered version

---

# License

MIT License

Free to use, modify, and improve.

---

# Author

ESP32 DigiMeter Project

Built with ESP32 and Arduino

# ⚡ ESP32 IoT Smart Power Meter & Power Factor Correction System

An ESP32-based smart energy monitoring system capable of measuring electrical parameters in real time, calculating power factor correction requirements, and securely publishing data to a cloud MQTT broker using TLS encryption.

The project combines embedded systems, power electronics, IoT communication, cloud integration, and real-time electrical analysis to create a compact smart power monitoring solution suitable for industrial, educational, and energy-management applications.

---

## Features

* Real-time voltage monitoring
* Real-time current measurement
* Active power calculation
* Frequency measurement
* Power factor monitoring
* Automatic power factor correction calculations
* OLED-based local display
* Secure MQTT cloud communication (TLS/SSL)
* Wi-Fi connectivity
* NTP time synchronization
* Real-time data logging
* Capacitor and inductor sizing recommendations
* Fault detection and communication monitoring

---

## Hardware Components

| Component                       | Purpose                          |
| ------------------------------- | -------------------------------- |
| ESP32-WROOM-32                  | Main controller                  |
| PZEM-004T V1                    | Electrical parameter measurement |
| SSD1306 OLED Display            | Local monitoring interface       |
| Wi-Fi Module (Integrated ESP32) | Cloud connectivity               |
| Status LED                      | System diagnostics               |

---

## Measured Parameters

The system continuously measures:

* Voltage (V)
* Current (A)
* Active Power (W)
* Frequency (Hz)
* Power Factor (PF)

---

## Power Factor Correction Engine

The firmware analyzes the measured power factor and compares it against a configurable target value.

Target Power Factor:

```cpp
targetPF = 0.95
```

When the measured power factor falls below the target, the system automatically calculates:

* Required Reactive Power Compensation (VAR)
* Capacitor Value (µF)
* Inductor Value (H)

These recommendations help improve system efficiency and reduce reactive power losses.

---

## Cloud Connectivity

The ESP32 securely connects to an MQTT broker using:

* TLS/SSL Encryption
* X.509 Certificate Validation
* Username/Password Authentication

Published data includes:

* Timestamp
* Voltage
* Current
* Power
* Frequency
* Power Factor
* Required Correction Values

---

## OLED Dashboard

The OLED display provides:

* Voltage
* Current
* Power
* Frequency
* Power Factor
* Correction Status
* Required Capacitor Value
* Required Inductor Value

---

## System Architecture

1. ESP32 acquires electrical parameters from the PZEM-004T.
2. Electrical measurements are processed locally.
3. Power factor correction requirements are calculated.
4. Results are displayed on the OLED.
5. NTP synchronizes system time.
6. Data is securely transmitted to the MQTT cloud server.
7. Continuous monitoring and reporting occur in real time.

---

## Skills Demonstrated

* Embedded Systems Development
* ESP32 Programming
* Power Electronics
* Power System Analysis
* Power Factor Correction
* MQTT Communication
* TLS/SSL Security
* IoT Systems
* Sensor Integration
* Real-Time Data Processing
* Cloud Connectivity
* Electrical Measurement Systems

---

## Future Improvements

* Web Dashboard
* Historical Energy Analytics
* Energy Consumption Prediction
* Automatic Capacitor Bank Control
* Mobile Application Integration
* OTA Firmware Updates
* Industrial Modbus Support

---

## Author

**Saptarshi Das**

Developed as a practical exploration of smart energy monitoring, industrial automation concepts, IoT communication, and embedded system design.

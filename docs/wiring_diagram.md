# Electrical Wiring & Pinout Guide

## ESP32 to Peripherals Interconnection

| Subsystem | Signal | ESP32 GPIO | Description |
|---|---|---|---|
| **PZEM-004T v1** | RX (to Sensor TX) | **GPIO 16** | Hardware Serial2 RX (9600 baud, 8N1) |
| **PZEM-004T v1** | TX (to Sensor RX) | **GPIO 17** | Hardware Serial2 TX (9600 baud, 8N1) |
| **SSD1306 SPI OLED** | MOSI (Data) | **GPIO 21** | Hardware SPI Data |
| **SSD1306 SPI OLED** | SCK (Clock) | **GPIO 19** | Hardware SPI Clock |
| **SSD1306 SPI OLED** | DC (Data/Cmd) | **GPIO 18** | High = Data, Low = Command |
| **SSD1306 SPI OLED** | CS (Chip Select)| **GPIO 23** | Active Low device select |
| **SSD1306 SPI OLED** | RESET | **GPIO 5** | Active Low hardware reset |
| **Status LED** | Indication | **GPIO 2** | Active High diagnostic indicator |

## AC Mains High-Voltage Warnings
- PZEM-004T measures 80-260V AC. Ensure split-core current transformer (CT) is installed around a single phase conductor (not both live and neutral).
- Maintain optical isolation between high-voltage AC terminals and ESP32 low-voltage logic.

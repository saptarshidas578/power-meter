/**
 * ====================================================================================================
 * @file main.cpp
 * @brief ESP32 Smart IoT Power Meter & Automated Power Factor Correction Calculator
 * @author saptarshi2007 (https://github.com/saptarshidas578)
 * 
 * @details
 * Interfaces an ESP32 microcontroller with a PZEM-004T v1 energy sensor to measure AC electrical
 * parameters (RMS Voltage, Current, Active Power, Frequency, and Power Factor) in real time.
 * Calculates necessary reactive compensation (Capacitance in uF for inductive lag, Inductance in H
 * for capacitive lead) to attain target Power Factor (0.95). Renders live diagnostics on an SPI
 * SSD1306 OLED display and publishes encrypted JSON telemetry over MQTTS (TLS 8883) to EMQX cloud.
 * 
 * Hardware Peripherals:
 * - ESP32 DevKit V1 (Xtensa dual-core 32-bit LX6 @ 240 MHz)
 * - PZEM-004T v1 AC Digital Multimeter Sensor (UART2: RX=GPIO 16, TX=GPIO 17)
 * - SSD1306 128x64 SPI OLED Display: MOSI=21, CLK=19, DC=18, CS=23, RESET=5
 * - Status Indicator LED: GPIO 2
 * ====================================================================================================
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PZEM004TV1.h> 
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>


// --- Pin Definitions ---
// SPI OLED Pins (Matches your wiring)
#define OLED_MOSI   21  
#define OLED_CLK    19  
#define OLED_DC     18  
#define OLED_CS     23  
#define OLED_RESET   5  

// PZEM-004T v1 Serial Pins (Serial2)
#define PZEM_RX_PIN 16  
#define PZEM_TX_PIN 17  

#define LED_PIN 2
#define targetPF 0.95

// WiFi credentials
const char *ssid = "Verizon-SM-N981U-7692";             // your WiFi name
const char *password = "nzjf807(";     // your WiFi password

// MQTT Broker settings
const int mqtt_port = 8883;  // MQTT port (TLS)
const char *mqtt_broker = "ycb1c6bc.ala.asia-southeast1.emqxsl.com";  // EMQX broker endpoint
const char *mqtt_topic = "user01/esp32";       // MQTT topic (changed to esp32)
const char *mqtt_username = "user01_esp32";  // MQTT username for authentication
const char *mqtt_password = "jfhueHJ90845$%^&%ruiruysjkhqw3eyur3";  // MQTT password for authentication

// NTP Server settings
const char *ntp_server = "pool.ntp.org";     // Default NTP server
const char* time_zone = "IST-5:30";                      // India (no DST)

// SSL certificate for MQTT broker
static const char ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

// Function declarations
/**
 * @brief Connects ESP32 to 2.4 GHz Wi-Fi access point with live OLED status updates.
 * @details Retries connection at 500 ms intervals until connected, then reports local IP on OLED.
 */
void connectToWiFi();
/**
 * @brief Establishes secure TLS/SSL MQTTS session with cloud broker (EMQX).
 * @details Validates broker certificate using bundled DigiCert root CA over port 8883,
 *          subscribes to device telemetry topic, and publishes welcome handshake.
 */
void connectToMQTT();
/**
 * @brief Synchronizes ESP32 internal clock with pool.ntp.org over UDP.
 * @details Configures timezone offset for India Standard Time (IST UTC+5:30) without daylight savings.
 */
void syncTime();
String getTimeString();

// --- Object Initializations ---
// Using the Software SPI constructor for SSD1306
Adafruit_SSD1306 display(128, 64, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// PZEM004T V1 Object
PZEM004TV1 pzem(&Serial2, PZEM_RX_PIN, PZEM_TX_PIN);

// WiFi and MQTT client initialization for ESP32
WiFiClientSecure espClient;  
PubSubClient mqtt_client(espClient);

// Function to get time as HH:MM:SS string
String getTimeString() {
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Format: HH:MM:SS
    char timeString[9];  // HH:MM:SS = 8 chars + null terminator
    strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);
    
    return String(timeString);
}

//Function to connect to WIFI
/**
 * @brief Connects ESP32 to 2.4 GHz Wi-Fi access point with live OLED status updates.
 * @details Retries connection at 500 ms intervals until connected, then reports local IP on OLED.
 */
void connectToWiFi() {
    Serial.print("Connecting to WiFi");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("Connecting to WiFi");   display.display();
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    display.println("Connected to WiFi");
    display.println("IP Address: "); 
    display.println(WiFi.localIP().toString());   display.display();
    delay(1000);
}

//Configure ans syncronize time with automatic DST handling
/**
 * @brief Synchronizes ESP32 internal clock with pool.ntp.org over UDP.
 * @details Configures timezone offset for India Standard Time (IST UTC+5:30) without daylight savings.
 */
void syncTime() {

    // Configure time with automatic DST handling
    configTzTime(time_zone, ntp_server);
    Serial.print("Waiting for NTP time sync");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("NTP time sync ...");  display.display();

    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("\nTime synchronized");
    display.println("NTP syncronized");   display.display();

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.print("Current time: ");
        display.println("Current Time:");
        Serial.println(asctime(&timeinfo));
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%d/%m %H:%M", &timeinfo); 
        display.println(buffer);   display.display();
    } else {
        Serial.println("Failed to obtain local time");
        display.println("Time sync failed.");   display.display();
    }

    delay(1000);
}

//Setup MQTT connection and publish welcome message
/**
 * @brief Establishes secure TLS/SSL MQTTS session with cloud broker (EMQX).
 * @details Validates broker certificate using bundled DigiCert root CA over port 8883,
 *          subscribes to device telemetry topic, and publishes welcome handshake.
 */
void connectToMQTT() {
    // Configure SSL certificate
    espClient.setCACert(ca_cert);  // ESP32 method, different from ESP8266
    
    while (!mqtt_client.connected()) {
        String client_id = "esp32-client-" + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.println("Connecting to MQTT ");  display.display();

        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
            display.println("Connected to MQTT ");  display.display();

            // Subscribe to topic
            mqtt_client.subscribe(mqtt_topic);
            Serial.printf("Subscribed to topic: %s\n", mqtt_topic);
            display.println("Subscribed to MQTT ");  display.display();
            // Publish message upon successful connection
            if (mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP32 ^^")) {
                Serial.println("Welcome message published");
                display.println("Welcome message pub");  display.display();
            } else {
                Serial.println("Failed to publish welcome message");
                display.println("Message failed");  display.display();
            }
        } else {
            Serial.print("Failed to connect to MQTT broker, state: ");
            Serial.println(mqtt_client.state());
            display.println("Connect Failed");   display.display();
            
            // Additional debug info for ESP32
            int espClientState = espClient.connected();
            Serial.print("ESPClient connection state: ");
            Serial.println(espClientState);
            display.print("ESPClient state:"); display.println(espClientState);   display.display();
            
            delay(1000);
        }
    }
}

/**
 * @brief Serializes electrical sensor measurements into structured JSON payload.
 * @param voltage Measured RMS AC voltage in Volts (V).
 * @param current Measured AC load current in Amperes (A).
 * @param power Active power in Watts (W).
 * @param frequency Mains electrical frequency in Hertz (Hz).
 * @param powerFactor Ratio of real power to apparent power (0.000 to 1.000).
 * @param L_henry Calculated inductance required in Henrys (H) for capacitive correction.
 * @param C_uF Calculated capacitance required in microfarads (uF) for inductive correction.
 * @param Correct Boolean indicating whether power factor meets or exceeds target (0.95).
 * @return Formatted JSON string ready for MQTT transmission.
 */
String formatPZEMData(float voltage, float current, float power, 
                      float frequency, float powerFactor, 
                      float L_henry, float C_uF, bool Correct) {
    
    char buffer[128];  // Adjust size as needed
    
    sprintf(buffer, ",%.2f,%.3f,%.2f,%.1f,%.3f,%.5f,%.5f,%s",
            voltage,
            current,
            power,
            frequency,
            powerFactor,
            L_henry,
            C_uF,
            Correct ? "T" : "F");
    
    return String(buffer);
}


/**
 * @brief Hardware peripheral and communication initialization lifecycle hook.
 * @details Initializes Serial (115200), PZEM UART2 (9600), SPI SSD1306 OLED, status LED,
 *          connects to Wi-Fi, synchronizes NTP time, and connects to secure MQTT broker.
 */
void setup() {
  Serial.begin(115200);
  delay(1000); 
  Serial.println("PZEM-004T v1 Startup...");

  // 1. Initialize PZEM Serial (Mandatory 9600 baud)
  Serial2.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);

  // 2. Initialize Debug LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 3. Initialize OLED
  // The begin() call for SPI doesn't need an I2C address
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("OLED failed - Check MOSI/CLK wiring!"));
    for(;;); 
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("PZEM-004T v1");
  display.println("Connecting...");
  display.display();
  delay(1000);

  connectToWiFi();
  syncTime();  // X.509 validation requires synchronized time
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  connectToMQTT();
  delay(1000);
}

/**
 * @brief Primary superloop executed continuously on Core 1.
 * @details Polls PZEM-004T for voltage, current, power, frequency, and power factor.
 *          Computes required reactive compensation (VAR, uF, H) against target PF (0.95).
 *          Updates OLED graphic dashboard and publishes telemetry packet over secure MQTT.
 */
void loop() {
  // Read PZEM data
  float voltage = pzem.readVoltage();
  float current = pzem.readCurrent();
  float power   = pzem.readPower();
  float frequency = pzem.readFrequency();
  float powerFactor = pzem.readPowerFactor();
  bool Correct = false;

  float varNeeded = (powerFactor < targetPF && powerFactor > 0) ? 
                      power * (tan(acos(powerFactor)) - tan(acos(targetPF))) : 0;

  float L_Henrys = pow(voltage, 2) / (2 * PI * frequency * varNeeded);
  float C_uF = (varNeeded / (2 * PI * frequency * pow(voltage, 2))) * 1000000;

  // If voltage is NaN or 0, the communication is failing
  if (!isnan(voltage) && voltage > 0) {
    digitalWrite(LED_PIN, LOW); 

    Serial.print("V: "); Serial.print(voltage);
    Serial.print(" | A: "); Serial.print(current);
    Serial.print(" | W: "); Serial.println(power);
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("PF Corrector");
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
    display.setCursor(0, 15);
    display.setTextSize(0);
    display.print("V: "); display.print(voltage, 1); display.print(", A: "); display.println(current, 2);
    display.print("F: "); display.print(frequency, 0); display.print(", W: "); display.println(power, 0);
    display.print("PF: "); display.print(powerFactor, 3); 
    if ((powerFactor < targetPF) && (powerFactor != 0))
    {
      Correct = true;
      display.println(", Correct:T");
      String L_str = String(L_Henrys, 5); // Get string with 5 decimals
      L_str = L_str.substring(0, 5);
      display.print("RL->Cap|Add L:"); display.print(L_str); display.println("H");
      String C_str = String(C_uF, 5); // Get string with 5 decimals
      C_str = C_str.substring(0, 5);
      display.print("RL->Ind|Add C:"); display.print(C_str); display.print("uF"); 
    }
    else
    {
      Correct = false;
      display.println(" Correct:F");
    }
    display.display();
  }
  else {
    // If we get here, the Level Shifter or Wiring is likely the issue
    Serial.println("Communication Error: No data from PZEM");
    digitalWrite(LED_PIN, HIGH);
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("PZEM004T COMM ERROR");
    display.display();
  }
  
  if (!mqtt_client.connected()) {
        connectToMQTT();
    }
  mqtt_client.loop();

      // Create formatted string
    String dataString = formatPZEMData(voltage, current, power, 
                                        frequency, powerFactor,
                                        L_Henrys, C_uF, Correct);
    // Publish message continously
    String message = getTimeString() + dataString;

     if (mqtt_client.publish(mqtt_topic, message.c_str())) {
          Serial.println("Periodic message published " + getTimeString());
        } else {
            Serial.println("Failed to publish Periodic message");
        }


  delay(200); 
}
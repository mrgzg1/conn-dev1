#include <SPI.h>
#include <WiFiNINA.h>
#include "web_server.h"
#include "sensor.h"
#include "secrets.h"
#include "web_files.h"

WiFiServer server(80);

void printFileInfo(const char* name, const uint8_t* content, size_t length) {
    Serial.print("\nFile info for ");
    Serial.println(name);
    Serial.print("Content length: ");
    Serial.println(length);
    Serial.print("First 32 bytes: ");
    for(int i = 0; i < 32 && i < length; i++) {
        if(content[i] >= 32 && content[i] <= 126) {
            Serial.print((char)content[i]);
        } else {
            Serial.print("[");
            Serial.print(content[i], HEX);
            Serial.print("]");
        }
    }
    Serial.println();
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // Wait for serial port to connect
    delay(10);
  }
  
  // Initialize I2C
  Wire.begin();

  Serial.println("\n=== IMU Data Collection Starting ===");
    
  // Print stored file information
  printFileInfo("index.html", index_html, index_html_len);
  printFileInfo("react_app.js", react_app_js, react_app_js_len);
  printFileInfo("chart.js", chart_js, chart_js_len);
    
  // Print MAC address
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC Address: ");
  for (int i = 0; i < 6; i++) {
      if (mac[i] < 0x10) Serial.print("0");
      Serial.print(mac[i], HEX);
      if (i < 5) Serial.print(":");
  }
  Serial.println();

  // Initialize sensors
  if (!setupSensors()) {
    Serial.println("Failed to setup sensors! Check wiring.");
    while(1); // Stop if sensor setup fails
  }

  // Connect to WiFi with timeout
  unsigned long wifiStartTime = millis();
  const unsigned long WIFI_TIMEOUT = 20000; // 20 seconds timeout
  int attemptCount = 0;
  bool connected = false;
  
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    // Check if we've timed out
    if (millis() - wifiStartTime > WIFI_TIMEOUT) {
      Serial.println("WiFi connection timeout!");
      
      // Try alternative networks if available
      if (attemptCount < WIFI_NETWORKS_COUNT) {
        Serial.print("Trying alternative network: ");
        Serial.println(WIFI_NETWORKS[attemptCount][0]);
        WiFi.begin(WIFI_NETWORKS[attemptCount][0], WIFI_NETWORKS[attemptCount][1]);
        wifiStartTime = millis(); // Reset timeout for the new attempt
        attemptCount++;
      } else {
        Serial.println("Failed to connect to any WiFi network! Continuing without WiFi.");
        break;
      }
    }
    
    // Print a dot every second while connecting
    Serial.print(".");
    delay(1000);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    connected = true;
  } else {
    Serial.println("\nNo WiFi connection established. Operating in offline mode.");
  }

  // Only start the server if WiFi is connected
  if (connected) {
    server.begin();
    Serial.println("Web server started");
  } else {
    Serial.println("Web server not started - no WiFi connection");
  }
}

// Status reporting variables
unsigned long lastStatusTime = 0;
const unsigned long STATUS_INTERVAL = 10000; // 10 seconds

void loop() {
  // Update all sensor readings
  updateSensors();

  // Handle any incoming client connections if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client = server.available();
    if (client) {
      handleClient(client);
    }
  }
  
  // Periodic status report
  unsigned long currentTime = millis();
  if (currentTime - lastStatusTime > STATUS_INTERVAL) {
    Serial.println("\n==== Status Update ====");
    Serial.print("Uptime: ");
    Serial.print(currentTime / 1000);
    Serial.println(" seconds");
    
    // Print WiFi status
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("WiFi Status: Connected, IP: ");
      Serial.println(WiFi.localIP());
      
      Serial.print("Signal Strength: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
    } else {
      Serial.println("WiFi Status: Disconnected, operating in offline mode");
    }
    
    // Print sample sensor data
    Serial.println("Sample sensor readings:");
    Serial.print("- IMU temperature: ");
    float temp = 0;
    if (IMU.temperatureAvailable()) {
      IMU.readTemperature(temp);
      Serial.print(temp);
      Serial.println(" °C");
    } else {
      Serial.println("N/A");
    }
    
    // Memory info not available on all boards
    Serial.println("Active and monitoring...");
    
    lastStatusTime = currentTime;
  }
  
  // Add a delay based on sensor configuration
  delay(SENSOR_UPDATE_INTERVAL);
}

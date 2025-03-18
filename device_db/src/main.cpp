#include <SPI.h>
#include <WiFiNINA.h>
#include "web_server.h"
#include "sensor.h"
#include "led_control.h"
#include "secrets.h"
#include "web_files.h"
#include "../include/storage_api.h"

WiFiServer server(80);

void printFileInfo(const char* name, const char* content) {
    Serial.print("\nFile info for ");
    Serial.println(name);
    Serial.print("Content length: ");
    Serial.println(strlen(content));
    Serial.print("First 32 bytes: ");
    for(int i = 0; i < 32 && content[i]; i++) {
        if(content[i] >= 32 && content[i] <= 126) {
            Serial.print((char)content[i]);
        } else {
            Serial.print("[");
            Serial.print((int)content[i], HEX);
            Serial.print("]");
        }
    }
    Serial.println();
}

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
  while (!Serial) {}

  Serial.println("\n=== WiFi Web Server with KVStore Storage Starting ===");
    
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

  // Initialize storage system
  Serial.println("\nInitializing storage system...");
  if (!storage_init()) {
    Serial.println("Storage initialization failed!");
    while (1); // Stop if storage init fails
  }
  Serial.println("Storage system initialized successfully");
  
  // Print storage summary
  storage_print_summary();
  
  // Test storage functionality
  Serial.println("\nTesting storage functionality...");
  SensorReading testReading;
  testReading.timestamp = millis();
  testReading.temperature = 22.5;
  testReading.humidity = 45.2;
  testReading.pressure = 1013.4;
  testReading.accel_x = 0.1;
  testReading.accel_y = 0.2;
  testReading.accel_z = 9.8;
  strcpy(testReading.label, "Test Reading");
  
  if (storage_store_reading(testReading)) {
    Serial.println("Test reading stored successfully!");
  } else {
    Serial.println("Failed to store test reading");
  }
  
  // Print updated storage summary
  storage_print_summary();

  setupSensor();
  setupLEDs();

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    delay(5000);
  }

  Serial.println("Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("Server started");
}

// Variables for periodic storage
unsigned long lastStorageTime = 0;
const unsigned long STORAGE_INTERVAL = 60000; // Store data every 1 minute

void loop() {
  updateSensor(); // Read sensor data

  // Handle any web client connections
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }
  
  // Periodically store sensor data
  unsigned long currentTime = millis();
  if (currentTime - lastStorageTime > STORAGE_INTERVAL) {
    lastStorageTime = currentTime;
    
    // Create a reading from the current sensor data
    SensorReading reading;
    reading.timestamp = currentTime;
    reading.temperature = getSensorValue("temperature");
    reading.humidity = getSensorValue("humidity");
    reading.pressure = getSensorValue("pressure");
    reading.accel_x = getSensorValue("accel_x");
    reading.accel_y = getSensorValue("accel_y");
    reading.accel_z = getSensorValue("accel_z");
    strcpy(reading.label, "Periodic Reading");
    
    // Store the reading
    bool success = storage_store_reading(reading);
    if (success) {
      Serial.print("Stored periodic reading at ");
      Serial.print(currentTime);
      Serial.print(" - Temp: ");
      Serial.print(reading.temperature);
      Serial.println("°C");
      
      // Every 10 readings, print a storage summary
      uint32_t count = storage_get_reading_count();
      if (count % 10 == 0) {
        storage_print_summary();
      }
    } else {
      Serial.println("Failed to store periodic reading");
    }
  }
}

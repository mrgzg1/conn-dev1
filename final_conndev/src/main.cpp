#include <Arduino.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include "../include/sensor.h"
#include "../include/storage_api.h"
#include "../include/web_server.h"
#include "../include/secrets.h"

// Global variables
SensorManager sensorManager;
int wifiMode = WIFI_MODE_AP;
WiFiServer webServer(80);
bool wifiConnected = false;
unsigned long lastStorageCheckTime = 0;

// Function prototypes
void setupSensors();
void setupStorage();
void pollSensors();

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(2000);  // Give serial monitor time to connect
  
  Serial.println("\n\n=============================");
  Serial.println(" Connected Device Platform");
  Serial.println("=============================");
  
  // Initialize storage system
  setupStorage();
  
  // Initialize sensors
  setupSensors();
  
  // Initialize WiFi - defaults to AP mode
  // To use client mode, change WIFI_MODE_AP to WIFI_MODE_CLIENT
  initWiFi(WIFI_MODE_AP);
  
  Serial.println("Setup complete!");
}

void loop() {
  // Poll sensors for new data
  pollSensors();
  
  // Handle web server client connections
  handleWebServer();
  
  // Check if any buffered storage data needs to be flushed
  unsigned long currentTime = millis();
  if (currentTime - lastStorageCheckTime >= 5000) {  // Every 5 seconds
    storage_check_flush();
    lastStorageCheckTime = currentTime;
  }
  
  // Small delay to prevent watchdog resets
  delay(10);
}

void setupSensors() {
  Serial.println("Initializing sensors...");
  
  // Create and initialize IMU sensor
  LSM6DS3Sensor* imuSensor = new LSM6DS3Sensor();
  if (sensorManager.addSensor(imuSensor)) {
    Serial.println("IMU sensor initialized successfully");
  } else {
    Serial.println("Failed to initialize IMU sensor");
  }
  
  // Add more sensors here if needed
  
  Serial.println("Sensor initialization complete");
}

void setupStorage() {
  Serial.println("Initializing storage system...");
  
  if (storage_init()) {
    Serial.println("Storage system initialized successfully");
    
    // Configure auto-flush behavior (every 60 seconds)
    storage_set_auto_flush(true, 60000);
    
    // Display storage summary
    storage_print_summary();
  } else {
    Serial.println("Failed to initialize storage system");
  }
}

void pollSensors() {
  // Update all sensors
  sensorManager.updateAll();
}
#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <LittleFS.h>
#include <Arduino.h>

// Configuration structure - modify this to match your storage needs
struct Config {
  int calibrationValue;
  float sensorOffset[3];
  bool ledState[3];
  char deviceName[32];
  // Add any other settings you want to persist
};

class FlashStorage {
private:
  bool initialized = false;
  const char* configFilePath = "/config.bin";
  Config currentConfig;
  
  // Default configuration
  void setDefaults() {
    currentConfig.calibrationValue = 0;
    currentConfig.sensorOffset[0] = 0.0f;
    currentConfig.sensorOffset[1] = 0.0f;
    currentConfig.sensorOffset[2] = 0.0f;
    currentConfig.ledState[0] = false;
    currentConfig.ledState[1] = false;
    currentConfig.ledState[2] = false;
    strncpy(currentConfig.deviceName, "RP2040 IMU Sensor", sizeof(currentConfig.deviceName));
  }

public:
  FlashStorage() {}
  
  bool begin() {
    // Initialize the LittleFS filesystem
    if (!LittleFS.begin()) {
      Serial.println("Failed to mount LittleFS filesystem!");
      return false;
    }
    
    initialized = true;
    Serial.println("LittleFS filesystem mounted successfully");
    
    // Try to load the configuration
    if (!loadConfig()) {
      Serial.println("No valid configuration found, using defaults");
      setDefaults();
      saveConfig(); // Save the defaults to create the file
    }
    
    return true;
  }
  
  bool saveConfig() {
    if (!initialized) return false;
    
    File configFile = LittleFS.open(configFilePath, "w");
    if (!configFile) {
      Serial.println("Failed to open config file for writing");
      return false;
    }
    
    size_t bytesWritten = configFile.write((uint8_t*)&currentConfig, sizeof(Config));
    configFile.close();
    
    if (bytesWritten != sizeof(Config)) {
      Serial.println("Failed to write config file");
      return false;
    }
    
    Serial.println("Configuration saved successfully");
    return true;
  }
  
  bool loadConfig() {
    if (!initialized) return false;
    
    if (!LittleFS.exists(configFilePath)) {
      Serial.println("Config file does not exist");
      return false;
    }
    
    File configFile = LittleFS.open(configFilePath, "r");
    if (!configFile) {
      Serial.println("Failed to open config file for reading");
      return false;
    }
    
    size_t bytesRead = configFile.read((uint8_t*)&currentConfig, sizeof(Config));
    configFile.close();
    
    if (bytesRead != sizeof(Config)) {
      Serial.println("Failed to read config file");
      return false;
    }
    
    Serial.println("Configuration loaded successfully");
    return true;
  }
  
  Config& getConfig() {
    return currentConfig;
  }
  
  // Save sensor data to a file
  bool saveSensorData(float accelX[], float accelY[], float accelZ[], 
                     float gyroX[], float gyroY[], float gyroZ[],
                     float temperature[], unsigned long timestamps[], 
                     int count) {
    if (!initialized) return false;
    
    // Create a timestamped filename
    char filename[32];
    snprintf(filename, sizeof(filename), "/data_%lu.bin", millis());
    
    File dataFile = LittleFS.open(filename, "w");
    if (!dataFile) {
      Serial.println("Failed to open data file for writing");
      return false;
    }
    
    // Write header with count
    dataFile.write((uint8_t*)&count, sizeof(count));
    
    // Write all arrays
    for (int i = 0; i < count; i++) {
      dataFile.write((uint8_t*)&accelX[i], sizeof(float));
      dataFile.write((uint8_t*)&accelY[i], sizeof(float));
      dataFile.write((uint8_t*)&accelZ[i], sizeof(float));
      dataFile.write((uint8_t*)&gyroX[i], sizeof(float));
      dataFile.write((uint8_t*)&gyroY[i], sizeof(float));
      dataFile.write((uint8_t*)&gyroZ[i], sizeof(float));
      dataFile.write((uint8_t*)&temperature[i], sizeof(float));
      dataFile.write((uint8_t*)&timestamps[i], sizeof(unsigned long));
    }
    
    dataFile.close();
    Serial.print("Saved data to file: ");
    Serial.println(filename);
    return true;
  }
  
  // List all files in the filesystem
  void listFiles() {
    if (!initialized) {
      Serial.println("Filesystem not initialized");
      return;
    }
    
    File root = LittleFS.open("/", "r");
    if (!root || !root.isDirectory()) {
      Serial.println("Failed to open root directory");
      return;
    }
    
    Serial.println("\nFiles in filesystem:");
    File file = root.openNextFile();
    while (file) {
      Serial.print("  ");
      Serial.print(file.name());
      Serial.print("  (");
      Serial.print(file.size());
      Serial.println(" bytes)");
      file = root.openNextFile();
    }
    
    // Print filesystem info
    FSInfo fs_info;
    LittleFS.info(fs_info);
    Serial.print("Total space: ");
    Serial.print(fs_info.totalBytes);
    Serial.println(" bytes");
    Serial.print("Used space: ");
    Serial.print(fs_info.usedBytes);
    Serial.print(" bytes (");
    Serial.print(fs_info.usedBytes * 100.0 / fs_info.totalBytes, 1);
    Serial.println("%)");
  }
};

#endif 
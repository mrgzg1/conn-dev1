#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <Arduino_LSM6DS3.h>
#include <ArduinoJson.h>
#include "storage_api.h"

// Sample rate for sensor readings
#define IMU_SAMPLE_RATE_MS 100
#define STORAGE_INTERVAL_MS 5000

// Define the sensor type identifiers
enum SensorIdentifier {
  SENSOR_ID_LSM6DS3 = 1,
  SENSOR_ID_CUSTOM = 99
};

// Base sensor class
class Sensor {
protected:
  bool initialized = false;
  uint32_t sampleRateMs;
  uint32_t lastSampleTime = 0;
  uint32_t sensorId;
  String sensorName;
  String apiEndpoint;
  
  // Circular buffer for history
  static const int MAX_HISTORY = 100; // Buffer size for history data
  DynamicJsonDocument* historyData[MAX_HISTORY] = { nullptr };
  int historyIndex = 0;
  int historyCount = 0;
  
public:
  Sensor(uint32_t id, String name, String endpoint, uint32_t rate) 
    : sensorId(id), sensorName(name), apiEndpoint(endpoint), sampleRateMs(rate) {}
    
  virtual ~Sensor() {}
  
  // Initialize the sensor
  virtual bool begin() = 0;
  
  // Update sensor readings (called in main loop)
  virtual bool update() = 0;
  
  // Get sensor information
  String getName() const { return sensorName; }
  String getApiEndpoint() const { return apiEndpoint; }
  uint32_t getSensorId() const { return sensorId; }
  bool isInitialized() const { return initialized; }
  
  // Serialize sensor data to JSON
  virtual void serializeCurrentData(JsonObject& json) = 0;
  
  // Save data to flash storage
  virtual bool saveToStorage() = 0;
  
  // Serialize history data to JSON array
  virtual void serializeHistoryData(JsonArray& array) {
    int count = min(historyCount, MAX_HISTORY);
    for (int i = 0; i < count; i++) {
      int idx = (historyIndex - count + i + MAX_HISTORY) % MAX_HISTORY;
      
      // Skip if there's no document at this index
      if (!historyData[idx]) continue;
      
      JsonObject entry = array.createNestedObject();
      
      // Manually copy properties instead of using serializeJson
      JsonObject srcObj = historyData[idx]->as<JsonObject>();
      for (JsonPair kv : srcObj) {
        entry[kv.key()] = kv.value();
      }
    }
  }
};

// LSM6DS3 IMU sensor implementation
class LSM6DS3Sensor : public Sensor {
private:
  float accelX, accelY, accelZ;
  float gyroX, gyroY, gyroZ;
  float temperature;
  
public:
  LSM6DS3Sensor() 
    : Sensor(SENSOR_ID_LSM6DS3, "IMU Sensor", "imu", IMU_SAMPLE_RATE_MS) {}
    
  bool begin() override {
    if (!IMU.begin()) {
      Serial.println("Failed to initialize IMU!");
      return false;
    }
    
    Serial.println("LSM6DS3 IMU initialized");
    Serial.print("Accelerometer sample rate = ");
    Serial.print(IMU.accelerationSampleRate());
    Serial.println(" Hz");
    Serial.print("Gyroscope sample rate = ");
    Serial.print(IMU.gyroscopeSampleRate());
    Serial.println(" Hz");
    
    initialized = true;
    return true;
  }
  
  bool update() override {
    if (!initialized) return false;
    
    uint32_t currentTime = millis();
    if (currentTime - lastSampleTime < sampleRateMs) {
      return false; // Not time to sample yet
    }
    
    // Read IMU data
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      IMU.readAcceleration(accelX, accelY, accelZ);
      IMU.readGyroscope(gyroX, gyroY, gyroZ);
      
      // Temperature is not directly available in the Arduino LSM6DS3 library
      // We'll simulate it for now (could use another sensor if available)
      temperature = 25.0 + sin(currentTime / 10000.0) * 2.0;  // Simulate temperature
      
      // Add to history buffer - create a new document if needed
      if (!historyData[historyIndex]) {
        historyData[historyIndex] = new DynamicJsonDocument(1024);
      }
      
      JsonObject entry = historyData[historyIndex]->to<JsonObject>();
      entry["timestamp"] = currentTime;
      JsonObject accel = entry["accel"].to<JsonObject>();
      accel["x"] = accelX;
      accel["y"] = accelY;
      accel["z"] = accelZ;
      JsonObject gyro = entry["gyro"].to<JsonObject>();
      gyro["x"] = gyroX;
      gyro["y"] = gyroY;
      gyro["z"] = gyroZ;
      entry["temperature"] = temperature;
      
      // Update history buffer
      historyIndex = (historyIndex + 1) % MAX_HISTORY;
      if (historyCount < MAX_HISTORY) historyCount++;
      
      lastSampleTime = currentTime;
      return true;
    }
    
    return false;
  }
  
  void serializeCurrentData(JsonObject& json) override {
    json["timestamp"] = millis();
    JsonObject accel = json["accel"].to<JsonObject>();
    accel["x"] = accelX;
    accel["y"] = accelY;
    accel["z"] = accelZ;
    JsonObject gyro = json["gyro"].to<JsonObject>();
    gyro["x"] = gyroX;
    gyro["y"] = gyroY;
    gyro["z"] = gyroZ;
    json["temperature"] = temperature;
  }
  
  bool saveToStorage() override {
    if (!initialized) return false;
    
    // Create storage data point
    SensorDataPoint dataPoint;
    dataPoint.sensorId = sensorId;
    dataPoint.timestamp = millis();
    dataPoint.values[0] = accelX;
    dataPoint.values[1] = accelY;
    dataPoint.values[2] = accelZ;
    dataPoint.values[3] = gyroX;
    dataPoint.values[4] = gyroY;
    dataPoint.values[5] = gyroZ;
    dataPoint.values[6] = temperature;
    dataPoint.valueCount = 7;
    
    // Set label
    snprintf(dataPoint.label, sizeof(dataPoint.label), "IMU");
    
    // Store in flash
    return storage_store_reading(dataPoint, SENSOR_IMU_COMBINED);
  }
};

// Sensor Manager class
class SensorManager {
private:
  static const int MAX_SENSORS = 5;
  Sensor* sensors[MAX_SENSORS] = {nullptr};
  int sensorCount = 0;
  uint32_t lastStorageTime = 0;
  
public:
  SensorManager() {}
  
  ~SensorManager() {
    // Clean up sensors
    for (int i = 0; i < sensorCount; i++) {
      if (sensors[i]) {
        delete sensors[i];
      }
    }
  }
  
  bool addSensor(Sensor* sensor) {
    if (sensorCount >= MAX_SENSORS) {
      return false;
    }
    
    if (sensor && sensor->begin()) {
      sensors[sensorCount++] = sensor;
      return true;
    }
    
    return false;
  }
  
  void updateAll() {
    for (int i = 0; i < sensorCount; i++) {
      if (sensors[i]) {
        sensors[i]->update();
      }
    }
    
    // Save to storage periodically
    uint32_t currentTime = millis();
    if (currentTime - lastStorageTime >= STORAGE_INTERVAL_MS) {
      for (int i = 0; i < sensorCount; i++) {
        if (sensors[i]) {
          sensors[i]->saveToStorage();
        }
      }
      lastStorageTime = currentTime;
    }
  }
  
  int getSensorCount() const {
    return sensorCount;
  }
  
  Sensor* getSensor(int index) {
    if (index >= 0 && index < sensorCount) {
      return sensors[index];
    }
    return nullptr;
  }
  
  // Serialize all current data
  void serializeAllCurrentData(JsonObject& json) {
    for (int i = 0; i < sensorCount; i++) {
      if (sensors[i]) {
        sensors[i]->serializeCurrentData(json);
      }
    }
  }
  
  // Serialize all history data
  void serializeAllHistoryData(JsonArray& array) {
    for (int i = 0; i < sensorCount; i++) {
      if (sensors[i]) {
        sensors[i]->serializeHistoryData(array);
      }
    }
  }
};

// Global instance
extern SensorManager sensorManager;

#endif // SENSOR_H
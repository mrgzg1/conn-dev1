#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino_LSM6DS3.h>
#include <ArduinoJson.h>
#include "sensor_config.h"

// Forward declaration for the SensorManager
class SensorManager;

// Base Sensor interface
class Sensor {
public:
  virtual bool setup() = 0;
  virtual void update() = 0;
  virtual void serializeCurrentData(JsonObject& json) = 0;
  virtual void serializeHistoryData(JsonArray& array) = 0;
  virtual ~Sensor() {}
};

// Vector3D structure to hold 3D data (acceleration, gyroscope)
struct Vector3D {
  float x = 0;
  float y = 0;
  float z = 0;
  
  void toJson(JsonObject& json) const {
    json["x"] = x;
    json["y"] = y;
    json["z"] = z;
  }
};

// IMUSensor for LSM6DS3
class IMUSensor : public Sensor {
public:
  IMUSensor(int bufferSize = SENSOR_BUFFER_SIZE) : bufferSize(bufferSize) {
    // Allocate memory for buffers
    accelBuffer = new Vector3D[bufferSize];
    gyroBuffer = new Vector3D[bufferSize];
    tempBuffer = new float[bufferSize];
    timestampBuffer = new unsigned long[bufferSize];
    
    // Initialize buffers
    for (int i = 0; i < bufferSize; i++) {
      timestampBuffer[i] = 0;
    }
  }
  
  ~IMUSensor() {
    delete[] accelBuffer;
    delete[] gyroBuffer;
    delete[] tempBuffer;
    delete[] timestampBuffer;
  }
  
  bool setup() override {
    if (!IMU.begin()) {
      Serial.println("Failed to initialize IMU!");
      return false;
    }
    
    Serial.println("IMU initialized successfully");
    Serial.print("Accelerometer sample rate = ");
    Serial.print(IMU.accelerationSampleRate());
    Serial.println(" Hz");
    Serial.print("Gyroscope sample rate = ");
    Serial.print(IMU.gyroscopeSampleRate());
    Serial.println(" Hz");
    
    return true;
  }
  
  void update() override {
    // Read accelerometer data if available
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(currentAccel.x, currentAccel.y, currentAccel.z);
    }
    
    // Read gyroscope data if available
    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(currentGyro.x, currentGyro.y, currentGyro.z);
    }
    
    // Read temperature data if available
    if (IMU.temperatureAvailable()) {
      IMU.readTemperature(currentTemp);
    }
    
    // Record timestamp and update buffer
    currentTimestamp = millis();
    
    // Store in circular buffer
    accelBuffer[bufferIndex] = currentAccel;
    gyroBuffer[bufferIndex] = currentGyro;
    tempBuffer[bufferIndex] = currentTemp;
    timestampBuffer[bufferIndex] = currentTimestamp;
    
    // Increment buffer index (circular buffer)
    bufferIndex = (bufferIndex + 1) % bufferSize;
  }
  
  void serializeCurrentData(JsonObject& json) override {
    json["timestamp"] = currentTimestamp;
    
    JsonObject accel = json.createNestedObject("accel");
    currentAccel.toJson(accel);
    
    JsonObject gyro = json.createNestedObject("gyro");
    currentGyro.toJson(gyro);
    
    json["temperature"] = currentTemp;
  }
  
  void serializeHistoryData(JsonArray& array) override {
    // Start from the current position in the circular buffer
    int currentPos = bufferIndex;
    
    // Iterate through the buffer and output each entry
    for (int i = 0; i < bufferSize; i++) {
      // Calculate the actual index, accounting for the circular nature
      int idx = (currentPos - bufferSize + i) % bufferSize;
      if (idx < 0) idx += bufferSize;
      
      // Only output entries with valid timestamps
      if (timestampBuffer[idx] > 0) {
        JsonObject entry = array.createNestedObject();
        
        entry["timestamp"] = timestampBuffer[idx];
        
        JsonObject accel = entry.createNestedObject("accel");
        accel["x"] = accelBuffer[idx].x;
        accel["y"] = accelBuffer[idx].y;
        accel["z"] = accelBuffer[idx].z;
        
        JsonObject gyro = entry.createNestedObject("gyro");
        gyro["x"] = gyroBuffer[idx].x;
        gyro["y"] = gyroBuffer[idx].y;
        gyro["z"] = gyroBuffer[idx].z;
        
        entry["temperature"] = tempBuffer[idx];
      }
    }
  }
  
private:
  int bufferSize;
  int bufferIndex = 0;
  
  // Current readings
  Vector3D currentAccel;
  Vector3D currentGyro;
  float currentTemp = 0;
  unsigned long currentTimestamp = 0;
  
  // Circular buffers
  Vector3D* accelBuffer;
  Vector3D* gyroBuffer;
  float* tempBuffer;
  unsigned long* timestampBuffer;
};

// SensorManager to handle multiple sensors
class SensorManager {
public:
  SensorManager(int maxSensors = 5) : maxSensors(maxSensors) {
    sensors = new Sensor*[maxSensors];
    for (int i = 0; i < maxSensors; i++) {
      sensors[i] = nullptr;
    }
  }
  
  ~SensorManager() {
    for (int i = 0; i < sensorCount; i++) {
      if (sensors[i]) {
        delete sensors[i];
      }
    }
    delete[] sensors;
  }
  
  bool addSensor(Sensor* sensor) {
    if (sensorCount >= maxSensors) {
      return false;
    }
    
    sensors[sensorCount++] = sensor;
    return true;
  }
  
  bool setup() {
    bool success = true;
    for (int i = 0; i < sensorCount; i++) {
      if (sensors[i] && !sensors[i]->setup()) {
        success = false;
      }
    }
    return success;
  }
  
  void update() {
    for (int i = 0; i < sensorCount; i++) {
      if (sensors[i]) {
        sensors[i]->update();
      }
    }
  }
  
  void serializeAllCurrentData(JsonObject& json) {
    for (int i = 0; i < sensorCount; i++) {
      if (sensors[i]) {
        // For now, we assume only one sensor, but this can be extended
        // to support multiple sensors by creating nested objects
        sensors[i]->serializeCurrentData(json);
      }
    }
  }
  
  void serializeAllHistoryData(JsonArray& array) {
    for (int i = 0; i < sensorCount; i++) {
      if (sensors[i]) {
        // This assumes we want one big array with all sensor data
        // Could be modified to have nested arrays per sensor
        sensors[i]->serializeHistoryData(array);
      }
    }
  }
  
  Sensor* getSensor(int index) {
    if (index >= 0 && index < sensorCount) {
      return sensors[index];
    }
    return nullptr;
  }
  
  int getSensorCount() const {
    return sensorCount;
  }
  
private:
  Sensor** sensors;
  int maxSensors;
  int sensorCount = 0;
};

// Global sensor manager instance
SensorManager sensorManager;

// Global setup function
bool setupSensors() {
  // Create sensors based on configuration
  for (int i = 0; i < NUM_AVAILABLE_SENSORS; i++) {
    const SensorConfig& config = AVAILABLE_SENSORS[i];
    
    // Skip disabled sensors
    if (!config.enabled) continue;
    
    // Create and add the appropriate sensor type
    Sensor* sensor = nullptr;
    
    switch (config.type) {
      case SENSOR_TYPE_IMU:
        sensor = new IMUSensor(config.bufferSize);
        break;
        
      // Add more sensor types here as needed
      // case SENSOR_TYPE_HUMIDITY:
      //   sensor = new HumiditySensor(config.bufferSize);
      //   break;
        
      default:
        Serial.print("Unknown sensor type: ");
        Serial.println(config.type);
        continue;
    }
    
    // Add the sensor to the manager
    if (sensor) {
      Serial.print("Adding sensor: ");
      Serial.println(config.name);
      sensorManager.addSensor(sensor);
    }
  }
  
  // Setup all sensors
  return sensorManager.setup();
}

// Global update function
void updateSensors() {
  sensorManager.update();
}

#endif

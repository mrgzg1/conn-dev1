#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino_LSM6DS3.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <BME280I2C.h>
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
      
      // Log every 100th reading to avoid flooding serial
      if (updateCount % 100 == 0) {
        Serial.print("Accel: x=");
        Serial.print(currentAccel.x);
        Serial.print(" y=");
        Serial.print(currentAccel.y);
        Serial.print(" z=");
        Serial.println(currentAccel.z);
      }
    }
    
    // Read gyroscope data if available
    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(currentGyro.x, currentGyro.y, currentGyro.z);
      
      // Log every 100th reading
      if (updateCount % 100 == 0) {
        Serial.print("Gyro: x=");
        Serial.print(currentGyro.x);
        Serial.print(" y=");
        Serial.print(currentGyro.y);
        Serial.print(" z=");
        Serial.println(currentGyro.z);
      }
    }
    
    // Read temperature data if available
    if (IMU.temperatureAvailable()) {
      IMU.readTemperature(currentTemp);
      
      // Log every 100th reading
      if (updateCount % 100 == 0) {
        Serial.print("Temp: ");
        Serial.println(currentTemp);
      }
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
    updateCount++;
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
  unsigned long updateCount = 0;
  
  // Circular buffers
  Vector3D* accelBuffer;
  Vector3D* gyroBuffer;
  float* tempBuffer;
  unsigned long* timestampBuffer;
};

// BME280 Environmental Sensor
class BME280Sensor : public Sensor {
public:
  BME280Sensor(int bufferSize = SENSOR_BUFFER_SIZE) : bufferSize(bufferSize) {
    // Allocate memory for buffers
    tempBuffer = new float[bufferSize];
    humBuffer = new float[bufferSize];
    presBuffer = new float[bufferSize];
    timestampBuffer = new unsigned long[bufferSize];
    
    // Initialize buffers
    for (int i = 0; i < bufferSize; i++) {
      timestampBuffer[i] = 0;
    }
  }
  
  ~BME280Sensor() {
    delete[] tempBuffer;
    delete[] humBuffer;
    delete[] presBuffer;
    delete[] timestampBuffer;
  }
  
  bool setup() override {
    // Wire.begin() is already called in main.cpp, don't call it again
    
    // Initialize BME280 sensor with explicit I2C address (0x76 or 0x77 are common)
    // Try both common addresses with a timeout
    unsigned long startTime = millis();
    bool sensorFound = false;
    
    // Define a 5 second timeout
    const unsigned long TIMEOUT = 5000;
    
    // Try to initialize BME280 sensor
    // Note: BME280I2C library doesn't accept address parameter in begin()
    while (millis() - startTime < TIMEOUT && !sensorFound) {
      Serial.println("Trying to initialize BME280...");
      if (bme.begin()) {
        sensorFound = true;
        sensorInitialized = true;
        Serial.println("BME280 initialized successfully");
      } else {
        Serial.println("Could not initialize BME280 sensor");
        delay(500);
      }
    }
    
    if (!sensorFound) {
      Serial.println("BME280 sensor not found after timeout! Continuing without environmental data.");
      sensorInitialized = false;
      return false;
    }
    
    // Report chip model
    switch(bme.chipModel()) {
      case BME280::ChipModel_BME280:
        Serial.println("Found BME280 sensor! Success.");
        break;
      case BME280::ChipModel_BMP280:
        Serial.println("Found BMP280 sensor! No Humidity available.");
        break;
      default:
        Serial.println("Found UNKNOWN sensor! Error!");
        sensorInitialized = false;
        return false;
    }
    
    return true;
  }
  
  void update() override {
    // Only try to read if the sensor was successfully initialized
    if (sensorInitialized) {
      // Read BME280 values
      bme.read(currentPres, currentTemp, currentHum, BME280::TempUnit_Celsius, BME280::PresUnit_Pa);
      
      // Log every 100th reading to avoid flooding serial
      if (updateCount % 100 == 0) {
        Serial.print("BME280: Temp=");
        Serial.print(currentTemp);
        Serial.print("°C, Humidity=");
        Serial.print(currentHum);
        Serial.print("%, Pressure=");
        Serial.print(currentPres);
        Serial.println(" Pa");
      }
      
      // Record timestamp and update buffer
      currentTimestamp = millis();
      
      // Store in circular buffer
      tempBuffer[bufferIndex] = currentTemp;
      humBuffer[bufferIndex] = currentHum;
      presBuffer[bufferIndex] = currentPres;
      timestampBuffer[bufferIndex] = currentTimestamp;
      
      // Increment buffer index (circular buffer)
      bufferIndex = (bufferIndex + 1) % bufferSize;
      updateCount++;
    }
  }
  
  void serializeCurrentData(JsonObject& json) override {
    json["timestamp"] = currentTimestamp;
    json["temperature"] = currentTemp;
    json["humidity"] = currentHum;
    json["pressure"] = currentPres;
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
        entry["temperature"] = tempBuffer[idx];
        entry["humidity"] = humBuffer[idx];
        entry["pressure"] = presBuffer[idx];
      }
    }
  }
  
private:
  int bufferSize;
  int bufferIndex = 0;
  unsigned long updateCount = 0;
  
  // BME280 instance
  BME280I2C bme;
  bool sensorInitialized = false;
  
  // Current readings
  float currentTemp = 0;
  float currentHum = 0;
  float currentPres = 0;
  unsigned long currentTimestamp = 0;
  
  // Circular buffers
  float* tempBuffer;
  float* humBuffer;
  float* presBuffer;
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
      
      case SENSOR_TYPE_BME280:
        sensor = new BME280Sensor(config.bufferSize);
        break;
        
      // Add more sensor types here as needed
      
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

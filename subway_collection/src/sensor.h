#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino_LSM6DS3.h>
#include "littlefs_storage.h" // Include for timestamp function

const int SENSOR_PIN = A0;
const int BUFFER_SIZE = 100;

// Buffers for IMU data
float accelX_buffer[BUFFER_SIZE];
float accelY_buffer[BUFFER_SIZE];
float accelZ_buffer[BUFFER_SIZE];
float gyroX_buffer[BUFFER_SIZE];
float gyroY_buffer[BUFFER_SIZE];
float gyroZ_buffer[BUFFER_SIZE];
float temperature_buffer[BUFFER_SIZE];
unsigned long timestamp_buffer[BUFFER_SIZE];

// Current buffer index
int bufferIndex = 0;

// Last read values
float accelX, accelY, accelZ;
float gyroX, gyroY, gyroZ;
int temperature;
unsigned long lastReadTime;

bool setupSensor() {
  // Initialize the IMU
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
  
  // Initialize the buffers
  for (int i = 0; i < BUFFER_SIZE; i++) {
    accelX_buffer[i] = 0;
    accelY_buffer[i] = 0;
    accelZ_buffer[i] = 0;
    gyroX_buffer[i] = 0;
    gyroY_buffer[i] = 0;
    gyroZ_buffer[i] = 0;
    temperature_buffer[i] = 0;
    timestamp_buffer[i] = 0;
  }
  
  return true;
}

void updateSensor() {
  // Read accelerometer data if available
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(accelX, accelY, accelZ);
    
    // Store in buffer
    accelX_buffer[bufferIndex] = accelX;
    accelY_buffer[bufferIndex] = accelY;
    accelZ_buffer[bufferIndex] = accelZ;
  }
  
  // Read gyroscope data if available
  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(gyroX, gyroY, gyroZ);
    
    // Store in buffer
    gyroX_buffer[bufferIndex] = gyroX;
    gyroY_buffer[bufferIndex] = gyroY;
    gyroZ_buffer[bufferIndex] = gyroZ;
  }
  
  // Read temperature data if available
  if (IMU.temperatureAvailable()) {
    float temp;  // Create a temporary float variable
    IMU.readTemperature(temp);  // Pass the reference to this variable
    temperature = temp;  // Store the value in our global variable
    
    // Store in buffer
    temperature_buffer[bufferIndex] = temperature;
  }
  
  // Record timestamp using our timestamp function
  lastReadTime = timestamp();
  timestamp_buffer[bufferIndex] = lastReadTime;
  
  // Increment buffer index (circular buffer)
  bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
}

#endif

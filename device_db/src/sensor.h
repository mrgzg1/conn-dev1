#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

// Basic analog sensor
const int SENSOR_PIN = A0;
const int BUFFER_SIZE = 100;
int sensorBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// Virtual sensor data (simulated values)
float temperature = 22.0;
float humidity = 50.0;
float pressure = 1013.25;
float accel_x = 0.0;
float accel_y = 0.0;
float accel_z = 9.8;

// Sensor update interval
const unsigned long SENSOR_UPDATE_INTERVAL = 1000; // 1 second
unsigned long lastSensorUpdate = 0;

void setupSensor() {
  pinMode(SENSOR_PIN, INPUT);
  for (int i = 0; i < BUFFER_SIZE; i++) sensorBuffer[i] = 0;
  
  // Seed the random number generator for simulated sensor values
  randomSeed(analogRead(A0));
}

void updateSensor() {
  // Update analog sensor reading
  sensorBuffer[bufferIndex] = analogRead(SENSOR_PIN);
  bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
  
  // Update simulated sensor values periodically
  unsigned long currentTime = millis();
  if (currentTime - lastSensorUpdate > SENSOR_UPDATE_INTERVAL) {
    lastSensorUpdate = currentTime;
    
    // Add small random variations to simulate real sensor readings
    temperature = 22.0 + (random(-100, 100) / 100.0);
    humidity = 50.0 + (random(-50, 50) / 10.0);
    pressure = 1013.25 + (random(-10, 10) / 10.0);
    
    // Simulate small movements
    accel_x = (random(-20, 20) / 100.0);
    accel_y = (random(-20, 20) / 100.0);
    accel_z = 9.8 + (random(-10, 10) / 100.0);
  }
}

// Get the current value of the specified sensor
// Possible sensor names: "analog", "temperature", "humidity", "pressure", "accel_x", "accel_y", "accel_z"
float getSensorValue(const char* sensorName) {
  if (strcmp(sensorName, "analog") == 0) {
    // Return the average of the last 10 readings
    int sum = 0;
    for (int i = 0; i < 10; i++) {
      int idx = (bufferIndex - i - 1 + BUFFER_SIZE) % BUFFER_SIZE;
      sum += sensorBuffer[idx];
    }
    return sum / 10.0;
  } 
  else if (strcmp(sensorName, "temperature") == 0) {
    return temperature;
  }
  else if (strcmp(sensorName, "humidity") == 0) {
    return humidity;
  }
  else if (strcmp(sensorName, "pressure") == 0) {
    return pressure;
  }
  else if (strcmp(sensorName, "accel_x") == 0) {
    return accel_x;
  }
  else if (strcmp(sensorName, "accel_y") == 0) {
    return accel_y;
  }
  else if (strcmp(sensorName, "accel_z") == 0) {
    return accel_z;
  }
  
  // Return 0 for unknown sensor
  return 0.0;
}

#endif
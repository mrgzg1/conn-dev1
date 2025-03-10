#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

// Sensor configuration
// This file centralizes all sensor configurations in one place

// Buffer size for storing history data
#define SENSOR_BUFFER_SIZE 100

// Sensor update frequency in milliseconds
#define SENSOR_UPDATE_INTERVAL 50

// Define sensor types
enum SensorType {
  SENSOR_TYPE_IMU,
  SENSOR_TYPE_TEMP,
  SENSOR_TYPE_HUMIDITY,
  SENSOR_TYPE_PRESSURE,
  SENSOR_TYPE_MAGNETIC,
  SENSOR_TYPE_LIGHT
};

// Sensor configuration structure
struct SensorConfig {
  bool enabled;
  SensorType type;
  int bufferSize;
  int updateInterval;
  const char* name;
  const char* apiEndpoint;
};

// Define available sensors
const SensorConfig AVAILABLE_SENSORS[] = {
  // IMU Sensor (accelerometer, gyroscope, temperature)
  {
    true,                 // enabled
    SENSOR_TYPE_IMU,      // type
    SENSOR_BUFFER_SIZE,   // bufferSize
    SENSOR_UPDATE_INTERVAL, // updateInterval
    "IMU Sensor",         // name
    "imu"                 // apiEndpoint
  }
  
  // Add more sensors here as needed
  // Example:
  // {
  //   false,               // disabled by default
  //   SENSOR_TYPE_HUMIDITY,// type
  //   SENSOR_BUFFER_SIZE,  // bufferSize
  //   SENSOR_UPDATE_INTERVAL, // updateInterval
  //   "Humidity Sensor",   // name
  //   "humidity"           // apiEndpoint
  // }
};

// Number of available sensors
const int NUM_AVAILABLE_SENSORS = sizeof(AVAILABLE_SENSORS) / sizeof(SensorConfig);

#endif
#ifndef STORAGE_API_H
#define STORAGE_API_H

#include <Arduino.h>

// Data structure for sensor readings
struct SensorReading {
  uint32_t timestamp;   // Timestamp when reading was taken
  float temperature;    // Temperature in Celsius
  float humidity;       // Humidity percentage
  float pressure;       // Pressure in hPa
  float accel_x;        // Acceleration X in m/s²
  float accel_y;        // Acceleration Y in m/s²
  float accel_z;        // Acceleration Z in m/s²
  char label[32];       // Optional label for the reading
};

// Initialize the storage system
bool storage_init();

// Store a sensor reading with given label
bool storage_store_reading(const SensorReading& reading);

// Get the most recent reading
bool storage_get_latest_reading(SensorReading& reading);

// Get a specific reading by key (key format: "reading-N" where N is a number)
bool storage_get_reading(const char* key, SensorReading& reading);

// Get the count of stored readings
uint32_t storage_get_reading_count();

// List all keys that start with a prefix
void storage_list_keys(const char* prefix);

// Remove a specific reading by key
bool storage_remove_reading(const char* key);

// Remove all readings 
bool storage_remove_all_readings();

// Print a summary of stored data
void storage_print_summary();

#endif // STORAGE_API_H
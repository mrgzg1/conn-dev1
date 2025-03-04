#ifndef STORAGE_API_H
#define STORAGE_API_H

#include <Arduino.h>

// Sensor types
enum SensorType {
  SENSOR_TEMPERATURE = 1,
  SENSOR_HUMIDITY = 2,
  SENSOR_PRESSURE = 3,
  SENSOR_ACCELERATION = 4,
  SENSOR_CUSTOM = 99
};

// Sensor data structure
struct SensorDataPoint {
  uint32_t sensorId;     // Identifier for the sensor
  uint32_t timestamp;    // When the reading was taken
  float values[4];       // Up to 4 values per reading
  uint8_t valueCount;    // How many values are used
  char label[16];        // Optional label
};

// Storage API functions
bool storage_init(bool forceFormat = false);
bool storage_store_reading(const SensorDataPoint& reading, SensorType type = SENSOR_CUSTOM);
bool storage_get_reading(uint32_t index, SensorDataPoint& reading, uint32_t* type = nullptr);
uint32_t storage_get_reading_count();
bool storage_format();
void storage_get_info(uint32_t* totalSectors, uint32_t* usedSectors, uint32_t* freeSectors);
uint32_t storage_get_boot_count();
bool storage_delete_reading(uint32_t index);
bool storage_delete_readings(uint32_t startIndex, uint32_t endIndex);
bool storage_compact();
bool storage_flush();
void storage_set_auto_flush(bool enabled, unsigned long intervalMs);
void storage_check_flush();
void storage_print_summary();
bool storage_get_time_range(uint32_t* startTime, uint32_t* endTime);
void storage_get_buffer_info(uint32_t* itemsBuffered, uint32_t* bufferCapacity);
bool storage_read_existing_data(uint32_t maxReadings);

// Repair storage by fixing counters and compacting if needed
bool storage_repair();

#endif // STORAGE_API_H 
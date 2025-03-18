#include <Arduino.h>
#include "../include/storage_api.h"

// Simplified storage implementation for demo/building purposes
bool storage_init(bool forceFormat) {
  Serial.println("Storage initialized (simplified demo)");
  return true;
}

bool storage_store_reading(const SensorDataPoint& reading, SensorType type) {
  Serial.println("Storing reading to memory (demo)");
  return true;
}

bool storage_get_reading(uint32_t index, SensorDataPoint& reading, uint32_t* type) {
  // Fill reading with demo data
  reading.sensorId = 1;
  reading.timestamp = millis();
  reading.valueCount = 7;
  reading.values[0] = 0.5;  // accel x
  reading.values[1] = -0.2; // accel y
  reading.values[2] = 9.8;  // accel z
  reading.values[3] = 0.1;  // gyro x
  reading.values[4] = 0.2;  // gyro y
  reading.values[5] = 0.3;  // gyro z
  reading.values[6] = 25.0; // temp
  
  snprintf(reading.label, sizeof(reading.label), "Demo");
  
  if (type) *type = SENSOR_IMU_COMBINED;
  
  return true;
}

uint32_t storage_get_reading_count() {
  return 100; // Demo value
}

bool storage_format() {
  Serial.println("Storage format (demo)");
  return true;
}

void storage_get_info(uint32_t* totalSectors, uint32_t* usedSectors, uint32_t* freeSectors) {
  if (totalSectors) *totalSectors = 1000;
  if (usedSectors) *usedSectors = 50;
  if (freeSectors) *freeSectors = 950;
}

void storage_get_buffer_info(uint32_t* itemsBuffered, uint32_t* bufferCapacity) {
  if (itemsBuffered) *itemsBuffered = 5;
  if (bufferCapacity) *bufferCapacity = 20;
}

uint32_t storage_get_boot_count() {
  return 3; // Demo value
}

bool storage_delete_reading(uint32_t index) {
  Serial.print("Deleting reading at index: ");
  Serial.println(index);
  return true;
}

bool storage_delete_readings(uint32_t startIndex, uint32_t endIndex) {
  Serial.print("Deleting readings from ");
  Serial.print(startIndex);
  Serial.print(" to ");
  Serial.println(endIndex);
  return true;
}

bool storage_compact() {
  Serial.println("Storage compaction (demo)");
  return true;
}

void storage_check_flush() {
  // Nothing in demo
}

void storage_set_auto_flush(bool enabled, unsigned long intervalMs) {
  // Nothing in demo
}

bool storage_flush() {
  Serial.println("Storage flush (demo)");
  return true;
}

bool storage_get_time_range(uint32_t* startTime, uint32_t* endTime) {
  if (startTime) *startTime = 1000;
  if (endTime) *endTime = 50000;
  return true;
}

void storage_print_summary() {
  Serial.println("=== Storage Summary (Demo) ===");
  Serial.println("100 readings stored");
  Serial.println("50/1000 sectors used");
}

bool storage_read_existing_data(uint32_t maxReadings) {
  Serial.print("Reading up to ");
  Serial.print(maxReadings);
  Serial.println(" data points (demo)");
  return true;
}

bool storage_repair() {
  Serial.println("Storage repair (demo)");
  return true;
}
// RP2040 Blob Storage Example
// Demonstrates how to use the blob storage API for sensor data
// Configuration
#define ENABLE_DEMO_WRITES true
#define FORMAT_STORAGE true

#include <Arduino.h>
#include "../include/storage_api.h"

// Define pins for buttons
#define DEBUG_BUTTON_PIN 20  // D20 for data dump
#define TEST_DATA_PIN 21     // D21 for writing test data

// Simulated sensors
float simulateTemperature() {
  return 20.0 + (random(100) - 50) / 10.0;  // 15.0 to 25.0 °C
}

float simulateHumidity() {
  return 50.0 + (random(200) - 100) / 10.0; // 40.0 to 60.0 %
}

float simulatePressure() {
  return 1013.0 + (random(100) - 50) / 10.0; // 1008.0 to 1018.0 hPa
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Blink LED to show we're alive
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
  
  // Initialize serial
  Serial.begin(9600);
  delay(500); // Give serial time to connect
  
  Serial.println("\nRP2040 Blob Storage Example");
  Serial.println("===========================");
  
  // Setup button pins
  pinMode(DEBUG_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TEST_DATA_PIN, INPUT_PULLUP);
  
  // Initialize storage with format flag if needed
  bool format = false; // Set this to true to format storage
  if (!storage_init(format)) {
    Serial.println("Failed to initialize storage!");
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
  
  // Get storage info
  uint32_t totalSectors, usedSectors, freeSectors;
  storage_get_info(&totalSectors, &usedSectors, &freeSectors);
  Serial.printf("Storage status: %u/%u sectors used, %u sectors free\n", 
              usedSectors, totalSectors, freeSectors);
  
  // Get stored blob count
  uint32_t blobCount = storage_get_reading_count();
  Serial.printf("Stored readings: %u\n\n", blobCount);
  
  // Check for debug button (D20) - if grounded, dump all data
  if (digitalRead(DEBUG_BUTTON_PIN) == LOW) {
    Serial.println("DEBUG BUTTON DETECTED - Dumping all data");
    Serial.println("=======================================");
    
    // Print storage summary
    storage_print_summary();
    
    // Read ALL data (not just limited to 5)
    Serial.println("\nReading ALL stored data:");
    storage_read_existing_data(1000); // Read up to 1000 readings
    
    // Run some diagnostics and repair if needed
    Serial.println("\nRunning storage diagnostics:");
    if (storage_repair()) {
      Serial.println("Storage repaired successfully!");
    } else {
      Serial.println("Storage repair failed or wasn't needed");
    }
    
    Serial.println("Debug data dump complete!");
    Serial.println("=======================================");
    delay(1000);
  }
  
  // Check for test data button (D21) - if grounded, write test data
  if (digitalRead(TEST_DATA_PIN) == LOW) {
    Serial.println("TEST DATA BUTTON DETECTED - Writing test data");
    Serial.println("=======================================");
    
    // Write a batch of test readings
    int successCount = 0;
    for (int i = 0; i < 10; i++) {
      SensorDataPoint reading;
      reading.sensorId = (i % 4) + 1; // Cycle through sensor types 1-4
      reading.timestamp = millis() + i*1000; // Staggered timestamps
      reading.valueCount = (reading.sensorId == 4) ? 3 : 1; // More values for accel
      
      // Set values based on sensor type
      switch (reading.sensorId) {
        case SENSOR_TEMPERATURE:
          reading.values[0] = 20.0 + (random(100) / 10.0);
          strcpy(reading.label, "Temp_Test");
          break;
        case SENSOR_HUMIDITY:
          reading.values[0] = 50.0 + (random(200) / 10.0);
          strcpy(reading.label, "Humid_Test");
          break;
        case SENSOR_PRESSURE:
          reading.values[0] = 1000.0 + (random(300) / 10.0);
          strcpy(reading.label, "Press_Test");
          break;
        case SENSOR_ACCELERATION:
          reading.values[0] = (random(100) / 10.0);
          reading.values[1] = (random(100) / 10.0);
          reading.values[2] = (random(100) / 10.0);
          strcpy(reading.label, "Accel_Test");
          break;
      }
      
      // Store reading and track success
      yield();
      if (storage_store_reading(reading, (SensorType)reading.sensorId)) {
        successCount++;
        Serial.printf("Stored test reading #%d: Sensor %d (%s)\n", 
                    i+1, reading.sensorId, reading.label);
      } else {
        Serial.printf("Failed to store test reading #%d\n", i+1);
      }
      
      delay(50);
      yield();
    }
    
    // Flush buffer to ensure all data is written
    storage_flush();
    
    Serial.printf("Test data writing complete! %d/10 readings stored\n", successCount);
    Serial.println("=======================================");
    delay(1000);
  }
  
  // Delay before printing summary
  delay(100);
  yield();
  
  // Display a summary of stored data
  Serial.println("Data summary before starting:");
  storage_print_summary();
  
  // After printing the data summary in setup():
  Serial.println("DEBUG: Summary printed, continuing to read existing data...");
  
  // IMPORTANT: Add a longer delay after intensive summary operation
  delay(500);
  yield();
  
  // If there are existing readings, demonstrate deletion of oldest reading
  if (storage_get_reading_count() > 10) {
    Serial.println("\nDemonstrating deletion of oldest reading:");
    
    // Add yield before deletion
    yield();
    delay(50);
    
    // Print before deletion
    Serial.printf("Before deletion: %u readings\n", storage_get_reading_count());
    
    // Perform deletion with debug print
    Serial.println("Calling storage_delete_reading(0)...");
    bool deleteSuccessful = storage_delete_reading(0);
    
    // Add substantial delay after deletion
    delay(200);
    yield();
    
    // Print deletion result
    Serial.printf("Deletion operation %s\n", deleteSuccessful ? "succeeded" : "failed");
    
    // Small delay before getting updated count
    delay(100);
    yield();
    
    // Get updated reading count
    uint32_t newCount = storage_get_reading_count();
    Serial.printf("Readings after deletion: %u\n\n", newCount);
    
    // Final yield
    delay(100);
    yield();
  }
  
  // Delay before reading data
  delay(200);
  yield();
  
  // Read and display existing data using safer method
  Serial.println("Reading existing data safely...");
  storage_read_existing_data(5); // Limit to 5 readings for safety
  
  // Delay before storing new reading
  delay(200);
  yield();
  
  // Store a new reading on each boot
  Serial.println("DEBUG: About to store new temperature reading...");
  SensorDataPoint tempReading;
  tempReading.sensorId = 1;
  tempReading.timestamp = millis();
  tempReading.values[0] = simulateTemperature();
  tempReading.valueCount = 1;
  strcpy(tempReading.label, "Temp Sensor");
  
  if (storage_store_reading(tempReading, SENSOR_TEMPERATURE)) {
    Serial.println("\nStored new temperature reading:");
    Serial.printf("  Value: %.2f °C\n", tempReading.values[0]);
  } else {
    Serial.println("\nFailed to store temperature reading!");
  }
  
  // Final delay before entering main loop
  delay(300);
  yield();
  
  Serial.println("DEBUG: Setup phase complete, entering main loop...");
}

void loop() {
  // Heartbeat LED
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
  
  // Add periodic yield to keep system responsive
  static unsigned long lastYield = 0;
  if (millis() - lastYield > 100) {
    lastYield = millis();
    yield();
  }
  
  // Generate new readings periodically
  if (ENABLE_DEMO_WRITES) {
    static unsigned long lastWrite = 0;
    if (millis() - lastWrite > 10000) {  // Increased to every 10 seconds (was 5 seconds)
      lastWrite = millis();
      
      // Random sensor selection
      int sensorType = random(4) + 1; // 1-4
      SensorDataPoint reading;
      reading.sensorId = sensorType;
      reading.timestamp = millis();
      reading.valueCount = 1;
      
      switch (sensorType) {
        case SENSOR_TEMPERATURE:
          reading.values[0] = simulateTemperature();
          strcpy(reading.label, "Temperature");
          break;
        case SENSOR_HUMIDITY:
          reading.values[0] = simulateHumidity();
          strcpy(reading.label, "Humidity");
          break;
        case SENSOR_PRESSURE:
          reading.values[0] = simulatePressure();
          strcpy(reading.label, "Pressure");
          break;
        case SENSOR_ACCELERATION:
          reading.values[0] = random(100) / 10.0;
          reading.values[1] = random(100) / 10.0;
          reading.values[2] = random(100) / 10.0;
          reading.valueCount = 3;
          strcpy(reading.label, "Accel");
          break;
      }
      
      // Yield before storage operation
      yield();
      
      // Store the reading
      if (storage_store_reading(reading, (SensorType)sensorType)) {
        Serial.printf("Stored %s reading: ", reading.label);
        for (int i = 0; i < reading.valueCount; i++) {
          Serial.printf("%.2f ", reading.values[i]);
        }
        Serial.println();
        
        // Display storage usage every 5 readings instead of 10
        static int writeCount = 0;
        if (++writeCount % 5 == 0) {
          // Yield before querying storage info
          yield();
          
          uint32_t totalSectors, usedSectors, freeSectors;
          storage_get_info(&totalSectors, &usedSectors, &freeSectors);
          Serial.printf("Storage: %u/%u sectors used, %u sectors free\n", 
                      usedSectors, totalSectors, freeSectors);
          
          // Manually flush buffer every 15 readings to avoid large flushes
          if (writeCount % 15 == 0) {
            Serial.println("Flushing buffer to maintain stability...");
            storage_flush();
            
            // Delay after flush
            delay(200);
            yield();
          }
        }
      } else {
        Serial.println("Failed to store reading!");
      }
    }
  }
  
  // Process serial commands
  if (Serial.available()) {
    // Add yield before serial processing
    yield();
    
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "summary") {
      // Show data summary
      storage_print_summary();
      
      // Add delay after intensive operation
      delay(300);
      yield();
    }
    else if (command == "flush") {
      // Force flush
      Serial.println("Forcing buffer flush...");
      if (storage_flush()) {
        Serial.println("Flush successful");
      }
      
      // Add delay after flush
      delay(200);
      yield();
    }
    else if (command == "compact") {
      // Compact storage
      Serial.println("Compacting storage...");
      if (storage_compact()) {
        Serial.println("Compaction successful");
      }
      
      // Add delay after compact
      delay(300);
      yield();
    }
    else if (command.startsWith("delete ")) {
      // Delete a specific reading
      int index = command.substring(7).toInt();
      Serial.printf("Deleting reading %d...\n", index);
      if (storage_delete_reading(index)) {
        Serial.println("Deletion successful");
      }
      
      // Add delay after deletion
      delay(100);
      yield();
    }
    else if (command == "help") {
      // Show help
      Serial.println("\nAvailable commands:");
      Serial.println("summary - Show storage summary");
      Serial.println("flush - Force buffer flush");
      Serial.println("compact - Compact storage");
      Serial.println("delete X - Delete reading at index X");
      Serial.println("help - Show this help");
    }
  }
  
  // At the beginning of loop():
  static bool firstLoop = true;
  if (firstLoop) {
    Serial.println("DEBUG: First loop() execution");
    firstLoop = false;
  }
  
  // Check if buffer needs to be flushed
  storage_check_flush();
  
  // Slightly longer delay to reduce CPU load
  delay(20);
}
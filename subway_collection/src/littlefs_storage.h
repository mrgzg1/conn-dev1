#ifndef LITTLEFS_STORAGE_H
#define LITTLEFS_STORAGE_H

#define LFS_MBED_RP2040_VERSION_MIN_TARGET      "LittleFS_Mbed_RP2040 v1.1.0"
#define LFS_MBED_RP2040_VERSION_MIN             1001000

#define _LFS_LOGLEVEL_          1
#define RP2040_FS_SIZE_KB       64

#define FORCE_REFORMAT          false

#include <LittleFS_Mbed_RP2040.h>
#include <Arduino.h>

// We need to include stdio.h for FILE operations
#include <stdio.h>

// Stub timestamp function - will be updated later with real time source
// Currently just returns millis() but could be replaced with RTC or NTP time
unsigned long timestamp() {
  // Future enhancements:
  // 1. Use an RTC module if available
  // 2. Use NTP time if connected to WiFi
  // 3. Use a combination of both with fallback options
  
  // For now, just return millis()
  return millis();
}

// Configuration for our data storage
struct SensorDataPoint {
  float accelX;
  float accelY;
  float accelZ;
  float gyroX;
  float gyroY;
  float gyroZ;
  float temperature;
  unsigned long timestamp;
};

class LittleFSStorage {
private:
  LittleFS_MBED *myFS;
  bool initialized = false;
  unsigned long lastFlushTime = 0;
  const unsigned long FLUSH_INTERVAL = 60000; // Flush every minute by default
  const char* dataPath = MBED_LITTLEFS_FILE_PREFIX "/sensor_data_";
  int fileCounter = 0;

public:
  LittleFSStorage() {
    myFS = new LittleFS_MBED();
  }

  ~LittleFSStorage() {
    if (myFS) {
      delete myFS;
    }
  }

  bool begin() {
    // Initialize the LittleFS filesystem
    if (!myFS->init()) {
      Serial.println("LITTLEFS Mount Failed");
      return false;
    }

    Serial.println("\n======== LittleFS Storage Info ========");
    Serial.print("Board: ");
    Serial.println(BOARD_NAME);
    Serial.println(LFS_MBED_RP2040_VERSION);
    
    // Print filesystem info
    printFSInfo();
    
    initialized = true;
    return true;
  }

  // List files in the filesystem using manual file manipulation
  // instead of directory iteration
  void printFSInfo() {
    if (!initialized) {
      Serial.println("Filesystem not initialized");
      return;
    }
    
    Serial.println("\nFiles in filesystem:");
    
    // Use a simpler approach for RP2040 - just try to list some known files
    // and report their sizes if they exist
    char knownPrefix[] = MBED_LITTLEFS_FILE_PREFIX "/sensor_data_";
    
    for (int i = 0; i < 10; i++) {  // Try the first 10 possible data files
      char filename[64];
      sprintf(filename, "%s%d.dat", knownPrefix, i);
      
      FILE* file = fopen(filename, "r");
      if (file) {
        // File exists, get its size
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fclose(file);
        
        Serial.print("  ");
        Serial.print(filename);
        Serial.print("  (");
        Serial.print(size);
        Serial.println(" bytes)");
      }
    }
    
    // Print filesystem capacity information if available
    Serial.print("Filesystem size: ");
    Serial.print(RP2040_FS_SIZE_KB);
    Serial.println(" KB");
  }

  bool saveDataPoint(const SensorDataPoint& dataPoint) {
    if (!initialized) return false;
    
    char filePath[128];
    sprintf(filePath, "%s%d.dat", dataPath, fileCounter);
    
    FILE* file = fopen(filePath, "a");
    if (!file) {
      Serial.println("Failed to open data file for writing");
      return false;
    }
    
    size_t bytesWritten = fwrite(&dataPoint, sizeof(SensorDataPoint), 1, file);
    fclose(file);
    
    if (bytesWritten != 1) {
      Serial.println("Failed to write data point");
      return false;
    }
    
    return true;
  }

  bool saveSensorBuffers(float accelX[], float accelY[], float accelZ[],
                        float gyroX[], float gyroY[], float gyroZ[],
                        float temperature[], unsigned long timestamps[],
                        int count) {
    if (!initialized) return false;
    
    char filePath[128];
    sprintf(filePath, "%s%d.dat", dataPath, fileCounter);
    
    FILE* file = fopen(filePath, "w");
    if (!file) {
      Serial.println("Failed to open data file for writing");
      return false;
    }
    
    // Write header with count
    fwrite(&count, sizeof(count), 1, file);
    
    // Write the buffer data
    for (int i = 0; i < count; i++) {
      SensorDataPoint dataPoint;
      dataPoint.accelX = accelX[i];
      dataPoint.accelY = accelY[i];
      dataPoint.accelZ = accelZ[i];
      dataPoint.gyroX = gyroX[i];
      dataPoint.gyroY = gyroY[i];
      dataPoint.gyroZ = gyroZ[i];
      dataPoint.temperature = temperature[i];
      dataPoint.timestamp = timestamps[i];
      
      fwrite(&dataPoint, sizeof(SensorDataPoint), 1, file);
    }
    
    fclose(file);
    
    // Increment file counter for next time
    fileCounter++;
    
    Serial.print("Saved ");
    Serial.print(count);
    Serial.print(" data points to file: ");
    Serial.println(filePath);
    
    return true;
  }

  void checkAndFlush(float accelX[], float accelY[], float accelZ[],
                    float gyroX[], float gyroY[], float gyroZ[],
                    float temperature[], unsigned long timestamps[],
                    int bufferIndex) {
    unsigned long currentTime = millis();
    
    // If it's time to flush to storage
    if (currentTime - lastFlushTime >= FLUSH_INTERVAL) {
      Serial.println("Flushing sensor data to flash storage...");
      
      // Save the current buffer
      saveSensorBuffers(accelX, accelY, accelZ, gyroX, gyroY, gyroZ,
                       temperature, timestamps, bufferIndex);
                       
      lastFlushTime = currentTime;
    }
  }

  // Delete a file
  bool deleteFile(const char* filename) {
    if (!initialized) return false;
    
    char filePath[128];
    sprintf(filePath, "%s/%s", MBED_LITTLEFS_FILE_PREFIX, filename);
    
    if (remove(filePath) == 0) {
      Serial.print("Deleted file: ");
      Serial.println(filePath);
      return true;
    } else {
      Serial.print("Failed to delete file: ");
      Serial.println(filePath);
      return false;
    }
  }
  
  // Read sensor data from a specified file
  bool readSensorData(const char* filename, SensorDataPoint* dataBuffer, int maxPoints, int* pointsRead) {
    if (!initialized) return false;
    
    char filePath[128];
    sprintf(filePath, "%s/%s", MBED_LITTLEFS_FILE_PREFIX, filename);
    
    FILE* file = fopen(filePath, "r");
    if (!file) {
      Serial.print("Failed to open data file for reading: ");
      Serial.println(filePath);
      return false;
    }
    
    // Read the count header
    int count = 0;
    size_t bytesRead = fread(&count, sizeof(count), 1, file);
    if (bytesRead != 1) {
      Serial.println("Failed to read count header");
      fclose(file);
      return false;
    }
    
    // Limit to maxPoints
    int pointsToRead = min(count, maxPoints);
    *pointsRead = pointsToRead;
    
    // Read the data points
    for (int i = 0; i < pointsToRead; i++) {
      bytesRead = fread(&dataBuffer[i], sizeof(SensorDataPoint), 1, file);
      if (bytesRead != 1) {
        Serial.print("Error reading data point ");
        Serial.println(i);
        fclose(file);
        return false;
      }
    }
    
    fclose(file);
    
    Serial.print("Read ");
    Serial.print(pointsToRead);
    Serial.print(" data points from file: ");
    Serial.println(filePath);
    
    return true;
  }
  
  // List all data files and their sizes - simplified method without directory listing
  void listDataFiles() {
    if (!initialized) {
      Serial.println("Filesystem not initialized");
      return;
    }
    
    Serial.println("\nData Files:");
    
    // Use a simpler approach - just try to open each potential file
    char knownPrefix[] = MBED_LITTLEFS_FILE_PREFIX "/sensor_data_";
    
    for (int i = 0; i < 10; i++) {  // Try the first 10 possible data files
      char filename[64];
      sprintf(filename, "%s%d.dat", knownPrefix, i);
      
      FILE* file = fopen(filename, "r");
      if (file) {
        // File exists, get its size
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        
        // Try to read the count header to determine number of records
        fseek(file, 0, SEEK_SET);
        int count = 0;
        fread(&count, sizeof(count), 1, file);
        
        fclose(file);
        
        Serial.print("  ");
        Serial.print(filename);
        Serial.print("  (");
        Serial.print(size);
        Serial.print(" bytes, ~");
        Serial.print(count);
        Serial.println(" records)");
      }
    }
  }
};

#endif // LITTLEFS_STORAGE_H 
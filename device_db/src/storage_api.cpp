#include "../include/storage_api.h"
#include <LittleFS.h>
#include <string.h>

// Prefix for all sensor readings
const char* KEY_PREFIX = "reading-";
// Key for the reading counter
const char* COUNT_KEY = "reading-count";

// Helper function to generate key names
void generate_key(char* buffer, uint32_t index) {
  snprintf(buffer, 32, "%s%u", KEY_PREFIX, index);
}

// Helper function to generate file paths
void generate_path(char* buffer, const char* key) {
  snprintf(buffer, 128, "/%s.dat", key);
}

// Initialize the storage system
bool storage_init() {
  Serial.println("Initializing LittleFS storage...");
  
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    if (!LittleFS.format()) {
      Serial.println("Failed to format LittleFS");
      return false;
    }
    if (!LittleFS.begin()) {
      Serial.println("Failed to mount LittleFS even after formatting");
      return false;
    }
    Serial.println("LittleFS formatted and mounted successfully");
  } else {
    Serial.println("LittleFS mounted successfully");
  }
  
  // Try to read the current count, if it fails, initialize it to 0
  uint32_t count = 0;
  char filePath[128];
  generate_path(filePath, COUNT_KEY);
  
  if (LittleFS.exists(filePath)) {
    File countFile = LittleFS.open(filePath, "r");
    if (!countFile) {
      Serial.println("Failed to open count file for reading");
      return false;
    }
    
    if (countFile.read((uint8_t*)&count, sizeof(count)) != sizeof(count)) {
      Serial.println("Failed to read count value");
      countFile.close();
      return false;
    }
    
    countFile.close();
    Serial.print("Found ");
    Serial.print(count);
    Serial.println(" existing readings in storage");
  } else {
    Serial.println("First time setup: initializing reading count to 0");
    File countFile = LittleFS.open(filePath, "w");
    if (!countFile) {
      Serial.println("Failed to create count file");
      return false;
    }
    
    if (countFile.write((uint8_t*)&count, sizeof(count)) != sizeof(count)) {
      Serial.println("Failed to write initial count value");
      countFile.close();
      return false;
    }
    
    countFile.close();
  }
  
  return true;
}

// Store a sensor reading
bool storage_store_reading(const SensorReading& reading) {
  // First get the current count
  uint32_t count = 0;
  char countPath[128];
  generate_path(countPath, COUNT_KEY);
  
  if (LittleFS.exists(countPath)) {
    File countFile = LittleFS.open(countPath, "r");
    if (!countFile) {
      Serial.println("Failed to open count file for reading");
      return false;
    }
    
    if (countFile.read((uint8_t*)&count, sizeof(count)) != sizeof(count)) {
      Serial.println("Failed to read count value");
      countFile.close();
      return false;
    }
    
    countFile.close();
  }
  
  // Generate a key for this reading
  char key[32];
  generate_key(key, count);
  
  // Generate file path
  char filePath[128];
  generate_path(filePath, key);
  
  // Store the reading
  File dataFile = LittleFS.open(filePath, "w");
  if (!dataFile) {
    Serial.print("Failed to create file for key ");
    Serial.println(key);
    return false;
  }
  
  if (dataFile.write((uint8_t*)&reading, sizeof(reading)) != sizeof(reading)) {
    Serial.print("Failed to write data for key ");
    Serial.println(key);
    dataFile.close();
    return false;
  }
  
  dataFile.close();
  
  // Increment and store the count
  count++;
  File countFile = LittleFS.open(countPath, "w");
  if (!countFile) {
    Serial.println("Failed to open count file for writing");
    return false;
  }
  
  if (countFile.write((uint8_t*)&count, sizeof(count)) != sizeof(count)) {
    Serial.println("Failed to write updated count value");
    countFile.close();
    return false;
  }
  
  countFile.close();
  
  Serial.print("Stored reading ");
  Serial.print(count - 1);
  Serial.print(" with key ");
  Serial.println(key);
  
  return true;
}

// Get the most recent reading
bool storage_get_latest_reading(SensorReading& reading) {
  // Get the current count
  uint32_t count = 0;
  char countPath[128];
  generate_path(countPath, COUNT_KEY);
  
  if (!LittleFS.exists(countPath)) {
    Serial.println("Count file does not exist");
    return false;
  }
  
  File countFile = LittleFS.open(countPath, "r");
  if (!countFile) {
    Serial.println("Failed to open count file for reading");
    return false;
  }
  
  if (countFile.read((uint8_t*)&count, sizeof(count)) != sizeof(count)) {
    Serial.println("Failed to read count value");
    countFile.close();
    return false;
  }
  
  countFile.close();
  
  if (count == 0) {
    Serial.println("No readings stored yet");
    return false;
  }
  
  // Get the most recent reading
  char key[32];
  generate_key(key, count - 1);
  
  return storage_get_reading(key, reading);
}

// Get a specific reading by key
bool storage_get_reading(const char* key, SensorReading& reading) {
  char filePath[128];
  generate_path(filePath, key);
  
  if (!LittleFS.exists(filePath)) {
    Serial.print("No file exists for key ");
    Serial.println(key);
    return false;
  }
  
  File dataFile = LittleFS.open(filePath, "r");
  if (!dataFile) {
    Serial.print("Failed to open file for key ");
    Serial.println(key);
    return false;
  }
  
  // Verify size
  size_t fileSize = dataFile.size();
  if (fileSize != sizeof(SensorReading)) {
    Serial.print("Reading size mismatch for key ");
    Serial.print(key);
    Serial.print(": expected ");
    Serial.print(sizeof(SensorReading));
    Serial.print(" but got ");
    Serial.println(fileSize);
    dataFile.close();
    return false;
  }
  
  // Read the data
  if (dataFile.read((uint8_t*)&reading, sizeof(reading)) != sizeof(reading)) {
    Serial.print("Failed to read data for key ");
    Serial.println(key);
    dataFile.close();
    return false;
  }
  
  dataFile.close();
  return true;
}

// Get the count of stored readings
uint32_t storage_get_reading_count() {
  uint32_t count = 0;
  char countPath[128];
  generate_path(countPath, COUNT_KEY);
  
  if (!LittleFS.exists(countPath)) {
    return 0;
  }
  
  File countFile = LittleFS.open(countPath, "r");
  if (!countFile) {
    Serial.println("Failed to open count file for reading");
    return 0;
  }
  
  if (countFile.read((uint8_t*)&count, sizeof(count)) != sizeof(count)) {
    Serial.println("Failed to read count value");
    countFile.close();
    return 0;
  }
  
  countFile.close();
  return count;
}

// List all keys that start with a prefix
void storage_list_keys(const char* prefix) {
  Serial.print("Listing keys with prefix: ");
  Serial.println(prefix);
  
  File root = LittleFS.open("/", "r");
  if (!root) {
    Serial.println("Failed to open root directory");
    return;
  }
  
  if (!root.isDirectory()) {
    Serial.println("Root is not a directory");
    root.close();
    return;
  }
  
  size_t prefixLen = strlen(prefix);
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      // Extract key from filename (remove .dat extension and leading /)
      String fileName = file.name();
      if (fileName.endsWith(".dat")) {
        String key = fileName.substring(1, fileName.length() - 4); // Remove leading / and .dat
        
        // Check if key starts with prefix
        if (key.startsWith(prefix)) {
          Serial.print("Found key: ");
          Serial.println(key);
        }
      }
    }
    
    file = root.openNextFile();
  }
  
  root.close();
}

// Remove a specific reading by key
bool storage_remove_reading(const char* key) {
  char filePath[128];
  generate_path(filePath, key);
  
  if (!LittleFS.exists(filePath)) {
    Serial.print("No file exists for key ");
    Serial.println(key);
    return false;
  }
  
  if (!LittleFS.remove(filePath)) {
    Serial.print("Failed to remove file for key ");
    Serial.println(key);
    return false;
  }
  
  Serial.print("Successfully removed key ");
  Serial.println(key);
  return true;
}

// Remove all readings
bool storage_remove_all_readings() {
  File root = LittleFS.open("/", "r");
  if (!root) {
    Serial.println("Failed to open root directory");
    return false;
  }
  
  if (!root.isDirectory()) {
    Serial.println("Root is not a directory");
    root.close();
    return false;
  }
  
  uint32_t removed = 0;
  size_t prefixLen = strlen(KEY_PREFIX);
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      // Extract key from filename (remove .dat extension and leading /)
      String fileName = file.name();
      if (fileName.endsWith(".dat")) {
        String key = fileName.substring(1, fileName.length() - 4); // Remove leading / and .dat
        
        // Check if key starts with KEY_PREFIX and is not COUNT_KEY
        if (key.startsWith(KEY_PREFIX) && key != COUNT_KEY) {
          file.close(); // Close the file before removing it
          
          if (LittleFS.remove(fileName)) {
            removed++;
          } else {
            Serial.print("Failed to remove file: ");
            Serial.println(fileName);
          }
          
          // Reopen the directory since we closed a file
          root.close();
          root = LittleFS.open("/", "r");
          if (!root) {
            Serial.println("Failed to reopen root directory");
            return false;
          }
          file = root.openNextFile();
          continue;
        }
      }
    }
    
    file = root.openNextFile();
  }
  
  root.close();
  
  // Reset the counter
  uint32_t count = 0;
  char countPath[128];
  generate_path(countPath, COUNT_KEY);
  
  File countFile = LittleFS.open(countPath, "w");
  if (!countFile) {
    Serial.println("Failed to open count file for writing");
    return false;
  }
  
  if (countFile.write((uint8_t*)&count, sizeof(count)) != sizeof(count)) {
    Serial.println("Failed to write reset count value");
    countFile.close();
    return false;
  }
  
  countFile.close();
  
  Serial.print("Removed ");
  Serial.print(removed);
  Serial.println(" readings");
  return true;
}

// Print a summary of stored data
void storage_print_summary() {
  uint32_t count = storage_get_reading_count();
  
  Serial.println("\n==== Storage Summary ====");
  Serial.print("Total readings: ");
  Serial.println(count);
  
  if (count > 0) {
    // Print details of first and last reading
    SensorReading firstReading;
    SensorReading lastReading;
    char firstKey[32];
    char lastKey[32];
    
    generate_key(firstKey, 0);
    generate_key(lastKey, count - 1);
    
    bool firstSuccess = storage_get_reading(firstKey, firstReading);
    bool lastSuccess = storage_get_reading(lastKey, lastReading);
    
    if (firstSuccess) {
      Serial.println("\nFirst reading:");
      Serial.print("  Timestamp: ");
      Serial.println(firstReading.timestamp);
      Serial.print("  Temperature: ");
      Serial.print(firstReading.temperature);
      Serial.println(" °C");
      Serial.print("  Label: ");
      Serial.println(firstReading.label);
    }
    
    if (lastSuccess) {
      Serial.println("\nLast reading:");
      Serial.print("  Timestamp: ");
      Serial.println(lastReading.timestamp);
      Serial.print("  Temperature: ");
      Serial.print(lastReading.temperature);
      Serial.println(" °C");
      Serial.print("  Label: ");
      Serial.println(lastReading.label);
    }
  }
  
  Serial.println("=======================");
}
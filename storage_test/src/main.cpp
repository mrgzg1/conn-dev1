// RP2040 Safe Flash Storage
// A reliable storage system that avoids bricking the device

#include <Arduino.h>
#include <hardware/flash.h>
#include <hardware/sync.h>

// Configuration options - TURN THESE OFF AFTER TESTING
#define FORCE_FORMAT true
#define CONTINUOUS_WRITE_TEST true

// SAFETY FIRST: Use a small, fixed portion of flash at a safe location 
// The RP2040 has program code at the beginning of flash, we'll use a small 
// section near the end that won't interfere with the program

// Avoid redefining FLASH_SECTOR_SIZE since it's already defined
#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE 4096
#endif

// FLASH MEMORY LAYOUT OPTIMIZATION
// Based on your device's specific memory map:

// Your RP2040 has 11.54MB total flash
// The filesystem uses 10MB (0x10188680 to 0x10b88680)
// The sketch uses up to 1.53MB (from beginning of flash)
// EEPROM starts at 0x10b88680

// Let's place our storage just after the EEPROM
#define FLASH_BASE_ADDR 0x10000000
#define EEPROM_START 0x10b88680
#define EEPROM_SIZE 0x1000  // Typical size (4KB)
#define STORAGE_START (EEPROM_START + EEPROM_SIZE)

// Calculate offset from base address
#define FLASH_TARGET_OFFSET (STORAGE_START - FLASH_BASE_ADDR)

// We have room for approximately 512KB (128 sectors)
#define STORAGE_SECTORS 1024
#define STORAGE_SIZE (STORAGE_SECTORS * FLASH_SECTOR_SIZE)

// Extended storage header with boot count
struct StorageHeader {
  uint32_t magic;        // Magic number to identify our storage
  uint32_t version;      // Version of the storage format
  uint32_t dataSize;     // How much actual data is stored
  uint32_t bootCount;    // Boot counter
  uint32_t writeCount;   // Number of writes since boot
  uint32_t crc;          // CRC32 checksum of the data
};

// Magic number to identify our storage (ASCII 'RPST' = RP2040 STorage)
#define STORAGE_MAGIC 0x52505354

// Buffer for reading/writing data - allocate only what we need
uint8_t buffer[FLASH_SECTOR_SIZE];
bool storageInitialized = false;
uint32_t bootCount = 0;
uint32_t writeCount = 0;

// Forward declarations
bool initStorage(bool forceFormat = false);
bool readStorage(uint8_t* data, size_t maxSize, size_t* actualSize);
bool writeStorage(const uint8_t* data, size_t size);
uint32_t calculateCRC32(const uint8_t* data, size_t size);
bool testWriteSector(uint32_t sectorIndex, const char* testMessage);
void testFlashBoundaries();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // Flash quickly at startup to show we're alive
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(100);
  }
  
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("\n\nRP2040 Safe Flash Storage");
  Serial.println("=========================");
  Serial.printf("Storage size: %d KB (%d sectors) at offset 0x%X\n", 
               STORAGE_SIZE / 1024, STORAGE_SECTORS, FLASH_TARGET_OFFSET);
  
  // Initialize our storage - no try/catch since exceptions are disabled
  bool success = initStorage(FORCE_FORMAT);
  
  if (!success) {
    Serial.println("ERROR: Failed to initialize storage!");
    // Flash error pattern
    while(1) {
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
      }
      delay(1000);
    }
  }
  
  Serial.printf("This is boot #%u\n\n", bootCount);
  
  // Test by reading existing data
  size_t dataSize = 0;
  if (readStorage(buffer, sizeof(buffer) - sizeof(StorageHeader), &dataSize)) {
    if (dataSize > 0) {
      Serial.printf("Read %u bytes of data from storage\n", dataSize);
      Serial.println("Data preview (ASCII):");
      for (size_t i = 0; i < min(dataSize, 128UL); i++) {
        if (buffer[i] >= 32 && buffer[i] <= 126) {
          Serial.print((char)buffer[i]);
        } else {
          Serial.print('.');
        }
      }
      Serial.println("\n");
    } else {
      Serial.println("No data found in storage");
    }
  } else {
    Serial.println("ERROR: Failed to read from storage");
  }
  
  // Write a small test message
  char testData[256];
  snprintf(testData, sizeof(testData), 
          "Boot #%u - Safe storage test\n", bootCount);
  
  size_t writeSize = strlen(testData);
  if (writeStorage((uint8_t*)testData, writeSize)) {
    Serial.printf("Successfully wrote %u bytes to storage\n", writeSize);
    Serial.printf("Content: %s\n", testData);
  } else {
    Serial.println("ERROR: Failed to write to storage");
  }
  
  Serial.println("Storage initialization completed!");
  Serial.printf("Next boot will be #%u\n", bootCount + 1);

  // Add this to setup() after initialization to test boundaries
  testFlashBoundaries();
}

void loop() {
  // Simple heartbeat
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
  
  // Perform continuous write test if enabled (but be conservative)
  if (CONTINUOUS_WRITE_TEST) {
    static unsigned long lastWrite = 0;
    // Only write every 30 seconds to avoid excessive flash wear
    if (millis() - lastWrite > 3000) {
      lastWrite = millis();
      writeCount++;
      
      char testData[64];
      snprintf(testData, sizeof(testData), 
              "Test write #%u at %lu ms\n", 
              writeCount, millis());
      
      if (writeStorage((uint8_t*)testData, strlen(testData))) {
        Serial.printf("Write test #%u successful\n", writeCount);
      } else {
        Serial.printf("Write test #%u FAILED\n", writeCount);
      }
    }
  }
  
  delay(10);
}

// Initialize the storage system
bool initStorage(bool forceFormat) {
  // Copy current flash content to our buffer (safely)
  memcpy(buffer, (void*)(XIP_BASE + FLASH_TARGET_OFFSET), sizeof(buffer));
  
  // Check if there's a valid header
  StorageHeader* header = (StorageHeader*)buffer;
  
  if (!forceFormat && header->magic == STORAGE_MAGIC) {
    Serial.printf("Storage header found: version %u, data size %u bytes, boot count %u\n", 
                 header->version, header->dataSize, header->bootCount);
    
    // Increment boot count
    bootCount = header->bootCount + 1;
    header->bootCount = bootCount;
    header->writeCount = 0; // Reset write count for this boot
    
    // Write the updated header with incremented boot count
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
    
  } else {
    // Initialize or reformat
    if (forceFormat) {
      Serial.println("Forced format requested - initializing storage");
    } else {
      Serial.println("No valid storage header found - initializing new storage");
    }
    
    // Clear buffer first for safety
    memset(buffer, 0, sizeof(buffer));
    
    // Set up new header
    header->magic = STORAGE_MAGIC;
    header->version = 1;
    header->dataSize = 0;
    header->bootCount = 1; // Start at 1 for the first boot
    header->writeCount = 0;
    header->crc = 0;
    
    // Write the empty header to flash
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
    
    bootCount = 1;
    writeCount = 0;
  }
  
  storageInitialized = true;
  return true;
}

// Read data from storage
bool readStorage(uint8_t* data, size_t maxSize, size_t* actualSize) {
  if (!storageInitialized && !initStorage(false)) {
    return false;
  }
  
  // Read header directly from flash
  StorageHeader header;
  memcpy(&header, (void*)(XIP_BASE + FLASH_TARGET_OFFSET), sizeof(StorageHeader));
  
  // Check header
  if (header.magic != STORAGE_MAGIC) {
    Serial.println("Invalid magic number in storage header during read");
    return false;
  }
  
  // Safety check data size
  if (header.dataSize > STORAGE_SIZE - sizeof(StorageHeader)) {
    Serial.println("Invalid data size in header");
    *actualSize = 0;
    return false;
  }
  
  // Calculate safe amount to copy
  size_t copySize = min(maxSize, header.dataSize);
  
  // Read data directly from flash
  memcpy(data, (void*)(XIP_BASE + FLASH_TARGET_OFFSET + sizeof(StorageHeader)), copySize);
  *actualSize = copySize;
  
  return true;
}

// Write data to storage - simplified version
bool writeStorage(const uint8_t* data, size_t size) {
  // Safety checks
  if (!storageInitialized && !initStorage(false)) {
    return false;
  }
  
  if (size > STORAGE_SIZE - sizeof(StorageHeader)) {
    Serial.println("ERROR: Data too large for storage");
    return false;
  }
  
  // Read the current header
  StorageHeader header;
  memcpy(&header, (void*)(XIP_BASE + FLASH_TARGET_OFFSET), sizeof(StorageHeader));
  
  // Update header for new write
  header.magic = STORAGE_MAGIC;
  header.version = 1;
  header.dataSize = size;
  header.bootCount = bootCount;
  header.writeCount = ++writeCount;
  header.crc = calculateCRC32(data, size);
  
  // Copy header to buffer
  memcpy(buffer, &header, sizeof(header));
  
  // Copy data after header
  size_t dataInFirstSector = min(size, FLASH_SECTOR_SIZE - sizeof(StorageHeader));
  memcpy(buffer + sizeof(StorageHeader), data, dataInFirstSector);
  
  // Erase and write first sector with header and initial data
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(FLASH_TARGET_OFFSET, buffer, FLASH_SECTOR_SIZE);
  
  // If there's more data, write it sector by sector
  if (size > dataInFirstSector) {
    size_t remaining = size - dataInFirstSector;
    size_t offset = dataInFirstSector;
    size_t sectorOffset = 1;
    
    while (remaining > 0 && sectorOffset < STORAGE_SECTORS) {
      // Clear buffer for safety
      memset(buffer, 0, FLASH_SECTOR_SIZE);
      
      // Copy next chunk of data
      size_t chunkSize = min(remaining, FLASH_SECTOR_SIZE);
      memcpy(buffer, data + offset, chunkSize);
      
      // Erase and write this sector
      flash_range_erase(FLASH_TARGET_OFFSET + (sectorOffset * FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
      flash_range_program(FLASH_TARGET_OFFSET + (sectorOffset * FLASH_SECTOR_SIZE), buffer, FLASH_SECTOR_SIZE);
      
      remaining -= chunkSize;
      offset += chunkSize;
      sectorOffset++;
    }
  }
  
  restore_interrupts(ints);
  return true;
}

// CRC32 calculation
uint32_t calculateCRC32(const uint8_t* data, size_t size) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
  }
  return ~crc;
}

// Test writing to a specific sector
bool testWriteSector(uint32_t sectorIndex, const char* testMessage) {
  if (sectorIndex >= STORAGE_SECTORS) {
    Serial.printf("Error: Sector %u is beyond our storage area\n", sectorIndex);
    return false;
  }
  
  uint32_t sectorOffset = FLASH_TARGET_OFFSET + (sectorIndex * FLASH_SECTOR_SIZE);
  
  // Prepare test data
  memset(buffer, 0xAA, FLASH_SECTOR_SIZE); // Fill with pattern
  size_t msgLen = strlen(testMessage);
  memcpy(buffer, testMessage, msgLen);
  sprintf((char*)buffer + msgLen, " (Sector %u at offset 0x%X)", 
         sectorIndex, sectorOffset);
  
  // Write to the sector
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(sectorOffset, FLASH_SECTOR_SIZE);
  flash_range_program(sectorOffset, buffer, FLASH_SECTOR_SIZE);
  restore_interrupts(ints);
  
  Serial.printf("Wrote test data to sector %u (offset 0x%X)\n", 
               sectorIndex, sectorOffset);
  return true;
}

// Add this to setup() after initialization to test boundaries
void testFlashBoundaries() {
  Serial.println("\nTesting flash storage boundaries...");
  
  // Test first sector (already contains our header)
  Serial.println("First sector already contains our header");
  
  // Test last sector
  if (testWriteSector(STORAGE_SECTORS - 1, "Testing last sector")) {
    Serial.println("Last sector write successful!");
  }
  
  // Test middle sector
  if (testWriteSector(STORAGE_SECTORS / 2, "Testing middle sector")) {
    Serial.println("Middle sector write successful!");
  }
  
  Serial.println("Boundary testing completed");
}
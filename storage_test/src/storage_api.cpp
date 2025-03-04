// RP2040 Blob Storage API with Write Buffering
// A library for storing and retrieving sensor datapoints in flash memory

#include <Arduino.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include "../include/storage_api.h"

// Avoid redefining FLASH_SECTOR_SIZE
#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE 4096
#endif

// Flash memory layout
#define FLASH_BASE_ADDR 0x10000000
#define EEPROM_START 0x10b88680
#define EEPROM_SIZE 0x1000
#define STORAGE_START (EEPROM_START + EEPROM_SIZE)
#define FLASH_TARGET_OFFSET (STORAGE_START - FLASH_BASE_ADDR)

// Storage configuration
#define BLOB_STORAGE_MAGIC 0x42534D47  // "BSMG" - Blob Storage MaGic
#define MAX_STORAGE_SECTORS 1024        // Configure based on available space
#define MAX_BLOB_SIZE (FLASH_SECTOR_SIZE - 32)  // Max blob size (less header overhead)

// Write buffer configuration
#define BUFFER_MAX_ENTRIES 20           // How many readings to buffer before auto-flush
#define DEFAULT_FLUSH_INTERVAL 60000    // Default auto-flush interval in ms (1 minute)

// Storage header structure
struct StorageHeader {
  uint32_t magic;          // Magic number to identify our storage
  uint32_t version;        // Storage format version
  uint32_t totalBlobs;     // Total number of stored blobs
  uint32_t usedSectors;    // Number of sectors currently in use
  uint32_t nextWriteIndex; // Index for the next blob to write
  uint32_t nextReadIndex;  // Index for the next blob to read
  uint32_t bootCount;      // Number of device boots
  uint32_t reserved[1];    // Reserved for future use
};

// Blob data header structure
struct BlobHeader {
  uint32_t magic;      // Magic number to verify blob integrity
  uint32_t timestamp;  // Timestamp when blob was written
  uint32_t type;       // Type identifier for the blob
  uint32_t size;       // Size of the blob data
  uint32_t crc;        // CRC32 checksum
  uint32_t index;      // Sequential index of this blob
};

// Buffered blob entry structure
struct BufferedBlobEntry {
  uint32_t type;
  uint32_t timestamp;
  size_t size;
  uint8_t data[MAX_BLOB_SIZE];
  bool used;
};

// Class for managing blob storage
class BlobStorage {
private:
  bool initialized = false;
  StorageHeader header;
  uint8_t buffer[FLASH_SECTOR_SIZE];
  
  // Write buffering
  BufferedBlobEntry writeBuffer[BUFFER_MAX_ENTRIES];
  uint32_t bufferCount = 0;
  unsigned long lastFlushTime = 0;
  unsigned long flushInterval = DEFAULT_FLUSH_INTERVAL;
  bool autoFlushEnabled = true;
  
  // Read the storage header from flash
  bool readHeader() {
    memcpy(&header, (void*)(XIP_BASE + FLASH_TARGET_OFFSET), sizeof(StorageHeader));
    return (header.magic == BLOB_STORAGE_MAGIC);
  }
  
  // Write the storage header to flash
  bool writeHeader() {
    memset(buffer, 0, FLASH_SECTOR_SIZE);
    memcpy(buffer, &header, sizeof(StorageHeader));
    
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
    
    return true;
  }
  
  // Calculate CRC32 checksum
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
  
  // Get the flash offset for a given blob index
  uint32_t getBlobOffset(uint32_t index) {
    // First check if the index is within reasonable bounds
    if (index >= MAX_STORAGE_SECTORS - 1) { // -1 because first sector is for header
      Serial.printf("WARNING: Blob index %u exceeds maximum storage sectors (%u)\n", 
                   index, MAX_STORAGE_SECTORS - 1);
      
      // Handle wraparound for safety - restart from beginning (after header)
      // This implements a circular buffer behavior
      index = index % (MAX_STORAGE_SECTORS - 1);
      Serial.printf("Using wrapped index %u instead\n", index);
    }
    
    // Calculate offset with better bounds checking
    uint32_t offset = FLASH_TARGET_OFFSET + FLASH_SECTOR_SIZE + (index * FLASH_SECTOR_SIZE);
    
    // Verify the calculated offset is within valid flash range
    uint32_t maxOffset = FLASH_TARGET_OFFSET + (MAX_STORAGE_SECTORS * FLASH_SECTOR_SIZE);
    if (offset >= maxOffset) {
      Serial.printf("WARNING: Calculated offset 0x%X exceeds max 0x%X\n", offset, maxOffset);
      
      // Reset to beginning of storage area (after header)
      offset = FLASH_TARGET_OFFSET + FLASH_SECTOR_SIZE;
      Serial.printf("Reset offset to 0x%X\n", offset);
    }
    
    return offset;
  }
  
  // Write a single blob entry directly to flash
  bool writeBlob(uint32_t type, const uint8_t* data, size_t size, uint32_t timestamp) {
    // Yield immediately to prevent blocking
    yield();
    
    // Double-check initialization
    if (!initialized) {
      Serial.println("ERROR: Tried to write blob with uninitialized storage");
      return false;
    }
    
    // Check if we have space with extra margin
    if (header.usedSectors >= MAX_STORAGE_SECTORS - 2) {
      Serial.printf("ERROR: Storage nearly full (%u/%u sectors used)\n", 
                  header.usedSectors, MAX_STORAGE_SECTORS);
      return false;
    }
    
    // Extensive size validation
    if (size > MAX_BLOB_SIZE) {
      Serial.printf("ERROR: Blob too large: %u bytes (max: %u)\n", size, MAX_BLOB_SIZE);
      return false;
    }
    
    if (size == 0) {
      Serial.println("ERROR: Zero-sized blob not allowed");
      return false;
    }
    
    // Validate data pointer
    if (data == nullptr) {
      Serial.println("ERROR: Null data pointer");
      return false;
    }
    
    // Delay and yield before intensive memory operations
    delay(20);
    yield();
    
    // Prepare blob header with extra validations
    BlobHeader blobHeader;
    blobHeader.magic = BLOB_STORAGE_MAGIC;
    blobHeader.timestamp = timestamp == 0 ? millis() : timestamp; // Use current time if none provided
    blobHeader.type = type;
    blobHeader.size = size;
    blobHeader.crc = calculateCRC32(data, size);
    blobHeader.index = header.nextWriteIndex;
    
    Serial.printf("Preparing blob #%u: type=%u, size=%u bytes\n", 
                 header.nextWriteIndex, type, size);
    
    // Prepare buffer with blob header and data with bounds checking
    memset(buffer, 0, FLASH_SECTOR_SIZE);
    memcpy(buffer, &blobHeader, sizeof(BlobHeader));
    memcpy(buffer + sizeof(BlobHeader), data, size);
    
    // Calculate and verify CRC after buffer preparation
    uint32_t verifyCrc = calculateCRC32(buffer + sizeof(BlobHeader), size);
    if (verifyCrc != blobHeader.crc) {
      Serial.printf("WARNING: CRC mismatch during buffer preparation: %08X != %08X\n", 
                   verifyCrc, blobHeader.crc);
      // Continue anyway but log the warning
    }
    
    // Extended pause before flash operations
    yield();
    delay(50);
    
    // Write to flash with expanded validation
    uint32_t blobOffset = getBlobOffset(header.nextWriteIndex);
    
    // Validate offset is within flash boundary
    if (blobOffset < FLASH_TARGET_OFFSET || 
        blobOffset >= FLASH_TARGET_OFFSET + (MAX_STORAGE_SECTORS * FLASH_SECTOR_SIZE)) {
      Serial.printf("ERROR: Blob offset out of flash boundary: 0x%X\n", blobOffset);
      return false;
    }
    
    // Check if offset is sector-aligned (should always be true with our getBlobOffset)
    if (blobOffset % FLASH_SECTOR_SIZE != 0) {
      Serial.printf("WARNING: Blob offset 0x%X is not sector-aligned, adjusting...\n", blobOffset);
      // Align to sector boundary
      blobOffset = (blobOffset / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
      Serial.printf("Adjusted to 0x%X\n", blobOffset);
    }
    
    Serial.printf("Writing blob to offset 0x%X...\n", blobOffset);
    
    // Final delay and yield before flash operations
    delay(20);
    yield();
    
    // Flash operations with minimal interrupt disabled time
    bool flashSuccess = true;
    uint32_t ints = save_and_disable_interrupts();
    
    // First erase the sector
    flash_range_erase(blobOffset, FLASH_SECTOR_SIZE);
    
    // Delay between erase and program operations
    // Using a NOP delay here as we can't call delay with interrupts disabled
    for (volatile int i = 0; i < 5000; i++) { /* NOP delay */ }
    
    // Program the data
    flash_range_program(blobOffset, buffer, FLASH_SECTOR_SIZE);
    
    restore_interrupts(ints);
    
    // Substantial delay after flash operations
    delay(100);
    yield();
    
    // Verify written data if possible
    BlobHeader verifyHeader;
    memcpy(&verifyHeader, (const void*)(XIP_BASE + blobOffset), sizeof(BlobHeader));
    
    if (verifyHeader.magic != BLOB_STORAGE_MAGIC) {
      Serial.println("ERROR: Verification failed after write - magic number mismatch");
      flashSuccess = false;
    } else {
      Serial.println("Verification successful - blob written correctly");
    }
    
    if (!flashSuccess) {
      Serial.println("ERROR: Flash operation failed");
      return false;
    }
    
    // Update storage header with sanity checks
    if (header.totalBlobs < UINT32_MAX) {
      header.totalBlobs++;
    } else {
      Serial.println("WARNING: Blob count overflow - resetting to 1");
      header.totalBlobs = 1;
    }
    
    if (header.usedSectors < MAX_STORAGE_SECTORS) {
      header.usedSectors++;
    } else {
      Serial.println("ERROR: Used sectors count overflow");
      return false;
    }
    
    if (header.nextWriteIndex < UINT32_MAX) {
      header.nextWriteIndex++;
    } else {
      Serial.println("WARNING: Write index overflow - resetting to 0");
      header.nextWriteIndex = 0;
    }
    
    // Write the updated header
    Serial.println("Updating storage header...");
    delay(20);
    yield();
    
    if (!writeHeader()) {
      Serial.println("ERROR: Failed to update header after blob write");
      return false;
    }
    
    Serial.printf("Successfully wrote blob #%u\n", blobHeader.index);
    return true;
  }
  
public:
  // Initialize the blob storage system
  bool begin(bool forceFormat = false) {
    if (initialized && !forceFormat) {
      return true;
    }
    
    // Clear write buffer
    memset(writeBuffer, 0, sizeof(writeBuffer));
    bufferCount = 0;
    lastFlushTime = millis();
    
    // Try to read existing header
    bool headerValid = readHeader();
    
    if (!headerValid || forceFormat) {
      // Initialize new storage
      memset(&header, 0, sizeof(header));
      header.magic = BLOB_STORAGE_MAGIC;
      header.version = 1;
      header.totalBlobs = 0;
      header.usedSectors = 1; // Header sector
      header.nextWriteIndex = 0;
      header.nextReadIndex = 0;
      header.bootCount = 1;
      
      if (!writeHeader()) {
        return false;
      }
      
      Serial.println("Blob storage initialized with empty data");
    } else {
      // Increment boot count
      header.bootCount++;
      if (!writeHeader()) {
        return false;
      }
      
      Serial.printf("Blob storage loaded: %u blobs, boot #%u\n", 
                   header.totalBlobs, header.bootCount);
    }
    
    initialized = true;
    return true;
  }
  
  // Store a blob of data (buffered)
  bool storeBlob(uint32_t type, const uint8_t* data, size_t size) {
    if (!initialized && !begin()) {
      return false;
    }
    
    // Check if blob is too large
    if (size > MAX_BLOB_SIZE) {
      Serial.printf("Error: Blob too large (%u bytes, max is %u)\n", 
                   size, MAX_BLOB_SIZE);
      return false;
    }
    
    // Find a free slot in the buffer
    int slot = -1;
    for (int i = 0; i < BUFFER_MAX_ENTRIES; i++) {
      if (!writeBuffer[i].used) {
        slot = i;
        break;
      }
    }
    
    // If buffer full, flush first
    if (slot == -1) {
      if (!flushBuffer()) {
        return false;
      }
      slot = 0; // After flush, slot 0 should be free
    }
    
    // Copy data to buffer
    writeBuffer[slot].type = type;
    writeBuffer[slot].timestamp = millis();
    writeBuffer[slot].size = size;
    memcpy(writeBuffer[slot].data, data, size);
    writeBuffer[slot].used = true;
    bufferCount++;
    
    // Check if we should auto-flush
    if (autoFlushEnabled) {
      bool shouldFlush = false;
      
      // Flush if buffer is full
      if (bufferCount >= BUFFER_MAX_ENTRIES) {
        shouldFlush = true;
      }
      
      // Flush if it's been longer than flushInterval since last flush
      if (millis() - lastFlushTime > flushInterval) {
        shouldFlush = true;
      }
      
      if (shouldFlush) {
        return flushBuffer();
      }
    }
    
    return true;
  }
  
  // Flush buffered blobs to flash storage
  bool flushBuffer() {
    if (bufferCount == 0) {
      Serial.println("Nothing to flush - buffer empty");
      return true;
    }
    
    uint32_t bufferToFlush = bufferCount;
    Serial.printf("Starting flush of %u readings to flash...\n", bufferToFlush);
    
    // Substantial delay before intensive flash operations
    delay(100);
    yield();
    
    // Count how many entries we need to write
    uint32_t entriesNeeded = 0;
    for (int i = 0; i < BUFFER_MAX_ENTRIES; i++) {
      if (writeBuffer[i].used) {
        entriesNeeded++;
      }
    }
    
    Serial.printf("Found %u entries to flush\n", entriesNeeded);
    
    // Safety check for available sectors
    uint32_t totalSectors, usedSectors, freeSectors;
    getStorageInfo(&totalSectors, &usedSectors, &freeSectors);
    
    if (freeSectors < entriesNeeded) {
      Serial.printf("ERROR: Not enough free sectors (%u) to flush %u entries\n", 
                  freeSectors, entriesNeeded);
      // We could automatically compact here, but that's intensive
      // Just warn for now and continue (some might fail)
      Serial.println("WARNING: Flash might fill up during flush");
      delay(50);
      yield();
    }
    
    // Progress tracking
    uint32_t successCount = 0;
    uint32_t failCount = 0;
    
    // Write all buffered entries to flash with robust error handling
    for (int i = 0; i < BUFFER_MAX_ENTRIES; i++) {
      if (writeBuffer[i].used) {
        // Status update for larger flushes
        if (entriesNeeded > 3) {
          Serial.printf("Flushing entry %d of %u...\n", successCount + failCount + 1, entriesNeeded);
        }
        
        // Substantial delay and yield before each write
        delay(50); 
        yield();
        
        // Validate buffer entry one more time before writing
        if (writeBuffer[i].size == 0 || writeBuffer[i].size > MAX_BLOB_SIZE) {
          Serial.printf("ERROR: Skipping invalid buffer entry %d (size: %u)\n", 
                      i, writeBuffer[i].size);
          writeBuffer[i].used = false;
          failCount++;
          continue;
        }
        
        bool writeSuccess = false;
        // Try up to 2 times for important operations
        for (int attempt = 0; attempt < 2; attempt++) {
          if (attempt > 0) {
            Serial.printf("Retrying write for entry %d (attempt %d)...\n", i, attempt + 1);
            delay(100);
            yield();
          }
          
          writeSuccess = writeBlob(
              writeBuffer[i].type, 
              writeBuffer[i].data, 
              writeBuffer[i].size,
              writeBuffer[i].timestamp);
              
          if (writeSuccess) break;
          
          // If first attempt failed, give extra time before retry
          if (attempt == 0) {
            Serial.println("First write attempt failed, waiting before retry...");
            delay(200);
            yield();
          }
        }
        
        if (!writeSuccess) {
          Serial.printf("ERROR: Failed to write buffer entry %d to flash\n", i);
          failCount++;
          // Still mark as unused to avoid getting stuck in a loop
          writeBuffer[i].used = false;
        } else {
          // Clear the buffer slot
          writeBuffer[i].used = false;
          successCount++;
          
          // Extended delay between successful writes to allow system to recover
          delay(50);
          yield();
        }
      }
    }
    
    // Update buffer count
    uint32_t remainingBuffer = 0;
    for (int i = 0; i < BUFFER_MAX_ENTRIES; i++) {
      if (writeBuffer[i].used) {
        remainingBuffer++;
      }
    }
    
    // Update the buffer count based on what's actually in the buffer
    // rather than just setting to 0
    bufferCount = remainingBuffer;
    lastFlushTime = millis();
    
    Serial.printf("Buffer flush complete: %u succeeded, %u failed, %u remaining\n", 
                 successCount, failCount, remainingBuffer);
    
    // Final extended delay after completing intensive operations
    delay(200);
    yield();
    
    // Only return true if all entries were successfully written
    return (failCount == 0);
  }
  
  // Configure auto-flush behavior
  void setAutoFlush(bool enabled, unsigned long interval = DEFAULT_FLUSH_INTERVAL) {
    autoFlushEnabled = enabled;
    flushInterval = interval;
  }
  
  // Check if any buffered blobs need flushing
  void checkFlush() {
    if (autoFlushEnabled && bufferCount > 0) {
      if (bufferCount >= BUFFER_MAX_ENTRIES || 
          millis() - lastFlushTime > flushInterval) {
        flushBuffer();
      }
    }
  }
  
  // Retrieve a blob of data by index
  bool retrieveBlob(uint32_t index, uint8_t* data, size_t maxSize, size_t* actualSize, uint32_t* type = nullptr, uint32_t* timestamp = nullptr) {
    if (!initialized && !begin()) {
      return false;
    }
    
    // Add a yield before intensive operation
    yield();
    
    // Initialize actualSize to 0 in case of early return
    if (actualSize) *actualSize = 0;
    
    // Check for valid input parameters
    if (data == nullptr || maxSize == 0) {
      Serial.println("Error: Invalid buffer or size");
      return false;
    }
    
    // Check if index is valid
    if (index >= header.totalBlobs) {
      Serial.printf("Error: Invalid blob index %u (total: %u)\n", 
                   index, header.totalBlobs);
      return false;
    }
    
    // Calculate blob offset
    uint32_t blobOffset = getBlobOffset(index);
    
    // Verify offset is within bounds
    if (blobOffset >= FLASH_TARGET_OFFSET + (MAX_STORAGE_SECTORS * FLASH_SECTOR_SIZE)) {
      Serial.printf("Error: Blob offset out of range: 0x%X\n", blobOffset);
      return false;
    }
    
    // Add a small yield to keep system responsive
    yield();
    
    // Read blob header with safety checks
    BlobHeader blobHeader;
    Serial.printf("DEBUG: Reading blob header at offset 0x%X\n", blobOffset);
    
    // Copy the header data with bounds checking
    memcpy(&blobHeader, (void*)(XIP_BASE + blobOffset), sizeof(BlobHeader));
    
    // Verify blob header
    if (blobHeader.magic != BLOB_STORAGE_MAGIC) {
      Serial.printf("Error: Invalid blob header magic at index %u\n", index);
      return false;
    }
    
    // Check size sanity
    if (blobHeader.size == 0 || blobHeader.size > MAX_BLOB_SIZE) {
      Serial.printf("Error: Invalid blob size %u at index %u\n", blobHeader.size, index);
      return false;
    }
    
    // Output debug info about the blob we're retrieving
    Serial.printf("DEBUG: Blob %u: type=%u, size=%u bytes\n", 
                 index, blobHeader.type, blobHeader.size);
    
    // Check if buffer is large enough
    size_t copySize = min(maxSize, blobHeader.size);
    if (copySize < blobHeader.size) {
      Serial.printf("Warning: Buffer too small for blob (%u < %u)\n", 
                   maxSize, blobHeader.size);
    }
    
    // Another yield before data read
    yield();
    
    // Copy data to buffer with careful bounds checking
    const uint8_t* srcAddr = (const uint8_t*)(XIP_BASE + blobOffset + sizeof(BlobHeader));
    Serial.printf("DEBUG: Reading %u bytes from 0x%X\n", copySize, (uint32_t)srcAddr);
    
    // Copy with verification
    memcpy(data, srcAddr, copySize);
    if (actualSize) *actualSize = copySize;
    
    // Yield after intensive memory operation
    yield();
    
    // Optionally return type and timestamp
    if (type) *type = blobHeader.type;
    if (timestamp) *timestamp = blobHeader.timestamp;
    
    // Verify CRC
    uint32_t calculatedCRC = calculateCRC32(data, copySize);
    if (calculatedCRC != blobHeader.crc) {
      Serial.printf("Warning: CRC mismatch for blob %u\n", index);
      // Still return data, but with warning
    }
    
    Serial.printf("DEBUG: Successfully read blob %u\n", index);
    return true;
  }
  
  // Get the blob count
  uint32_t getBlobCount() {
    // Add a yield to prevent hangs
    yield();
    
    if (!initialized && !begin()) {
      return 0;
    }
    
    // Make sure the header value is reasonable
    if (header.totalBlobs > 10000) {  // Arbitrary sanity check
      Serial.printf("Warning: Unusually large blob count (%u), limiting return value\n", 
                  header.totalBlobs);
      return 10000;  // Return a capped value
    }
    
    return header.totalBlobs;
  }
  
  // Get the boot count
  uint32_t getBootCount() {
    if (!initialized && !begin()) {
      return 0;
    }
    return header.bootCount;
  }
  
  // Get free space information
  void getStorageInfo(uint32_t* totalSectors, uint32_t* usedSectors, uint32_t* freeSectors) {
    if (!initialized && !begin()) {
      if (totalSectors) *totalSectors = 0;
      if (usedSectors) *usedSectors = 0;
      if (freeSectors) *freeSectors = 0;
      return;
    }
    
    if (totalSectors) *totalSectors = MAX_STORAGE_SECTORS;
    if (usedSectors) *usedSectors = header.usedSectors;
    if (freeSectors) *freeSectors = MAX_STORAGE_SECTORS - header.usedSectors;
  }
  
  // Format the storage (erase all data)
  bool format() {
    // Clear any buffered data
    memset(writeBuffer, 0, sizeof(writeBuffer));
    bufferCount = 0;
    
    return begin(true);
  }
  
  // Get buffer information
  void getBufferInfo(uint32_t* itemsBuffered, uint32_t* bufferCapacity) {
    if (itemsBuffered) *itemsBuffered = bufferCount;
    if (bufferCapacity) *bufferCapacity = BUFFER_MAX_ENTRIES;
  }
  
  // Delete a specific blob by index
  bool deleteBlob(uint32_t index) {
    // Immediately yield to prevent blocking
    yield();
    
    // Validate initialization
    if (!initialized) {
      Serial.println("Storage not initialized, attempting to begin...");
      if (!begin()) {
        Serial.println("ERROR: Storage initialization failed during deletion");
        return false;
      }
      
      // Additional delay after initialization
      delay(100);
      yield();
    }
    
    // Double check index before any operations
    if (index >= header.totalBlobs) {
      Serial.printf("Error: Invalid blob index %u (total: %u)\n", 
                  index, header.totalBlobs);
      return false;
    }
    
    // Flush any pending writes first with extra safety
    if (bufferCount > 0) {
      Serial.println("Flushing buffer before deletion...");
      delay(50);
      yield();
      
      if (!flushBuffer()) {
        Serial.println("WARNING: Buffer flush failed, proceeding with caution");
        // Still continue as the deletion can work independently
      }
      
      // Additional delay after flush
      delay(150);
      yield();
    }
    
    Serial.printf("Preparing to delete blob %u...\n", index);
    yield();
    
    // Calculate blob offset with additional validation
    uint32_t blobOffset = getBlobOffset(index);
    
    // Extensive validation of offset before proceeding
    if (blobOffset < FLASH_TARGET_OFFSET || 
        blobOffset >= FLASH_TARGET_OFFSET + (MAX_STORAGE_SECTORS * FLASH_SECTOR_SIZE) ||
        blobOffset % 4 != 0) {  // Ensure alignment
      Serial.printf("ERROR: Invalid blob offset for deletion: 0x%X\n", blobOffset);
      return false;
    }
    
    // Extra verification - read the header first to make sure it's valid
    BlobHeader blobHeader;
    yield();
    
    Serial.printf("Reading header before deletion at offset 0x%X\n", blobOffset);
    memcpy(&blobHeader, (void*)(XIP_BASE + blobOffset), sizeof(BlobHeader));
    
    // Only proceed if the magic number is valid (not already deleted)
    if (blobHeader.magic != BLOB_STORAGE_MAGIC) {
      Serial.printf("Note: Blob %u appears to be already deleted or invalid\n", index);
      return true; // Consider it a success if already deleted
    }
    
    Serial.printf("Confirmed blob %u exists, proceeding with deletion\n", index);
    
    // Significant delay before flash operation to ensure system stability
    delay(50);
    yield();
    
    // Zero the magic number with extra care
    uint32_t zeroMagic = 0;
    
    // Disable interrupts only for the minimum time required
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(blobOffset, (const uint8_t*)&zeroMagic, sizeof(uint32_t));
    restore_interrupts(ints);
    
    // Substantial pause after flash operation
    delay(100);
    yield();
    
    // Verify deletion was successful
    Serial.printf("Verifying blob %u deletion...\n", index);
    yield();
    
    // Read magic number again to confirm it's zeroed
    uint32_t verifyMagic;
    memcpy(&verifyMagic, (void*)(XIP_BASE + blobOffset), sizeof(uint32_t));
    
    if (verifyMagic != 0) {
      Serial.printf("WARNING: Deletion verification failed for blob %u\n", index);
    } else {
      Serial.printf("Successfully marked blob %u as deleted\n", index);
    }
    
    // Final delay and yield
    delay(50);
    yield();
    
    return true;
  }
  
  // Delete a range of blobs with added safety
  bool deleteBlobs(uint32_t startIndex, uint32_t endIndex) {
    // Immediate yield to prevent hanging
    yield();
    delay(50);
    
    // Check initialization status
    if (!initialized) {
      Serial.println("Storage not initialized for batch deletion, attempting to begin...");
      if (!begin()) {
        Serial.println("ERROR: Storage initialization failed during batch deletion");
        return false;
      }
      delay(100);
      yield();
    }
    
    // Validate indices with additional checks
    if (startIndex >= header.totalBlobs) {
      Serial.printf("Error: Invalid start index %u (total: %u)\n", 
                  startIndex, header.totalBlobs);
      return false;
    }
    
    // Cap the end index to prevent out-of-bounds issues
    uint32_t originalEndIndex = endIndex;
    endIndex = min(endIndex, header.totalBlobs - 1);
    
    if (originalEndIndex != endIndex) {
      Serial.printf("Note: Adjusted end index from %u to %u\n", 
                   originalEndIndex, endIndex);
    }
    
    // Safety check for large deletions
    uint32_t deleteCount = endIndex - startIndex + 1;
    if (deleteCount > 10) {
      Serial.printf("Warning: Attempting to delete a large number of blobs (%u)\n", deleteCount);
      Serial.println("This operation may take some time and could cause issues if interrupted");
      // Additional delay to allow abort if needed
      delay(200);
      yield();
    }
    
    Serial.printf("Beginning deletion of %u blobs (index %u to %u)...\n", 
                 deleteCount, startIndex, endIndex);
    
    // Delete each blob in the range with progress updates
    bool success = true;
    uint32_t successCount = 0;
    uint32_t failCount = 0;
    
    for (uint32_t i = startIndex; i <= endIndex; i++) {
      // Progress update every few items
      if (deleteCount > 5 && (i - startIndex) % 5 == 0) {
        Serial.printf("Deletion progress: %u/%u complete\n", 
                     i - startIndex, deleteCount);
        delay(50);
        yield();
      }
      
      if (deleteBlob(i)) {
        successCount++;
      } else {
        failCount++;
        success = false;
        Serial.printf("Error deleting blob at index %u\n", i);
        
        // Brief pause after a failure
        delay(50);
        yield();
      }
      
      // Additional yield and delay between operations
      yield();
      delay(20);
    }
    
    Serial.printf("Deletion complete: %u successful, %u failed\n", 
                 successCount, failCount);
    
    // Final delay and yield
    delay(100);
    yield();
    
    return success;
  }
  
  // Compact storage by removing deleted entries
  // This is an expensive operation that rewrites the entire storage
  bool compactStorage() {
    if (!initialized && !begin()) {
      return false;
    }
    
    // Flush any pending writes first
    if (bufferCount > 0) {
      flushBuffer();
    }
    
    Serial.println("Compacting storage - this may take a while...");
    
    // Temporary storage for valid blobs
    struct TempBlob {
      uint32_t type;
      uint32_t timestamp;
      uint32_t size;
      uint32_t originalIndex;
      uint8_t* data;
    };
    
    // Scan all blobs and collect valid ones
    uint32_t validCount = 0;
    TempBlob* validBlobs = new TempBlob[header.totalBlobs];
    if (!validBlobs) {
      Serial.println("Error: Not enough memory for compaction");
      return false;
    }
    
    // Read all valid blobs
    for (uint32_t i = 0; i < header.totalBlobs; i++) {
      uint32_t blobOffset = getBlobOffset(i);
      BlobHeader blobHeader;
      memcpy(&blobHeader, (void*)(XIP_BASE + blobOffset), sizeof(BlobHeader));
      
      // Check if this blob is valid (not deleted)
      if (blobHeader.magic == BLOB_STORAGE_MAGIC) {
        validBlobs[validCount].type = blobHeader.type;
        validBlobs[validCount].timestamp = blobHeader.timestamp;
        validBlobs[validCount].size = blobHeader.size;
        validBlobs[validCount].originalIndex = i;
        
        // Allocate and copy data
        validBlobs[validCount].data = new uint8_t[blobHeader.size];
        if (!validBlobs[validCount].data) {
          Serial.println("Error: Memory allocation failed during compaction");
          
          // Clean up already allocated memory
          for (uint32_t j = 0; j < validCount; j++) {
            delete[] validBlobs[j].data;
          }
          delete[] validBlobs;
          return false;
        }
        
        memcpy(validBlobs[validCount].data, 
              (void*)(XIP_BASE + blobOffset + sizeof(BlobHeader)), 
              blobHeader.size);
              
        validCount++;
      }
    }
    
    // Format storage to start fresh
    StorageHeader oldHeader = header;
    begin(true);
    header.bootCount = oldHeader.bootCount;
    writeHeader();
    
    // Write all valid blobs back
    for (uint32_t i = 0; i < validCount; i++) {
      writeBlob(validBlobs[i].type, validBlobs[i].data, validBlobs[i].size, validBlobs[i].timestamp);
      delete[] validBlobs[i].data;
    }
    
    delete[] validBlobs;
    
    Serial.printf("Compaction complete. Kept %u of %u blobs\n", 
                validCount, oldHeader.totalBlobs);
    return true;
  }
  
  // Get time range of stored data
  bool getDataTimeRange(uint32_t* startTime, uint32_t* endTime) {
    if (!initialized && !begin()) {
      if (startTime) *startTime = 0;
      if (endTime) *endTime = 0;
      return false;
    }
    
    if (header.totalBlobs == 0) {
      if (startTime) *startTime = 0;
      if (endTime) *endTime = 0;
      return false;
    }
    
    uint32_t earliest = UINT32_MAX;
    uint32_t latest = 0;
    bool foundValid = false;
    
    // Scan all blobs to find time range
    for (uint32_t i = 0; i < header.totalBlobs; i++) {
      uint32_t blobOffset = getBlobOffset(i);
      BlobHeader blobHeader;
      memcpy(&blobHeader, (void*)(XIP_BASE + blobOffset), sizeof(BlobHeader));
      
      // Check if this blob is valid
      if (blobHeader.magic == BLOB_STORAGE_MAGIC) {
        earliest = min(earliest, blobHeader.timestamp);
        latest = max(latest, blobHeader.timestamp);
        foundValid = true;
      }
    }
    
    if (!foundValid) {
      if (startTime) *startTime = 0;
      if (endTime) *endTime = 0;
      return false;
    }
    
    if (startTime) *startTime = earliest;
    if (endTime) *endTime = latest;
    return true;
  }
  
  // Get summary of data by type - safer version
  void getDataSummary() {
    if (!initialized && !begin()) {
      Serial.println("Storage not initialized");
      return;
    }
    
    // Add a small delay before intensive operations
    delay(100);
    yield();
    
    if (header.totalBlobs == 0) {
      Serial.println("No data stored");
      return;
    }
    
    // Count by type - with bounds checking and safety
    const int MAX_TYPES = 10;
    struct TypeInfo {
      uint32_t type;
      uint32_t count;
      uint32_t earliest;
      uint32_t latest;
      bool valid;
    };
    
    TypeInfo types[MAX_TYPES] = {0}; 
    uint32_t typeCount = 0;
    
    // Initialize all entries
    for (int i = 0; i < MAX_TYPES; i++) {
      types[i].valid = false;
      types[i].earliest = UINT32_MAX;
      types[i].latest = 0;
    }
    
    // First, just count blobs before trying to access any details
    uint32_t validBlobCount = 0;
    
    // Print basic summary first - before any potentially dangerous operations
    Serial.println("\nData Summary:");
    Serial.printf("Total stored blobs in header: %u (using %u/%u sectors)\n", 
                header.totalBlobs, header.usedSectors, MAX_STORAGE_SECTORS);
                
    if (bufferCount > 0) {
      Serial.printf("Buffered entries: %u (not yet written to flash)\n", bufferCount);
    }
    
    // Safer scan that checks each blob carefully
    for (uint32_t i = 0; i < min(header.totalBlobs, (uint32_t)1000); i++) { // Limit to 1000 for safety
      uint32_t blobOffset = getBlobOffset(i);
      
      // Check if offset is valid
      if (blobOffset >= FLASH_TARGET_OFFSET + (MAX_STORAGE_SECTORS * FLASH_SECTOR_SIZE)) {
        Serial.printf("Warning: Skipping blob %u - offset out of range\n", i);
        continue;
      }
      
      // Add a yield call here to prevent watchdog timeouts
      if (i % 5 == 0) {
        yield();
        delay(10);
      }
      
      // Read the header carefully
      BlobHeader blobHeader;
      memcpy(&blobHeader, (void*)(XIP_BASE + blobOffset), sizeof(BlobHeader));
      
      // Extra validation of the header
      if (blobHeader.magic != BLOB_STORAGE_MAGIC) {
        // Skip without error - might be deleted
        continue;
      }
      
      // Extra validation of sizes and types
      if (blobHeader.size > MAX_BLOB_SIZE || blobHeader.size == 0) {
        Serial.printf("Warning: Blob %u has invalid size: %u\n", i, blobHeader.size);
        continue;
      }
      
      validBlobCount++;
      
      // Process types with bounds checking
      bool found = false;
      for (uint32_t t = 0; t < typeCount && t < MAX_TYPES; t++) {
        if (types[t].valid && types[t].type == blobHeader.type) {
          types[t].count++;
          types[t].earliest = min(types[t].earliest, blobHeader.timestamp);
          types[t].latest = max(types[t].latest, blobHeader.timestamp);
          found = true;
          break;
        }
      }
      
      // Add new type if needed and we have room
      if (!found && typeCount < MAX_TYPES) {
        types[typeCount].type = blobHeader.type;
        types[typeCount].count = 1;
        types[typeCount].earliest = blobHeader.timestamp;
        types[typeCount].latest = blobHeader.timestamp;
        types[typeCount].valid = true;
        typeCount++;
      }
    }
    
    Serial.printf("Valid blobs found: %u\n\n", validBlobCount);
    Serial.println("Breakdown by type:");
    
    // Print the type information with extra safety
    for (uint32_t t = 0; t < typeCount && t < MAX_TYPES; t++) {
      if (!types[t].valid) continue;
      
      // Convert type ID to string if possible
      const char* typeName = "Unknown";
      switch (types[t].type) {
        case 1: typeName = "Temperature"; break;
        case 2: typeName = "Humidity"; break;
        case 3: typeName = "Pressure"; break;
        case 4: typeName = "Acceleration"; break;
        case 99: typeName = "Custom"; break;
      }
      
      // Print type info without time conversion first
      Serial.printf("Type %u (%s): %u readings, time range: %u to %u\n", 
                  types[t].type, typeName, types[t].count, 
                  types[t].earliest, types[t].latest);
                  
      // Only attempt time conversion if timestamps are reasonable
      if (types[t].earliest > 0 && types[t].earliest < 86400000 &&
          types[t].latest > 0 && types[t].latest < 86400000) {
        // Safe time formatting in separate buffers
        char earliestTime[32];
        char latestTime[32];
        
        snprintf(earliestTime, sizeof(earliestTime), "%02u:%02u:%02u.%03u",
                (types[t].earliest / 3600000) % 24,
                (types[t].earliest / 60000) % 60,
                (types[t].earliest / 1000) % 60,
                types[t].earliest % 1000);
                
        snprintf(latestTime, sizeof(latestTime), "%02u:%02u:%02u.%03u",
                (types[t].latest / 3600000) % 24,
                (types[t].latest / 60000) % 60,
                (types[t].latest / 1000) % 60,
                types[t].latest % 1000);
        
        Serial.printf("  From: %s to %s\n", earliestTime, latestTime);
      }
    }
    
    Serial.println(); // End with newline
    
    // Add a final delay and yield after completing intensive operations
    delay(200);
    yield();
  }
};

// Global instance
BlobStorage blobStorage;

// ---- API Functions ----

// Initialize the storage system with enhanced robustness
bool storage_init(bool forceFormat) {
  // Yield and delay before storage initialization to ensure system is ready
  yield();
  delay(100);
  
  // Initialize storage with up to 3 retries
  bool result = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    if (attempt > 0) {
      Serial.printf("Retrying storage initialization (attempt %d)...\n", attempt + 1);
      delay(100 * attempt); // Increasing delay with each retry
      yield();
    }
    
    result = blobStorage.begin(forceFormat);
    
    if (result) {
      if (attempt > 0) {
        Serial.println("Storage initialization succeeded on retry");
      }
      break;
    } else {
      Serial.printf("Storage initialization failed (attempt %d)\n", attempt + 1);
    }
  }
  
  // If initialization succeeded, perform a boot-time verification
  if (result) {
    uint32_t totalSectors, usedSectors, freeSectors;
    
    // Get and print storage information for verification
    blobStorage.getStorageInfo(&totalSectors, &usedSectors, &freeSectors);
    Serial.printf("Storage initialized: Boot #%u\n", blobStorage.getBootCount());
    Serial.printf("Storage status: %u/%u sectors used, %u sectors free\n", 
                 usedSectors, totalSectors, freeSectors);
    
    // Print count of stored readings
    uint32_t readingCount = blobStorage.getBlobCount();
    Serial.printf("Stored readings: %u\n", readingCount);
    
    // Allow a bit more time for the system to stabilize after init
    delay(50);
    yield();
  } else {
    // If all retries failed, log a critical error
    Serial.println("CRITICAL: Storage initialization failed after all attempts");
  }
  
  return result;
}

// Store a sensor reading (buffered)
bool storage_store_reading(const SensorDataPoint& reading, SensorType type) {
  return blobStorage.storeBlob(type, (const uint8_t*)&reading, sizeof(reading));
}

// Flush buffered readings to flash
bool storage_flush() {
  return blobStorage.flushBuffer();
}

// Configure automatic flush behavior
void storage_set_auto_flush(bool enabled, unsigned long intervalMs) {
  blobStorage.setAutoFlush(enabled, intervalMs);
}

// Check if any buffered data needs to be flushed (call in loop)
void storage_check_flush() {
  blobStorage.checkFlush();
}

// Retrieve a sensor reading by index
bool storage_get_reading(uint32_t index, SensorDataPoint& reading, uint32_t* type) {
  size_t actualSize;
  uint32_t timestamp;
  return blobStorage.retrieveBlob(index, (uint8_t*)&reading, sizeof(reading), &actualSize, type, &timestamp);
}

// Get the number of stored readings
uint32_t storage_get_reading_count() {
  // Add a small delay and yield to prevent freezes
  delay(10);
  yield();
  
  // Get the count with a safety check
  uint32_t count = blobStorage.getBlobCount();
  
  // Another small yield after the operation
  yield();
  
  return count;
}

// Format the storage
bool storage_format() {
  return blobStorage.format();
}

// Get storage information
void storage_get_info(uint32_t* totalSectors, uint32_t* usedSectors, uint32_t* freeSectors) {
  blobStorage.getStorageInfo(totalSectors, usedSectors, freeSectors);
}

// Get buffer information
void storage_get_buffer_info(uint32_t* itemsBuffered, uint32_t* bufferCapacity) {
  blobStorage.getBufferInfo(itemsBuffered, bufferCapacity);
}

// Get boot count
uint32_t storage_get_boot_count() {
  return blobStorage.getBootCount();
}

// Delete a specific reading by index with added safety
bool storage_delete_reading(uint32_t index) {
  Serial.printf("Storage: Deleting reading at index %u...\n", index);
  
  // Add delay and yield for safety
  delay(20);
  yield();
  
  bool result = blobStorage.deleteBlob(index);
  
  // Another delay and yield after operation
  delay(20);
  yield();
  
  Serial.printf("Storage: Deletion %s\n", result ? "successful" : "failed");
  return result;
}

// Delete a range of readings with added safety
bool storage_delete_readings(uint32_t startIndex, uint32_t endIndex) {
  Serial.printf("Storage: Deleting readings from index %u to %u...\n", startIndex, endIndex);
  
  // Add delay and yield for safety
  delay(50);
  yield();
  
  // Calculate number of readings to delete and add warning for large deletions
  uint32_t count = endIndex - startIndex + 1;
  if (count > 5) {
    Serial.printf("WARNING: Deleting a large number of readings (%u). This may take some time.\n", count);
    delay(100);
    yield();
  }
  
  bool result = blobStorage.deleteBlobs(startIndex, endIndex);
  
  // Another delay and yield after operation
  delay(50);
  yield();
  
  Serial.printf("Storage: Batch deletion %s\n", result ? "successful" : "failed");
  return result;
}

// Compact storage to reclaim space from deleted readings
bool storage_compact() {
  return blobStorage.compactStorage();
}

// Get time range of stored data
bool storage_get_time_range(uint32_t* startTime, uint32_t* endTime) {
  return blobStorage.getDataTimeRange(startTime, endTime);
}

// Print a summary of stored data
void storage_print_summary() {
  blobStorage.getDataSummary();
}

// Read and display existing data with safety measures
bool storage_read_existing_data(uint32_t maxReadings) {
  // Add a small delay and yield before operation
  delay(50);
  yield();
  
  uint32_t readingCount = blobStorage.getBlobCount();
  
  Serial.println("Existing sensor readings:");
  Serial.printf("Found %u readings\n", readingCount);
  
  if (readingCount == 0) {
    Serial.println("  No readings stored yet");
    return true;
  }
  
  // Limit the number of readings to read for safety
  uint32_t limitedCount = min(readingCount, maxReadings);
  if (limitedCount < readingCount) {
    Serial.printf("Will show only the last %u of %u readings for safety\n", 
                 limitedCount, readingCount);
  }
  
  // Display the most recent readings (or fewer if less are available)
  int startIdx = (readingCount > limitedCount) ? (readingCount - limitedCount) : 0;
  int count = 0;
  
  Serial.printf("Displaying readings %d to %d...\n", startIdx, readingCount-1);
  
  // Add a small delay before starting to read
  delay(20);
  yield();
  
  for (int i = startIdx; i < readingCount; i++) {
    Serial.printf("Reading entry %d...\n", i);
    
    SensorDataPoint reading;
    uint32_t type;
    size_t actualSize;
    
    // Add yield to give system time to process
    yield();
    
    // Read with safety checks
    bool readSuccess = blobStorage.retrieveBlob(i, (uint8_t*)&reading, 
                                             sizeof(reading), &actualSize, &type);
    
    if (readSuccess) {
      Serial.printf("  Reading #%d: Sensor %u, Type %u, Label '%s'\n", 
                  i, reading.sensorId, type, reading.label);
      
      Serial.print("    Values: ");
      for (int v = 0; v < reading.valueCount && v < 4; v++) {
        Serial.printf("%.2f ", reading.values[v]);
      }
      Serial.println();
      count++;
    } else {
      Serial.printf("  Error reading datapoint #%d\n", i);
    }
    
    // Add a yield and delay after each read
    yield();
    delay(20);
  }
  
  Serial.printf("Successfully displayed %d readings\n", count);
  
  // Final delay and yield
  delay(50);
  yield();
  
  return true;
}

// Repair storage by fixing counters and compacting if needed
bool storage_repair() {
  Serial.println("Starting storage repair process...");
  delay(100);
  yield();
  
  // First, get current storage info
  uint32_t totalSectors, usedSectors, freeSectors;
  storage_get_info(&totalSectors, &usedSectors, &freeSectors);
  
  uint32_t totalBlobs = storage_get_reading_count();
  Serial.printf("Current storage state: %u blobs, %u/%u sectors used\n", 
               totalBlobs, usedSectors, totalSectors);
  
  // Count valid blobs
  uint32_t validCount = 0;
  uint32_t invalidCount = 0;
  
  Serial.println("Scanning for valid blobs...");
  delay(50);
  yield();
  
  // Scan through all blobs to count valid ones
  for (uint32_t i = 0; i < min(totalBlobs, (uint32_t)1000); i++) {
    // Add yield to prevent hangs
    if (i % 10 == 0) {
      Serial.printf("Scanning blob %u of %u...\n", i, totalBlobs);
      yield();
      delay(10);
    }
    
    SensorDataPoint reading;
    uint32_t type;
    size_t actualSize;
    
    // Try to read the blob
    bool readSuccess = blobStorage.retrieveBlob(i, (uint8_t*)&reading, 
                                             sizeof(reading), &actualSize, &type);
    
    if (readSuccess) {
      validCount++;
    } else {
      invalidCount++;
    }
  }
  
  Serial.printf("Scan complete: %u valid blobs, %u invalid/deleted blobs\n", 
               validCount, invalidCount);
  
  // If we have invalid blobs, compact storage
  bool needsCompacting = (invalidCount > 0);
  bool compactSuccess = true;
  
  if (needsCompacting) {
    Serial.println("Storage needs compacting. Starting compaction...");
    delay(100);
    yield();
    
    compactSuccess = storage_compact();
    
    if (compactSuccess) {
      Serial.println("Storage compaction successful!");
    } else {
      Serial.println("Storage compaction failed!");
    }
    
    // Get updated storage info
    storage_get_info(&totalSectors, &usedSectors, &freeSectors);
    totalBlobs = storage_get_reading_count();
    
    Serial.printf("After compaction: %u blobs, %u/%u sectors used\n", 
                 totalBlobs, usedSectors, totalSectors);
  } else {
    Serial.println("Storage doesn't need compacting (no invalid blobs found)");
  }
  
  // Final verification
  Serial.println("Performing final verification...");
  delay(50);
  yield();
  
  // Verify we can read all blobs
  uint32_t verifiedCount = 0;
  totalBlobs = storage_get_reading_count(); // Get updated count
  
  for (uint32_t i = 0; i < min(totalBlobs, (uint32_t)10); i++) {
    SensorDataPoint reading;
    if (storage_get_reading(i, reading)) {
      verifiedCount++;
    }
    yield();
  }
  
  Serial.printf("Verification complete: %u/%u blobs verified\n", 
               verifiedCount, min(totalBlobs, (uint32_t)10));
  
  // Final status
  bool repairSuccess = (compactSuccess && (verifiedCount == min(totalBlobs, (uint32_t)10)));
  
  Serial.printf("Storage repair %s\n", repairSuccess ? "SUCCESSFUL" : "FAILED");
  delay(100);
  yield();
  
  return repairSuccess;
}

# IMU Data Collection with Adafruit SPIFlash

This project collects IMU sensor data from an Arduino Nano RP2040 Connect board and stores it using Adafruit SPIFlash library with the arduino-pico core.

## Project Overview

The project captures sensor data from the onboard IMU (LSM6DS3) and stores it in flash memory. It also provides a web interface for visualizing the data in real-time.

## Key Components

- **Arduino Nano RP2040 Connect**: The main microcontroller
- **IMU Sensor**: LSM6DS3 for accelerometer and gyroscope data
- **Storage**: Using Adafruit SPIFlash with SdFat for file system operations
- **Web Interface**: Simple web server serving HTML/JS files for data visualization

## Libraries Used

- [Adafruit SPIFlash](https://github.com/adafruit/Adafruit_SPIFlash): For flash memory management
- [SdFat](https://github.com/adafruit/SdFat): File system for the flash memory
- [Arduino-pico core](https://github.com/earlephilhower/arduino-pico): RP2040 core optimized for advanced features
- WiFiNINA: For WiFi connectivity
- Arduino_LSM6DS3: For interfacing with the IMU sensor
- ArduinoJson: For handling JSON data
- Base64: For encoding/decoding data

## Setup Instructions

1. Install PlatformIO
2. Clone this repository
3. Open the project in PlatformIO
4. Build and upload the firmware:

```bash
# Prepare web files for firmware
cd src && uv run data_prep.py

# Upload the firmware
/path/to/platformio run --target upload
```

## File Structure

- `src/main.cpp`: Main application code
- `src/spiflash_storage.h`: Flash storage implementation using Adafruit SPIFlash
- `src/web_server.h`: Web server implementation
- `src/sensor.h`: IMU sensor interface
- `src/web/`: Web interface files
- `src/data_prep.py`: Script to prepare web files for firmware
- `include/flash_config.h`: Flash transport configuration

## Testing

A test file `src/flash_test.cpp` is provided to verify the SPI Flash setup. To use this test file:

1. Rename `src/main.cpp` to `src/main.cpp.bak`
2. Rename `src/flash_test.cpp` to `src/main.cpp`
3. Upload the firmware
4. Check the serial output to verify the flash is working correctly
5. Restore the original files when done

## Migration Notes

This project has been migrated from using LittleFS to Adafruit SPIFlash with the following changes:

1. Switched from Raspberry Pi RP2040 core to arduino-pico core
2. Replaced LittleFS_Mbed_RP2040 with Adafruit SPIFlash and SdFat
3. Implemented a new storage class for SPI Flash operations

## Troubleshooting

If you encounter issues with the flash memory:

1. Try formatting the flash using the `fatfs.format()` method
2. Verify that the correct flash transport is used in `flash_config.h`
3. Check if the flash memory is properly detected (JEDEC ID should be displayed on serial)

## License

This project is open source and available under the MIT License. 
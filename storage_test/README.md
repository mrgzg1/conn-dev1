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

# Storage Test with LittleFS

This project demonstrates how to use LittleFS for persistent storage on the Raspberry Pi Pico (RP2040) microcontroller. The storage persists between boot cycles, making it ideal for configuration data, logs, and other data that needs to be retained when power is lost.

## Features

- Tracks the number of times the device has booted using persistent storage
- Creates and appends to a log file every 10 seconds
- Displays filesystem information (total space, used space, etc.)
- Lists all files in the root directory with their sizes and timestamps

## Hardware Requirements

- Raspberry Pi Pico or any RP2040-based board (this example uses the Arduino Nano RP2040 Connect)
- USB cable for programming and serial monitoring

## Software Requirements

- PlatformIO IDE (or PlatformIO Core with your preferred editor)
- Arduino framework for RP2040

## How LittleFS Works on RP2040

LittleFS is a lightweight filesystem designed for microcontrollers with limited RAM. On the RP2040, it uses a portion of the onboard flash memory to store files. The filesystem is separate from the program storage area, so uploading new code will not erase your stored data.

The flash layout on the RP2040 is as follows:

```
|----|---------------------|-------------|----|
^    ^                     ^             ^
OTA  Sketch                File system   EEPROM
```

## Project Configuration

This project allocates 10MB of flash for the filesystem. This is configured in the `platformio.ini` file:

```ini
board_build.filesystem_size = 10m
```

## Usage

1. Clone this repository
2. Open the project in PlatformIO
3. Build and upload the code to your RP2040 board
4. Open the Serial Monitor at 115200 baud
5. Observe the boot count incrementing each time you reset the device
6. Watch as log entries are added every 10 seconds

## Uploading Files to LittleFS

To upload files to the LittleFS filesystem from your computer:

### For Arduino IDE 1.x:
1. Download the PicoLittleFS tool: https://github.com/earlephilhower/arduino-pico-littlefs-plugin/releases
2. Install it in your Arduino sketchbook directory's `tools` folder
3. Create a `data` directory in your sketch folder
4. Place files you want to upload in the `data` directory
5. Select `Tools > Pico LittleFS Data Upload` in the Arduino IDE

### For Arduino IDE 2.x:
1. Download the tool: https://github.com/earlephilhower/arduino-littlefs-upload/releases
2. Install the VSIX file in your Arduino IDE 2.x plugins directory
3. Create a `data` directory in your sketch folder
4. Place files you want to upload in the `data` directory
5. Press `Ctrl+Shift+P` and select "Upload LittleFS to Pico/ESP8266"

### For PlatformIO:
1. Create a `data` directory in your project root
2. Place files you want to upload in the `data` directory
3. Run the "Upload Filesystem Image" task in PlatformIO

## API Reference

The main LittleFS functions used in this project:

- `LittleFS.begin()` - Mount the filesystem
- `LittleFS.format()` - Format the filesystem
- `LittleFS.open(path, mode)` - Open a file
- `LittleFS.exists(path)` - Check if a file exists
- `LittleFS.openDir(path)` - Open a directory for listing
- `LittleFS.info(fs_info)` - Get filesystem information

## License

This project is open source and available under the MIT License.

## Troubleshooting

If you encounter issues with the filesystem:

1. Try formatting the filesystem using `LittleFS.format()`
2. Ensure you have enough flash space allocated in `platformio.ini`
3. Check that you're closing files after opening them
4. Verify that your board has enough flash memory for your filesystem size 
# Arduino RP2040 Connect Sensor Data Collection

This repository contains multiple prototype implementations for collecting, storing, and visualizing sensor data from an Arduino Nano RP2040 Connect board. Each approach explores different solutions to the challenges of sensor integration, flash storage, and wireless connectivity.

## Project Overview

The core goal is to create a connected device that can:
1. Collect data from onboard sensors (IMU, temperature, etc.)
2. Store data persistently in flash memory
3. Provide wireless access to both real-time and historical data

## Repository Structure

This repository contains several distinct prototype implementations:

- **📁 pySub**: MicroPython-based implementation with reliable flash storage but WiFi challenges
- **📁 storage_test**: Arduino core implementation focused on robust flash storage
- **📁 device_db**: Web interface with database storage attempt
- **📁 subway_collection**: Working real-time visualization without persistent storage
- **📁 final_conndev**: Work-in-progress integration attempt
- **📁 kerilog**: Additional utilities and tools
- **📁 mqtt**: MQTT client implementation for data transmission

## Current Status

As of the latest development, no single implementation successfully combines all three core requirements (sensor collection, storage, and connectivity). Each approach has specific strengths and limitations:

- ✅ **pySub**: Reliable flash storage, unreliable WiFi
- ✅ **storage_test**: Advanced storage implementation with stability issues
- ✅ **subway_collection**: Working web interface and sensors, no persistent storage

The latest commit messages indicate ongoing challenges with I2C device detection and WiFi reliability on newer firmware versions.

## Hardware Requirements

- Arduino Nano RP2040 Connect
- Various I2C sensors (BME280, LSM6DSOX included in some implementations)
- USB connection for flashing and debugging

## Software Dependencies

Dependencies vary by implementation but generally include:
- PlatformIO for Arduino implementations
- MicroPython for pySub
- WiFiNINA for connectivity
- ArduinoJson for data formatting
- React and Chart.js for web interfaces

## Getting Started

For detailed information about each implementation:

1. Review the [WORKBOOK.md](WORKBOOK.md) file for comprehensive analysis
2. Examine the CLAUDE.md file for build commands and code style guidelines
3. Each project directory contains its own README with specific instructions

## Build & Upload

Build commands are standardized using PlatformIO:
```bash
# For Arduino-based implementations
cd [project_directory]
platformio run --target upload

# For MicroPython implementation
cd pySub
# See pySub/README.md for detailed flashing instructions
```

## License

MIT License

## Additional Resources

- [WORKBOOK.md](WORKBOOK.md): Detailed analysis of each implementation
- [CLAUDE.md](CLAUDE.md): Build commands and code style guidelines
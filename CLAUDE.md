# CLAUDE.md - IoT Device Development Project Guidelines

## Project Overview

This repository contains multiple prototype implementations for collecting sensor data from an Arduino Nano RP2040 Connect board. Each implementation takes a different approach to the core challenges of sensor data collection, storage, and connectivity.

## Build Commands

### Arduino Core Projects (device_db, storage_test, subway_collection)
- Build and upload: `pio run --target upload`
- Upload filesystem: `pio run --target uploadfs`
- Monitor serial: `pio device monitor`
- Process web files: `cd src && python data_prep.py`
- Clean build: `pio run --target clean`

### MicroPython Project (pySub)
- Install dependencies: `uv add rshell esptool mpremote adafruit-ampy`
- Flash firmware: See pySub/README.md
- Upload code: `uv run ampy --port /dev/tty.usbmodem* put main.py`
- Monitor: `uv run mpremote repl`
- Run tests: `python -m pySub.sensor_test` // uv equivalent of this

## Test Commands
- Run Arduino tests: `pio test`
- Run specific test: `pio test -e nanorp2040connect -f <test_file>`
- Run Python tests: `python <test_file.py>`

## Code Style Guidelines

### C++ Style (Arduino projects)
- **Naming**: snake_case functions, PascalCase classes, camelCase variables, UPPER_SNAKE_CASE constants
- **File Structure**: Headers (.h) in include/, implementations (.cpp) in src/
- **Includes**: Standard libraries first, then third-party, then project files
- **Comments**: Function purpose, parameters and complex logic
- **Error Handling**: Return boolean success/failure values
- **Indentation**: 2 spaces
- **Hardware**: Use yield() to prevent watchdog timeouts
- **Safety**: Bounds checking, input validation, and retry critical operations

### Python Style (pySub)
- **Naming**: snake_case functions/variables, PascalCase classes, UPPER_SNAKE_CASE constants
- **Imports**: Standard libraries first, then third-party, then project modules
- **Comments**: Use docstrings for classes and functions; comment complex logic
- **Error Handling**: Use try/except with specific exception types
- **Indentation**: 4 spaces

## Common Challenges & Solutions

### Flash Storage
- Only use storage_flush() when necessary to reduce wear
- Implement buffering to batch write operations
- Always check return values from storage operations
- Implement CRC or other verification for critical data

### WiFi Connectivity
- Always provide fallback to AP mode if client mode fails
- Implement reconnection logic with increasing backoff
- Monitor signal strength and adapt data rate accordingly
- Test thoroughly with different firmware versions

### Sensor Reading
- Initialize I2C with appropriate clock speed (typically 100kHz)
- Add delays between sensor operations
- Implement retry logic for sensor readings
- Validate sensor data ranges before storing

## Project-Specific Notes

### subway_collection
- Web interface with real-time visualization
- No persistent storage implementation
- Good WiFi stability with fallback networks

### storage_test
- Focus on reliable flash storage operations
- Implements sophisticated error recovery
- Uses direct flash manipulation
- Add extensive yield() calls to prevent watchdog resets

### pySub
- MicroPython-based implementation
- Reliable flash access through native filesystem
- WiFi reliability issues on newer firmware
- Best storage solution currently

## Secrets Management
- Never hardcode credentials in source code
- Use secrets.h or secrets.json files (excluded from git)
- WiFi credentials should be in SSID/Password format
- API keys should be stored separately from connection details
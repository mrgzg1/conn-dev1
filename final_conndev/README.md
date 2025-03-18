# Connected Device Platform

A versatile IoT device platform that collects, stores, and visualizes sensor data.

## Features

- IMU sensor data collection (acceleration, gyroscope, temperature)
- Persistent storage on onboard flash memory
- Works with or without WiFi connectivity
- Web interface for real-time visualization and data download
- Data analytics and export capabilities

## Hardware Requirements

- Raspberry Pi Pico W (RP2040 with WiFi)
- LSM6DS3 IMU sensor (built into Arduino Nano RP2040 Connect)

## Getting Started

### Build and Upload

To build and upload the firmware to your device, use these PlatformIO commands:

```bash
# Build and upload firmware
pio run --target upload

# Upload web interface to device
pio run --target uploadfs

# Monitor serial output
pio device monitor
```

### Using the Platform

1. **Power on the device**:
   - The device will start collecting sensor data immediately
   - Data is stored in onboard flash memory

2. **Connect to the device**:
   - By default, the device creates a WiFi access point named "ConnDevSensor"
   - Connect to this network using password "connecteddevice"
   - Navigate to http://192.168.4.1 in your web browser

3. **Using the web interface**:
   - View real-time sensor data
   - Download data as CSV files
   - Monitor storage status

4. **Data collection without WiFi**:
   - The device continues collecting data without WiFi
   - Data is stored in flash memory
   - Connect to WiFi later to access the data

## Project Structure

- `/include`: Header files (*.h)
- `/src`: Implementation files (*.cpp)
  - `/web`: Web interface files (HTML, JavaScript)
- `/lib`: External libraries
- `/test`: Test files

## Customization

- Edit `include/secrets.h` to configure WiFi credentials
- Modify `src/web/` files to customize the web interface
- After changing web files, run `data_prep.py` to update `web_files.h`

## Data Management

Data is stored on the device's flash memory:
- Each reading is timestamped and stored in a circular buffer
- When storage is full, oldest entries are overwritten
- Use the web interface to download data periodically

## Troubleshooting

- **Serial output**: Monitor serial output at 115200 baud for debug information
- **Storage issues**: Use 'Format Storage' in the web interface if storage becomes corrupted
- **Reset device**: Press the reset button if the device becomes unresponsive

## License

MIT License - Feel free to use, modify, and distribute this code.
#!/bin/bash
# Script to deploy files to Raspberry Pi Pico RP2040

# Function to copy files and check result
copy_files() {
    local files=$1
    local desc=$2
    
    echo "Copying $desc files..."
    for file in $files; do
        if [ -f "$file" ]; then
            echo "  $file -> RP2040"
            uv run mpremote fs cp "$file" ":"
            if [ $? -ne 0 ]; then
                echo "  ERROR: Failed to copy $file"
            fi
        else
            echo "  WARNING: $file not found, skipping"
        fi
    done
    echo "Done."
}

# If a filename is provided as argument, just copy that file
if [ $# -eq 1 ]; then
    echo "=== Deploying single file to RP2040 ==="
    copy_files "$1" "specified"
    exit 0
fi

echo "=== Deploying files to RP2040 ==="

# Essential core files
CORE_FILES="boot.py main.py wifi_manager.py flash_storage.py data_logger.py sensors.py hardware_config.py"

# Sensor drivers
SENSOR_FILES="bme280.py lsm6dsox.py"

# Test files
TEST_FILES="sensor_test.py wifi_test.py incremental_test.py bme280_test.py lsm6dsox_test.py i2c_scanner.py i2c_validator.py raw_imu_test.py"

# Utility files
UTIL_FILES="espflash.py update_firmware.py check_espflash.py"

# Configuration files
CONFIG_FILES="secrets.json"

# Copy all files by category
copy_files "$CORE_FILES" "core"
copy_files "$SENSOR_FILES" "sensor driver"
copy_files "$TEST_FILES" "test"
copy_files "$UTIL_FILES" "utility"
copy_files "$CONFIG_FILES" "configuration"

echo "=== Deployment complete ==="
echo "You can now run various test scripts:"
echo "  uv run mpremote run raw_imu_test.py       - Direct low-level test of sensors"
echo "  uv run mpremote run i2c_validator.py     - Validate I2C devices and find working config"
echo "  uv run mpremote run incremental_test.py - Run all tests incrementally"
echo "  uv run mpremote run bme280_test.py      - Test only BME280 sensor"
echo "  uv run mpremote run lsm6dsox_test.py    - Test only LSM6DSOX IMU"
echo "  uv run mpremote run sensor_test.py      - Test all sensors together"
echo "  uv run mpremote run wifi_test.py        - Test WiFi connectivity"
echo "  uv run mpremote reset                   - Reset and run main.py"

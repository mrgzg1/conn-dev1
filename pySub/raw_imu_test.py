"""
Raw LSM6DSOX IMU test for Arduino Nano RP2040 Connect
This script tests the IMU without using the full driver
"""

import time
from machine import Pin, I2C

# Use the official Arduino Nano RP2040 Connect I2C pins
i2c = I2C(1, scl=Pin(27), sda=Pin(26), freq=100000)  # Use lower frequency for reliability

# LED for visual feedback
led = Pin(25, Pin.OUT)

def scan_i2c():
    """Scan the I2C bus and print all devices found"""
    print("Scanning I2C bus...")
    devices = i2c.scan()
    
    if devices:
        print(f"Found {len(devices)} I2C devices:")
        for addr in devices:
            print(f"  - 0x{addr:02X}")
    else:
        print("No I2C devices found!")
    
    return devices

def test_lsm6dsox_raw():
    """Test the LSM6DSOX IMU with raw register access"""
    print("\nTesting LSM6DSOX IMU (raw register access)...")
    
    # LSM6DSOX addresses and registers
    LSM_ADDR = 0x6A  # LSM6DSOX default address
    LSM_WHO_AM_I = 0x0F  # Who Am I register
    LSM_EXPECTED_ID = 0x6C  # Expected chip ID
    
    # Try alternative address if needed
    alt_addr = 0x6B
    
    # Try to read WHO_AM_I register
    for addr in [LSM_ADDR, alt_addr]:
        try:
            print(f"Trying LSM6DSOX at address 0x{addr:02X}...")
            
            # Read WHO_AM_I register
            try:
                who_am_i = i2c.readfrom_mem(addr, LSM_WHO_AM_I, 1)
                who_am_i_val = who_am_i[0]
                print(f"  WHO_AM_I = 0x{who_am_i_val:02X} (expected 0x{LSM_EXPECTED_ID:02X})")
                
                if who_am_i_val == LSM_EXPECTED_ID:
                    print("  LSM6DSOX identified correctly!")
                    
                    # Try to configure and read accelerometer
                    print("\nConfiguring LSM6DSOX...")
                    
                    # Configure accelerometer - CTRL1_XL (0x10)
                    # Set ODR_XL (output data rate) = 0100 (104 Hz)
                    # Set FS_XL (full scale) = 00 (2g)
                    i2c.writeto_mem(addr, 0x10, bytes([0x40]))
                    
                    # Configure gyroscope - CTRL2_G (0x11)
                    # Set ODR_G (output data rate) = 0100 (104 Hz)
                    # Set FS_G (full scale) = 00 (250 dps)
                    i2c.writeto_mem(addr, 0x11, bytes([0x40]))
                    
                    # Wait a moment for settings to apply
                    time.sleep(0.1)
                    
                    print("Reading accelerometer values...")
                    for _ in range(5):
                        # Read accelerometer values
                        # OUT_X_L_A, OUT_X_H_A, OUT_Y_L_A, OUT_Y_H_A, OUT_Z_L_A, OUT_Z_H_A
                        data = i2c.readfrom_mem(addr, 0x28, 6)
                        
                        # Combine values (little endian)
                        x = (data[1] << 8) | data[0]
                        y = (data[3] << 8) | data[2]
                        z = (data[5] << 8) | data[4]
                        
                        # Convert to signed values
                        if x > 32767:
                            x -= 65536
                        if y > 32767:
                            y -= 65536
                        if z > 32767:
                            z -= 65536
                        
                        # Convert to g (LSB sensitivity is 0.061 mg/LSB for ±2g range)
                        # And then to m/s²
                        accel_x = x * 0.061 * 9.80665 / 1000
                        accel_y = y * 0.061 * 9.80665 / 1000
                        accel_z = z * 0.061 * 9.80665 / 1000
                        
                        print(f"  Accel: X={accel_x:.2f}, Y={accel_y:.2f}, Z={accel_z:.2f} m/s²")
                        led.toggle()
                        time.sleep(0.5)
                    
                    return True, addr
                else:
                    print("  WHO_AM_I value doesn't match! Not an LSM6DSOX or communication error.")
            except Exception as e:
                print(f"  Error reading WHO_AM_I: {e}")
                
        except Exception as e:
            print(f"  Error communicating with LSM6DSOX at 0x{addr:02X}: {e}")
    
    print("LSM6DSOX not found or not responding correctly.")
    return False, None

def test_bme280_raw():
    """Test the BME280 sensor with raw register access"""
    print("\nTesting BME280 environmental sensor (raw register access)...")
    
    # BME280 addresses and registers
    BME_ADDR = 0x76  # BME280 default address
    BME_ID_REG = 0xD0  # Chip ID register
    BME_EXPECTED_ID = 0x60  # Expected chip ID
    
    # Try alternative address if needed
    alt_addr = 0x77
    
    # Try to read chip ID register
    for addr in [BME_ADDR, alt_addr]:
        try:
            print(f"Trying BME280 at address 0x{addr:02X}...")
            
            # Read chip ID register
            try:
                chip_id = i2c.readfrom_mem(addr, BME_ID_REG, 1)
                chip_id_val = chip_id[0]
                print(f"  Chip ID = 0x{chip_id_val:02X} (expected 0x{BME_EXPECTED_ID:02X})")
                
                if chip_id_val == BME_EXPECTED_ID:
                    print("  BME280 identified correctly!")
                    
                    # Read calibration data
                    try:
                        calib_data = i2c.readfrom_mem(addr, 0x88, 26)
                        print(f"  Read {len(calib_data)} bytes of calibration data")
                        
                        # Just print the first few bytes as hex
                        print("  Calibration data (first 8 bytes):", " ".join([f"{b:02X}" for b in calib_data[:8]]))
                        
                        return True, addr
                    except Exception as e:
                        print(f"  Error reading calibration data: {e}")
                else:
                    print("  Chip ID value doesn't match! Not a BME280 or communication error.")
            except Exception as e:
                print(f"  Error reading chip ID: {e}")
                
        except Exception as e:
            print(f"  Error communicating with BME280 at 0x{addr:02X}: {e}")
    
    print("BME280 not found or not responding correctly.")
    return False, None

def main():
    led.value(1)  # Turn on LED to show we're running
    
    print("\n--- Arduino Nano RP2040 Connect - Raw Sensor Test ---")
    
    # First scan the I2C bus
    devices = scan_i2c()
    
    if not devices:
        print("\nNo I2C devices found. Check connections and pin assignments.")
        led.value(0)
        return
    
    # Test the LSM6DSOX IMU
    success_lsm, addr_lsm = test_lsm6dsox_raw()
    
    # Test the BME280 sensor
    success_bme, addr_bme = test_bme280_raw()
    
    # Summary
    print("\n--- Test Results Summary ---")
    print(f"LSM6DSOX IMU:       {'✓ Working' if success_lsm else '✗ Not working'}")
    print(f"BME280 Environment: {'✓ Working' if success_bme else '✗ Not working'}")
    
    # Recommendations
    print("\n--- Recommendations ---")
    if success_lsm or success_bme:
        print("Update hardware_config.py with:")
        if success_lsm:
            print(f"LSM6DSOX_I2C_ADDR = {addr_lsm}")
        if success_bme:
            print(f"BME280_I2C_ADDR = {addr_bme}")
    else:
        print("Try other I2C pin configurations or check hardware connections.")
        print("Make sure pull-up resistors (4.7kΩ) are connected to SDA and SCL.")
    
    led.value(0)  # Turn off LED when done

if __name__ == "__main__":
    main()
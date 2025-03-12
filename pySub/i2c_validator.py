import time
from machine import Pin, I2C

"""
Enhanced I2C validator that tests different I2C configurations
and validates sensors by checking their WHO_AM_I registers
"""

# LED for visual feedback
led = Pin(25, Pin.OUT)

# Common sensor WHO_AM_I register addresses and expected values
WHO_AM_I_REGISTERS = {
    # LSM6DSOX IMU
    0x6A: {"reg": 0x0F, "value": 0x6C, "name": "LSM6DSOX IMU"},
    0x6B: {"reg": 0x0F, "value": 0x6C, "name": "LSM6DSOX IMU (alt address)"},
    
    # BME280 - has no WHO_AM_I, but we can check chip ID
    0x76: {"reg": 0xD0, "value": 0x60, "name": "BME280 (Environmental)"},
    0x77: {"reg": 0xD0, "value": 0x60, "name": "BME280 (Environmental, alt address)"},
}

# I2C configurations to try
I2C_CONFIGS = [
    # Arduino Nano RP2040 Connect official I2C1 pins
    {"bus": 1, "scl": 27, "sda": 26, "name": "Arduino RP2040 I2C1 (GPIO27/GPIO26)"},
    
    # Other standard configurations
    {"bus": 0, "scl": 1, "sda": 0, "name": "I2C0 default (GPIO1/GPIO0)"},
    {"bus": 1, "scl": 3, "sda": 2, "name": "I2C1 default (GPIO3/GPIO2)"},
    
    # LSM6DSOX specific SPI pins might be needed
    {"bus": 0, "scl": 13, "sda": 12, "name": "I2C0 alt3 (GPIO13/GPIO12)"},
    
    # Arduino configurations for SPI pins (might be I2C capable)
    {"bus": 1, "scl": 14, "sda": 11, "name": "Arduino SPI1_SCK/MOSI (GPIO14/GPIO11)"},
    {"bus": 0, "scl": 9, "sda": 8, "name": "Arduino UART1_RX/TX (GPIO9/GPIO8)"},
]

def test_i2c_config(config):
    """Test a specific I2C configuration and validate devices"""
    print(f"\nTesting {config['name']}...")
    
    try:
        # Initialize I2C with lower frequency for reliability
        i2c = I2C(config["bus"], scl=Pin(config["scl"]), sda=Pin(config["sda"]), freq=100000)
        
        # Scan for devices
        devices = i2c.scan()
        
        if not devices:
            print("  No devices found")
            return False, None
            
        # Check if we're getting an unreasonable number of devices
        if len(devices) > 10:
            print(f"  Warning: Found {len(devices)} devices - likely false positives!")
            print("  This I2C bus might have floating pins or resistor issues")
            
        # Try to validate devices by checking WHO_AM_I registers
        valid_devices = []
        
        for addr in devices:
            led.toggle()  # Visual feedback
            if addr in WHO_AM_I_REGISTERS:
                reg_info = WHO_AM_I_REGISTERS[addr]
                try:
                    # Read the WHO_AM_I or chip ID register
                    chip_id = i2c.readfrom_mem(addr, reg_info["reg"], 1)[0]
                    if chip_id == reg_info["value"]:
                        print(f"  ✓ Valid device at 0x{addr:02X}: {reg_info['name']} (ID=0x{chip_id:02X})")
                        valid_devices.append({
                            "address": addr,
                            "name": reg_info["name"],
                            "valid": True
                        })
                    else:
                        print(f"  ✗ Device at 0x{addr:02X} has incorrect ID: 0x{chip_id:02X} (expected 0x{reg_info['value']:02X})")
                        valid_devices.append({
                            "address": addr,
                            "name": reg_info["name"],
                            "valid": False
                        })
                except Exception as e:
                    print(f"  ✗ Error reading WHO_AM_I from 0x{addr:02X}: {e}")
            else:
                # Just report devices we don't recognize or validate
                if addr < 8 or addr > 0x77:
                    # These addresses are typically out of range for I2C
                    print(f"  ⚠ Suspicious device at 0x{addr:02X} - likely a false positive")
                else:
                    print(f"  ? Unknown device at 0x{addr:02X}")
                    
        return len(valid_devices) > 0, i2c
        
    except Exception as e:
        print(f"  ✗ Error initializing I2C: {e}")
        return False, None

def test_lsm6dsox(i2c, address=0x6A):
    """Try to read LSM6DSOX data to verify it's working"""
    print(f"\nTesting LSM6DSOX functionality at 0x{address:02X}...")
    
    try:
        # First verify WHO_AM_I again
        who_am_i = i2c.readfrom_mem(address, 0x0F, 1)[0]
        print(f"  WHO_AM_I = 0x{who_am_i:02X} (should be 0x6C)")
        
        if who_am_i != 0x6C:
            print("  ✗ WHO_AM_I check failed!")
            return False
            
        # Try to initialize the gyroscope and accelerometer
        # Set CTRL1_XL (0x10) - accelerometer control
        # ODR_XL = 0100 (104 Hz) | FS_XL = 00 (2g) | LPF1_BW_SEL = 0 | BW0_XL = 0
        i2c.writeto_mem(address, 0x10, bytes([0x40]))
        
        # Set CTRL2_G (0x11) - gyroscope control
        # ODR_G = 0100 (104 Hz) | FS_G = 00 (250 dps) | FS_125 = 0 | 0 | 0
        i2c.writeto_mem(address, 0x11, bytes([0x40]))
        
        # Wait a bit for the sensor to apply settings
        time.sleep(0.1)
        
        # Read accelerometer data
        # X, Y, Z (6 bytes total)
        data = i2c.readfrom_mem(address, 0x28, 6)
        
        # Convert to 16-bit signed integers (raw values)
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
            
        # Scale to m/s^2 (2g range, 16-bit resolution)
        # LSB sensitivity is 0.061 mg/LSB for 2g range
        accel_x = x * 0.061 * 9.80665 / 1000
        accel_y = y * 0.061 * 9.80665 / 1000
        accel_z = z * 0.061 * 9.80665 / 1000
        
        print("  Accelerometer readings:")
        print(f"    X: {accel_x:.2f} m/s²")
        print(f"    Y: {accel_y:.2f} m/s²")
        print(f"    Z: {accel_z:.2f} m/s²")
        
        # Success if we got here without errors
        print("  ✓ LSM6DSOX is working!")
        return True
        
    except Exception as e:
        print(f"  ✗ Error reading LSM6DSOX: {e}")
        return False
        
def test_bme280(i2c, address=0x76):
    """Try to read BME280 data to verify it's working"""
    print(f"\nTesting BME280 functionality at 0x{address:02X}...")
    
    try:
        # Check chip ID
        chip_id = i2c.readfrom_mem(address, 0xD0, 1)[0]
        print(f"  Chip ID = 0x{chip_id:02X} (should be 0x60)")
        
        if chip_id != 0x60:
            print("  ✗ Chip ID check failed!")
            return False
        
        # Try to read calibration data
        try:
            calib_data = i2c.readfrom_mem(address, 0x88, 26)
            print(f"  ✓ Read {len(calib_data)} bytes of calibration data")
        except Exception as e:
            print(f"  ✗ Failed to read calibration data: {e}")
            return False
            
        # Try to read humidity calibration data
        try:
            h_calib_data = i2c.readfrom_mem(address, 0xE1, 7)
            print(f"  ✓ Read {len(h_calib_data)} bytes of humidity calibration data")
        except Exception as e:
            print(f"  ✗ Failed to read humidity calibration data: {e}")
            return False
            
        # Success if we got here without errors
        print("  ✓ BME280 is working!")
        return True
            
    except Exception as e:
        print(f"  ✗ Error reading BME280: {e}")
        return False

def main():
    """Test different I2C configurations and validate devices"""
    print("=== RP2040 I2C Validator ===")
    print("Testing I2C configurations and validating sensors")
    
    led.value(1)  # Turn on LED to show we're running
    
    # Try each I2C configuration
    working_configs = []
    
    for config in I2C_CONFIGS:
        led.toggle()
        success, i2c = test_i2c_config(config)
        
        if success:
            print(f"  ✓ Found at least one valid device with {config['name']}")
            
            # Try to verify specific sensors
            lsm_success = False
            bme_success = False
            
            # Test LSM6DSOX if it was found in the scan
            for addr in [0x6A, 0x6B]:
                try:
                    if addr in i2c.scan():
                        lsm_success = test_lsm6dsox(i2c, addr)
                        if lsm_success:
                            break
                except:
                    pass
                    
            # Test BME280 if it was found in the scan
            for addr in [0x76, 0x77]:
                try:
                    if addr in i2c.scan():
                        bme_success = test_bme280(i2c, addr)
                        if bme_success:
                            break
                except:
                    pass
                
            # Add this config to working configs
            working_configs.append({
                "config": config,
                "lsm_working": lsm_success,
                "bme_working": bme_success
            })
            
    # Print summary
    print("\n=== I2C Configuration Summary ===")
    if working_configs:
        print(f"Found {len(working_configs)} working I2C configurations:")
        
        for i, result in enumerate(working_configs):
            config = result["config"]
            lsm_status = "✓" if result["lsm_working"] else "✗"
            bme_status = "✓" if result["bme_working"] else "✗"
            
            print(f"{i+1}. {config['name']}")
            print(f"   LSM6DSOX: {lsm_status}  BME280: {bme_status}")
            
        # Find best config (the one with most working sensors)
        best_config = max(working_configs, key=lambda x: (x["lsm_working"] + x["bme_working"]))
        
        print("\n=== Recommended Configuration ===")
        print(f"Use: {best_config['config']['name']}")
        print(f"I2C({best_config['config']['bus']}, scl=Pin({best_config['config']['scl']}), sda=Pin({best_config['config']['sda']}), freq=400000)")
        
        # Update hardware_config.py values
        print("\nTo update hardware_config.py, use these values:")
        print("I2C_BUS =", best_config['config']['bus'])
        print("I2C_SCL_PIN =", best_config['config']['scl'])
        print("I2C_SDA_PIN =", best_config['config']['sda'])
        
    else:
        print("No working I2C configurations found!")
        print("Check your connections and make sure pull-up resistors are present.")
        
    led.value(0)  # Turn off LED when done

if __name__ == "__main__":
    main()
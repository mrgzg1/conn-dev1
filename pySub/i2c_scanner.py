import time
from machine import Pin, I2C

print("RP2040 I2C Scanner - Test all possible I2C configurations")

# Define possible I2C bus configurations to try
i2c_configs = [
    # I2C0 with default pins (GPIO0 and GPIO1)
    {"bus": 0, "scl": 1, "sda": 0, "name": "I2C0 default (SCL=GPIO1, SDA=GPIO0)"},
    
    # I2C1 with default pins (GPIO2 and GPIO3)
    {"bus": 1, "scl": 3, "sda": 2, "name": "I2C1 default (SCL=GPIO3, SDA=GPIO2)"},
    
    # Alternative pins for I2C0 (see RP2040 datasheet)
    {"bus": 0, "scl": 5, "sda": 4, "name": "I2C0 alt1 (SCL=GPIO5, SDA=GPIO4)"},
    {"bus": 0, "scl": 9, "sda": 8, "name": "I2C0 alt2 (SCL=GPIO9, SDA=GPIO8)"},
    {"bus": 0, "scl": 13, "sda": 12, "name": "I2C0 alt3 (SCL=GPIO13, SDA=GPIO12)"},
    {"bus": 0, "scl": 17, "sda": 16, "name": "I2C0 alt4 (SCL=GPIO17, SDA=GPIO16)"},
    {"bus": 0, "scl": 21, "sda": 20, "name": "I2C0 alt5 (SCL=GPIO21, SDA=GPIO20)"},
    
    # Alternative pins for I2C1 (see RP2040 datasheet)
    {"bus": 1, "scl": 7, "sda": 6, "name": "I2C1 alt1 (SCL=GPIO7, SDA=GPIO6)"},
    {"bus": 1, "scl": 11, "sda": 10, "name": "I2C1 alt2 (SCL=GPIO11, SDA=GPIO10)"},
    {"bus": 1, "scl": 15, "sda": 14, "name": "I2C1 alt3 (SCL=GPIO15, SDA=GPIO14)"},
    {"bus": 1, "scl": 19, "sda": 18, "name": "I2C1 alt4 (SCL=GPIO19, SDA=GPIO18)"},
    {"bus": 1, "scl": 27, "sda": 26, "name": "I2C1 alt5 (SCL=GPIO27, SDA=GPIO26)"}
]

# Define common I2C device addresses and names
devices_dict = {
    0x1E: "LIS3MDL/HMC5883L (Magnetometer)",
    0x6A: "LSM6DSOX (IMU)",
    0x6B: "LSM6DSOX (IMU, alt address)",
    0x76: "BME280/BMP280 (Environmental)",
    0x77: "BME280/BMP280 (Environmental, alt address)",
    0x68: "MPU6050/MPU9250 (IMU)",
    0x69: "MPU6050/MPU9250 (IMU, alt address)",
    0x40: "HDC1080/Si7021 (Humidity sensor)",
    0x44: "SHT30/31/35 (Humidity sensor)",
    0x48: "ADS1115/ADS1015 (ADC)",
    0x50: "AT24C32/EEPROM",
    0x57: "MAX30102 (Pulse Oximeter)",
    0x3C: "SSD1306 OLED Display",
    0x3D: "SSD1306 OLED Display (alt address)"
}

def scan_i2c(config):
    """Scan I2C bus with given configuration and return discovered devices"""
    print(f"\nTrying {config['name']}...")
    
    try:
        # Initialize I2C
        i2c = I2C(config["bus"], 
                 scl=Pin(config["scl"]), 
                 sda=Pin(config["sda"]), 
                 freq=100000)  # Lower frequency for reliability
        
        # Scan for devices
        devices = i2c.scan()
        
        if devices:
            print(f"Success! Found {len(devices)} I2C devices:")
            for addr in devices:
                device_name = devices_dict.get(addr, "Unknown device")
                print(f"  - 0x{addr:02X} : {device_name}")
            return True, i2c, devices
        else:
            print("  No devices found on this bus configuration")
            return False, None, []
            
    except Exception as e:
        print(f"  Error: {e}")
        return False, None, []

def main():
    # LED for visual indication
    led = Pin(25, Pin.OUT)
    led.value(1)  # Turn on LED to indicate we're running
    
    # Store successful configurations
    successful_configs = []
    
    print("\n====== Starting I2C Bus Scanning ======")
    print(f"Testing {len(i2c_configs)} different I2C configurations...")
    
    for config in i2c_configs:
        success, i2c, devices = scan_i2c(config)
        if success:
            successful_configs.append({
                "config": config,
                "devices": devices
            })
            
        # Toggle LED to show progress
        led.toggle()
        time.sleep(0.2)
    
    # Summary
    print("\n====== I2C Scan Summary ======")
    if successful_configs:
        print(f"Found {len(successful_configs)} working I2C configurations:")
        for i, result in enumerate(successful_configs):
            config = result["config"]
            devices = result["devices"]
            print(f"\n{i+1}. {config['name']}")
            print(f"   Devices found: {len(devices)}")
            for addr in devices:
                device_name = devices_dict.get(addr, "Unknown device")
                print(f"   - 0x{addr:02X} : {device_name}")
        
        # Recommendation
        if successful_configs:
            # Find config with most devices
            best_config = max(successful_configs, key=lambda x: len(x["devices"]))
            config = best_config["config"]
            print("\n====== Recommended Configuration ======")
            print(f"Use: {config['name']}")
            print(f"I2C({config['bus']}, scl=Pin({config['scl']}), sda=Pin({config['sda']}), freq=400000)")
    else:
        print("No working I2C configurations found.")
        print("Check your connections and make sure pull-up resistors are present.")
    
    # Turn off LED when done
    led.value(0)

if __name__ == "__main__":
    main()
import time
import machine
from machine import Pin, I2C
import gc

# LED for visual feedback
led = Pin(25, Pin.OUT)

def memory_status():
    """Print memory allocation status"""
    gc.collect()
    free = gc.mem_free()
    alloc = gc.mem_alloc()
    total = free + alloc
    print(f"Memory: {free/1024:.1f}KB free, {alloc/1024:.1f}KB used, {total/1024:.1f}KB total")

def test_i2c_scan():
    """Test I2C scan functionality"""
    print("\n--- I2C Scan Test ---")
    
    # Try I2C1 first (GPIO2/3)
    print("Testing I2C1 (GPIO2=SDA, GPIO3=SCL)...")
    i2c = I2C(1, scl=Pin(3), sda=Pin(2), freq=400000)
    devices = i2c.scan()
    
    if devices:
        print(f"Found {len(devices)} I2C devices on I2C1:")
        for addr in devices:
            print(f"  - Device at address: 0x{addr:02x}")
    else:
        print("No I2C devices found on I2C1, trying I2C0...")
        # Try I2C0 (GPIO0/1)
        try:
            i2c = I2C(0, scl=Pin(1), sda=Pin(0), freq=400000)
            devices = i2c.scan()
            if devices:
                print(f"Found {len(devices)} I2C devices on I2C0:")
                for addr in devices:
                    print(f"  - Device at address: 0x{addr:02x}")
            else:
                print("No I2C devices found on I2C0 either.")
        except Exception as e:
            print(f"Error initializing I2C0: {e}")
    
    return i2c, devices

def test_bme280(i2c):
    """Test BME280 sensor"""
    print("\n--- BME280 Test ---")
    
    try:
        from bme280 import BME280
        
        # Check if BME280 is present at either address
        addresses = [0x76, 0x77]
        bme = None
        
        for addr in addresses:
            try:
                print(f"Trying BME280 at address 0x{addr:02x}...")
                bme = BME280(i2c=i2c, address=addr)
                # If we get here, initialization worked
                print(f"Found BME280 at address 0x{addr:02x}")
                break
            except Exception as e:
                print(f"  Error: {e}")
        
        if bme:
            print("\nReading BME280 sensor...")
            for i in range(3):
                try:
                    temp, pressure, humidity = bme.read_compensated_data()
                    print(f"  Reading {i+1}:")
                    print(f"    Temperature: {temp:.2f} °C")
                    print(f"    Pressure: {pressure/100:.2f} hPa")
                    print(f"    Humidity: {humidity:.2f} %")
                    print(f"    Altitude: {bme.altitude:.2f} m")
                    led.toggle()
                    time.sleep(1)
                except Exception as e:
                    print(f"  Error reading BME280: {e}")
        else:
            print("BME280 not found")
    
    except ImportError:
        print("BME280 module not found")

def test_lsm6dsox(i2c):
    """Test LSM6DSOX IMU"""
    print("\n--- LSM6DSOX Test ---")
    
    try:
        from lsm6dsox import LSM6DSOX
        
        # Check if LSM6DSOX is present at either address
        addresses = [0x6A, 0x6B]
        imu = None
        
        for addr in addresses:
            try:
                print(f"Trying LSM6DSOX at address 0x{addr:02x}...")
                imu = LSM6DSOX(i2c=i2c, address=addr)
                # If we get here, initialization worked
                print(f"Found LSM6DSOX at address 0x{addr:02x}")
                break
            except Exception as e:
                print(f"  Error: {e}")
        
        if imu:
            print("\nReading LSM6DSOX sensor...")
            for i in range(3):
                try:
                    accel = imu.read_acceleration()
                    gyro = imu.read_gyro()
                    temp = imu.read_temperature()
                    
                    print(f"  Reading {i+1}:")
                    print(f"    Acceleration (m/s²): X={accel[0]:.3f}, Y={accel[1]:.3f}, Z={accel[2]:.3f}")
                    print(f"    Gyroscope (deg/s): X={gyro[0]:.3f}, Y={gyro[1]:.3f}, Z={gyro[2]:.3f}")
                    print(f"    Temperature: {temp:.2f} °C")
                    
                    # Calculate acceleration magnitude
                    accel_mag = (accel[0]**2 + accel[1]**2 + accel[2]**2)**0.5
                    print(f"    Acceleration magnitude: {accel_mag:.3f} m/s²")
                    
                    led.toggle()
                    time.sleep(1)
                except Exception as e:
                    print(f"  Error reading LSM6DSOX: {e}")
        else:
            print("LSM6DSOX not found")
    
    except ImportError:
        print("LSM6DSOX module not found")

def test_sensor_manager():
    """Test the SensorManager class"""
    print("\n--- SensorManager Test ---")
    
    try:
        from sensors import SensorManager
        
        print("Initializing SensorManager...")
        sensor_mgr = SensorManager()
        
        print(f"Available sensors: {sensor_mgr.available_sensors}")
        print(f"Initialized sensors: {list(sensor_mgr.sensors.keys())}")
        
        print("\nReading all sensor data...")
        for i in range(3):
            try:
                data = sensor_mgr.read_all()
                print(f"\nReading {i+1}:")
                
                # Print environmental data
                print("  Environmental data:")
                print(f"    Temperature: {data.get('temperature', 'N/A'):.2f} °C")
                print(f"    Pressure: {data.get('pressure', 'N/A'):.2f} hPa")
                print(f"    Humidity: {data.get('humidity', 'N/A'):.2f} %")
                print(f"    Altitude: {data.get('altitude', 'N/A'):.2f} m")
                
                # Print IMU data
                print("  IMU data:")
                print(f"    Acceleration (m/s²): X={data.get('accel_x', 'N/A'):.3f}, Y={data.get('accel_y', 'N/A'):.3f}, Z={data.get('accel_z', 'N/A'):.3f}")
                print(f"    Gyroscope (deg/s): X={data.get('gyro_x', 'N/A'):.3f}, Y={data.get('gyro_y', 'N/A'):.3f}, Z={data.get('gyro_z', 'N/A'):.3f}")
                
                # Print analog data
                print("  Analog data:")
                print(f"    ADC0: {data.get('analog0', 'N/A')}")
                print(f"    ADC1: {data.get('analog1', 'N/A')}")
                print(f"    ADC2: {data.get('analog2', 'N/A')}")
                
                led.toggle()
                time.sleep(1)
            except Exception as e:
                print(f"  Error reading sensors: {e}")
    
    except ImportError:
        print("SensorManager module not found")
    except Exception as e:
        print(f"Error initializing SensorManager: {e}")

def test_data_logger():
    """Test the DataLogger class"""
    print("\n--- DataLogger Test ---")
    
    try:
        from data_logger import DataLogger
        
        print("Initializing DataLogger...")
        data_logger = DataLogger(log_interval=2)  # 2 seconds between logs
        
        print("\nLogging data...")
        for i in range(3):
            try:
                success = data_logger.log_data()
                if success:
                    print(f"  Log {i+1}: Data logged successfully")
                else:
                    print(f"  Log {i+1}: Failed to log data")
                
                time.sleep(2)
            except Exception as e:
                print(f"  Error logging data: {e}")
    
    except ImportError:
        print("DataLogger module not found")
    except Exception as e:
        print(f"Error initializing DataLogger: {e}")

def main():
    """Run all incremental tests"""
    print("\n=== Starting Incremental Sensor Tests ===\n")
    print(f"Machine: {machine.unique_id()}")
    
    memory_status()
    
    # Test I2C and get the bus for further tests
    i2c, devices = test_i2c_scan()
    
    # Test individual sensors if devices were found
    if devices:
        # Look for BME280 addresses (0x76, 0x77)
        if 0x76 in devices or 0x77 in devices:
            test_bme280(i2c)
        else:
            print("\nBME280 not detected at expected addresses")
        
        # Look for LSM6DSOX addresses (0x6A, 0x6B)
        if 0x6A in devices or 0x6B in devices:
            test_lsm6dsox(i2c)
        else:
            print("\nLSM6DSOX not detected at expected addresses")
    
    # Test higher-level modules
    test_sensor_manager()
    test_data_logger()
    
    memory_status()
    print("\n=== Incremental Sensor Tests Complete ===")

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"Error during tests: {e}")
    finally:
        led.off()  # Ensure LED is off when done
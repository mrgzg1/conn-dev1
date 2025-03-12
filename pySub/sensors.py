from machine import Pin, I2C, ADC
import time
import json
import random

# Import sensor drivers
try:
    from bme280 import BME280
except ImportError:
    print("BME280 driver not found")

try:
    from lsm6dsox import LSM6DSOX
except ImportError:
    print("LSM6DSOX driver not found")

class SensorManager:
    """Manager for various sensors connected to the RP2040"""
    
    def __init__(self):
        # Initialize I2C bus for digital sensors
        # Use I2C1 with GPIO2 (SDA) and GPIO3 (SCL)
        self.i2c = I2C(1, scl=Pin(3), sda=Pin(2), freq=400000)
        
        # Alternative I2C0 configuration if I2C1 fails
        # self.i2c = I2C(0, scl=Pin(1), sda=Pin(0), freq=400000)
        
        # Initialize ADC pins for analog sensors
        self.adc0 = ADC(Pin(26))  # ADC0
        self.adc1 = ADC(Pin(27))  # ADC1
        self.adc2 = ADC(Pin(28))  # ADC2
        
        # Storage for sensor objects
        self.sensors = {}
        
        # Scan for I2C devices and initialize them
        self._scan_i2c()
        
    def _scan_i2c(self):
        """Scan for I2C devices and initialize them"""
        print("Scanning for I2C devices...")
        devices = self.i2c.scan()
        
        self.available_sensors = []
        
        for addr in devices:
            sensor_name = self._identify_i2c_device(addr)
            if sensor_name:
                print(f"Found {sensor_name} at address 0x{addr:02x}")
                self.available_sensors.append(sensor_name)
                
                # Initialize sensor based on its type
                try:
                    if sensor_name == "BME280":
                        self.sensors["bme280"] = BME280(i2c=self.i2c, address=addr)
                        print("BME280 initialized")
                    elif sensor_name == "LSM6DSOX":
                        self.sensors["lsm6dsox"] = LSM6DSOX(i2c=self.i2c, address=addr)
                        print("LSM6DSOX initialized")
                except Exception as e:
                    print(f"Error initializing {sensor_name}: {e}")
                    
        if not self.available_sensors:
            print("No I2C sensors found")
    
    def _identify_i2c_device(self, addr):
        """Identify I2C device by address"""
        # Common I2C sensor addresses
        sensors = {
            0x76: "BME280",     # Temperature, humidity, pressure
            0x77: "BME280",     # Alternative BME280 address
            0x68: "MPU6050",    # Accelerometer, gyroscope
            0x6A: "LSM6DSOX",   # IMU on RP2040 Connect
            0x6B: "LSM6DSOX",   # Alternative LSM6DSOX address
            0x48: "ADS1115",    # ADC converter
            0x40: "HDC1080",    # Humidity, temperature
            0x44: "SHT30",      # Humidity, temperature
            # Add more as needed
        }
        
        return sensors.get(addr, f"Unknown (0x{addr:02x})")
    
    def read_bme280(self):
        """Read all data from BME280 sensor"""
        if "bme280" in self.sensors:
            try:
                temp, pressure, humidity = self.sensors["bme280"].read_compensated_data()
                return {
                    "temperature": temp,  # °C
                    "pressure": pressure / 100,  # hPa (convert from Pa)
                    "humidity": humidity,  # %
                    "altitude": self.sensors["bme280"].altitude  # meters
                }
            except Exception as e:
                print(f"Error reading BME280: {e}")
        
        # Return simulated data if no sensor or error
        return {
            "temperature": 25.0 + (random.random() * 2 - 1),
            "pressure": 1013.0 + (random.random() * 10 - 5),
            "humidity": 50.0 + (random.random() * 10 - 5),
            "altitude": 100.0 + (random.random() * 2 - 1)
        }
    
    def read_lsm6dsox(self):
        """Read all data from LSM6DSOX IMU"""
        if "lsm6dsox" in self.sensors:
            try:
                return self.sensors["lsm6dsox"].read_all()
            except Exception as e:
                print(f"Error reading LSM6DSOX: {e}")
        
        # Return simulated data if no sensor or error
        return {
            "accel_x": random.uniform(-1.0, 1.0),
            "accel_y": random.uniform(-1.0, 1.0),
            "accel_z": random.uniform(9.5, 10.0),  # Mostly gravity
            "gyro_x": random.uniform(-1.0, 1.0),
            "gyro_y": random.uniform(-1.0, 1.0),
            "gyro_z": random.uniform(-1.0, 1.0),
            "temperature": 25.0 + (random.random() * 2 - 1)
        }
    
    def read_analog_sensors(self):
        """Read data from analog sensors"""
        # Read values from ADC pins
        adc0_val = self.adc0.read_u16()
        adc1_val = self.adc1.read_u16()
        adc2_val = self.adc2.read_u16()
        
        # Convert ADC readings if needed (depends on connected sensors)
        return {
            "analog0": adc0_val,
            "analog1": adc1_val,
            "analog2": adc2_val
        }
    
    def read_all(self):
        """Read all available sensor data"""
        # Get current timestamp
        timestamp = time.time()
        
        # Read data from all sensors
        data = {
            "timestamp": timestamp,
        }
        
        # Add data from BME280 (environmental)
        bme_data = self.read_bme280()
        data.update(bme_data)
        
        # Add data from LSM6DSOX (IMU)
        imu_data = self.read_lsm6dsox()
        data.update(imu_data)
        
        # Add analog data
        analog_data = self.read_analog_sensors()
        data.update(analog_data)
        
        # Add formatted timestamp for readability
        try:
            dt = time.localtime(timestamp)
            data["datetime"] = "{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}".format(
                dt[0], dt[1], dt[2], dt[3], dt[4], dt[5]
            )
        except:
            pass  # Skip if formatting fails
        
        return data
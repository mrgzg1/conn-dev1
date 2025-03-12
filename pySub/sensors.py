from machine import Pin, I2C, ADC
import time

class SensorManager:
    """Manager for various sensors connected to the RP2040"""
    
    def __init__(self):
        # Initialize I2C bus for digital sensors
        self.i2c = I2C(0, scl=Pin(5), sda=Pin(4), freq=400000)
        
        # Initialize ADC pins for analog sensors
        self.adc0 = ADC(Pin(26))  # ADC0
        self.adc1 = ADC(Pin(27))  # ADC1
        self.adc2 = ADC(Pin(28))  # ADC2
        
        # Scan for I2C devices
        self._scan_i2c()
        
    def _scan_i2c(self):
        """Scan for I2C devices and initialize them"""
        print("Scanning for I2C devices...")
        devices = self.i2c.scan()
        
        self.available_sensors = []
        
        for addr in devices:
            # Try to identify device by address
            sensor_name = self._identify_i2c_device(addr)
            if sensor_name:
                print(f"Found {sensor_name} at address 0x{addr:02x}")
                self.available_sensors.append(sensor_name)
                
        if not self.available_sensors:
            print("No I2C sensors found")
    
    def _identify_i2c_device(self, addr):
        """Identify I2C device by address"""
        # Common I2C sensor addresses
        sensors = {
            0x76: "BME280",     # Temperature, humidity, pressure
            0x77: "BME680",     # Temperature, humidity, pressure, gas
            0x68: "MPU6050",    # Accelerometer, gyroscope
            0x48: "ADS1115",    # ADC converter
            0x40: "HDC1080",    # Humidity, temperature
            0x44: "SHT30",      # Humidity, temperature
            # Add more as needed
        }
        
        return sensors.get(addr, f"Unknown (0x{addr:02x})")
    
    def read_temperature(self):
        """Read temperature from available sensor"""
        # This is a placeholder - implement actual sensor reading
        # based on available sensors
        if "BME280" in self.available_sensors:
            # Read from BME280
            # bme = BME280(i2c=self.i2c)
            # return bme.temperature
            return 25.0  # Placeholder
        elif "SHT30" in self.available_sensors:
            # Read from SHT30
            # sht = SHT30(i2c=self.i2c)
            # return sht.temperature
            return 25.0  # Placeholder
        else:
            # Fallback to analog temperature sensor or fixed value
            return self._read_analog_temperature()
    
    def read_humidity(self):
        """Read humidity from available sensor"""
        # Placeholder like above
        return 50.0
    
    def read_pressure(self):
        """Read pressure from available sensor"""
        # Placeholder
        return 1013.0
    
    def _read_analog_temperature(self):
        """Read temperature from analog sensor"""
        # Example for LM35 temperature sensor
        adc_value = self.adc0.read_u16()
        # Convert to voltage
        voltage = adc_value * 3.3 / 65535
        # Convert voltage to temperature (depends on sensor)
        # For LM35: 10mV per degree C
        temperature = voltage * 100
        return temperature
    
    def read_all(self):
        """Read all available sensor data"""
        data = {
            "timestamp": time.time(),
            "temperature": self.read_temperature(),
            "humidity": self.read_humidity(),
            "pressure": self.read_pressure(),
            # Add more sensors as needed
        }
        return data
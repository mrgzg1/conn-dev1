import machine
import time
import ujson
from machine import Pin, SPI
from flash_storage import FlashStorage
from sensors import SensorManager

# Import hardware configuration 
from hardware_config import (
    LED_PIN, SPI_BUS, SPI_SCK_PIN, SPI_MOSI_PIN, 
    SPI_MISO_PIN, SPI_CS_PIN
)

class DataLogger:
    def __init__(self, log_interval=10):
        """Initialize the data logger
        
        Args:
            log_interval: Time between logs in seconds
        """
        self.log_interval = log_interval
        self.led = Pin(LED_PIN, Pin.OUT)
        
        # Initialize flash storage
        self.storage = FlashStorage()
            
        # Initialize SPI for flash storage
        self.spi = SPI(SPI_BUS, baudrate=40000000, 
                      sck=Pin(SPI_SCK_PIN), mosi=Pin(SPI_MOSI_PIN), miso=Pin(SPI_MISO_PIN))
        self.cs = Pin(SPI_CS_PIN, Pin.OUT)
        
        # Initialize sensor manager
        try:
            self.sensor_manager = SensorManager()
            print("Sensor manager initialized")
            
            # Print available sensors
            if hasattr(self.sensor_manager, 'available_sensors'):
                print(f"Available sensors: {self.sensor_manager.available_sensors}")
            
            # Print initialized sensors
            if hasattr(self.sensor_manager, 'sensors') and self.sensor_manager.sensors:
                print(f"Initialized sensors: {list(self.sensor_manager.sensors.keys())}")
            
        except Exception as e:
            print(f"Error initializing sensors: {e}")
            self.sensor_manager = None
        
    def read_sensors(self):
        """Read sensor data"""
        if self.sensor_manager:
            return self.sensor_manager.read_all()
        else:
            # Fallback to dummy data if sensor manager failed
            return {
                "timestamp": time.time(),
                "temperature": 25.0,
                "humidity": 50.0,
                "pressure": 1013.0,
                "accel_x": 0.0,
                "accel_y": 0.0,
                "accel_z": 9.8,
                "gyro_x": 0.0,
                "gyro_y": 0.0,
                "gyro_z": 0.0,
            }
        
    def log_data(self):
        """Log sensor data to file"""
        data = self.read_sensors()
        
        # Generate filename with date
        date_str = "{:04d}-{:02d}-{:02d}".format(*time.localtime()[0:3])
        filename = f"/data/log_{date_str}.txt"
        
        # Create a compact representation for logging
        # Just keep a small summary for terminal output
        log_summary = {
            "timestamp": data.get("datetime", time.time()),
            "temp": f"{data.get('temperature', 0):.1f}°C",
            "humid": f"{data.get('humidity', 0):.1f}%",
            "press": f"{data.get('pressure', 0):.1f}hPa"
        }
        
        # Add IMU data summary if available
        if all(k in data for k in ["accel_x", "accel_y", "accel_z"]):
            accel_mag = (data["accel_x"]**2 + data["accel_y"]**2 + data["accel_z"]**2)**0.5
            log_summary["accel"] = f"{accel_mag:.2f}m/s²"
        
        # Log to file using storage manager
        if self.storage.append_file(filename, ujson.dumps(data) + '\n'):
            self.led.toggle()  # Blink LED to indicate successful log
            print("Data logged:", log_summary)
            return True
        return False
            
    def run(self):
        """Run the data logger main loop"""
        print("Starting data logger...")
        
        # Print storage info
        storage_info = self.storage.get_storage_info()
        if storage_info:
            print(f"Storage: {storage_info['free']/1024:.2f}KB free of {storage_info['total']/1024:.2f}KB")
        
        while True:
            try:
                self.log_data()
                time.sleep(self.log_interval)
            except KeyboardInterrupt:
                print("Data logger stopped")
                break
            except Exception as e:
                print("Error in data logger:", e)
                time.sleep(1)
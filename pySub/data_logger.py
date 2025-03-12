import machine
import time
import ujson
from machine import Pin, SPI
from flash_storage import FlashStorage
from sensors import SensorManager

class DataLogger:
    def __init__(self, log_interval=10):
        """Initialize the data logger
        
        Args:
            log_interval: Time between logs in seconds
        """
        self.log_interval = log_interval
        self.led = Pin(25, Pin.OUT)
        
        # Initialize flash storage
        self.storage = FlashStorage()
            
        # Initialize SPI for flash storage
        self.spi = SPI(1, baudrate=40000000, 
                      sck=Pin(10), mosi=Pin(11), miso=Pin(12))
        self.cs = Pin(13, Pin.OUT)
        
        # Initialize sensor manager
        try:
            self.sensor_manager = SensorManager()
            print("Sensor manager initialized")
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
            }
        
    def log_data(self):
        """Log sensor data to file"""
        data = self.read_sensors()
        
        # Generate filename with date
        date_str = "{:04d}-{:02d}-{:02d}".format(*time.localtime()[0:3])
        filename = f"/data/log_{date_str}.txt"
        
        # Log to file using storage manager
        if self.storage.append_file(filename, ujson.dumps(data) + '\n'):
            self.led.toggle()  # Blink LED to indicate successful log
            print("Data logged:", data)
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
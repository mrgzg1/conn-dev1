import time
from wifi_manager import WiFiManager
from data_logger import DataLogger

# Create WiFi manager
wifi = WiFiManager()

# Try to connect to WiFi (replace with your credentials)
# wifi.connect("YourSSID", "YourPassword")

# If that fails, start access point
if not wifi.wlan_sta.isconnected():
    wifi.start_ap()

# Initialize and run data logger
logger = DataLogger(log_interval=10)  # Log every 10 seconds
logger.run()
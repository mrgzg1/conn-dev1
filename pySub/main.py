import time
import json
from wifi_manager import WiFiManager
from data_logger import DataLogger

# Create WiFi manager
wifi = WiFiManager()

# Try to connect to WiFi using credentials from secrets.json
connected = False

try:
    with open('secrets.json', 'r') as f:
        networks = json.load(f)
        
    print(f"Found {len(networks)} WiFi networks in secrets.json")
    
    # Try each network until one connects
    for network in networks:
        ssid = network.get('ssid')
        password = network.get('pass')
        
        if ssid and password:
            print(f"Trying to connect to {ssid}...")
            if wifi.connect(ssid, password, timeout=5):
                connected = True
                print(f"Successfully connected to {ssid}")
                break
        else:
            print("Invalid network entry in secrets.json, missing ssid or password")
            
except (OSError, ValueError) as e:
    print(f"Error loading secrets.json: {e}")
except Exception as e:
    print(f"Unexpected error: {e}")

# If no connection was established, start access point
if not connected:
    print("Could not connect to any WiFi network")
    wifi.start_ap()

# Initialize and run data logger
print("Starting data logger...")
logger = DataLogger(log_interval=10)  # Log every 10 seconds
logger.run()
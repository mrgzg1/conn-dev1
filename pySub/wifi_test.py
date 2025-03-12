import time
import json
from wifi_manager import WiFiManager

print("Starting WiFi test...")

# Create WiFi manager
try:
    print("Initializing WiFi manager...")
    wifi = WiFiManager()
    print("WiFi manager initialized successfully")
except Exception as e:
    print(f"Error initializing WiFi manager: {e}")
    import sys
    sys.exit(1)

# Try to connect to WiFi using credentials from secrets.json
connected = False

try:
    print("Trying to load secrets.json...")
    with open('secrets.json', 'r') as f:
        networks = json.load(f)
        
    print(f"Found {len(networks)} WiFi networks in secrets.json")
    
    # Try each network until one connects
    for i, network in enumerate(networks):
        ssid = network.get('ssid')
        password = network.get('pass')
        
        if ssid and password:
            print(f"Trying to connect to network {i+1}: {ssid}...")
            try:
                if wifi.connect(ssid, password, timeout=10):
                    connected = True
                    print(f"Successfully connected to {ssid}")
                    break
                else:
                    print(f"Failed to connect to {ssid}")
            except Exception as e:
                print(f"Error connecting to {ssid}: {e}")
        else:
            print(f"Network {i+1} has invalid configuration in secrets.json")
            
except (OSError, ValueError) as e:
    print(f"Error loading secrets.json: {e}")
except Exception as e:
    print(f"Unexpected error: {e}")

# If no connection was established, start access point
if not connected:
    print("Could not connect to any WiFi network, starting AP mode...")
    try:
        ap_ssid = wifi.start_ap()
        print(f"AP started with SSID: {ap_ssid}")
    except Exception as e:
        print(f"Error starting AP mode: {e}")

print("WiFi test completed")
import network
import time

class WiFiManager:
    def __init__(self, ssid_prefix="RP2040_AP"):
        try:
            self.wlan_sta = network.WLAN(network.STA_IF)
            self.wlan_sta.active(True)
            self.wlan_ap = network.WLAN(network.AP_IF)
            self.ssid_prefix = ssid_prefix
        except Exception as e:
            print(f"WiFi initialization error: {e}")
            # Fallback initialization
            try:
                self.wlan_sta = network.WLAN(network.STA_IF)
                self.wlan_ap = network.WLAN(network.AP_IF)
                self.ssid_prefix = ssid_prefix
            except Exception as e2:
                print(f"Fallback initialization also failed: {e2}")
        
    def connect(self, ssid, password, timeout=10):
        """Connect to a WiFi network"""
        print(f"Connecting to {ssid}...")
        self.wlan_sta.connect(ssid, password)
            
        # Wait for connection with timeout
        start_time = time.time()
        while not self.wlan_sta.isconnected():
            if time.time() - start_time > timeout:
                print("Connection timeout")
                return False
            time.sleep(0.1)
        
        print("Connected!")
        print("IP:", self.wlan_sta.ifconfig()[0])
        return True
            
    def start_ap(self):
        """Start access point mode"""
        ap_ssid = f"{self.ssid_prefix}_{self._get_id()}"
        self.wlan_ap.active(True)
        
        # Try different config approaches to handle firmware differences
        try:
            # Check which parameters are accepted
            param_name = None
            for param in ["ssid", "essid"]:
                try:
                    test_config = {param: ap_ssid}
                    self.wlan_ap.config(**test_config)
                    param_name = param
                    break
                except:
                    continue
            
            if param_name:
                config = {param_name: ap_ssid, "password": "micropython", "authmode": 3}
                self.wlan_ap.config(**config)
            else:
                # Simplified approach as last resort
                self.wlan_ap.config(ssid=ap_ssid)
                
        except Exception as e:
            print(f"AP config error: {e}")
            # Try minimal configuration as fallback
            try:
                self.wlan_ap.config(ssid=ap_ssid)
            except:
                print("Could not configure AP mode")
        
        print(f"AP started: {ap_ssid}")
        print("IP:", self.wlan_ap.ifconfig()[0])
        return ap_ssid
        
    def _get_id(self):
        """Get unique ID from MAC address"""
        try:
            mac = self.wlan_sta.config("mac")
            return "{:02x}{:02x}{:02x}".format(mac[0], mac[1], mac[2])
        except Exception as e:
            print(f"Error getting MAC address: {e}")
            # Return fallback ID
            import urandom
            random_id = urandom.getrandbits(24)
            return "{:06x}".format(random_id)
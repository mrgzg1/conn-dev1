import time
from machine import Pin, I2C
from bme280 import BME280

# LED for visual feedback
led = Pin(25, Pin.OUT)

def main():
    print("BME280 Sensor Test")
    
    # Initialize I2C (I2C1 with correct pins)
    i2c = I2C(1, scl=Pin(3), sda=Pin(2), freq=400000)
    
    # Try alternative I2C0 if needed
    # i2c = I2C(0, scl=Pin(1), sda=Pin(0), freq=400000)
    
    # Scan I2C bus
    devices = i2c.scan()
    print("I2C devices found:", [hex(addr) for addr in devices])
    
    # Try both common BME280 addresses
    bme = None
    for addr in [0x76, 0x77]:
        try:
            print(f"Trying BME280 at address 0x{addr:02x}...")
            bme = BME280(i2c=i2c, address=addr)
            print(f"BME280 initialized at 0x{addr:02x}")
            break
        except Exception as e:
            print(f"Not found at 0x{addr:02x}: {e}")
    
    if not bme:
        print("BME280 not found! Check connections and try again.")
        return
    
    # Continuously read and display sensor data
    print("\nContinuously reading BME280 data. Press Ctrl+C to stop.\n")
    
    try:
        while True:
            # Read sensor data
            temp, pressure, humidity = bme.read_compensated_data()
            
            # Display readings
            print("Temperature: {:.2f} °C".format(temp))
            print("Pressure: {:.2f} hPa".format(pressure / 100))
            print("Humidity: {:.2f} %".format(humidity))
            print("Altitude: {:.2f} m".format(bme.altitude))
            print("Dew Point: {:.2f} °C".format(bme.dew_point))
            print("-" * 30)
            
            # Toggle LED to show we're alive
            led.toggle()
            
            # Wait before next reading
            time.sleep(2)
            
    except KeyboardInterrupt:
        print("\nTest stopped by user")
    finally:
        led.off()

if __name__ == "__main__":
    main()
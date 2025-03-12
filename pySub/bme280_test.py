import time
from machine import Pin, I2C
from bme280 import BME280

# Import hardware configuration
from hardware_config import (
    I2C_BUS, I2C_SCL_PIN, I2C_SDA_PIN, I2C_FREQ,
    BME280_I2C_ADDR, LED_PIN
)

# LED for visual feedback
led = Pin(LED_PIN, Pin.OUT)

def main():
    print("BME280 Sensor Test")
    
    # Initialize I2C using hardware config
    print(f"Initializing I2C{I2C_BUS} with SCL=GPIO{I2C_SCL_PIN}, SDA=GPIO{I2C_SDA_PIN}")
    i2c = I2C(I2C_BUS, scl=Pin(I2C_SCL_PIN), sda=Pin(I2C_SDA_PIN), freq=100000)  # Lower frequency for better reliability
    
    # Scan I2C bus
    devices = i2c.scan()
    print("I2C devices found:", [hex(addr) for addr in devices])
    
    # Use the address from hardware config
    bme = None
    try:
        print(f"Trying BME280 at address 0x{BME280_I2C_ADDR:02X}...")
        bme = BME280(i2c=i2c, address=BME280_I2C_ADDR)
        print(f"BME280 initialized at 0x{BME280_I2C_ADDR:02X}")
    except Exception as e:
        print(f"Not found at 0x{BME280_I2C_ADDR:02X}: {e}")
        # Try alternative address as fallback
        alt_addr = 0x77 if BME280_I2C_ADDR == 0x76 else 0x76
        try:
            print(f"Trying BME280 at alternative address 0x{alt_addr:02X}...")
            bme = BME280(i2c=i2c, address=alt_addr)
            print(f"BME280 initialized at 0x{alt_addr:02X}")
        except Exception as e:
            print(f"Not found at 0x{alt_addr:02X}: {e}")
    
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
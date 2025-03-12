import time
from machine import Pin, I2C
from lsm6dsox import LSM6DSOX

# Import hardware configuration
from hardware_config import (
    I2C_BUS, I2C_SCL_PIN, I2C_SDA_PIN, I2C_FREQ,
    LSM6DSOX_I2C_ADDR, LED_PIN
)

# LED for visual feedback
led = Pin(LED_PIN, Pin.OUT)

def main():
    print("LSM6DSOX IMU Test")
    
    # Initialize I2C using hardware config
    print(f"Initializing I2C{I2C_BUS} with SCL=GPIO{I2C_SCL_PIN}, SDA=GPIO{I2C_SDA_PIN}")
    i2c = I2C(I2C_BUS, scl=Pin(I2C_SCL_PIN), sda=Pin(I2C_SDA_PIN), freq=100000)  # Lower frequency for better reliability
    
    # Scan I2C bus
    devices = i2c.scan()
    print("I2C devices found:", [hex(addr) for addr in devices])
    
    # Use the address from hardware config
    imu = None
    try:
        print(f"Trying LSM6DSOX at address 0x{LSM6DSOX_I2C_ADDR:02X}...")
        imu = LSM6DSOX(i2c=i2c, address=LSM6DSOX_I2C_ADDR)
        print(f"LSM6DSOX initialized at 0x{LSM6DSOX_I2C_ADDR:02X}")
    except Exception as e:
        print(f"Not found at 0x{LSM6DSOX_I2C_ADDR:02X}: {e}")
        # Try alternative address as fallback
        alt_addr = 0x6B if LSM6DSOX_I2C_ADDR == 0x6A else 0x6A
        try:
            print(f"Trying LSM6DSOX at alternative address 0x{alt_addr:02X}...")
            imu = LSM6DSOX(i2c=i2c, address=alt_addr)
            print(f"LSM6DSOX initialized at 0x{alt_addr:02X}")
        except Exception as e:
            print(f"Not found at 0x{alt_addr:02X}: {e}")
    
    if not imu:
        print("LSM6DSOX not found! Check connections and try again.")
        return
    
    # Continuously read and display sensor data
    print("\nContinuously reading LSM6DSOX data. Press Ctrl+C to stop.\n")
    
    try:
        while True:
            # Read accelerometer data
            accel = imu.read_acceleration()
            accel_x, accel_y, accel_z = accel
            accel_mag = (accel_x**2 + accel_y**2 + accel_z**2)**0.5
            
            # Read gyroscope data
            gyro = imu.read_gyro()
            gyro_x, gyro_y, gyro_z = gyro
            
            # Read temperature
            temp = imu.read_temperature()
            
            # Display readings
            print("Accelerometer (m/s²):")
            print(f"  X: {accel_x:.3f}, Y: {accel_y:.3f}, Z: {accel_z:.3f}")
            print(f"  Magnitude: {accel_mag:.3f}")
            
            print("\nGyroscope (deg/s):")
            print(f"  X: {gyro_x:.3f}, Y: {gyro_y:.3f}, Z: {gyro_z:.3f}")
            
            print(f"\nTemperature: {temp:.2f} °C")
            print("-" * 40)
            
            # Toggle LED to show we're alive
            led.toggle()
            
            # Wait before next reading
            time.sleep(1)
            
    except KeyboardInterrupt:
        print("\nTest stopped by user")
    finally:
        led.off()

if __name__ == "__main__":
    main()
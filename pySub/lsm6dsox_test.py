import time
from machine import Pin, I2C
from lsm6dsox import LSM6DSOX

# LED for visual feedback
led = Pin(25, Pin.OUT)

def main():
    print("LSM6DSOX IMU Test")
    
    # Initialize I2C (I2C1 with correct pins)
    i2c = I2C(1, scl=Pin(3), sda=Pin(2), freq=400000)
    
    # Try alternative I2C0 if needed
    # i2c = I2C(0, scl=Pin(1), sda=Pin(0), freq=400000)
    
    # Scan I2C bus
    devices = i2c.scan()
    print("I2C devices found:", [hex(addr) for addr in devices])
    
    # Try both common LSM6DSOX addresses
    imu = None
    for addr in [0x6A, 0x6B]:
        try:
            print(f"Trying LSM6DSOX at address 0x{addr:02x}...")
            imu = LSM6DSOX(i2c=i2c, address=addr)
            print(f"LSM6DSOX initialized at 0x{addr:02x}")
            break
        except Exception as e:
            print(f"Not found at 0x{addr:02x}: {e}")
    
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
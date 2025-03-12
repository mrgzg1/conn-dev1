import time
from machine import Pin, I2C
from sensors import SensorManager

# LED for visual feedback
led = Pin(25, Pin.OUT)

def main():
    """Test all sensors and print readings"""
    print("Initializing sensor manager...")
    sensor_mgr = SensorManager()
    
    print("\nAvailable sensors:", sensor_mgr.available_sensors)
    print("Initialized sensors:", list(sensor_mgr.sensors.keys()))
    
    print("\nStarting sensor readings. Press Ctrl+C to stop.")
    
    try:
        while True:
            # Read all sensor data
            data = sensor_mgr.read_all()
            
            # Print sensor data in a readable format
            print("-" * 50)
            print(f"Time: {data.get('datetime', 'unknown')}")
            
            # Environmental data (BME280)
            print("\nEnvironmental Data:")
            print(f"  Temperature: {data.get('temperature', 'N/A'):.2f} °C")
            print(f"  Pressure: {data.get('pressure', 'N/A'):.2f} hPa")
            print(f"  Humidity: {data.get('humidity', 'N/A'):.2f} %")
            print(f"  Altitude: {data.get('altitude', 'N/A'):.2f} m")
            
            # IMU data (LSM6DSOX)
            print("\nIMU Data:")
            print(f"  Acceleration (m/s²): X={data.get('accel_x', 'N/A'):.3f}, Y={data.get('accel_y', 'N/A'):.3f}, Z={data.get('accel_z', 'N/A'):.3f}")
            print(f"  Gyroscope (deg/s): X={data.get('gyro_x', 'N/A'):.3f}, Y={data.get('gyro_y', 'N/A'):.3f}, Z={data.get('gyro_z', 'N/A'):.3f}")
            print(f"  IMU Temperature: {data.get('imu_temperature', data.get('temperature', 'N/A')):.2f} °C")
            
            # Analog sensors
            print("\nAnalog Sensors:")
            print(f"  ADC0: {data.get('analog0', 'N/A')}")
            print(f"  ADC1: {data.get('analog1', 'N/A')}")
            print(f"  ADC2: {data.get('analog2', 'N/A')}")
            
            # Toggle LED to show we're running
            led.toggle()
            
            # Wait before next reading
            time.sleep(2)
            
    except KeyboardInterrupt:
        print("\nTest stopped by user")
    except Exception as e:
        print(f"\nError during test: {e}")
    finally:
        led.off()
        print("Test complete")

if __name__ == "__main__":
    main()
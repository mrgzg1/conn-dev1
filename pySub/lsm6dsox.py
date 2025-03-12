import time
from machine import Pin, I2C
from ustruct import unpack

# LSM6DSOX default address
LSM6DSOX_I2CADDR = 0x6A

# LSM6DSOX registers
LSM6DSOX_WHO_AM_I = 0x0F
LSM6DSOX_CTRL1_XL = 0x10
LSM6DSOX_CTRL2_G = 0x11
LSM6DSOX_STATUS_REG = 0x1E
LSM6DSOX_CTRL6_C = 0x15
LSM6DSOX_CTRL7_G = 0x16
LSM6DSOX_CTRL8_XL = 0x17
LSM6DSOX_OUT_TEMP_L = 0x20
LSM6DSOX_OUT_TEMP_H = 0x21
LSM6DSOX_OUTX_L_G = 0x22
LSM6DSOX_OUTX_H_G = 0x23
LSM6DSOX_OUTY_L_G = 0x24
LSM6DSOX_OUTY_H_G = 0x25
LSM6DSOX_OUTZ_L_G = 0x26
LSM6DSOX_OUTZ_H_G = 0x27
LSM6DSOX_OUTX_L_A = 0x28
LSM6DSOX_OUTX_H_A = 0x29
LSM6DSOX_OUTY_L_A = 0x2A
LSM6DSOX_OUTY_H_A = 0x2B
LSM6DSOX_OUTZ_L_A = 0x2C
LSM6DSOX_OUTZ_H_A = 0x2D

# Accelerometer range options
ACCEL_2G = const(0x00)  # ±2g
ACCEL_4G = const(0x02)  # ±4g
ACCEL_8G = const(0x03)  # ±8g
ACCEL_16G = const(0x01)  # ±16g

# Gyroscope range options
GYRO_250DPS = const(0x00)  # 250 degrees per second
GYRO_500DPS = const(0x01)  # 500 degrees per second
GYRO_1000DPS = const(0x02)  # 1000 degrees per second
GYRO_2000DPS = const(0x03)  # 2000 degrees per second

# Accelerometer output data rate
ACCEL_RATE_OFF = const(0x00)  # Power-down mode
ACCEL_RATE_12_5 = const(0x01)  # 12.5 Hz
ACCEL_RATE_26 = const(0x02)  # 26 Hz
ACCEL_RATE_52 = const(0x03)  # 52 Hz
ACCEL_RATE_104 = const(0x04)  # 104 Hz
ACCEL_RATE_208 = const(0x05)  # 208 Hz
ACCEL_RATE_416 = const(0x06)  # 416 Hz
ACCEL_RATE_833 = const(0x07)  # 833 Hz
ACCEL_RATE_1660 = const(0x08)  # 1660 Hz
ACCEL_RATE_3330 = const(0x09)  # 3330 Hz
ACCEL_RATE_6660 = const(0x0A)  # 6660 Hz

# Gyroscope output data rate
GYRO_RATE_OFF = const(0x00)  # Power-down mode
GYRO_RATE_12_5 = const(0x01)  # 12.5 Hz
GYRO_RATE_26 = const(0x02)  # 26 Hz
GYRO_RATE_52 = const(0x03)  # 52 Hz
GYRO_RATE_104 = const(0x04)  # 104 Hz
GYRO_RATE_208 = const(0x05)  # 208 Hz
GYRO_RATE_416 = const(0x06)  # 416 Hz
GYRO_RATE_833 = const(0x07)  # 833 Hz
GYRO_RATE_1660 = const(0x08)  # 1660 Hz
GYRO_RATE_3330 = const(0x09)  # 3330 Hz
GYRO_RATE_6660 = const(0x0A)  # 6660 Hz

class LSM6DSOX:
    def __init__(self, i2c, address=LSM6DSOX_I2CADDR, accel_range=ACCEL_4G, gyro_range=GYRO_1000DPS,
                 accel_rate=ACCEL_RATE_104, gyro_rate=GYRO_RATE_104):
        self.i2c = i2c
        self.address = address
        
        # Verify the device
        who_am_i = self.i2c.readfrom_mem(self.address, LSM6DSOX_WHO_AM_I, 1)[0]
        if who_am_i != 0x6C:  # The LSM6DSOX should return 0x6C
            raise RuntimeError("Could not find LSM6DSOX, got 0x%x instead" % who_am_i)
        
        # Configure accelerometer
        # Combine data rate and range: (rate << 4) | (range << 2)
        accel_config = (accel_rate << 4) | (accel_range << 2)
        self.i2c.writeto_mem(self.address, LSM6DSOX_CTRL1_XL, bytes([accel_config]))
        
        # Configure gyroscope
        # Combine data rate and range: (rate << 4) | (range << 2)
        gyro_config = (gyro_rate << 4) | (gyro_range << 2)
        self.i2c.writeto_mem(self.address, LSM6DSOX_CTRL2_G, bytes([gyro_config]))
        
        # Save the range values for later conversion
        self.accel_range = accel_range
        self.gyro_range = gyro_range
        
        # Set scale factors based on selected range
        self._accel_scale = {
            ACCEL_2G: 0.061,  # mg per LSB
            ACCEL_4G: 0.122,
            ACCEL_8G: 0.244,
            ACCEL_16G: 0.488
        }[accel_range] / 1000.0 * 9.80665  # Convert to m/s^2
        
        self._gyro_scale = {
            GYRO_250DPS: 8.75,  # mdps per LSB
            GYRO_500DPS: 17.5,
            GYRO_1000DPS: 35.0,
            GYRO_2000DPS: 70.0
        }[gyro_range] / 1000.0  # Convert to dps
    
    def read_temperature(self):
        """Read the temperature sensor and return degrees C"""
        temp_l = self.i2c.readfrom_mem(self.address, LSM6DSOX_OUT_TEMP_L, 1)[0]
        temp_h = self.i2c.readfrom_mem(self.address, LSM6DSOX_OUT_TEMP_H, 1)[0]
        
        # Combine high and low bytes
        temp = (temp_h << 8 | temp_l)
        # Convert to signed value
        if temp > 32767:
            temp -= 65536
        
        # Convert to degrees C (from datasheet)
        return 25.0 + temp / 256.0
    
    def read_acceleration(self):
        """Read the accelerometer and return (x, y, z) in m/s^2"""
        # Read 6 bytes (x, y, z) each with low and high byte
        data = self.i2c.readfrom_mem(self.address, LSM6DSOX_OUTX_L_A, 6)
        
        # Convert to signed 16-bit values
        x = unpack('<h', data[0:2])[0]
        y = unpack('<h', data[2:4])[0]
        z = unpack('<h', data[4:6])[0]
        
        # Scale to m/s^2
        return (x * self._accel_scale, y * self._accel_scale, z * self._accel_scale)
    
    def read_gyro(self):
        """Read the gyroscope and return (x, y, z) in degrees/s"""
        # Read 6 bytes (x, y, z) each with low and high byte
        data = self.i2c.readfrom_mem(self.address, LSM6DSOX_OUTX_L_G, 6)
        
        # Convert to signed 16-bit values
        x = unpack('<h', data[0:2])[0]
        y = unpack('<h', data[2:4])[0]
        z = unpack('<h', data[4:6])[0]
        
        # Scale to degrees/s
        return (x * self._gyro_scale, y * self._gyro_scale, z * self._gyro_scale)
        
    def read_all(self):
        """Read accelerometer, gyroscope and temperature. Return dict with all values."""
        # Read all sensor values in one go
        accel = self.read_acceleration()
        gyro = self.read_gyro()
        temp = self.read_temperature()
        
        return {
            "accel_x": accel[0],
            "accel_y": accel[1],
            "accel_z": accel[2],
            "gyro_x": gyro[0],
            "gyro_y": gyro[1],
            "gyro_z": gyro[2],
            "temperature": temp
        }
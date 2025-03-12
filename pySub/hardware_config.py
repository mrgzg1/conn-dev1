"""
Hardware configuration for RP2040 Connect board
Contains pin assignments and hardware-specific settings
"""

# I2C Configuration
# Based on Arduino Nano RP2040 Connect hardware config
# I2C1: SCL=GPIO27, SDA=GPIO26
I2C_BUS = 1
I2C_SCL_PIN = 27
I2C_SDA_PIN = 26
I2C_FREQ = 400000

# BME280 Configuration
BME280_I2C_ADDR = 0x76  # Default BME280 address, can be 0x76 or 0x77

# LSM6DSOX Configuration (IMU on RP2040 Connect)
LSM6DSOX_I2C_ADDR = 0x6A  # Default LSM6DSOX address

# LED Configuration
LED_PIN = 25

# ADC Configuration
ADC0_PIN = 26
ADC1_PIN = 27
ADC2_PIN = 28

# SPI Configuration for flash storage
SPI_BUS = 1
SPI_SCK_PIN = 10
SPI_MOSI_PIN = 11
SPI_MISO_PIN = 12
SPI_CS_PIN = 13
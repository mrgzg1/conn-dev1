# boot.py -- run on boot-up
import machine
import time

# Initialize hardware
led = machine.Pin(25, machine.Pin.OUT)

# Blink LED to indicate boot
for _ in range(3):
    led.value(1)
    time.sleep(0.1)
    led.value(0)
    time.sleep(0.1)

print("RP2040 Connect booted successfully!")
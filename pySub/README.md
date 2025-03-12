# MicroPython Data Logger for RP2040 Connect

A MicroPython-based data logger for the Raspberry Pi Pico RP2040 Connect board.

## Features

- Data logging to internal flash storage
- WiFi connectivity for remote access
- Simple API for sensor integration
- LED status indication

## Installation

### Prerequisites

- Raspberry Pi Pico RP2040 Connect board
- Python 3.6+ with uv installed

### Setup Environment

```bash
# Initialize the project
uv init

# Install required packages
uv add rshell esptool mpremote adafruit-ampy

# Create lock file and sync
uv lock
uv sync
```

### Flash MicroPython to RP2040

1. Download the latest MicroPython firmware for RP2040:
```bash
curl -O https://micropython.org/download/rp2-pico-w/rp2-pico-w-latest.uf2
```

2. Connect your RP2040 while holding the BOOTSEL button to enter bootloader mode

3. Copy the firmware to the mounted drive:
```bash
# On macOS (adjust path as needed)
cp rp2-pico-w-latest.uf2 /Volumes/RPI-RP2/
```

### Upload Code to the Device

```bash
# Find your device port (usually something like /dev/tty.usbmodem*)
ls /dev/tty.usb*

# Upload the code (replace PORT with your actual port)
PORT=/dev/tty.usbmodem14101
ampy --port $PORT put boot.py
ampy --port $PORT put main.py
ampy --port $PORT put wifi_manager.py
ampy --port $PORT put data_logger.py
ampy --port $PORT put flash_storage.py
```

## Usage

1. After uploading the code, the board will automatically start logging data
2. It will create an access point named `RP2040_AP_XXXX` if WiFi credentials aren't set
3. To view the logs, connect to the REPL:
```bash
screen $PORT 115200
# or
mpremote repl
```

4. To retrieve logged data:
```bash
ampy --port $PORT get /data/log_YYYY-MM-DD.txt
```

## Customization

Edit `main.py` to set your WiFi credentials:
```python
wifi.connect("YourSSID", "YourPassword")
```

Modify `data_logger.py` to integrate your specific sensors in the `read_sensors()` method.

## License

MIT
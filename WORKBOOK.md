# Arduino RP2040 Connect Sensor Data Collection Workbook

## Overview of Projects

The repository contains approaches to solve the same problem: collecting sensor data from an RP2040-based board with persistent storage and network connectivity.

### Current State Summary

As of the latest commit, there are persistent issues with:
- I2C devices not being detected consistently
- WiFi connectivity being unreliable on micropython core
- Flash storage access challenges through Arduino core

No single implementation successfully combines all three key requirements, but some combo of the requirement works based on what firmware we are on:
1. ✅ Reliable sensor data collection
2. ✅ Persistent data storage
3. ✅ WiFi connectivity for remote access

## Project Details

### 1. 📁 pySub: MicroPython Implementation

**Core Concept**: Using MicroPython to access the full 16MB flash storage while providing a clean interface for sensor integration.
#### Strengths
- ✅ Successfully accesses all 16MB of flash storage
- ✅ Clean, modular Python code architecture

#### Limitations
- ❌ Unreliable WiFi connectivity (main blocker)
- ❌ I2C sensor detection problems

#### Current Status
This version worked quite well for offline data collection. [This example](https://docs.arduino.cc/tutorials/nano-rp2040-connect/rp2040-data-logger/) which I used in prompting as well, shows how easy it is to get a data collector that writes to csv. This would be my pick for data collection, but we have to have good LED / button interface to make it work, or have a BLE workflow to configure and add more data to it like location & time.

The main flaws are network connectivity and I2C devices. On network connectivity, I tried updating the nina firmware as well, in one of the combos of the micropython firmware
 OpenMv: https://github.com/openmv/openmv/releases/ 
 Micropython: https://micropython.org/download/ARDUINO_NANO_RP2040_CONNECT/

### 2. 📁 storage_test: Arduino Core Flash Storage Implementation
#### Current Status
The code shows considerable effort to stabilize flash operations, with buffering, CRC verification, and multiple safety checks. The complexity suggests ongoing challenges with reliable flash storage. The implementation focuses almost exclusively on storage reliability rather than sensor integration.

### 3. 📁 device_db: Web Interface with Storage Attempt
#### Current Status
This was a fork off subway_collection in attempt to let LLM go crazy in isolation to get storage integrated.

### 2 & 3 Limitations
- ❌ Minimal actual data persistence
I couldn't get access to the 16MB onboard flash via any of the arduino cores, not sure if it was related to previously mentioned issue or I just unlucky with firmware version

### 4. 📁 subway_collection: Working Visualization without Persistence
#### Code Structure
```
subway_collection/
├── src/
│   ├── data_prep.py     # Embeds web resources in firmware
│   ├── led_control.h    # LED control functions
│   ├── main.cpp         # Main application logic
│   ├── sensor.h         # Sensor reading implementation
│   ├── sensor_config.h  # Sensor configuration
│   ├── web/             # Web interface files (HTML, JS)
│   ├── web_files.h      # Generated web resource header
│   └── web_server.h     # Web server implementation
└── platformio.ini       # Project configuration
```

#### Strengths
- ✅ Working real-time visualization through web interface
- ✅ Reliable sensor data collection
- ✅ Functional WiFi connectivity with fallback mechanisms
- ✅ Interactive dashboard with charts

#### Limitations
- ❌ No flash storage implementation (main limitation)
- ❌ Real-time only; no historical data persistence

#### Current Status
Most functional for immediate use (real-time data collection and visualization) but lacks persistence for historical data. The implementation has prioritized a working web interface over storage capabilities.

### 5. 📁 final_conndev: Integrated Approach Attempt

Another one-shot attempt at having `claude code` write mostly working firmware.

## Technical Challenges

### 1. Flash Storage Access
- MicroPython succeeds at accessing the full 16MB flash but has WiFi issues
- Arduino approaches struggle with reliable flash access despite sophisticated implementations

### 2. WiFi Reliability
- Fails consistently in MicroPython implementation
- An approach that worked for a short period was some combo of micropython firmware with specific wifi nina firmware version, but alas I didn't document the exact version combo that made micropython's wifi work.

### 3. I2C Device Detection
- Another thing that didn't work for the micropython setup were I2C devices.

## Retro
The wifi / sensors don't work in micropython world, but the 16MB flash does. The wifi and sensors work in arduino core world, but not the flash storage. Oh this RP2040 nano board is fun, and needs some community love. To continue or wrap this project, I'd switch boards or settle for a 

Low hanging fruits that I didn't get to:
- LED / Button workflow:
there are 2 LEDs on board the device, one RGB & another plain orange LED. I have one button on my protoboard
  - button to start / stop data collection --> use RGB LED indicate state
  - long press button to start wifi in AP mode --> use orange color led to blink fast
  - double press button to search known wifi --> blink slow when searching, breath when connected
- RTC 
  - I procured the the RTC, but since I2C was broken in micropython I lost motiviation to get it tested in arduino core
- Enclosure
  - I wanted to make an enclosure that had carabiner hole, so I could easily attach it to my backpack.

# RP2040 Web-Connected Sensor Dashboard

A learning project demonstrating how to use the Raspberry Pi Pico RP2040 (specifically the Arduino Nano RP2040 Connect) to create a web-connected sensor dashboard with LED controls.

## Features
- Real-time sensor data monitoring
- Interactive web dashboard built with React
- LED control interface (RGB LEDs)
- Live data graphing with Chart.js
- WiFi connectivity using WiFiNINA

## Hardware Requirements
- Arduino Nano RP2040 Connect
- Analog sensor (connected to A0)
- Built-in RGB LEDs (already on the board)

## Software Dependencies
As shown in the platformio.ini configuration:
- Base64
- WiFiNINA library
- ArduinoJson
- Base64

## Project Structure
- `/src` - Main source code
  - `main.cpp` - Primary Arduino sketch
  - `web_server.h` - Web server implementation
  - `sensor.h` - Sensor reading functionality
  - `led_control.h` - LED control functions
- `/web` - Web interface files
  - `index.html` - Dashboard HTML
  - `react_app.js` - React frontend application
  - `chart.js` - Chart configuration

## Features Explained

### 1. Sensor Reading
The project continuously reads from an analog sensor connected to pin A0. The sensor data is buffered and can be accessed through the web API.

### 2. LED Control
The onboard RGB LEDs can be controlled through the web interface. Each LED can be toggled independently:
- Red LED
- Green LED
- Blue LED

### 3. Web Dashboard
The dashboard provides:
- Real-time sensor value display
- Interactive LED control buttons
- Live-updating graph of sensor readings
- Responsive design

## Getting Started
1. Clone the repository
2. Create a `secrets.h` file with your WiFi credentials
3. Install PlatformIO in VS Code
4. Connect your Arduino Nano RP2040
5. Build and upload the project
6. Open Serial Monitor to get the device's IP address
7. Access the dashboard by visiting the IP address in your web browser

## How It Works
1. The device connects to WiFi on startup
2. A web server is initialized to serve the dashboard
3. The main loop:
   - Reads sensor data
   - Updates LED states based on web requests
   - Serves the web interface
   - Provides JSON API endpoints for real-time data

## API Endpoints
- `/` - Serves the main dashboard
- `/sensor` - Returns current sensor data and LED states
- `/[R|G|B][H|L]` - Controls LEDs (e.g., `/RH` turns red LED on)

## Development
The project uses PlatformIO for development. The web interface is built using React and Chart.js, with the HTML/JS files being converted to C++ strings during compilation for storage in program memory.

## Learning Objectives
This project demonstrates:
- RP2040 programming with Arduino framework
- Web server implementation on microcontrollers
- Real-time sensor monitoring
- Frontend development with React
- Hardware control through web interfaces
- WiFi networking on embedded devices

## License
MIT License

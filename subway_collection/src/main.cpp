#include <SPI.h>
#include <WiFiNINA.h>
#include "web_server.h"
#include "sensor.h"
#include "secrets.h"
#include "web_files.h"
#include "littlefs_storage.h"

WiFiServer server(80);
LittleFSStorage flashStorage;

// Define the data flush interval (in milliseconds)
const unsigned long DATA_FLUSH_INTERVAL = 60000; // 1 minute
unsigned long lastFlushTime = 0;

void printFileInfo(const char* name, const uint8_t* content, size_t length) {
    Serial.print("\nFile info for ");
    Serial.println(name);
    Serial.print("Content length: ");
    Serial.println(length);
    Serial.print("First 32 bytes: ");
    for(int i = 0; i < 32 && i < length; i++) {
        if(content[i] >= 32 && content[i] <= 126) {
            Serial.print((char)content[i]);
        } else {
            Serial.print("[");
            Serial.print(content[i], HEX);
            Serial.print("]");
        }
    }
    Serial.println();
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // Wait for serial port to connect
    delay(10);
  }

  Serial.println("\n=== IMU Data Collection Starting ===");
  
  // Initialize flash storage
  if (!flashStorage.begin()) {
    Serial.println("Failed to initialize flash storage! Check your configuration.");
    // Proceed even if storage fails - we can still collect and transmit data
  }
  else {
    // List any existing data files
    flashStorage.listDataFiles();
  }
    
  // Print stored file information
  printFileInfo("index.html", index_html, index_html_len);
  printFileInfo("react_app.js", react_app_js, react_app_js_len);
  printFileInfo("chart.js", chart_js, chart_js_len);
    
  // Print MAC address
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC Address: ");
  for (int i = 0; i < 6; i++) {
      if (mac[i] < 0x10) Serial.print("0");
      Serial.print(mac[i], HEX);
      if (i < 5) Serial.print(":");
  }
  Serial.println();

  // Initialize the IMU sensor
  if (!setupSensor()) {
    Serial.println("Failed to setup sensor! Check wiring.");
    while(1); // Stop if sensor setup fails
  }

  // Connect to WiFi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    delay(5000);
  }

  Serial.println("Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("Server started");
}

void loop() {
  // Update IMU sensor readings
  updateSensor();

  // Handle any incoming client connections
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }
  
  // Check if it's time to flush data to flash storage
  unsigned long currentTime = millis();
  if (currentTime - lastFlushTime >= DATA_FLUSH_INTERVAL) {
    // Flush sensor data to flash storage
    flashStorage.saveSensorBuffers(
      accelX_buffer, accelY_buffer, accelZ_buffer,
      gyroX_buffer, gyroY_buffer, gyroZ_buffer,
      temperature_buffer, timestamp_buffer,
      BUFFER_SIZE
    );
    
    lastFlushTime = currentTime;
  }
  
  // Add a small delay to prevent overwhelming the IMU
  delay(50);
}

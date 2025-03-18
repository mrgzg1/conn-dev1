#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include "sensor.h"
#include "web_files.h"

// WiFi modes
#define WIFI_MODE_AP 1     // Access Point mode
#define WIFI_MODE_CLIENT 2 // Client mode (connect to existing network)

// WiFi AP configuration
#define AP_SSID "ConnDevSensor"
#define AP_PASS "connecteddevice"

// Forward declarations
void initWiFi();
bool connectToWiFi(const char* ssid, const char* password, int timeout = 10000);
void startWebServer();
void handleWebServer();
void serveFile(WiFiClient& client, const uint8_t* content, size_t length, const char* mime);
void handleAPIRequest(WiFiClient& client, String path);
void serveSensorData(WiFiClient& client, String sensorEndpoint);
void serveSensorHistory(WiFiClient& client, String sensorEndpoint);
void serveStorageStats(WiFiClient& client);
void serveStorageData(WiFiClient& client, int startIndex, int endIndex);

// Global variables
extern int wifiMode;
extern WiFiServer webServer;
extern bool wifiConnected;

// Initialize WiFi in the requested mode
void initWiFi(int mode = WIFI_MODE_AP) {
  wifiMode = mode;
  
  // Check if WiFi module is present
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    return;
  }
  
  String fv = WiFi.firmwareVersion();
  Serial.print("WiFi firmware version: ");
  Serial.println(fv);
  
  // Try to connect to WiFi if in client mode
  if (mode == WIFI_MODE_CLIENT) {
    // Read WiFi credentials from a configuration source
    // For now, hardcoded for demo (replace with secrets.h in real project)
    const char* ssid = "YourWiFiSSID";
    const char* pass = "YourWiFiPassword";
    
    wifiConnected = connectToWiFi(ssid, pass);
    
    if (!wifiConnected) {
      Serial.println("Falling back to AP mode");
      mode = WIFI_MODE_AP;
      wifiMode = WIFI_MODE_AP;
    }
  }
  
  // Start AP mode if requested or if client mode failed
  if (mode == WIFI_MODE_AP) {
    Serial.print("Creating access point: ");
    Serial.println(AP_SSID);
    
    // Create open network
    int status = WiFi.beginAP(AP_SSID, AP_PASS);
    if (status != WL_AP_LISTENING) {
      Serial.println("Failed to create access point");
      return;
    }
    
    // Wait for AP to start
    delay(5000);
    
    // Print AP IP address
    IPAddress ip = WiFi.localIP();
    Serial.print("AP IP address: ");
    Serial.println(ip);
    
    wifiConnected = true;
  }
  
  // Start web server
  startWebServer();
}

// Connect to a WiFi network
bool connectToWiFi(const char* ssid, const char* password, int timeout) {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  int status = WiFi.begin(ssid, password);
  
  // Wait for connection
  unsigned long startTime = millis();
  while (status != WL_CONNECTED) {
    // Check timeout
    if (millis() - startTime > timeout) {
      Serial.println("WiFi connection timed out");
      return false;
    }
    
    Serial.print(".");
    delay(500);
    status = WiFi.status();
    
    // Exit if connection failed
    if (status == WL_CONNECT_FAILED) {
      Serial.println("Connection failed");
      return false;
    }
  }
  
  // Connection successful
  Serial.println();
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());
  
  return true;
}

// Start the web server
void startWebServer() {
  webServer.begin();
  Serial.println("Web server started");
}

// Handle web server client requests
void handleWebServer() {
  // Check if WiFi is connected
  if (!wifiConnected) return;
  
  // Check for new client connections
  WiFiClient client = webServer.available();
  if (client) {
    Serial.println("New client connected");
    
    // HTTP request
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n') {
          break;
        }
      }
    }
    
    // Parse the request
    if (request.length() > 0) {
      Serial.println("Received request: " + request);
      
      // Extract the path
      int startPos = request.indexOf("GET ") + 4;
      int endPos = request.indexOf(" HTTP/");
      if (startPos > 0 && endPos > 0) {
        String path = request.substring(startPos, endPos);
        Serial.println("Path: " + path);
        
        // Handle different paths
        if (path == "/" || path == "/index.html") {
          serveFile(client, index_html, index_html_len, "text/html");
        }
        else if (path == "/react_app.js") {
          serveFile(client, react_app_js, react_app_js_len, "application/javascript");
        }
        else if (path == "/chart.js") {
          serveFile(client, chart_js, chart_js_len, "application/javascript");
        }
        else if (path.startsWith("/api/")) {
          handleAPIRequest(client, path);
        }
        else {
          // Not found
          client.println("HTTP/1.1 404 Not Found");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          client.println("404 Not Found");
        }
      }
    }
    
    // Close connection
    client.stop();
    Serial.println("Client disconnected");
  }
}

// Serve a file to the client
void serveFile(WiFiClient& client, const uint8_t* content, size_t length, const char* mime) {
  Serial.print("Serving file with MIME type: ");
  Serial.println(mime);
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: " + String(mime));
  client.println("Content-Length: " + String(length));
  client.println("Connection: close");
  client.println("Cache-Control: no-cache");
  client.println("Access-Control-Allow-Origin: *");
  client.println();
  
  // Send file content
  client.write(content, length);
}

// Handle API requests
void handleAPIRequest(WiFiClient& client, String path) {
  // List all available sensors
  if (path == "/api/sensors") {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    // Create JSON array of sensors
    JsonDocument doc;
    JsonArray sensorsArray = doc.to<JsonArray>();
    
    // Add each sensor
    for (int i = 0; i < sensorManager.getSensorCount(); i++) {
      Sensor* sensor = sensorManager.getSensor(i);
      if (sensor) {
        JsonObject sensorObj = sensorsArray.add<JsonObject>();
        sensorObj["name"] = sensor->getName();
        sensorObj["endpoint"] = sensor->getApiEndpoint();
        sensorObj["id"] = sensor->getSensorId();
      }
    }
    
    // Send JSON response
    serializeJson(doc, client);
  }
  // Get current data for a specific sensor
  else if (path.startsWith("/api/") && path.endsWith("/data")) {
    // Extract sensor endpoint
    String sensorEndpoint = path.substring(5, path.length() - 5);
    serveSensorData(client, sensorEndpoint);
  }
  // Get history data for a specific sensor
  else if (path.startsWith("/api/") && path.endsWith("/history")) {
    // Extract sensor endpoint
    String sensorEndpoint = path.substring(5, path.length() - 8);
    serveSensorHistory(client, sensorEndpoint);
  }
  // Get storage statistics
  else if (path == "/api/storage/stats") {
    serveStorageStats(client);
  }
  // Get storage data (range)
  else if (path.startsWith("/api/storage/data")) {
    // Parse parameters for range
    int startIndex = 0;
    int endIndex = 100; // Default limit
    
    // Extract query parameters if present
    int queryPos = path.indexOf("?");
    if (queryPos > 0) {
      String query = path.substring(queryPos + 1);
      
      // Parse start parameter
      int startPos = query.indexOf("start=");
      if (startPos >= 0) {
        int valuePos = startPos + 6;
        int endPos = query.indexOf("&", valuePos);
        if (endPos < 0) endPos = query.length();
        String startStr = query.substring(valuePos, endPos);
        startIndex = startStr.toInt();
      }
      
      // Parse end parameter
      int endPos = query.indexOf("end=");
      if (endPos >= 0) {
        int valuePos = endPos + 4;
        int ampPos = query.indexOf("&", valuePos);
        if (ampPos < 0) ampPos = query.length();
        String endStr = query.substring(valuePos, ampPos);
        endIndex = endStr.toInt();
      }
    }
    
    serveStorageData(client, startIndex, endIndex);
  }
  else {
    // API endpoint not found
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    client.println("{\"error\":\"API endpoint not found\"}");
  }
}

// Serve data for a specific sensor
void serveSensorData(WiFiClient& client, String sensorEndpoint) {
  Serial.print("Serving data for sensor: ");
  Serial.println(sensorEndpoint);
  
  // Find the sensor
  Sensor* sensor = nullptr;
  for (int i = 0; i < sensorManager.getSensorCount(); i++) {
    Sensor* s = sensorManager.getSensor(i);
    if (s && s->getApiEndpoint() == sensorEndpoint) {
      sensor = s;
      break;
    }
  }
  
  // Send response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println("Access-Control-Allow-Origin: *");
  client.println();
  
  // Create JSON document
  JsonDocument doc;
  JsonObject json = doc.to<JsonObject>();
  
  if (sensor) {
    // Get data from specific sensor
    sensor->serializeCurrentData(json);
  } else {
    // Fallback to all sensors
    sensorManager.serializeAllCurrentData(json);
  }
  
  // Send JSON response
  serializeJson(doc, client);
}

// Serve history data for a specific sensor
void serveSensorHistory(WiFiClient& client, String sensorEndpoint) {
  Serial.print("Serving history for sensor: ");
  Serial.println(sensorEndpoint);
  
  // Find the sensor
  Sensor* sensor = nullptr;
  for (int i = 0; i < sensorManager.getSensorCount(); i++) {
    Sensor* s = sensorManager.getSensor(i);
    if (s && s->getApiEndpoint() == sensorEndpoint) {
      sensor = s;
      break;
    }
  }
  
  // Send response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println("Access-Control-Allow-Origin: *");
  client.println();
  
  // Create JSON document
  DynamicJsonDocument doc(8192);
  JsonArray array = doc.to<JsonArray>();
  
  if (sensor) {
    // Get history from specific sensor
    sensor->serializeHistoryData(array);
  } else {
    // Fallback to all sensors
    sensorManager.serializeAllHistoryData(array);
  }
  
  // Send JSON response
  serializeJson(doc, client);
}

// Serve storage statistics
void serveStorageStats(WiFiClient& client) {
  uint32_t totalSectors, usedSectors, freeSectors;
  uint32_t itemsBuffered, bufferCapacity;
  
  storage_get_info(&totalSectors, &usedSectors, &freeSectors);
  storage_get_buffer_info(&itemsBuffered, &bufferCapacity);
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println("Access-Control-Allow-Origin: *");
  client.println();
  
  // Create JSON document
  DynamicJsonDocument doc(4096);
  doc["readings"] = storage_get_reading_count();
  doc["bootCount"] = storage_get_boot_count();
  doc["sectors"] = JsonObject();
  doc["sectors"]["total"] = totalSectors;
  doc["sectors"]["used"] = usedSectors;
  doc["sectors"]["free"] = freeSectors;
  doc["buffer"] = JsonObject();
  doc["buffer"]["items"] = itemsBuffered;
  doc["buffer"]["capacity"] = bufferCapacity;
  
  // Get time range if available
  uint32_t startTime, endTime;
  if (storage_get_time_range(&startTime, &endTime)) {
    doc["timeRange"] = JsonObject();
    doc["timeRange"]["start"] = startTime;
    doc["timeRange"]["end"] = endTime;
  }
  
  // Send JSON response
  serializeJson(doc, client);
}

// Serve storage data (range)
void serveStorageData(WiFiClient& client, int startIndex, int endIndex) {
  // Check if range is valid
  uint32_t totalReadings = storage_get_reading_count();
  if (startIndex < 0) startIndex = 0;
  if (endIndex < 0 || endIndex >= totalReadings) endIndex = totalReadings - 1;
  if (startIndex > endIndex) startIndex = endIndex;
  
  // Limit the range to a reasonable size
  if (endIndex - startIndex > 200) endIndex = startIndex + 200;
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println("Access-Control-Allow-Origin: *");
  client.println();
  
  // Create JSON document
  DynamicJsonDocument doc(8192);
  JsonArray array = doc.to<JsonArray>();
  
  // Read and add each datapoint
  for (int i = startIndex; i <= endIndex; i++) {
    SensorDataPoint reading;
    uint32_t type;
    
    if (storage_get_reading(i, reading, &type)) {
      JsonObject entry = array.createNestedObject();
      entry["index"] = i;
      entry["timestamp"] = reading.timestamp;
      entry["type"] = type;
      entry["sensorId"] = reading.sensorId;
      entry["label"] = reading.label;
      
      // Add values
      JsonArray values = entry["values"].to<JsonArray>();
      for (int j = 0; j < reading.valueCount; j++) {
        values.add(reading.values[j]);
      }
    }
  }
  
  // Add metadata
  JsonObject meta = doc["meta"].to<JsonObject>();
  meta["startIndex"] = startIndex;
  meta["endIndex"] = endIndex;
  meta["totalReadings"] = totalReadings;
  
  // Send JSON response
  serializeJson(doc, client);
}

#endif // WEB_SERVER_H
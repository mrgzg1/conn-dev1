#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFiNINA.h>
#include "sensor.h"
#include "led_control.h"
#include "web_files.h"

// Forward declarations
void serveIMUData(WiFiClient &client);
void serveIMUHistory(WiFiClient &client);
void serveSensorData(WiFiClient &client, String sensorEndpoint);
void serveSensorHistory(WiFiClient &client, String sensorEndpoint);
void serveCompressedFile(WiFiClient &client, const uint8_t *content, size_t length, const char *mime);

void serveCompressedFile(WiFiClient &client, const uint8_t *content, size_t length, const char *mime) {
    Serial.print("\nServing compressed file with mime type: ");
    Serial.println(mime);
    Serial.print("Content length: ");
    Serial.println(length);
    
    // Send headers
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: " + String(mime));
    client.println("Content-Encoding: gzip");
    client.println("Cache-Control: no-cache");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    // Send compressed content directly
    client.write(content, length);
}

void handleClient(WiFiClient &client) {
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

    Serial.println("\n==== Web Request ====");
    Serial.println("Received: " + request);

    String path = request.substring(request.indexOf("GET ") + 4);
    path = path.substring(0, path.indexOf(" "));
    
    Serial.print("Processing path: ");
    Serial.println(path);

    if (path == "/" || path == "/index.html") {
        serveCompressedFile(client, index_html, index_html_len, "text/html");
    }
    else if (path == "/react_app.js") {
        serveCompressedFile(client, react_app_js, react_app_js_len, "application/javascript");
    }
    else if (path == "/chart.js") {
        serveCompressedFile(client, chart_js, chart_js_len, "application/javascript");
    }
    else if (path == "/api/sensors") {
        Serial.println("Serving sensors list");
        
        // Return list of all available sensors
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Access-Control-Allow-Origin: *");
        client.println();
        
        // Create JSON document for sensors list
        JsonDocument doc;
        JsonArray sensorsArray = doc.to<JsonArray>();
        
        for (int i = 0; i < NUM_AVAILABLE_SENSORS; i++) {
            const SensorConfig& config = AVAILABLE_SENSORS[i];
            if (config.enabled) {
                JsonObject sensor = sensorsArray.add<JsonObject>();
                sensor["name"] = config.name;
                sensor["endpoint"] = config.apiEndpoint;
                sensor["type"] = config.type;
                
                Serial.print("Added sensor to list: ");
                Serial.println(config.name);
            }
        }
        
        // Debug output
        Serial.print("Sensors list JSON size: ");
        Serial.println(measureJson(doc));
        Serial.print("Number of sensors: ");
        Serial.println(sensorsArray.size());
        
        serializeJson(doc, client);
        client.println();
        
        Serial.println("Sensors list sent successfully");
    }
    else if (path.startsWith("/api/") && path.endsWith("/data")) {
        // Get sensor endpoint from path
        String sensorEndpoint = path.substring(5, path.length() - 5);
        
        // Legacy endpoint support
        if (path == "/api/imu/data" || path == "/imu_data") {
            sensorEndpoint = "imu";
        }
        
        Serial.print("Sensor data requested for: ");
        Serial.println(sensorEndpoint);
        
        serveSensorData(client, sensorEndpoint);
    }
    else if (path.startsWith("/api/") && path.endsWith("/history")) {
        // Get sensor endpoint from path
        String sensorEndpoint = path.substring(5, path.length() - 8);
        
        // Legacy endpoint support
        if (path == "/api/imu/history" || path == "/imu_history") {
            sensorEndpoint = "imu";
        }
        
        Serial.print("Sensor history requested for: ");
        Serial.println(sensorEndpoint);
        
        serveSensorHistory(client, sensorEndpoint);
    }
    // Handle LED control requests
    else if (path.startsWith("/PWMR") || path.startsWith("/PWMG") || path.startsWith("/PWMB")) {
        String color = path.substring(4, 5); // Get R, G, or B
        int value = path.substring(5).toInt(); // Get the PWM value
        setPWM(color, value);
        
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Access-Control-Allow-Origin: *");
        client.println();
        client.println("OK");
    }
    // Legacy support for simple on/off
    else if (path.startsWith("/R") || path.startsWith("/G") || path.startsWith("/B")) {
        toggleLED(path);
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Access-Control-Allow-Origin: *");
        client.println();
        client.println("OK");
    }

    client.stop();
}

void serveIMUData(WiFiClient &client) {
    // Legacy function - calls serveSensorData with "imu" endpoint
    serveSensorData(client, "imu");
}

void serveIMUHistory(WiFiClient &client) {
    // Legacy function - calls serveSensorHistory with "imu" endpoint
    serveSensorHistory(client, "imu");
}

// Serve data for a specific sensor by endpoint name
void serveSensorData(WiFiClient &client, String sensorEndpoint) {
    Serial.print("Serving data for sensor: ");
    Serial.println(sensorEndpoint);
    
    // Find the sensor with matching endpoint
    int sensorIndex = -1;
    for (int i = 0; i < NUM_AVAILABLE_SENSORS; i++) {
        if (sensorEndpoint == AVAILABLE_SENSORS[i].apiEndpoint && AVAILABLE_SENSORS[i].enabled) {
            sensorIndex = i;
            break;
        }
    }
    
    if (sensorIndex == -1) {
        // Sensor not found
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: application/json");
        client.println("Access-Control-Allow-Origin: *");
        client.println();
        client.println("{\"error\":\"Sensor not found\"}");
        return;
    }
    
    // Create a JSON document
    JsonDocument doc;
    
    // Let the specific sensor populate it with current data
    JsonObject json = doc.to<JsonObject>();
    
    // Find the specific sensor in our sensor manager
    Sensor* sensor = nullptr;
    for (int i = 0; i < sensorManager.getSensorCount(); i++) {
        if (sensorManager.getSensor(i) && 
            AVAILABLE_SENSORS[sensorIndex].type == AVAILABLE_SENSORS[i].type) {
            sensor = sensorManager.getSensor(i);
            break;
        }
    }
    
    if (sensor) {
        // Get data from specific sensor
        sensor->serializeCurrentData(json);
    } else {
        // Fallback to all sensors if specific one not found
        sensorManager.serializeAllCurrentData(json);
    }
    
    // Debug output
    Serial.print("JSON data size: ");
    Serial.println(measureJson(doc));
    
    // Send HTTP headers
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    // Serialize JSON directly to client
    serializeJson(doc, client);
    client.println();
    
    Serial.println("Sensor data sent successfully");
}

// Serve history data for a specific sensor by endpoint name
void serveSensorHistory(WiFiClient &client, String sensorEndpoint) {
    Serial.print("Serving history for sensor: ");
    Serial.println(sensorEndpoint);
    
    // Find the sensor with matching endpoint
    int sensorIndex = -1;
    for (int i = 0; i < NUM_AVAILABLE_SENSORS; i++) {
        if (sensorEndpoint == AVAILABLE_SENSORS[i].apiEndpoint && AVAILABLE_SENSORS[i].enabled) {
            sensorIndex = i;
            break;
        }
    }
    
    if (sensorIndex == -1) {
        // Sensor not found
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: application/json");
        client.println("Access-Control-Allow-Origin: *");
        client.println();
        client.println("{\"error\":\"Sensor not found\"}");
        return;
    }
    
    // Create a JSON document for history data
    JsonDocument doc;
    
    // Create JSON array
    JsonArray array = doc.to<JsonArray>();
    
    // Find the specific sensor in our sensor manager
    Sensor* sensor = nullptr;
    for (int i = 0; i < sensorManager.getSensorCount(); i++) {
        if (sensorManager.getSensor(i) && 
            AVAILABLE_SENSORS[sensorIndex].type == AVAILABLE_SENSORS[i].type) {
            sensor = sensorManager.getSensor(i);
            break;
        }
    }
    
    if (sensor) {
        // Get history from specific sensor
        sensor->serializeHistoryData(array);
    } else {
        // Fallback to all sensors if specific one not found
        sensorManager.serializeAllHistoryData(array);
    }
    
    // Debug output
    Serial.print("History JSON data size: ");
    Serial.println(measureJson(doc));
    Serial.print("Number of entries: ");
    Serial.println(array.size());
    
    // Send HTTP headers
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    // Serialize JSON directly to client
    serializeJson(doc, client);
    client.println();
    
    Serial.println("History data sent successfully");
}

#endif

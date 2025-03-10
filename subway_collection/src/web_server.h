#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFiNINA.h>
#include "sensor.h"
#include "led_control.h"
#include "web_files.h"

// Forward declarations
void serveIMUData(WiFiClient &client);
void serveIMUHistory(WiFiClient &client);
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

    Serial.println("Received request: " + request);

    String path = request.substring(request.indexOf("GET ") + 4);
    path = path.substring(0, path.indexOf(" "));

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
        // Return list of all available sensors
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Access-Control-Allow-Origin: *");
        client.println();
        
        // Create JSON document for sensors list
        StaticJsonDocument<1024> doc;
        JsonArray sensorsArray = doc.to<JsonArray>();
        
        for (int i = 0; i < NUM_AVAILABLE_SENSORS; i++) {
            const SensorConfig& config = AVAILABLE_SENSORS[i];
            if (config.enabled) {
                JsonObject sensor = sensorsArray.createNestedObject();
                sensor["name"] = config.name;
                sensor["endpoint"] = config.apiEndpoint;
                sensor["type"] = config.type;
            }
        }
        
        serializeJson(doc, client);
        client.println();
    }
    else if (path == "/api/imu/data" || path == "/imu_data") {
        // Legacy endpoint support
        serveIMUData(client);
    }
    else if (path == "/api/imu/history" || path == "/imu_history") {
        // Legacy endpoint support
        serveIMUHistory(client);
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
    // Create a JSON document
    StaticJsonDocument<256> doc;
    
    // Let the sensor manager populate it with current data
    JsonObject json = doc.to<JsonObject>();
    sensorManager.serializeAllCurrentData(json);
    
    // Send HTTP headers
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    // Serialize JSON directly to client
    serializeJson(doc, client);
    client.println();
}

void serveIMUHistory(WiFiClient &client) {
    // Create a JSON document for history data
    // Size calculated based on buffer entries and estimated size per entry
    const size_t capacity = JSON_ARRAY_SIZE(SENSOR_BUFFER_SIZE) + 
                           SENSOR_BUFFER_SIZE * JSON_OBJECT_SIZE(4) + 
                           SENSOR_BUFFER_SIZE * 2 * JSON_OBJECT_SIZE(3);
    DynamicJsonDocument doc(capacity);
    
    // Create JSON array
    JsonArray array = doc.to<JsonArray>();
    
    // Let the sensor manager populate it with history data
    sensorManager.serializeAllHistoryData(array);
    
    // Send HTTP headers
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    // Serialize JSON directly to client
    serializeJson(doc, client);
    client.println();
}

#endif

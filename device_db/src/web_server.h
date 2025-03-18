#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include "sensor.h"
#include "led_control.h"
#include "web_files.h"
#include "../include/storage_api.h"

// Forward declarations
void serveSensorData(WiFiClient &client);
void serveStoredData(WiFiClient &client);
void serveLatestReading(WiFiClient &client);
void serveStorageSummary(WiFiClient &client);
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
    else if (path == "/sensor") {
        serveSensorData(client);
    }
    // New storage API endpoints
    else if (path == "/api/history") {
        serveStoredData(client);
    }
    else if (path == "/api/latest") {
        serveLatestReading(client);
    }
    else if (path == "/api/summary") {
        serveStorageSummary(client);
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

void serveSensorData(WiFiClient &client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    int sensorValue = analogRead(A0);
    client.print("{\"reading\":");
    client.print(sensorValue);
    client.print(",\"leds\":{\"red\":");
    client.print(redPWM);
    client.print(",\"green\":");
    client.print(greenPWM); 
    client.print(",\"blue\":");
    client.print(bluePWM);
    client.print("},");
    client.print("\"temperature\":");
    client.print(getSensorValue("temperature"));
    client.print(",\"humidity\":");
    client.print(getSensorValue("humidity"));
    client.print(",\"pressure\":");
    client.print(getSensorValue("pressure"));
    client.print(",\"accel\":{\"x\":");
    client.print(getSensorValue("accel_x"));
    client.print(",\"y\":");
    client.print(getSensorValue("accel_y"));
    client.print(",\"z\":");
    client.print(getSensorValue("accel_z"));
    client.println("}}");
}

// Serve a history of stored readings
void serveStoredData(WiFiClient &client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    uint32_t count = storage_get_reading_count();
    
    // Start JSON array
    client.println("{\"count\":" + String(count) + ",\"readings\":[");
    
    // Get up to the last 20 readings (to avoid memory issues)
    uint32_t startIndex = (count > 20) ? (count - 20) : 0;
    uint32_t endIndex = count;
    
    for (uint32_t i = startIndex; i < endIndex; i++) {
        SensorReading reading;
        char key[32];
        snprintf(key, sizeof(key), "reading-%u", i);
        
        if (storage_get_reading(key, reading)) {
            // Create JSON object
            client.print("{\"index\":");
            client.print(i);
            client.print(",\"timestamp\":");
            client.print(reading.timestamp);
            client.print(",\"temperature\":");
            client.print(reading.temperature);
            client.print(",\"humidity\":");
            client.print(reading.humidity);
            client.print(",\"pressure\":");
            client.print(reading.pressure);
            client.print(",\"accel\":{\"x\":");
            client.print(reading.accel_x);
            client.print(",\"y\":");
            client.print(reading.accel_y);
            client.print(",\"z\":");
            client.print(reading.accel_z);
            client.print("},\"label\":\"");
            client.print(reading.label);
            client.print("\"}");
            
            // Add comma if not the last element
            if (i < endIndex - 1) {
                client.println(",");
            } else {
                client.println();
            }
        }
    }
    
    // End JSON array
    client.println("]}");
}

// Serve the latest stored reading
void serveLatestReading(WiFiClient &client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    SensorReading reading;
    bool success = storage_get_latest_reading(reading);
    
    if (success) {
        // Create JSON object
        client.print("{\"success\":true,");
        client.print("\"timestamp\":");
        client.print(reading.timestamp);
        client.print(",\"temperature\":");
        client.print(reading.temperature);
        client.print(",\"humidity\":");
        client.print(reading.humidity);
        client.print(",\"pressure\":");
        client.print(reading.pressure);
        client.print(",\"accel\":{\"x\":");
        client.print(reading.accel_x);
        client.print(",\"y\":");
        client.print(reading.accel_y);
        client.print(",\"z\":");
        client.print(reading.accel_z);
        client.print("},\"label\":\"");
        client.print(reading.label);
        client.println("\"}");
    } else {
        client.println("{\"success\":false,\"message\":\"No reading available\"}");
    }
}

// Serve a summary of storage data
void serveStorageSummary(WiFiClient &client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    uint32_t count = storage_get_reading_count();
    
    // Get first and last reading if available
    SensorReading firstReading;
    SensorReading lastReading;
    bool hasFirst = false;
    bool hasLast = false;
    
    if (count > 0) {
        char firstKey[32];
        char lastKey[32];
        snprintf(firstKey, sizeof(firstKey), "reading-0");
        snprintf(lastKey, sizeof(lastKey), "reading-%u", count - 1);
        
        hasFirst = storage_get_reading(firstKey, firstReading);
        hasLast = storage_get_reading(lastKey, lastReading);
    }
    
    // Create JSON object
    client.print("{\"count\":");
    client.print(count);
    
    if (hasFirst) {
        client.print(",\"first\":{\"timestamp\":");
        client.print(firstReading.timestamp);
        client.print(",\"temperature\":");
        client.print(firstReading.temperature);
        client.print("}");
    }
    
    if (hasLast) {
        client.print(",\"last\":{\"timestamp\":");
        client.print(lastReading.timestamp);
        client.print(",\"temperature\":");
        client.print(lastReading.temperature);
        client.print("}");
    }
    
    client.println("}");
}

#endif

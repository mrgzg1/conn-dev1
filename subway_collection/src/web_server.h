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
    else if (path == "/imu_data") {
        serveIMUData(client);
    }
    else if (path == "/imu_history") {
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
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    client.print("{\"timestamp\":");
    client.print(lastReadTime);
    client.print(",\"accel\":{\"x\":");
    client.print(accelX);
    client.print(",\"y\":");
    client.print(accelY);
    client.print(",\"z\":");
    client.print(accelZ);
    client.print("},\"gyro\":{\"x\":");
    client.print(gyroX);
    client.print(",\"y\":");
    client.print(gyroY);
    client.print(",\"z\":");
    client.print(gyroZ);
    client.print("},\"temperature\":");
    client.print(temperature);
    client.println("}");
}

void serveIMUHistory(WiFiClient &client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    // Start JSON array
    client.println("[");
    
    // Current position in the circular buffer
    int currentPos = bufferIndex;
    
    // Iterate through the buffer and output each entry
    for (int i = 0; i < BUFFER_SIZE; i++) {
        // Calculate the actual index, accounting for the circular nature
        int idx = (currentPos - BUFFER_SIZE + i) % BUFFER_SIZE;
        if (idx < 0) idx += BUFFER_SIZE;
        
        // Only output entries with valid timestamps
        if (timestamp_buffer[idx] > 0) {
            // If not the first entry, add a comma
            if (i > 0) client.println(",");
            
            client.print("{\"timestamp\":");
            client.print(timestamp_buffer[idx]);
            client.print(",\"accel\":{\"x\":");
            client.print(accelX_buffer[idx]);
            client.print(",\"y\":");
            client.print(accelY_buffer[idx]);
            client.print(",\"z\":");
            client.print(accelZ_buffer[idx]);
            client.print("},\"gyro\":{\"x\":");
            client.print(gyroX_buffer[idx]);
            client.print(",\"y\":");
            client.print(gyroY_buffer[idx]);
            client.print(",\"z\":");
            client.print(gyroZ_buffer[idx]);
            client.print("},\"temperature\":");
            client.print(temperature_buffer[idx]);
            client.print("}");
        }
    }
    
    // End JSON array
    client.println("]");
}

#endif

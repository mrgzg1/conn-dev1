#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFiNINA.h>
#include "sensor.h"
#include "led_control.h"
#include "web_files.h"
#include "littlefs_storage.h"

// Forward declarations
void serveIMUData(WiFiClient &client);
void serveIMUHistory(WiFiClient &client);
void serveCompressedFile(WiFiClient &client, const uint8_t *content, size_t length, const char *mime);

// Forward declarations for new flash storage API endpoints
void serveStorageList(WiFiClient &client, LittleFSStorage &storage);
void serveStorageData(WiFiClient &client, LittleFSStorage &storage, String filename);

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
    // New API endpoint for flash storage file list
    else if (path == "/storage/list") {
        // Create an instance of LittleFSStorage
        extern LittleFSStorage flashStorage;
        serveStorageList(client, flashStorage);
    }
    // New API endpoint for retrieving flash storage data
    else if (path.startsWith("/storage/data/")) {
        String filename = path.substring(14); // Strip "/storage/data/"
        extern LittleFSStorage flashStorage;
        serveStorageData(client, flashStorage, filename);
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

void serveStorageList(WiFiClient &client, LittleFSStorage &storage) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    // Start JSON array for files
    client.println("[");
    
    // Use a simpler approach - just try to open each potential file
    char knownPrefix[] = MBED_LITTLEFS_FILE_PREFIX "/sensor_data_";
    
    bool firstFile = true;
    
    for (int i = 0; i < 10; i++) {  // Try the first 10 possible data files
        char filename[64];
        sprintf(filename, "%s%d.dat", knownPrefix, i);
        
        FILE* file = fopen(filename, "r");
        if (file) {
            // File exists, get its size
            fseek(file, 0, SEEK_END);
            long size = ftell(file);
            
            // Try to read the count header to determine number of records
            fseek(file, 0, SEEK_SET);
            int count = 0;
            fread(&count, sizeof(count), 1, file);
            
            fclose(file);
            
            // Add comma if not the first entry
            if (!firstFile) {
                client.println(",");
            }
            firstFile = false;
            
            // Get the short filename without the path
            char* shortFilename = strrchr(filename, '/');
            if (shortFilename) {
                shortFilename++; // Skip the slash
            } else {
                shortFilename = filename; // Use the full name if no slash found
            }
            
            // Output file info as JSON
            client.print("{\"filename\":\"");
            client.print(shortFilename);
            client.print("\",\"size\":");
            client.print(size);
            client.print(",\"records\":");
            client.print(count);
            client.print("}");
        }
    }
    
    // End JSON array
    client.println("]");
}

void serveStorageData(WiFiClient &client, LittleFSStorage &storage, String filename) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    
    // Verify filename for security (should only contain alphanumeric and underscore)
    bool validFilename = true;
    for (unsigned int i = 0; i < filename.length(); i++) {
        char c = filename.charAt(i);
        if (!(isalnum(c) || c == '_' || c == '.')) {
            validFilename = false;
            break;
        }
    }
    
    if (!validFilename) {
        client.println("{\"error\":\"Invalid filename\"}");
        return;
    }
    
    // Allocate buffer for data points
    const int MAX_POINTS = 100; // Limit to 100 data points to avoid memory issues
    SensorDataPoint dataBuffer[MAX_POINTS];
    int pointsRead = 0;
    
    // Try to read the file
    if (!storage.readSensorData(filename.c_str(), dataBuffer, MAX_POINTS, &pointsRead)) {
        client.println("{\"error\":\"Failed to read file\"}");
        return;
    }
    
    // Start JSON response
    client.println("{");
    client.print("\"filename\":\"");
    client.print(filename);
    client.println("\",");
    client.print("\"points\":");
    client.print(pointsRead);
    client.println(",");
    client.println("\"data\":[");
    
    // Output each data point
    for (int i = 0; i < pointsRead; i++) {
        if (i > 0) {
            client.println(",");
        }
        
        client.print("{\"timestamp\":");
        client.print(dataBuffer[i].timestamp);
        client.print(",\"accel\":{\"x\":");
        client.print(dataBuffer[i].accelX);
        client.print(",\"y\":");
        client.print(dataBuffer[i].accelY);
        client.print(",\"z\":");
        client.print(dataBuffer[i].accelZ);
        client.print("},\"gyro\":{\"x\":");
        client.print(dataBuffer[i].gyroX);
        client.print(",\"y\":");
        client.print(dataBuffer[i].gyroY);
        client.print(",\"z\":");
        client.print(dataBuffer[i].gyroZ);
        client.print("},\"temperature\":");
        client.print(dataBuffer[i].temperature);
        client.print("}");
    }
    
    // End JSON array and object
    client.println("]");
    client.println("}");
}

#endif

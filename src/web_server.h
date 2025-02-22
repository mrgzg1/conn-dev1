#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFiNINA.h>
#include "sensor.h"
#include "led_control.h"
#include "web_files.h"

// Forward declarations
void serveSensorData(WiFiClient &client);
void serveFile(WiFiClient &client, const char *content, const char *mime);

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
    serveFile(client, index_html, "text/html");
  }
  else if (path == "/react_app.js") {
    serveFile(client, react_app_js, "application/javascript");
  }
  else if (path == "/chart.js") {
    serveFile(client, chart_js, "application/javascript");
  }
  else if (path == "/sensor") {
    serveSensorData(client);
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
  client.println("}}");
}

void serveFile(WiFiClient &client, const char *content, const char *mime) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: " + String(mime));
  client.println("Cache-Control: no-cache");
  client.println("Access-Control-Allow-Origin: *");
  client.println();
  client.print(content);
}

#endif

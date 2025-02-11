#include <SPI.h>
#include <WiFiNINA.h>
#include "web_server.h"
#include "sensor.h"
#include "led_control.h"
#include "secrets.h"


WiFiServer server(80);

void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  Serial.println("WiFi Web Server");
  
  
  // Print MAC address
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC Address: ");
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i < 5) {
      Serial.print(":");
    }
  }
  Serial.println();




  setupSensor(); // Initialize sensor
  setupLEDs();   // Initialize LED pins

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    delay(5000);
  }

  Serial.println("Connected!");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  updateSensor(); // Read sensor data

  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }
}

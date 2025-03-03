#include <Arduino.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h" // Include the secrets header file with all credentials

// Initialize WiFi client
WiFiClient wifiClient;
// Initialize MQTT client
PubSubClient mqttClient(wifiClient);

// Function prototypes
void connectToWiFi();
void connectToMQTT();
void publishSensorData();

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("\n=== MQTT Client Starting ===");
  
  // Set MQTT server and port
  mqttClient.setServer(mqtt_server, mqtt_port);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Connect to MQTT broker
  connectToMQTT();
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  
  // Check MQTT connection
  if (!mqttClient.connected()) {
    connectToMQTT();
  }
  
  // Keep MQTT connection alive
  mqttClient.loop();
  
  // Publish sensor data
  publishSensorData();
  
  // Wait before sending next message
  delay(5000);
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
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
}

void connectToMQTT() {
  Serial.print("Connecting to MQTT broker at ");
  Serial.print(mqtt_server);
  Serial.print(":");
  Serial.println(mqtt_port);
  
  // Loop until we're connected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect with username and password
    if (mqttClient.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void publishSensorData() {
  // Collect sensor data (replace with your actual sensor code)
  float temperature = 25.5;
  float humidity = 60.2;
  
  // Create JSON document
  JsonDocument doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["timestamp"] = millis();
  
  // Serialize JSON to string
  char msgBuffer[256];
  serializeJson(doc, msgBuffer);
  
  // Publish message
  Serial.print("Publishing message: ");
  Serial.println(msgBuffer);
  
  if (mqttClient.publish(mqtt_topic, msgBuffer)) {
    Serial.println("Publish successful");
  } else {
    Serial.println("Publish failed");
  }
}

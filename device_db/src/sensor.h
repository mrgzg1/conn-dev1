#ifndef SENSOR_H
#define SENSOR_H

const int SENSOR_PIN = A0;
const int BUFFER_SIZE = 100;
int sensorBuffer[BUFFER_SIZE];
int bufferIndex = 0;

void setupSensor() {
  pinMode(SENSOR_PIN, INPUT);
  for (int i = 0; i < BUFFER_SIZE; i++) sensorBuffer[i] = 0;
}

void updateSensor() {
  sensorBuffer[bufferIndex] = analogRead(SENSOR_PIN);
  bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
}

#endif

#ifndef LED_CONTROL_H
#define LED_CONTROL_H


// Track LED states
bool redLED = false;
bool greenLED = false;
bool blueLED = false;


void setupLEDs() {
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  digitalWrite(LEDR, LOW);
  digitalWrite(LEDG, LOW);
  digitalWrite(LEDB, LOW);
  redLED = false;
  greenLED = false; 
  blueLED = false;
}

void toggleLED(String command) {
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
  // LEDs are active-LOW on the Arduino Nano RP2040 Connect
  if (command == "/RH") {
    digitalWrite(LEDR, HIGH);  // Turn ON (active-LOW)
    redLED = true;
  }
  if (command == "/RL") {
    digitalWrite(LEDR, LOW); // Turn OFF (active-LOW)
    redLED = false;
  }
  if (command == "/GH") {
    digitalWrite(LEDG, HIGH);  // Turn ON (active-LOW)
    greenLED = true;
  }
  if (command == "/GL") {
    digitalWrite(LEDG, LOW); // Turn OFF (active-LOW)
    greenLED = false;
  }
  if (command == "/BH") {
    digitalWrite(LEDB, HIGH);  // Turn ON (active-LOW)
    blueLED = true;
  }
  if (command == "/BL") {
    digitalWrite(LEDB, LOW); // Turn OFF (active-LOW)
    blueLED = false;
  }
}

#endif

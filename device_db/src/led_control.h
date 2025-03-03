#ifndef LED_CONTROL_H
#define LED_CONTROL_H

// Track LED states (0-100 for PWM)
int redPWM = 0;
int greenPWM = 0;
int bluePWM = 0;

// Define LED pins
const int RED_PIN = 10;   // D10
const int GREEN_PIN = 11; // D11 
const int BLUE_PIN = 12;  // D12

void setupLEDs() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 0);
  analogWrite(BLUE_PIN, 0);
  redPWM = 0;
  greenPWM = 0;
  bluePWM = 0;
}

// Set PWM value (0-100) for an LED
void setPWM(String color, int value) {
  // Constrain value between 0-100
  value = value < 0 ? 0 : (value > 100 ? 100 : value);
  
  // Map 0-100 to 0-255 for analogWrite
  int pwmValue = map(value, 0, 100, 0, 255);
  
  if (color == "R") {
    analogWrite(RED_PIN, pwmValue);
    redPWM = value;
  }
  else if (color == "G") {
    analogWrite(GREEN_PIN, pwmValue);
    greenPWM = value;
  }
  else if (color == "B") {
    analogWrite(BLUE_PIN, pwmValue);
    bluePWM = value;
  }
}

// Legacy support for simple on/off
void toggleLED(String command) {
  if (command == "/RH") setPWM("R", 100);
  if (command == "/RL") setPWM("R", 0);
  if (command == "/GH") setPWM("G", 100);
  if (command == "/GL") setPWM("G", 0);
  if (command == "/BH") setPWM("B", 100);
  if (command == "/BL") setPWM("B", 0);
}

#endif

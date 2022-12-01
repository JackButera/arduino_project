#include "keypad.h"
#include <Arduino.h>

int readButtons(){
  int buttonIN = 0;
  buttonIN = analogRead(ANALOG_PIN);
  if (buttonIN < 50)   return 1;
  if (buttonIN < 195)  return 2;
  if (buttonIN < 380)  return 3;
  if (buttonIN < 555)  return 4;
  if (buttonIN < 790)  return 5;
  return 0;
}


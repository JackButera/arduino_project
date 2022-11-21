#include "led_controller.h"
#include <Arduino.h>

const uint8_t redPin = 12; //red pin led
const uint8_t greenPin = 11; //green pin led

//sets the color of the led
void setColor(char color){
    //Serial.println("testing");
    if (color == 'R'){
        analogWrite(redPin, 255);
        analogWrite(greenPin, 0);
    }
    else if (color == 'G'){
        analogWrite(greenPin, 255);
        analogWrite(redPin, 0);
    }
    else{
        analogWrite(greenPin, 0);
        analogWrite(redPin, 0);
    }
    
    
}

void led_setup(){
    pinMode(13,OUTPUT); 
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
}


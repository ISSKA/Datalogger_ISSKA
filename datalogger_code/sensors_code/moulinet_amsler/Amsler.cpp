#include <Arduino.h>
#include "Amsler.h"
const int currentMeterPin = 25;  // Pin connected to the current meter
volatile int pulseCount;
unsigned long debounceDelay = 20;
unsigned long length_of_measurement = 60000;
unsigned long startTime = 0;
unsigned long currentTime = 0;
volatile bool check = true;
float v;
float Amsler::measure_vel() {
    // Set up the current meter pin as an input with internal pull-up
    pinMode(currentMeterPin, INPUT_PULLUP);
    //attachInterrupt(digitalPinToInterrupt(currentMeterPin), countPulse, CHANGE);
    startTime = millis();
    do {
        currentTime = millis();
        int digitalValue = digitalRead(currentMeterPin);
        if (digitalValue == LOW && check == true) {
            pulseCount++;
            check = false;
        }
        else if (digitalValue == HIGH && check == false) {
            check = true;
        }
        delay(debounceDelay);
    }
    while (currentTime-startTime < length_of_measurement);
    float n = 10.0 * pulseCount / (float)(length_of_measurement / 1000);
    if (n <= 4.857) {
        v = 0.008 + 0.3062*n;
    }
    else if (n > 4.857) {
        v = -0.009 + 0.3097*n;
    }
    pulseCount = 0;
    return v;
}

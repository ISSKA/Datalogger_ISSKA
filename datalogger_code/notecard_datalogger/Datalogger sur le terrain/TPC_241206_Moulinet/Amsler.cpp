#include "Amsler.h"

Amsler::Amsler(int pin, unsigned long debounce, unsigned long duration)
    : currentMeterPin(pin),
      debounceDelay(debounce),
      measurementDuration(duration),
      pulseCount(0),
      lastPulseTime(0),
      isHighState(false),
      velocity(0),
      pulsesPerSecond(0) {}

float Amsler::measure_vel() {
    pinMode(currentMeterPin, INPUT_PULLUP);
    pulseCount = 0;
    isHighState = false;
    lastPulseTime = 0;

    unsigned long startTime = millis();

    while (millis() - startTime < measurementDuration) {
        int currentState = digitalRead(currentMeterPin);

        bool pulseDetected = (currentState == LOW && isHighState);
        if (pulseDetected && (millis() - lastPulseTime > debounceDelay)) {
            pulseCount++;
            lastPulseTime = millis();
        }

        isHighState = (currentState == HIGH);

        delayMicroseconds(200);
    }

    pulsesPerSecond = 10.0 * pulseCount / (measurementDuration / 1000.0);

    if (pulsesPerSecond <= 4.857) {
        velocity = 0.008 + 0.3062 * pulsesPerSecond;
    } else {
        velocity = -0.009 + 0.3097 * pulsesPerSecond;
    }

    if (velocity < 0) {
        Serial.println("Alerte: vitesse négative détectée. Vérifiez le capteur.");
        velocity = 0;
    }

    return velocity;
}

void Amsler::debug_output() {
    Serial.print("Impulsions: ");
    Serial.print(pulseCount);
    Serial.print(" | Pulses/sec: ");
    Serial.print(pulsesPerSecond);
    Serial.print(" | Vitesse: ");
    Serial.println(velocity);
}


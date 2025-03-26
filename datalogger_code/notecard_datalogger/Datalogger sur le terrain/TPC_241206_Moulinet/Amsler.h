#ifndef AMSLER_H
#define AMSLER_H

#include <Arduino.h>

class Amsler {
public:
    Amsler(int pin, unsigned long debounce = 1000, unsigned long duration = 30000);

    /** Mesure la vitesse en détectant les impulsions sur la pin. */
    float measure_vel();

    /** Affiche les données internes pour debug (impulsions, vitesse, etc). */
    void debug_output();

private:
    int currentMeterPin;
    volatile int pulseCount;
    unsigned long debounceDelay;
    unsigned long measurementDuration;
    unsigned long lastPulseTime;
    volatile bool isHighState;
    float velocity;
    float pulsesPerSecond;
};

#endif
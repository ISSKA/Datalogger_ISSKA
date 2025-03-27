#include <SoftwareSerial.h> 

#define rx 32                                          //define what pin rx is going to be
#define tx 33

#define MOSFET_SENSORS_PIN 14

SoftwareSerial RS485Serial(rx, tx);  

void setup() {
  Serial.begin (115200); //*****  make sure serial monitor baud matches *****
  pinMode(MOSFET_SENSORS_PIN,OUTPUT);
  digitalWrite(MOSFET_SENSORS_PIN, HIGH);
  delay(100);
  RS485Serial.begin(9600);
  delay(1000);
} 

void loop() {
}
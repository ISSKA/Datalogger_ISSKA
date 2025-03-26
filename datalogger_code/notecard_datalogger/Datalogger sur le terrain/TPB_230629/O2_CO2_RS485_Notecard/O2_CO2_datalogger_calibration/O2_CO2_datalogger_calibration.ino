

#include <Wire.h>
#include <Ezo_uart.h> 
#include <SoftwareSerial.h> 
#include "SHT31.h"

#define MOSFET_SENSORS_PIN 14
#define EXT_SENSORS_MOSFET 26 //allow current to flow in external sensor's box
#define SELECT_RXTX 27 //low selects P1, high selects P2
#define SHT35_sensor_ADRESS 0x44 //values set on the PCB hardware (not possible to change)

#define RX 32
#define TX 33


SoftwareSerial myserial(RX,TX);
Ezo_uart O2sensor(myserial, "O2");
SHT31 sht;

void setup() {
  Serial.begin (115200); //*****  make sure serial monitor baud matches *****
  delay(1000);
  pinMode(MOSFET_SENSORS_PIN,OUTPUT);
  digitalWrite(MOSFET_SENSORS_PIN, HIGH);
  delay(1000);
  Wire.begin();
  Wire.setClock(50000); // set I2C clock frequency to 50kHz which is rather slow but more reliable //change to lower value if cables get long

}  // end of setup

void loop() {
  //turn on external sensors
  pinMode(EXT_SENSORS_MOSFET,OUTPUT);
  digitalWrite(EXT_SENSORS_MOSFET,HIGH);

  //select sensor 1 (O2)
  pinMode(SELECT_RXTX,OUTPUT);
  digitalWrite(SELECT_RXTX,LOW);
  delay(100);

  tcaselect(1);
  delay(10);
  sht.begin(SHT35_sensor_ADRESS);    //Sensor I2C Address
  delay(10);
  sht.read();
  float temp_calib=sht.getTemperature();
  Serial.print("temp calib: ");
  Serial.println(temp_calib);
  delay(50);

  //measure O2 value
  myserial.begin(2400);
  
  Serial.println("Show the O2 values 4 times:");
  
  for(int i = 0; i< 4; i++){
    O2sensor.send_read_with_temp_comp(temp_calib);
    delay(1000);
    //get the response data and put it into the [Device].reading variable if successful
    String response = String(O2sensor.get_reading(), 2).c_str();  //print either the reading or an error message
    Serial.print("O2: ");
    Serial.print(response);
    Serial.println("%");
    delay(2000);
  }
  
  
  
  Serial.println("do you want to change the calibration of the O2 sensor? (y/n): ");

  bool calibrate = false;
  while (Serial.available() == 0) {}
  String cal_choice = Serial.readString();
  delay(100);
  if (cal_choice=="y") calibrate = true;
  
  if (calibrate){
    Serial.println("to which percentage should the sensor be right now? (e.g 20.95) ");
    while (Serial.available() == 0) {}
    float cal_value = Serial.parseFloat();
    delay(100);

    String calib_command = "Cal," + String(cal_value,2);
    Serial.print("Command sent: ");
    Serial.println(calib_command);

    for(int i = 0; i < 5; i ++){
      O2sensor.send_read_with_temp_comp(temp_calib);
      delay(500);
      delay(500);
      Serial.print("O2 warm up: ");
      Serial.println(String(O2sensor.get_reading(), 3).c_str());
    }
    delay(500);

    O2sensor.send_cmd_no_resp(calib_command.c_str());
    delay(5000);
    Serial.println("calibrated!");
  }

  O2sensor.send_read_with_temp_comp(temp_calib);
  delay(1000);

  String response = String(O2sensor.get_reading(), 2).c_str();  //print either the reading or an error message
  Serial.print("O2: ");
  Serial.print(response);
  Serial.println("%");
  delay(2000);
  
}

void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(0x73);
  Wire.write(1 << i);
  Wire.endTransmission();
}
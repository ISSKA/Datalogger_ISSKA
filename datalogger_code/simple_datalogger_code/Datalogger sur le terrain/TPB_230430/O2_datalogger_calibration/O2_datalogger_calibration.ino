

#include <Wire.h>
#include <Ezo_i2c.h> 
#include <SparkFun_TMP117.h> 
#define MOSFET_SENSORS_PIN 14
#define I2C_MUX_ADDRESS 0x70
#define I2C_ATLAS_O2_ADRESS 108 

Ezo_board O2sensor = Ezo_board(I2C_ATLAS_O2_ADRESS); 
TMP117 temp_sensor; // Initalize sensor

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

  tcaselect(6);
  delay(100);
  temp_sensor.begin();
  delay(100);
  float tempC = temp_sensor.readTempC();
  Serial.print("temp_TMP117: ");
  Serial.print(tempC);
  Serial.println("C");
  

  
  Serial.println("Show the O2 values 4 times:");
  
  for(int i = 0; i< 4; i++){
    O2sensor.send_read_with_temp_comp(tempC);
    delay(1000);

    O2sensor.receive_read_cmd();              //get the response data and put it into the [Device].reading variable if successful
    String response = String(O2sensor.get_last_received_reading(), 2).c_str();  //print either the reading or an error message
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
      O2sensor.send_read_with_temp_comp(tempC);
      delay(500);
      O2sensor.receive_read_cmd();              //get the response data and put it into the [Device].reading variable if successful
      delay(500);
      Serial.print("O2 warm up: ");
      Serial.println(String(O2sensor.get_last_received_reading(), 3).c_str());
    }
    delay(500);

    O2sensor.send_cmd(calib_command.c_str());
    delay(2000);
    Serial.println("calibrated!");
  }

  O2sensor.send_read_with_temp_comp(tempC);
  delay(1000);

  O2sensor.receive_read_cmd();              //get the response data and put it into the [Device].reading variable if successful
  String response = String(O2sensor.get_last_received_reading(), 2).c_str();  //print either the reading or an error message
  Serial.print("O2: ");
  Serial.print(response);
  Serial.println("%");
  delay(2000);
  
}

// multiplex bus selection for the first multiplexer 
void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}


/******************************************************************************
   Arduino - Low Power Board
   ISSKA www.isska.ch
   August 2022 - MS8607, CO2, O2
******************************************************************************/

#include <Wire.h>

#include "sensors.h"

//Library for the MS8607 PHT sensor
#include <SparkFun_PHT_MS8607_Arduino_Library.h>
//Library for the O2 sensor
#include <Ezo_uart.h>


Ezo_uart O2sensor(UART_O2, "O");
Ezo_uart CO2sensor(UART_CO2, "CO2");

// A structure to store all sensors values
typedef struct {

  float temp1;
  float press1;
  float humidity1;
  float O2;
  float CO2;
  float SensorVbat;
} Mesures;




Mesures mesures;

// Initialize sensors
MS8607 barometricSensor;



Sensors::Sensors() {}

/**
   Initialize sensors
*/



/**
   Return the file header for the sensors part
   This should be something like "<sensor 1 name>;<sensor 2 name>;"
*/
String Sensors::getFileHeader () {
  return "temperature [degC];pressure [mbar];humidity [%];O2 [%];CO2 [ppm];Vbatsensor[V]";
}

/**
   Return sensors data formated to be write to the CSV file
   This should be something like "<sensor 1 value>;<sensor 2 value>;"
*/
String Sensors::getFileData () {
  char str[50];

  sprintf(str, "%s;%s;%s;%s;%s;%s",
          String(mesures.temp1, 3).c_str(), String(mesures.press1, 3).c_str(), String(mesures.humidity1, 3).c_str(), String(mesures.O2, 3).c_str(), String(mesures.CO2, 3).c_str(), String(mesures.SensorVbat, 3).c_str());

  return str;
}

/**
   Return sensor data to print on the OLED display
*/
String Sensors::getDisplayData (uint8_t idx) {
  char str[50];

  switch (idx) {
    // Sensor 1 data

    case 0:
      sprintf(str, "Temp: %s", String(mesures.temp1).c_str());
      break;
    case 1:
      sprintf(str, "Patm: %s", String(mesures.press1).c_str());
      break;
    case 2:
      sprintf(str, "HumB: %s", String(mesures.humidity1).c_str());
      break;
    case 3:
      sprintf(str, "O2: %s", String(mesures.O2).c_str());
      break;
    case 4:
      sprintf(str, "CO2: %s", String(mesures.CO2).c_str());
      break;
    case 5:
      sprintf(str, "vbat2: %s", String(mesures.SensorVbat).c_str());
      break;
    default:
      return "";
  }

  return str;
}

/**
   Display sensor mesures to Serial for debug purposes
*/
void Sensors::serialPrint() {
  // First sensor
  Serial.print(F("Temp: "));
  Serial.println(mesures.temp1);

  Serial.print(F("Patm: "));
  Serial.println(mesures.press1);

  Serial.print(F("Humi: "));
  Serial.println(mesures.humidity1);

  Serial.print(F("O2: "));
  Serial.println(mesures.O2);

  Serial.print(F("CO2: "));
  Serial.println(mesures.CO2);

  Serial.print(F("Vbat2: "));
  Serial.println(mesures.SensorVbat);


}

void Sensors::mesure() {
  Serial.println("Entering mesure function !");

  //MS8607 measurement (PHT)
  tcaselect(4);
  Serial.println("tcaselect made!");

  if (!barometricSensor.begin())
  {
    Serial.println("I2C communication with MS8607 failed !");
  }
  delay(100);
  mesures.temp1 = barometricSensor.getTemperature();
  mesures.press1 = barometricSensor.getPressure();
  mesures.humidity1 = barometricSensor.getHumidity();
  Serial.println("PHT measurement made !");
  const uint8_t bufferlen = 32;                         //total buffer size for the response_data array
  char response_data[bufferlen]; 
  int timeOutCounter=0;
  //Power on the sensors side of the datalogger
  digitalWrite(MOSFET_SENSORS_PIN, HIGH);
  delay(1000);
  // Select channel 1 of serial port expander
  pinMode(SERIAL_PORT_EXPANDER_SELECTOR, OUTPUT);
  digitalWrite(SERIAL_PORT_EXPANDER_SELECTOR, LOW);
  delay(100);
  //O2 measurement
  //O2 sensor is set at baudrate 2400!!
  UART_O2.begin(2400, SERIAL_8N1, RX_485, TX_485);
  delay(200);
  O2sensor.flush_rx_buffer();
  delay(200);
  while(mesures.O2==0 && timeOutCounter<20)
  {
    O2sensor.send_read_with_temp_comp(mesures.temp1);
    //O2sensor.send_read();
    delay(100);
    mesures.O2 = O2sensor.get_reading();
    delay(200);
    O2sensor.send_cmd("Status", response_data, bufferlen);
    Serial.println(response_data);
    timeOutCounter++;
  }
  
  //flush the serial port to erase all incoming data
  //O2sensor.flush_rx_buffer();
  UART_O2.end();
  delay(200);

  //Select channel 2 of serial port expander
  digitalWrite(SERIAL_PORT_EXPANDER_SELECTOR, HIGH);
  //UART_CO2.begin(9600,SERIAL_8N1, RX_485, TX_485);
  //CO2 sensor is set at baudrate 2400!!
  UART_CO2.begin(2400, SERIAL_8N1, RX_485, TX_485);
                       //character array to hold the response data from modules
  delay(11000);
  //CO2sensor.send_cmd_no_resp("C,0");
  //CO2sensor.flush_rx_buffer();
  //Get the battery voltage of the sensors side
  /*CO2sensor.send_cmd("Status",response_data,bufferlen);
    Serial.print("Status: ");
    Serial.println(response_data);//Repsonse of status is warm if it's still warming or ?Status,P,3.055
    while(strcmp(response_data, "*WARM") == 0)
    {
    delay(200);
    CO2sensor.send_cmd("Status", response_data, bufferlen);
    Serial.println(response_data);

    }*/
  /*
    String sensorVoltage="";
    int charCounter=0;


    while(response_data[charCounter]!=',')
    {
    sensorVoltage+=response_data[charCounter];
    charCounter++;
    }
    sensorVoltage.remove(0);
    mesures.SensorVbat = sensorVoltage.toFloat();
  */
  Serial.println("Measure of CO2");
  CO2sensor.flush_rx_buffer();
  delay(200);
  timeOutCounter=0;
  while (mesures.CO2 == 0 && timeOutCounter<20)
  {
    delay(300);
    CO2sensor.send_read();
    delay(100);
    //Lecture des valeurs de CO2 avec un shift de -550 ppm déduit 
    //après comparaison avec autres capteurs et CO2 atmos.
    mesures.CO2 = CO2sensor.get_reading()-550;
    O2sensor.send_cmd("Status", response_data, bufferlen);
    Serial.println(response_data);
    timeOutCounter++;
  }
  timeOutCounter=0;
  while ((mesures.SensorVbat == 0 || mesures.SensorVbat > 10)&&timeOutCounter<20)
  {
    CO2sensor.send_cmd("Status", response_data, bufferlen);
    //find the last comma in the array
    char * pch;
    pch = strchr(response_data, ',');
    int index_coma = 0;
    while (pch != NULL)
    {
      Serial.print("found at %d\n");
      Serial.println(pch - response_data + 1);
      index_coma = pch - response_data + 1;
      pch = strchr(pch + 1, ',');
    }
    String tempString = response_data;
    String sensorVoltage = tempString.substring(index_coma);
    mesures.SensorVbat = sensorVoltage.toFloat();
    //CO2sensor.flush_rx_buffer();
    delay(300);
    timeOutCounter++;
  }

  delay(200);
  UART_CO2.end();
  digitalWrite(MOSFET_SENSORS_PIN, LOW);
}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

//This function returns all the measured values by the sensors as float numbers
void Sensors::getMeasuredValues(float arr[][Nb_values_sensors + 2], int RowSelected )
{
  arr[RowSelected][2] = mesures.temp1;
  arr[RowSelected][3] = mesures.press1;
  arr[RowSelected][4] = mesures.humidity1;
  arr[RowSelected][5] = mesures.O2;
  arr[RowSelected][6] = mesures.CO2;
  arr[RowSelected][7] = mesures.SensorVbat;
}

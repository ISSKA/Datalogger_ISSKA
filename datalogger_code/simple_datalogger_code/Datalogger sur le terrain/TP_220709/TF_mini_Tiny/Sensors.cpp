/******************************************************************************
   Arduino - Low Power Board
   ISSKA www.isska.ch
   March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"

//#include "SDP3x.h"
#include <SparkFun_PHT_MS8607_Arduino_Library.h>

#define TX_lidar 25
#define RX_lidar 26
#define TX_sonar 27
#define RX_sonar 15

// A structure to store all sensors values
typedef struct {
  float temp1;
  float press1;
  float humidityBox;
  unsigned int distance;
  int16_t distance2;
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
  return "temperature;pressure;humidity;distanceLidar_cm;distanceSonar_mm;";
}

/**
   Return sensors data formated to be write to the CSV file
   This should be something like "<sensor 1 value>;<sensor 2 value>;"
*/
String Sensors::getFileData () {
  char str[50];

  sprintf(str, "%s;%s;%s;%s;%s;",
          String(mesures.temp1, 3).c_str(), String(mesures.press1, 3).c_str(), String(mesures.humidityBox).c_str(), String(mesures.distance).c_str(), String(mesures.distance2).c_str());

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
      sprintf(str, "HumBox: %s", String(mesures.humidityBox).c_str());
      break;
    case 3:
      sprintf(str, "Dist: %s", String(mesures.distance).c_str());
      break;
    case 4:
      sprintf(str, "Dist: %s", String(mesures.distance2).c_str());
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

  Serial.print(F("Hum: "));
  Serial.println(mesures.distance);

  Serial.print(F("Dist: "));
  Serial.println(mesures.distance);

  Serial.print(F("Dist: "));
  Serial.println(mesures.distance2);
}

void Sensors::mesure() {

  tcaselect(0);
  Wire.begin();
  barometricSensor.begin();
  delay(200);
  mesures.temp1 = barometricSensor.getTemperature();
  mesures.press1 = barometricSensor.getPressure();
  mesures.humidityBox = barometricSensor.getHumidity();

  // Serial connected to LIDAR sensor
  Serial2.begin( 115200, SERIAL_8N1, TX_lidar, RX_lidar );

  mesures.distance = readLIDAR( 2000 );
  if ( mesures.distance > 0 ) {
    Serial.printf( "Distance (cm): %d\n", mesures.distance );
  }
  else {
    Serial.println( "Timeout reading LIDAR" );
  }

  //Read distance measured by the sonar sensor
  bool error = readSonar();
  if (error)
  {
    Serial.println("Sonar reading error !");
  }


}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

unsigned int Sensors::readLIDAR( long timeout ) {

  unsigned char readBuffer[ 9 ];

  long t0 = millis();

  while ( Serial2.available() < 9 ) {

    if ( millis() - t0 > timeout ) {
      // Timeout
      return 0;
    }

    delay( 10 );
  }

  for ( int i = 0; i < 9; i++ ) {
    readBuffer[ i ] = Serial2.read();
  }

  while ( ! detectFrame( readBuffer ) ) {

    if ( millis() - t0 > timeout ) {
      // Timeout
      return 0;
    }

    while ( Serial2.available() == 0 ) {
      delay( 10 );
    }

    for ( int i = 0; i < 8; i++ ) {
      readBuffer[ i ] = readBuffer[ i + 1 ];
    }

    readBuffer[ 8 ] = Serial2.read();

  }

  // Distance is in bytes 2 and 3 of the 9 byte frame.
  unsigned int distance = ( (unsigned int)( readBuffer[ 2 ] ) ) |
                          ( ( (unsigned int)( readBuffer[ 3 ] ) ) << 8 );

  return distance;

}
bool Sensors::detectFrame( unsigned char *readBuffer ) {

  return  readBuffer[ 0 ] == 0x59 &&
          readBuffer[ 1 ] == 0x59 &&
          (unsigned char)(
            0x59 +
            0x59 +
            readBuffer[ 2 ] +
            readBuffer[ 3 ] +
            readBuffer[ 4 ] +
            readBuffer[ 5 ] +
            readBuffer[ 6 ] +
            readBuffer[ 7 ]
          ) == readBuffer[ 8 ];
}
bool Sensors::readSonar()
{
  int16_t distance_mm = -1;  // The last measured distance
  bool newData = false; // Whether new data is available from the sensor
  uint8_t buffer[4];  // our buffer for storing data
  uint8_t idx = 0;  // our idx into the storage buffer
  Serial1.begin(9600, SERIAL_8N1, TX_sonar, RX_sonar);
  while (!newData)
  {
    if (Serial1.available())
    {
      uint8_t c = Serial1.read();
      //Serial.println(c, HEX);


      // See if this is a header byte
      if (idx == 0 && c == 0xFF) {
        //Serial.print("buffer[0]="); Serial.println(c);
        buffer[idx++] = c;
      }
      // Two middle bytes can be anything
      else if ((idx == 1) || (idx == 2)) {
        if (idx == 1)
        {
          //Serial.print("buffer[1]="); Serial.println(c);
        }
        else if (idx == 2)
        {
          //Serial.print("buffer[2]="); Serial.println(c);
        }
        buffer[idx++] = c;
      }
      else if (idx == 3) {
        uint8_t sum = 0;
        sum = buffer[0] + buffer[1] + buffer[2];
        if (sum == c) {
          //<< 8 multiplies le buffer[1] (Data_H) par 256 (2^8)
          distance_mm = ((uint16_t)buffer[1] << 8) | buffer[2];
          //distancev2 = ((uint16_t)buffer[1] << 8) + buffer[2];
          newData = true;
        }
        idx = 0;
        //Serial.print("buffer[3]="); Serial.println(c);
        //Serial.print("sum of buffer ="); Serial.println(sum);
        //Serial.print("sum of buffer (binary)="); Serial.println(sum, BIN);
      }
    }
  }
  Serial.print("Distance: ");
  Serial.print(distance_mm);
  Serial.println(" mm");

  mesures.distance2 = distance_mm;

  if (distance_mm != -1)
  {
    return false;
  }
  else
  {
    return true;
  }

}

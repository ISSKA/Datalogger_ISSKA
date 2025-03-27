/******************************************************************************

******************************************************************************/

#ifndef sensors_h
#define sensors_h

#include "Arduino.h"

#define MAX_MEASUREMENTS 12
#define I2C_MUX_ADDRESS 0x70
#define UART_O2 Serial1
#define UART_CO2 Serial2

#define RX_485 25
#define TX_485 26
#define SERIAL_PORT_EXPANDER_SELECTOR 27
#define MOSFET_SENSORS_PIN 15

//Number of values given by the sensors
#define Nb_values_sensors 6

#define NOMBRE_ECHANTILLONS 10 // Nombre d'échantillons récoltés pour calculer la moyenne

class Sensors {
  public: 
    Sensors(); 
    void setup(); 
    String getFileHeader();
    String getFileData();
    String getDisplayData(uint8_t idx);
    void serialPrint();
    void mesure();
    void getMeasuredValues(float arr[][Nb_values_sensors+2],int RowSelected );
    void tcaselect(uint8_t i);
  private:
    
};

#endif

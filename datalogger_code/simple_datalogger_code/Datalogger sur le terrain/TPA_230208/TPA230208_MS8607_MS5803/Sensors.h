/******************************************************************************

******************************************************************************/

#ifndef sensors_h
#define sensors_h

#include "Arduino.h"

#define I2C_MUX_ADDRESS 0x70
#define MAX_MEASUREMENTS 12
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
    void tcaselect(uint8_t i);
  private:
    
};

#endif

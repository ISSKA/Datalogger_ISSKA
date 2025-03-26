#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SD.h>




Adafruit_INA219 ina219;



String log_file = "VOLT.CSV";

void setup(void) 
{
  // Les deux prochaines lignes sont HYPER IMPORTANTES !!!!!
  pinMode(9, OUTPUT);      //pin D9 is the enable line for the Stalker v3.1 switched 3.3/5v power line
  digitalWrite(9, HIGH);   //set this pin high and leave it on for the rest of the sketch
    Serial.begin(9600);

  pinMode(10,OUTPUT);//SD Card power control pin. LOW = On, HIGH = Off
  digitalWrite(10,LOW); //Power On SD Card.

  Serial.print(F("Load SD card..."));

  //     Check if SD card can be intialized.
  if (!SD.begin(10))  //Chipselect is on pin 10
  {
      Serial.println(F("SD Card could not be intialized, or not found"));
      return;
  }
  Serial.println(F("SD Card found and initialized."));
  
  File logFile = SD.open(log_file, FILE_WRITE);

  if(logFile) {
      logFile.println("");
      logFile.println("");
      logFile.println("");
      logFile.println("Columns are: #1: time(s), #2: Bus Voltage(V), #3: Shunt Voltage(mV), #4: Load Voltage(V), #5: Current(mA)");
      logFile.close();
    }
    
  digitalWrite(10,HIGH); //Power Off SD Card
  
  #ifndef ESP8266
    while (!Serial);     // will pause Zero, Leonardo, etc until serial console opens
  #endif
  uint32_t currentFrequency;
    
  //Serial.begin(9600);
  
  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  ina219.begin();
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  ina219.setCalibration_16V_400mA();
  oled.begin();    // Initialize the OLED
  oled.clear(ALL); // Clear the display's internal memory
  oled.display();  // Display what's in the buffer (splashscreen)
  delay(1000);     // Delay 1000 ms
  oled.clear(PAGE); // Clear the buffer.

  Serial.println(F("Columns are: #1: time(ms), #2: Bus Voltage(V), #3: Shunt Voltage(mV), #4: Load Voltage(V), #5: Current(mA)"));
}

void loop(void) 
{
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  loadvoltage = busvoltage + (shuntvoltage / 1000);

  oled.clear(ALL);
  oled.clear(PAGE);
  oled.setFontType(0);  // Set font to type 1     // Clear the page
  oled.setCursor(0, 0); // Set cursor to top-left
  oled.print("Current mA: ");
  oled.setCursor(5, 20);
  oled.setFontType(1);
  oled.print(current_mA);
  oled.display();       // Refresh the display
  delay(600);
  
  Serial.print(millis()); Serial.print(" ");
  Serial.print(busvoltage); Serial.print(" ");
  Serial.print(shuntvoltage); Serial.print(" ");
  Serial.print(loadvoltage); Serial.print(" ");
  Serial.println(current_mA);
  
  delay(50);        

  digitalWrite(10,LOW); //Power On SD Card.

  File logFile = SD.open(log_file, FILE_WRITE);

  if(logFile) {
      logFile.print(millis()/1000.); logFile.print(" ");
      logFile.print(busvoltage); logFile.print(" ");
      logFile.print(shuntvoltage); logFile.print(" ");
      logFile.print(loadvoltage); logFile.print(" ");
      logFile.println(current_mA);
      logFile.close();
    }

    digitalWrite(10,HIGH); //Power Off SD Card.
 }

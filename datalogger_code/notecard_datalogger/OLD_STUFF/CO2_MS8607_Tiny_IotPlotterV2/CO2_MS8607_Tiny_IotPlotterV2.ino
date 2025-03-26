//Code version du 07.02.2022 avec Tstep en seconde

#define SERIAL_DEBUG true
// Allow to display each mesures done to the OLED display (should be disabled for production)
#define ALWAYS_DISPLAY_TO_OLED false
// microSD
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
//#include "WiFi.h"
#include "driver/adc.h"
//#include <esp_wifi.h>
//#include <esp_bt.h>
#include <Notecard.h>
#define myProductID "com.gmail.ouaibeer:datalogger_isska"
#define NOTECARD_I2C_ADDRESS 0x17
//If an antenna is connected
#define SENDING_TIME_MS 180000
//If there is no antenna
//#define SENDING_TIME_MS 360000
Notecard notecard;
//THIS A TEST
//Include libraries for multidimensional arrays
#include <cstring>
#include <CSV_Parser.h>
#include <U8x8lib.h>
// sensors
#include "Sensors.h"
#include <RTClib.h>

#define RTC_PIN 27
#define YEAR(y) y + 1970
#define MOSFET_PIN 14

// Define CS pin for the SD card module
#define SD_CS 5

// SD card
#define DATA_FILENAME "/data.csv"
#define CONFIG_FILENAME "/conf.txt"
#define uS_TO_S_FACTOR 1000000
String dataMessage;
float batvolt;

//Send the measurement each x values measured (defined in sensors.h)
//#define MAX_MEASUREMENTS 3

RTC_DS3231 RTC;
/***************************************************************
   Variables
 ***************************************************************/
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

// DS3231
time_t wakeUpInterval = 10*60;//in sec, here: 10min

boolean firstRun = true;
// sensors
Sensors sensors;

#include <TinyPICO.h>
TinyPICO tp = TinyPICO();

// Save reading number on RTC memory
RTC_DATA_ATTR int readingID = 0;
RTC_DATA_ATTR int measurementSendingThreshold = MAX_MEASUREMENTS;
//Array of arrays in which the last measurements are saved before sending this latter on the server
RTC_DATA_ATTR float LastMeasurements[MAX_MEASUREMENTS][7] = {};
//here 7 is for ID;VBatt;temperature;pressure;humidityBOX;humidity;CO2;
RTC_DATA_ATTR const char* dates[MAX_MEASUREMENTS];//Dates are saved in another array
RTC_DATA_ATTR char datesV3[MAX_MEASUREMENTS][25];
RTC_DATA_ATTR String savedDates[MAX_MEASUREMENTS];
RTC_DATA_ATTR int DatesUNIX[MAX_MEASUREMENTS];
//times is saved in another array [y,m,d,h,m,s][MAX_measurements]
//int timeSaved[MAX_MEASUREMENTS][6]={0};



void setup()
{
  //Start the ADC conversion https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html
  adc_power_acquire();
  Serial.begin(115200);
  //Wire.begin();
  //Wire.setClock(400000); //Communicate at faster 400kHz I2C
  tp.DotStar_SetPower(true);
  pinMode(RTC_PIN, OUTPUT);         // RTC and status LED power via pin D7 (about 3 mA)
  digitalWrite(RTC_PIN, HIGH);      // Switch DS3231 RTC Power and Status LED on - Switched off in ESP12 Deep Sleep mode
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, HIGH);


  // SD optimization
  // https://github.com/EKMallon/UNO-Breadboard-Datalogger/blob/master/_20160110_UnoBasedDataLogger_v1/_20160110_UnoBasedDataLogger_v1.ino
  // Setting the SPI pins high helps some sd cards go into sleep mode
  pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH); // ALWAYS pullup the ChipSelect pin with the SD library
  pinMode(MOSI, OUTPUT); digitalWrite(MOSI, HIGH); // pullup the MOSI pin
  pinMode(MISO, INPUT_PULLUP); // pullup the MISO pin
  delay(10);

  // Wait for system voltage regulation
  tp.DotStar_SetPixelColor( 44, 65, 203 );
  sleep(2);
  if (readingID == 0) {
    firstRun = true;
  }
  else {
    firstRun = false;
  }

  // Initialisation du OLED
  if (firstRun || ALWAYS_DISPLAY_TO_OLED) {
    u8x8.begin();
    u8x8.setFont(u8x8_font_amstrad_cpc_extended_r);
    //u8x8.setContrast(0);
    //u8x8.clear();
    u8x8.setCursor(0, 0);
    u8x8.print(F("Init..."));
  }

  // see if the card is present and can be initialized:
  SD.begin(SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println(F("Card Mount Failed"));
    u8x8.setCursor(0, 2);
    u8x8.print(F("SD Init Error!"));
    tp.DotStar_SetPixelColor( 255, 0, 0 );

    // Enter sleep mode
    sleep(4);
    u8x8.noDisplay();
    deep_sleep_mode(wakeUpInterval);
    // while(1);
  }
  delay(25); //SD.begin hits the power supply pretty hard

  if (firstRun) {
    if (!writeFileHeader()) {
      u8x8.setCursor(0, 2);
      u8x8.print(F("SD Write Error!"));

      // Enter sleep mode
      sleep(4);
      u8x8.noDisplay();
      deep_sleep_mode(wakeUpInterval);
      // while(1);
    }
  }
  // read step config
  int steps = readConfigFile();
  if (steps > 0) {
    wakeUpInterval = steps;
  }

  // sensors init
  //sensors.setup();
  // display to Oled only at the starting
  if (firstRun || ALWAYS_DISPLAY_TO_OLED) {
    u8x8.setCursor(0, 2);
    u8x8.print(F("Success !"));
    tp.DotStar_SetPixelColor( 0, 255, 0 );
    sleep(2);
    tp.DotStar_Clear();
  }
  //we don't need the 32K Pin, so disable it

  if (! RTC.begin()) {
    Serial.println(F("Couldn't find RTC"));
    Serial.flush();
    abort();
  }
  RTC.disable32K();
  batvolt = tp.GetBatteryVoltage();

  // Configure the DS3231 clock to the build time
  //RTC.adjust(DateTime(2021, 12, 20, 12, 58, 0));

  //Get current time
  fetchTime();
  //Sample the sensors
  sensors.mesure();
  //Save the values in JSON array
  Serial.println("Try to put sensors values in an array...");
  //sensors.getMeasuredValues(&LastMeasurements[readingID - (measurementSendingThreshold - MAX_MEASUREMENTS)][0]);
  Serial.print("IDX is: "); Serial.println(readingID % MAX_MEASUREMENTS);
  sensors.getMeasuredValues(LastMeasurements, readingID % MAX_MEASUREMENTS);
  //sensors.getMeasuredValuesV2(LastMeasurements[readingID%MAX_MEASUREMENTS]);
  Serial.println("Done");
  //Save time
  // fetch the time
  DateTime now;
  now = RTC.now();
  char tempBuffer [35] = "";
  sprintf(tempBuffer, "%04d/%02d/%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  savedDates[readingID % MAX_MEASUREMENTS] = String(tempBuffer);
  //dates[readingID%MAX_MEASUREMENTS]=dateTime.c_str();
  /* timeSaved[readingID%MAX_MEASUREMENTS][0]=now.year(); timeSaved[readingID%MAX_MEASUREMENTS][1]=now.month();
    timeSaved[readingID%MAX_MEASUREMENTS][2]=now.day(); timeSaved[readingID%MAX_MEASUREMENTS][3]=now.hour();
    timeSaved[readingID%MAX_MEASUREMENTS][4]=now.minute(); timeSaved[readingID%MAX_MEASUREMENTS][5]=now.second();*/

  DatesUNIX[readingID % MAX_MEASUREMENTS] = now.unixtime();
  Serial.print("Try to put date, Vbat and ID in the array...");
  dates[readingID % MAX_MEASUREMENTS] = tempBuffer;
  strcpy(datesV3[readingID % MAX_MEASUREMENTS], (const char *) tempBuffer);
  LastMeasurements[readingID % MAX_MEASUREMENTS][0] = (float)readingID;
  LastMeasurements[readingID % MAX_MEASUREMENTS][1] = (float)tp.GetBatteryVoltage();
  Serial.println("Done");
  int tempIdx = readingID % MAX_MEASUREMENTS;
  Serial.print("Data saved: "); Serial.print(LastMeasurements[tempIdx][0]);
  Serial.print(" "); Serial.print(dates[tempIdx]); Serial.print(" "); Serial.print(LastMeasurements[tempIdx][1]); Serial.print(" ");
  Serial.print(LastMeasurements[tempIdx][2]); Serial.print(" "); Serial.print(LastMeasurements[tempIdx][3]); Serial.print(" ");
  Serial.print(LastMeasurements[tempIdx][4]); Serial.print(" "); Serial.print(LastMeasurements[tempIdx][5]); Serial.print(" ");
  Serial.println(LastMeasurements[tempIdx][6]);
  //Serial.println(DateTime);
  //Print only the array that stores the dates
  Serial.println("Dates array");
  for (int i = 0; i < MAX_MEASUREMENTS; i++)
  {
    Serial.print("||"); Serial.print(dates[i]); Serial.println("  ");
  }
  Serial.println("Dates array V2");
  for (int i = 0; i < MAX_MEASUREMENTS; i++)
  {
    Serial.print("||"); Serial.print(savedDates[i]); Serial.println("  ");
  }
  Serial.println("Dates array V3");
  for (int i = 0; i < MAX_MEASUREMENTS; i++)
  {
    Serial.print("||"); Serial.print(datesV3[i]); Serial.println("  ");
  }

  logSDCard();

  if (firstRun || ALWAYS_DISPLAY_TO_OLED) {
    displayToOled(batvolt);
    sleep(2);
    u8x8.noDisplay();
  }
  tp.DotStar_SetPower(false);
  readingID ++;
  Serial.println("Start of the final if statement");
  if (readingID == 1)
  {
    deep_sleep_mode(wakeUpInterval - 22); //22 is the time shift for the program execution
  }
  else if (readingID >= measurementSendingThreshold)
  {
    //The SD card is connected, try to display the last line
    //ReadCSVfile();
    Serial.println("Display the saved array");
    //Before sending the values, they are printed in serial monitor
    float tempSum = 0;
    for (int j = 0; j < MAX_MEASUREMENTS; j++)
    {
      tempSum = 0;
      Serial.print(datesV3[j]); Serial.print("  ");
      for (int i = 0; i < 7; i++)
      {
        Serial.print(LastMeasurements[j][i]);
        Serial.print("  ");
        tempSum += LastMeasurements[j][i];
      }
      Serial.print("Sum");
      Serial.println(tempSum);
    }
    Serial.println("Saved array has been succesfully displayed");




    //If MAX_measurements have been done, these data are sent to the NoteHub.
    //Turn on the notecard
    Serial.println("Turn on notecard");
    delay(2000);
    //Connect the notecard to NoteHub
    InitializeNoteHubConnection();
    Serial.println("Connection ...");
    delay(2000);
    //Send the measurements to NoteHub
    Serial.println("Send data");
    NoteHubSendData();
    delay(SENDING_TIME_MS);
    measurementSendingThreshold += MAX_MEASUREMENTS;
    deep_sleep_mode(wakeUpInterval - 14); //2 is the time shift for the program execution
  }
  else
  {
    deep_sleep_mode(wakeUpInterval - 14); //2 is the time shift for the program execution
  }
}


/***************************************************************
   Main loop
 ***************************************************************/

void loop() {
}

/**
   Write a line of (sensors) data to the SD file
*/

void fetchTime()
{
  DateTime now;

  // fetch the time
  now = RTC.now();

  char buffer [25] = "";

  sprintf(buffer, "%04d/%02d/%02d, %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  String dateTime = String(buffer);
  Serial.println(dateTime);
}

void logSDCard() {
  String sensorsData = sensors.getFileData();
  batvolt = tp.GetBatteryVoltage();
  DateTime now;

  // fetch the time
  now = RTC.now();

  char buffer [35] = "";

  sprintf(buffer, "%04d/%02d/%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  String dateTime = String(buffer);
  dataMessage = String(readingID) + ";" + String(dateTime) + ";" + String(batvolt) + ";" + String(sensorsData) + "\r\n";
  Serial.print("Save data: ");
  Serial.println(dataMessage);
  appendFile(SD, "/data.csv", dataMessage.c_str());
}

/**
   Read the config file from the SD
*/
int readConfigFile() {
  File configFile = SD.open(CONFIG_FILENAME);

  // if the file is available, read it
  if (configFile) {
    String steps;

    if (configFile.available()) {
      steps = configFile.readStringUntil(';');
    }

    configFile.close();

    return steps.toInt();
  }

  return 0;
}
/**
   Display (sensors) data to the OLED display
*/
void displayToOled(float batvolt) {
  // write to display
  Serial.println("display to oled");
  u8x8.display(); // Power on the OLED display
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print(F("Batt.: "));
  u8x8.print(batvolt);
  u8x8.print(F(" V"));

  u8x8.setCursor(0, 2);
  u8x8.print(F("SD write: "));
  //sdStatus ? u8x8.print(F("OK")) : u8x8.print(F("Error"));
  sleep(4);
  Serial.println("display sensors data");
  String data = sensors.getDisplayData(0);
  byte idx = 0;

  while (data != "") {
    u8x8.clear();
    u8x8.setCursor(0, 0);
    u8x8.print(data);

    data = sensors.getDisplayData(++idx);
    if (data != "") {
      u8x8.setCursor(0, 2);
      u8x8.print(data);
    }

    data = sensors.getDisplayData(++idx);
    sleep(4);
  }

  if (firstRun) {
    u8x8.clear();
    u8x8.setCursor(0, 0);
    u8x8.print(F("Steps: "));
    u8x8.print(wakeUpInterval);
    u8x8.print(F(" sec."));

    u8x8.setCursor(0, 2);
    u8x8.print(F("Mesures begin !"));
    sleep(4);
  }

  // Power off the OLED display
  u8x8.noDisplay();
}

boolean writeFileHeader() {
  Serial.println("file header write");
  File logFile = SD.open(DATA_FILENAME, FILE_APPEND);
  //logFile.open(DATA_FILENAME, O_WRITE | O_AT_END);
  //logFile.open(DATA_FILENAME, O_WRITE);
  if (!logFile) {
    return false;
  }
  /*if (!logFile) {
    writeFile(SD, "/DATA.csv", "");
    logFile = SD.open("/DATA.csv", FILE_WRITE);
    }*/
  // if the file is available, write to it:
  Serial.println("logFile available");
  String header = sensors.getFileHeader();

  logFile.println();
  logFile.println();
  logFile.println();
  logFile.print(F("ID;DateTime;VBatt;"));
  logFile.print(header);

  logFile.println();
  logFile.close();

  return true;
}

/*boolean writeFileHeader() {
  String header = sensors.getFileHeader();
  appendFile(SD, DATA_FILENAME, "\n\n\nDateTime;VBatt;");
  const char * c = header.c_str();
  appendFile(SD, DATA_FILENAME, c);
  appendFile(SD, DATA_FILENAME, "\n");
  }*/

void deep_sleep_mode(int nbSeconds) {
  //  WiFi.disconnect(true);
  //  WiFi.mode(WIFI_OFF);
  //  btStop();
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW);
  adc_power_release();
  //  esp_wifi_stop();
  //  esp_bt_controller_disable();
  esp_sleep_enable_timer_wakeup(nbSeconds * uS_TO_S_FACTOR);
  esp_deep_sleep_start();

}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

/////////////////////////////////////////////////////////////////////
//------------FUNCTION TO CONNECT AND SEND DATA TO NOTEHUB-----------
/////////////////////////////////////////////////////////////////////

void InitializeNoteHubConnection()
{
  // Initialize the physical I/O channel to the Notecard
  //Notecard I2C is on channel 0 of the I2C multiplexer
  sensors.tcaselect(0);
  Wire.begin();
  notecard.begin();
  delay(100);

  //delay(5000);
  // Service configuration request
  J *req = notecard.newRequest("hub.set");
  // This command (required) causes the data to be delivered to the Project on notehub.io that has claimed
  // this Product ID.  (see above)
  JAddStringToObject(req, "product:", myProductID);
  // This sets the notecard's connectivity mode to continous
  JAddStringToObject(req, "mode", "continuous");
  //It should decrease the number of irrelevant data sent
  //Don't know if it really works
  JAddBoolToObject(req, "unsecure",true);
  notecard.sendRequest(req);
}

void ReadCSVfile()
{
  unsigned long tempPosition;
  int lineCnt = 0;
  //Open the csv file where the measurements are stored
  File myFile = SD.open(DATA_FILENAME, FILE_READ);

  //-----------Put the "cursor" at the begining of the max_measurements last row---------
  //Go to the end of the file (Check the -2)
  myFile.seek(myFile.size() - 2);
  while (lineCnt < MAX_MEASUREMENTS)
  {
    //Go in reverse until \n is found
    while (myFile.peek() != '\n')
    {
      myFile.seek(myFile.position() - 1);
    }
    //save the position of the end of the previous line
    tempPosition = myFile.position();
    myFile.seek(myFile.position() - 1);
    lineCnt++;
  }

  //Print the last max_measurement row in the serial monitor
  Serial.println("Data read on the SD card:");
  // read from the file until there's nothing else in it:
  myFile.seek(myFile.position() + 2);
  while (myFile.available())
  {
    Serial.write(myFile.read());

  }
  Serial.println(); Serial.println("Last line has been displayed");

  //-----------Put the "cursor" at the begining of the max_measurements last row---------
  //Go to the end of the file (Check the -2)
  lineCnt = 0;
  myFile.seek(myFile.size() - 2);
  while (lineCnt < MAX_MEASUREMENTS)
  {
    //Go in reverse until \n is found
    while (myFile.peek() != '\n')
    {
      myFile.seek(myFile.position() - 1);
    }
    //save the position of the end of the previous line
    tempPosition = myFile.position();
    myFile.seek(myFile.position() - 1);
    lineCnt++;
  }
  //could also read character by character instead of read line by line and use string variable type
  //Write the line into an array
  myFile.seek(myFile.position() + 2);
  char recordArray[MAX_MEASUREMENTS][50];
  char aRecord[50];
  byte recordNum = 0;
  byte charNum = 0;
  //come from https://forum.arduino.cc/t/read-sd-card-file-and-put-values-into-variables/644367/8
  Serial.println("Display character by character the csv");
  while (myFile.available())
  {
    char inChar = myFile.read();
    Serial.print(inChar);
    if (inChar == '\n')
    {
      strcpy(recordArray[recordNum], aRecord);
      recordNum++;
      charNum = 0;
    }
    else if (inChar == '\r')
    {
      //do nothing
    }
    else
    {
      aRecord[charNum] = inChar;
      charNum++;
      aRecord[charNum] = '\0';

    }
  }
  myFile.close();

  //Print the array
  Serial.println("-----------ARRAY OF VALUES-------------");
  for (int i = 0; i < MAX_MEASUREMENTS; i++)
  {
    for (int j = 0; j < 50; j++)
    {
      Serial.print(recordArray[i][j]);
    }
    Serial.println();
  }
  Serial.println("The recordArray has been sucessfully saved.");
}

float DecimalRound(float valin, int decimals)
{
  Serial.print("Value before is:");
  Serial.println(valin,6);
  Serial.print("Value after is:");
  
  int scale = pow(10,decimals);
  
  int valtemp = 0.5+valin*scale;
  float valout = valtemp/scale;
  Serial.println(valout,6);
  return valout;
}



void NoteHubSendData()
{
  //The JSON object is sent to IotPlotter platform;
  
  //Create a JSON object

  J *body = JCreateObject();
  if (body == NULL)
  {
    Serial.println("Body creation failed!");
  }
  J* tempData = JCreateObject();
  JAddItemToObject(body,"data",tempData);
  J* Vbatt_J = JCreateArray();
  if (Vbatt_J == NULL)
  {
    Serial.println("Vbat creation failed!");
  }
  JAddItemToObject(tempData, "Voltage", Vbatt_J);
  J* ID_J = JAddArrayToObject(tempData, "ID_Measurement");
  J* Temperature_J = JAddArrayToObject(tempData,"Temperature");
  J* Pressure_J = JAddArrayToObject(tempData, "Pression");
  J* HumidityBox_J = JAddArrayToObject(tempData, "HumidityBox");
  J* Humidity_J = JAddArrayToObject(tempData, "Humidity");
  J* CO2_J = JAddArrayToObject(tempData, "CO2");
  //J* TestString = JAddArrayToObject(tempData,"TestArray");
  J *sample = NULL;
  J* value = NULL;
  J* epoch = NULL;
  
  for (int i = 0; i < MAX_MEASUREMENTS; i++)
  {
    //Battery voltage
    sample = JCreateObject();
    JAddItemToArray(Vbatt_J, sample);
    //Add value of the sample
    //value = JCreateNumber(DecimalRound(LastMeasurements[i][1],2));
    value = JCreateString(String(LastMeasurements[i][1],2).c_str());
    JAddItemToObject(sample, "value", value);
    //Add timeStamp of the sample
    epoch = JCreateNumber(DatesUNIX[i]);
    JAddItemToObject(sample, "epoch", epoch);

    //ID measurement
    sample = JCreateObject();
    JAddItemToArray(ID_J, sample);
    //Add value of the sample
    value = JCreateNumber(LastMeasurements[i][0]);
    JAddItemToObject(sample, "value", value);
    //Add timeStamp of the sample
    epoch = JCreateNumber(DatesUNIX[i]);
    JAddItemToObject(sample, "epoch", epoch);

    //Temperature
    sample = JCreateObject();
    JAddItemToArray(Temperature_J, sample);
    //Add value of the sample
    //value = JCreateNumber(DecimalRound(LastMeasurements[i][2],2));
    value = JCreateString(String(LastMeasurements[i][2],2).c_str());
    JAddItemToObject(sample, "value", value);
    //Add timeStamp of the sample
    epoch = JCreateNumber(DatesUNIX[i]);
    JAddItemToObject(sample, "epoch", epoch);

    //Pressure
    sample = JCreateObject();
    JAddItemToArray(Pressure_J, sample);
    //Add value of the sample
    //value = JCreateNumber(DecimalRound(LastMeasurements[i][3],1));
    value = JCreateString(String(LastMeasurements[i][3],2).c_str());
    JAddItemToObject(sample, "value", value);
    //Add timeStamp of the sample
    epoch = JCreateNumber(DatesUNIX[i]);
    JAddItemToObject(sample, "epoch", epoch);

    //Humidity Box
    sample = JCreateObject();
    JAddItemToArray(HumidityBox_J, sample);
    //Add value of the sample
    //value = JCreateNumber(DecimalRound(LastMeasurements[i][4],1));
    value = JCreateString(String(LastMeasurements[i][4],2).c_str());
    JAddItemToObject(sample, "value", value);
    //Add timeStamp of the sample
    epoch = JCreateNumber(DatesUNIX[i]);
    JAddItemToObject(sample, "epoch", epoch);

    //Humidity
    sample = JCreateObject();
    JAddItemToArray(Humidity_J, sample);
    //Add value of the sample
    //value = JCreateNumber(LastMeasurements[i][5]);
    value = JCreateString(String(LastMeasurements[i][5],2).c_str());
    JAddItemToObject(sample, "value", value);
    //Add timeStamp of the sample
    epoch = JCreateNumber(DatesUNIX[i]);
    JAddItemToObject(sample, "epoch", epoch);

    //CO2
    sample = JCreateObject();
    JAddItemToArray(CO2_J, sample);
    //Add value of the sample
    value = JCreateNumber(LastMeasurements[i][6]);
    JAddItemToObject(sample, "value", value);
    //Add timeStamp of the sample
    epoch = JCreateNumber(DatesUNIX[i]);
    JAddItemToObject(sample, "epoch", epoch);  
  }
  //Print the body object
  char* tempString = JPrint(body);
  Serial.println("Print the body object ...");
  Serial.println(tempString);
  Serial.println("Printed !");

  //Send the object through request
  J *req = notecard.newRequest("note.add");
  JAddItemToObject(req, "body", body);
  JAddBoolToObject(req, "sync", true);
  Serial.println("Try to send request");
  notecard.sendRequest(req);
  Serial.println("Request sent !!!");
}

//Code version du 26.04.2022 avec Tstep en seconde
//Un MOSFET est utilisé pour alimenter la notecard seulement lors de l'envoi
//050422: Add connection after first measurement & Set time of RTC with notecard time after each reset
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
Notecard notecard;
//Include libraries for multidimensional arrays
#include <cstring>
#include <CSV_Parser.h>
#include <U8x8lib.h>
#define SCL_PIN 22
#define SDA_PIN 21
// sensors
#include "Sensors.h"
#include <RTClib.h>

#define RTC_PIN 27
#define YEAR(y) y + 1970
#define MOSFET_PIN 14
#define MOSFET_NOTECARD_PIN 4

#include <string.h>
#include <stdio.h>
//Short filename of the current code running
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

//Define CS pin for the SD card module
#define SD_CS 5

//SD card
#define DATA_FILENAME "/data.csv"
#define CONFIG_FILENAME "/conf.txt"
#define uS_TO_S_FACTOR 1000000
String dataMessage;
float batvolt;


RTC_DS3231 RTC;
/***************************************************************
   Variables
 ***************************************************************/
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

//#define SENDING_TIME_MS 180*1000
// DS3231
time_t wakeUpInterval = 1 * 60; //in sec, here: 10min
unsigned int Sending_time_ms = 180 * 1000;
boolean SetRTC = true;

boolean firstRun = true;
// sensors
Sensors sensors;

#include <TinyPICO.h>
TinyPICO tp = TinyPICO();

// Save reading number on RTC memory
RTC_DATA_ATTR unsigned int readingID = 0;
RTC_DATA_ATTR unsigned int measurementSendingThreshold = MAX_MEASUREMENTS;
//Array of arrays in which the last measurements are saved before sending this latter on the server
RTC_DATA_ATTR float LastMeasurements[MAX_MEASUREMENTS][Nb_values_sensors + 2] = {};
//here 2 is for ID;VBatt;+number of values given by the sensors (defined in sensor.h)
RTC_DATA_ATTR char SavedDates[MAX_MEASUREMENTS][25];
RTC_DATA_ATTR unsigned int DatesUNIX[MAX_MEASUREMENTS];
// each int and float takes 4 bytes, a char = 1 bytes
//Example with MAX_MEASUREMENTS = 24 and Nb_values_sensors = 6
// Bytes: 2*4 + 192*4 + 600*1 + 24 * 4= 1472 bytes = 1,47 KB
// The available memory is around 8 KB and maybe the half 4KB




void setup()
{
  //Start the ADC conversion https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html
  adc_power_acquire();
  Serial.begin(115200);
  //Set the SCL at 50kHz since we can have long cables in some cases
  Wire.begin();
  Wire.setClock(50000);
  tp.DotStar_SetPower(true);
  pinMode(RTC_PIN, OUTPUT);         // RTC and status LED power via pin D7 (about 3 mA)
  digitalWrite(RTC_PIN, HIGH);      // Switch DS3231 RTC Power and Status LED on - Switched off in ESP12 Deep Sleep mode
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, HIGH);
  //Pin to control the gate of MOSFET for notecard
  pinMode(MOSFET_NOTECARD_PIN, OUTPUT);
  digitalWrite(MOSFET_NOTECARD_PIN, LOW);


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
    //Set the SCL at 50kHz since we can have long cables in some cases
    //Otherwise, the OLED set the frequency at 500kHz
    u8x8.setBusClock(50000);
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
  Serial.print("Measurement step [s]:");
  Serial.println(wakeUpInterval);
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
  //32K pin is the frequency signal of the RTC (32KHz)
  if (! RTC.begin()) {
    Serial.println(F("Couldn't find RTC"));
    Serial.flush();
    abort();
  }
  RTC.disable32K();
  batvolt = tp.GetBatteryVoltage();
  //If it's the first run, the RTC time is set by requesting the time to the notecard
  if (firstRun && SetRTC)
  {
    Serial.println("Connection to the notehub to request the time ....");
    Serial.println("Status of the connection is checked each 10 seconds.");
    //Connection of the notecard to the notehub
    InitializeNoteHubConnection();
    //Request of the notecard time
    unsigned int RecievedTime = 0;
    String TimeZoneRecieved;
    J *rsp = notecard.requestAndResponse(notecard.newRequest("card.time"));
    if (rsp != NULL) {
      RecievedTime = (unsigned int)JGetNumber(rsp, "time");
      TimeZoneRecieved = (String)JGetString(rsp, "zone");
      notecard.deleteResponse(rsp);
    }
    //Turn off the notecard
    digitalWrite(MOSFET_NOTECARD_PIN, LOW);

    Serial.print("Epoch time recieved (UTC+0): ");
    Serial.println(RecievedTime);
    Serial.print("TimeZone:");
    Serial.println(TimeZoneRecieved);
    if (RecievedTime != 0)
    {
      //Recieved unix time is in UTC+0 -> put it in UTC+1
      RecievedTime += 3600;
      Serial.println("RTC time has been set with the notecard time !");
      RTC.adjust(DateTime(RecievedTime));
      DateTime TempNow = RTC.now();
      Serial.print("Time of the RTC (UTC+1): ");
      Serial.println(TempNow.unixtime());
      Serial.println(String("Date (UTC+1):")+TempNow.timestamp(DateTime::TIMESTAMP_DATE));
      Serial.println(String("Time (UTC+1):")+TempNow.timestamp(DateTime::TIMESTAMP_TIME));
    }
    else
    {
      Serial.println("---------------!!!WARNING!!!---------------");
      Serial.println("We didn't recieve a time from the notecard");
      DateTime TempNow = RTC.now();
      Serial.print("Current RTC time (unix) is: ");
      Serial.println(TempNow.unixtime());
      Serial.println(String("DateTime: ")+TempNow.timestamp(DateTime::TIMESTAMP_FULL));
    }
  }
  else
  {
    Serial.print("SetRTC value is:");Serial.println(SetRTC);
  }
  
  // Configure the DS3231 clock to the build time
  //RTC.adjust(DateTime(2022, 04, 26, 17, 22, 0));
  //RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  Serial.println("-----------------------------------NEW MEASUREMENT-----------------------------");
  Serial.print("Measurement ID ");
  Serial.print(readingID); Serial.println(" begins...");
  //Get current time
  fetchTime();
  //Sample the sensors
  sensors.mesure();
  //Save the values in JSON array
  Serial.println("Try to put sensors values in an array...");
  sensors.getMeasuredValues(LastMeasurements, readingID % MAX_MEASUREMENTS);
  Serial.println("Done");
  //Save time
  // fetch the time
  DateTime now;
  now = RTC.now();
  char tempBuffer [35] = "";
  sprintf(tempBuffer, "%04d/%02d/%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

  //The RTC is set to UTC+1 (Winter time) but unixtime is conventionaly given in UTC+0 so we remove one hour in seconds
  DatesUNIX[readingID % MAX_MEASUREMENTS] = now.unixtime() - 3600;
  Serial.print("Unixtime of the measurement (UTC+0):");
  Serial.println(DatesUNIX[readingID % MAX_MEASUREMENTS]);
  strcpy(SavedDates[readingID % MAX_MEASUREMENTS], (const char *) tempBuffer);
  LastMeasurements[readingID % MAX_MEASUREMENTS][0] = (float)readingID;
  LastMeasurements[readingID % MAX_MEASUREMENTS][1] = (float)tp.GetBatteryVoltage();
  int tempIdx = readingID % MAX_MEASUREMENTS;

  //Print only the array that stores the dates
  Serial.println("Dates array");
  for (int i = 0; i < MAX_MEASUREMENTS; i++)
  {
    Serial.print("||"); Serial.print(SavedDates[i]); Serial.println("  ");
  }

  logSDCard();

  if (firstRun || ALWAYS_DISPLAY_TO_OLED) {
    displayToOled(batvolt);
    sleep(2);
    u8x8.noDisplay();
  }
  tp.DotStar_SetPower(false);
  readingID ++;
  if (readingID == 1)
  {
    deep_sleep_mode(wakeUpInterval - 35); //22 is the time shift for the program execution
  }
  else if (readingID >= measurementSendingThreshold)//Data are sent to notehub
  {
    Serial.println("Display the saved array before sending");
    //Before sending the values, they are printed in serial monitor
    String headerTemp = sensors.getFileHeader();
    Serial.print("Date;readingID;BatteryVoltage");
    Serial.println(headerTemp);
    for (int j = 0; j < MAX_MEASUREMENTS; j++)
    {
      Serial.print(SavedDates[j]); Serial.print("  ");
      for (int i = 0; i < Nb_values_sensors + 2; i++)
      {
        Serial.print(LastMeasurements[j][i]);
        Serial.print("  ");
      }
      Serial.println("");
    }
    Serial.println("Saved array has been succesfully displayed");

    //If MAX_MEASUREMENTSs have been done, these data are sent to the NoteHub.
    //Turn on the notecard
    Serial.println("Turn on notecard");
    //Connect the notecard to NoteHub
    unsigned long InitTime_s = InitializeNoteHubConnection();
    //Send the measurements to NoteHub
    unsigned long SendingTime_s = NoteHubSendData();
    //turn off the notecard
    digitalWrite(MOSFET_NOTECARD_PIN, LOW);
    //Increase the sending threshold
    measurementSendingThreshold += MAX_MEASUREMENTS;
    Serial.print("Time for initalization [s]: ");Serial.println(InitTime_s);
    Serial.print("Time to send data [s]: ");Serial.println(SendingTime_s);
    
    int wakeUpIntervalAfterSending = (int)(wakeUpInterval - SendingTime_s - InitTime_s - 4);
    if (wakeUpIntervalAfterSending > 0)
    {
      deep_sleep_mode(wakeUpIntervalAfterSending);
    }
    else
    {
      deep_sleep_mode(wakeUpInterval - 4); //2 is the time shift for the program execution
    }

  }
  else//else a normal deep sleed mode is performed
  {
    deep_sleep_mode(wakeUpInterval - 4); //2 is the time shift for the program execution
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
    String SetRTC_str = "3";

    if (configFile.available()) {
      steps = configFile.readStringUntil(';');
    }
    if (configFile.available()) {
      //Keep going until the end of line is detected
      while (configFile.peek() != '\n')
      {
        configFile.seek(configFile.position() + 1);
      }
      //Go over \n character
      configFile.seek(configFile.position() + 1);
      SetRTC_str = configFile.readStringUntil(';');
      Serial.print("SetRTC read from SD: ");
      Serial.println(SetRTC_str);
    }
    configFile.close();

    if (SetRTC_str!="3")
    {
      SetRTC = SetRTC_str=="1"; 
    }
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
  //Display measurement step to oled and filename
  if (firstRun) {
    u8x8.clear();
    u8x8.setCursor(0, 0);
    u8x8.print(F("Steps: "));
    u8x8.print(wakeUpInterval);
    u8x8.print(F(" sec."));
    //Display the connection interval
    u8x8.setCursor(0, 2);
    u8x8.print(F("Sending step: "));
    u8x8.setCursor(0, 3);
    u8x8.print((float)(wakeUpInterval)*MAX_MEASUREMENTS / 60);
    u8x8.print(F(" min"));
    //Display the time of the RTC
    u8x8.setCursor(0, 4);
    u8x8.print(F("RTC time: "));
    DateTime TempNow = RTC.now();
    u8x8.setCursor(0, 5);
    u8x8.print(TempNow.timestamp(DateTime::TIMESTAMP_DATE));
    u8x8.setCursor(0, 6);
    u8x8.print(TempNow.timestamp(DateTime::TIMESTAMP_TIME));
    u8x8.setCursor(0, 7);
    u8x8.print(F("Mesures begin !"));
    sleep(10);

    //Display the filename of the running code
    u8x8.clear();
    u8x8.setCursor(0, 0);
    u8x8.print(F("Code Filename:"));

    char* tempFileName = (strrchr(__FILE__, '\\') + 1);

    Serial.print("Filename is: ");
    Serial.println(tempFileName);
    
    int charCounter=0;
    int lineCounter=0;
    //Print the filename until a dot is found. The .ino is not printed on the oled !
    while(tempFileName[charCounter]!='.')
    {
        u8x8.setCursor(charCounter-16*lineCounter, lineCounter + 2);
        u8x8.print(tempFileName[charCounter]);
        charCounter++;
        //16 is the maximum number of characters per line on the oled screen
        if(charCounter>=16*(lineCounter+1))
        {
            lineCounter++;
        }     
    }

    delay(8000);
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
  logFile.println(strrchr(__FILE__, '\\') + 1);
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
  //pinMode(MOSFET_PIN, OUTPUT);
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

unsigned long InitializeNoteHubConnection()
{
  unsigned long startTime = micros();
  //Turn on the mosfet controlling the notecarde assura

  digitalWrite(MOSFET_NOTECARD_PIN, HIGH);
  delay(500);
  // Initialize the physical I/O channel to the Notecard
  //Notecard I2C is on channel 3 of the I2C multiplexer
  sensors.tcaselect(3);
  delay(50);
  notecard.begin();
  delay(500);

  // Service configuration request
  J *req = notecard.newRequest("hub.set");
  // This command (required) causes the data to be delivered to the Project on notehub.io that has claimed
  // this Product ID.  (see above)
  JAddStringToObject(req, "product:", myProductID);
  // This sets the notecard's connectivity mode to minimum
  JAddStringToObject(req, "mode", "minimum");
  //It should decrease the number of irrelevant data sent
  //Don't know if it really works
  JAddBoolToObject(req, "unsecure", true);
  notecard.sendRequest(req);
  bool connectionDone = false;
  bool timeOut = false;
  unsigned long currentTime;
  String hubStatus_str;
  while(hubStatus_str != "connected (session open) {connected}" && !connectionDone && !timeOut)//while(!connectionDone && !timeOut)
  {
    delay(10000);
    J *ConnectionStatus = notecard.requestAndResponse(notecard.newRequest("hub.status"));
    char* tempString = JPrint(ConnectionStatus);
    Serial.println("Connection Status");
    Serial.println(tempString);
    connectionDone = JGetBool(ConnectionStatus, "connected");
    hubStatus_str = String(JGetString(ConnectionStatus,"status"));
    Serial.print(" \"connected\" value: ");
    Serial.println(connectionDone);
    currentTime = micros();
    //If connection is not made in less than 3 minutes ->Time out 
    if((currentTime-startTime)/1000000>180)
    {
      timeOut = true;
      Serial.println("Connection error: Time out reached !");
      Serial.println("The request will be sent at the next connection.");
    }
  } 
  currentTime = micros();
  if(!timeOut)
  {
    Serial.print("Connection sucessfully done after [s]: ");
    Serial.println((currentTime-startTime)/1000000);
  }
  
  return (currentTime-startTime)/1000000;
}

void ReadCSVfile()
{
  unsigned long tempPosition;
  int lineCnt = 0;
  //Open the csv file where the measurements are stored
  File myFile = SD.open(DATA_FILENAME, FILE_READ);

  //-----------Put the "cursor" at the begining of the MAX_MEASUREMENTSs last row---------
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
  Serial.println(valin, 6);
  Serial.print("Value after is:");

  int scale = pow(10, decimals);

  int valtemp = 0.5 + valin * scale;
  float valout = valtemp / scale;
  Serial.println(valout, 6);
  return valout;
}



unsigned long NoteHubSendData()
{
  unsigned long startTime = micros();
  //The JSON object is sent to IotPlotter platform;

  //Create a JSON object

  J *body = JCreateObject();
  if (body == NULL)
  {
    Serial.println("Body creation failed!");
  }
  J* tempData = JCreateObject();
  JAddItemToObject(body, "data", tempData);
  J* Vbatt_J = JCreateArray();
  if (Vbatt_J == NULL)
  {
    Serial.println("Vbat creation failed!");
  }
  JAddItemToObject(tempData, "Battery_Voltage_(V)", Vbatt_J);
  J* ID_J = JAddArrayToObject(tempData, "ID_Measurement");
  J* Temperature_J = JAddArrayToObject(tempData, "Air_Temperature_(degC)");
  J* Pressure_J = JAddArrayToObject(tempData, "Air_Pressure_(mbar)");
  J* Humidity_J = JAddArrayToObject(tempData, "Air_Relative_Humidity_(percentage)");
  J* Debit_J = JAddArrayToObject(tempData, "Debit_Air_(Lmin)");
  J* Velocity_J = JAddArrayToObject(tempData, "Velocity_Air_(ms)");

  J *sample = NULL;
  J* value = NULL;
  J* epoch = NULL;

  for (int i = 0; i < MAX_MEASUREMENTS; i++)
  {
    //Battery voltage
    sample = JCreateObject();
    JAddItemToArray(Vbatt_J, sample);
    //Add value of the sample
    value = JCreateString(String(LastMeasurements[i][1], 2).c_str());
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

    //Temperature Air
    sample = JCreateObject();
    JAddItemToArray(Temperature_J, sample);
    //Add value of the sample
    value = JCreateString(String(LastMeasurements[i][2], 2).c_str());
    JAddItemToObject(sample, "value", value);
    //Add timeStamp of the sample
    epoch = JCreateNumber(DatesUNIX[i]);
    JAddItemToObject(sample, "epoch", epoch);

    //Pressure Air
    sample = JCreateObject();
    JAddItemToArray(Pressure_J, sample);
    //Add value of the sample
    value = JCreateString(String(LastMeasurements[i][3], 2).c_str());
    JAddItemToObject(sample, "value", value);
    //Add timeStamp of the sample
    epoch = JCreateNumber(DatesUNIX[i]);
    JAddItemToObject(sample, "epoch", epoch);

    //Humidity Air
    sample = JCreateObject();
    JAddItemToArray(Humidity_J, sample);
    //Add value of the sample
    value = JCreateString(String(LastMeasurements[i][4], 2).c_str());
    JAddItemToObject(sample, "value", value);
    //Add timeStamp of the sample
    epoch = JCreateNumber(DatesUNIX[i]);
    JAddItemToObject(sample, "epoch", epoch);

    //Debit Air
    sample = JCreateObject();
    JAddItemToArray(Debit_J, sample);
    //Add value of the sample
    value = JCreateString(String(LastMeasurements[i][5], 6).c_str());
    JAddItemToObject(sample, "value", value);
    //Add timeStamp of the sample
    epoch = JCreateNumber(DatesUNIX[i]);
    JAddItemToObject(sample, "epoch", epoch);

    //Velocity Air
    sample = JCreateObject();
    JAddItemToArray(Velocity_J, sample);
    //Add value of the sample
    value = JCreateString(String(LastMeasurements[i][6], 6).c_str());
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

  Serial.println("Sending this JSON object to notehub ...");
  //Send the object through request
  J *req = notecard.newRequest("note.add");
  JAddItemToObject(req, "body", body);
  JAddBoolToObject(req, "sync", true);
  Serial.println("Try to send request");
  if(!notecard.sendRequest(req))
  {
    Serial.println("-------sendRequest Error-------------");
  }
  
  bool needToWait = true;
  int counterTime = 0;
  bool timeOut = false;
  while(needToWait && !timeOut)
  {
    delay(5000);
    J *SyncStatus = notecard.requestAndResponse(notecard.newRequest("hub.sync.status"));
    char* tempString = JPrint(SyncStatus);
    Serial.println("Connection Status");
    Serial.println(tempString);
    needToWait = JGetBool(SyncStatus, "sync");
    Serial.print("needToWait value: ");
    Serial.println(needToWait);
    counterTime++;
    if(counterTime>11)
    {
      Serial.println("Sending the data is longer than expected...");
      Serial.println("Time Out reached (1min) !");
      timeOut = true;
    }
      
  }
  if(!timeOut)
  {
  Serial.println("Data are successfully sent ! ");
  }
  delay(5000);
  unsigned long endTime = micros();
  //Return the number of second needed to send the data
  return (endTime-startTime)/1000000;
}

//Code version du 30.03.2022 avec Tstep en secondes GMT+1

#define SERIAL_DEBUG false
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

RTC_DS3231 RTC;
/***************************************************************
 * Variables                                                   *
 ***************************************************************/
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

// DS3231
uint64_t wakeUpInterval = 60;

boolean firstRun = true;
// sensors
Sensors sensors;

#include <TinyPICO.h>
TinyPICO tp = TinyPICO();

// Save reading number on RTC memory
RTC_DATA_ATTR int readingID = 0;

void setup() {
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
  if(!SD.begin(SD_CS)) {
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

  // Configure the DS3231 clock to the build time (GMT+1)
  //RTC.adjust(DateTime(2022, 06, 15, 14, 23, 0));

  fetchTime();
  sensors.mesure();

 
  //Serial.println(DateTime);
  logSDCard();

  if (firstRun || ALWAYS_DISPLAY_TO_OLED) {
    displayToOled(batvolt);
    sleep(2);
    u8x8.noDisplay();
  }
  tp.DotStar_SetPower(false);
  readingID ++;
  if (readingID == 1){
    deep_sleep_mode(wakeUpInterval - 28); //28 is the time shift for the program execution
  }
  else {
    deep_sleep_mode(wakeUpInterval - 16); //2 is the time shift for the program execution
  }
}


/***************************************************************
 * Main loop                                                   *
 ***************************************************************/

void loop() {
}

/**
 * Write a line of (sensors) data to the SD file
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
  dataMessage = String(readingID) + ";" + String(dateTime) + ";"+ String(batvolt)+";"+ String(sensorsData)+"\r\n";
  Serial.print("Save data: ");
  Serial.println(dataMessage);
  appendFile(SD, "/data.csv", dataMessage.c_str());
}

/**
 * Read the config file from the SD
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
 * Display (sensors) data to the OLED display
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

  while(data != "") {
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

  if(firstRun) {
    u8x8.clear();
    u8x8.setCursor(0, 0);
    u8x8.print(F("Steps: "));
    u8x8.print(wakeUpInterval);
    u8x8.print(F(" sec."));
    
    //Display the time of the RTC
    u8x8.setCursor(0, 2);
    u8x8.print(F("RTC time: "));
    DateTime TempNow = RTC.now();
    u8x8.setCursor(0, 3);
    u8x8.print(TempNow.timestamp(DateTime::TIMESTAMP_DATE));
    u8x8.setCursor(0, 4);
    u8x8.print(TempNow.timestamp(DateTime::TIMESTAMP_TIME));
    u8x8.setCursor(0, 5);
  
    u8x8.setCursor(0, 7);
    u8x8.print(F("Mesures begin !"));
    sleep(4);

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

    delay(6000);
  }
  
  // Power off the OLED display
  u8x8.noDisplay();
}

boolean writeFileHeader() {
  Serial.println("file header write");
  File logFile = SD.open(DATA_FILENAME, FILE_APPEND);
  //logFile.open(DATA_FILENAME, O_WRITE | O_AT_END);
  //logFile.open(DATA_FILENAME, O_WRITE);
  if(!logFile){
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

void deep_sleep_mode(uint64_t nbSeconds){
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
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

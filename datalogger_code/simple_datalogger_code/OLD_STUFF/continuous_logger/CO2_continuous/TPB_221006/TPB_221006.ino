#include <Wire.h>
#include <TinyPICO.h>
#include <RTClib.h>
#include <SD.h>
#include <U8x8lib.h>
#include "SparkFun_SCD4x_Arduino_Library.h" 

#define I2C_MUX_ADDRESS 0x70 
#define MOSFET_SENSORS_PIN 14 //pin to control the power to 
#define SD_CS_PIN 5//Define CS pin for the SD card module

#define DATA_FILENAME "/data.csv"
#define CONFIG_FILENAME "/conf.txt"

//values which must be on the SD card in the config file otherwise there will be an error
int deep_sleep_time; //read this value from the SD card later 
DateTime start_time;
int IDcount;

TinyPICO tp = TinyPICO();
RTC_DS3231 rtc; 
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE); //initialise something for the OLED display
SCD4x co2_sensor(SCD4x_SENSOR_SCD41); //initialise SCD41 CO2 sensor in soil

void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

void setup() {
  pinMode(MOSI, OUTPUT);
  digitalWrite(MOSI, HIGH);
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin(); // initialize I2C bus 
  Wire.setClock(50000); // set I2C clock frequency to 50kHz which is rather slow but more reliable //change to lower value if cables get long

  tp.DotStar_SetPower(false); //This method turns on the Dotstar power switch, which enables the LED

  pinMode(MOSFET_SENSORS_PIN, OUTPUT);
  digitalWrite(MOSFET_SENSORS_PIN, HIGH);

  delay(50);

  u8x8.begin();
  u8x8.setBusClock(50000); //Set the SCL at 50kHz since we can have long cables in some cases. Otherwise, the OLED set the frequency at 500kHz
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_r);
  u8x8.setCursor(0, 0);
  u8x8.print("Init...");
  Serial.println("Init...");

  delay(1000);

  //initialize sd card
  bool SD_error = false;
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    SD_error = true;
  }
  else{
    File configFile = SD.open(CONFIG_FILENAME);
    if (configFile.available()) {
      deep_sleep_time = configFile.readStringUntil(';').toInt();
    }
    configFile.close();
    Serial.println("Values read from SD Card:");
    Serial.print("- Deep sleep time: ");
    Serial.println(deep_sleep_time);
  }
  u8x8.setCursor(0, 2);
  if (SD_error == true){
    u8x8.print("SD Failure !");
  }
  //write header in SD
  else {
    File logFile = SD.open(DATA_FILENAME, FILE_APPEND);
    logFile.println();
    logFile.print("ID;DateTime;VBatt;CO2ppm;\n");
    logFile.close();

    // show on display and LED if values on SD card are OK
    u8x8.setCursor(0, 2);
    u8x8.print("Success !");
  
    u8x8.setCursor(0, 4);
    u8x8.print("timestep: ");
    u8x8.print(deep_sleep_time);
    u8x8.print("s");
    delay(2000);
  }

  rtc.begin();
  rtc.disable32K();

  float batvolt = tp.GetBatteryVoltage();

  u8x8.display(); // Power on the OLED display
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print("Batt: ");
  u8x8.print(batvolt);
  u8x8.print("V");

  u8x8.setCursor(0, 2);
  DateTime curr_time = rtc.now();
  String time_str = String(curr_time.day()) + "." + String(curr_time.month()) + " " + String(curr_time.hour())+ ":" + String(curr_time.minute()) + ":" + String(curr_time.second());
  u8x8.print(time_str);

  delay(7000); //in [ms] in order to see the LED
  u8x8.clear();

  tcaselect(0);
  co2_sensor.begin();


  start_time = rtc.now();
  IDcount = 0;

}

void loop() {
  //get time
  DateTime measure_time = rtc.now();
  Serial.print("Measure time : ");
  Serial.println(measure_time.timestamp());
  //measure all sensors
  IDcount++;
  Serial.print("ID count: ");
  Serial.println(IDcount);
  float batvolt = tp.GetBatteryVoltage();

  //connect and read CO2 sensor
  tcaselect(0);
  co2_sensor.readMeasurement();
  float CO2ppm=co2_sensor.getCO2();
  delay(40);

  //store data on sd card
  String data_message = (String) IDcount;
  char buffer [35] = "";
  sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d", measure_time.day(), measure_time.month(), measure_time.year(), measure_time.hour(), measure_time.minute(), measure_time.second());
  data_message = data_message + ";"+ buffer + ";";
  data_message = data_message + (String)batvolt + ";";
  char str_sensor_values[200];
  sprintf(str_sensor_values, "%s;", 
  String(String(CO2ppm).c_str()));
  data_message = data_message + str_sensor_values;
  Serial.print("Line stored: ");
  Serial.println(data_message);
  //write to SD card
  File logFile = SD.open(DATA_FILENAME, FILE_APPEND);
  logFile.print(data_message);
  logFile.println();
  logFile.close();

  //display CO2
  u8x8.setFont(u8x8_font_courB24_3x4_r);
  u8x8.clear();
  u8x8.setCursor(0,0);
  u8x8.print(String(CO2ppm,0));
  u8x8.setCursor(0, 4);
  u8x8.print(String("ppm"));


  int next_measurement_time = (start_time + IDcount*deep_sleep_time).unixtime();
  int now_second = rtc.now().unixtime();
  int delta = next_measurement_time-now_second;


  if (delta>0){
    Serial.print("delta time: ");
    Serial.println(delta);
    delay(delta*1000+10);
  }

}

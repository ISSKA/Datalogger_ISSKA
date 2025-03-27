/* *******************************************************************************
Base code for datalogger with Notecard, with only MS8067 (temp, hum, press)

This code was entirely rewritten by Nicolas Schmid. Here are some important remarks when modifying to add or  add or remove a sensor:
    - adjust the array sensor_names which is in this .ino file
    - in Sensors::getFileData () don't forget to add a "%s;" for each new sensor (if you forget the ";" it will read something completely wrong)
    - adjust the struct, getFileHeader (), getFileData (), serialPrint() and mesure() with the sensor used
    - in Sensors.h don't forget to adjust the number of sensors in the variable num_sensors. This is actually rather a count of the number of sotred measurement
      values.For example if you have an oxygen sensor and you measure the voltage but also directy compute the O2 percentage, this counts as 2 measurements

The conf.txt file must have the following structure:
  300; //deepsleep time in seconds (put at least 120 seconds)
  1; //boolean value to choose if you want to set the RTC clock with GSM time (recommended to put 1)
  12; // number of measurements to be sent at once

If you have any questions contact me per email at nicolas.schmid.6035@gmail.com
**********************************************************************************
*/ 


#include <Wire.h>
#include <TinyPICO.h>
#include <RTClib.h>
#include <SD.h>
#include <U8x8lib.h>
#include <Notecard.h>
#include "Sensors.h" //file sensor.cpp and sensor.h must be in the same folder


TinyPICO tp = TinyPICO();
RTC_DS3231 rtc; 
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE); //initialise something for the OLED display
Notecard notecard;
Sensors sensor = Sensors();

//values which must be on the SD card in the config file otherwise there will be an error
int deep_sleep_time; //read this value from the SD card later
bool SetRTC; //read this value from the SD card. True means set the clock time with GSM signal
int MAX_MEASUREMENTS; //number of measurements sent at once with the notecard


//PINS to power different devices
#define RTC_PIN 27 //pin which powers the external RTC Clock, is set to High to power the clock but clock has an independant battery to work even when truned the TinyPICO is turned off
#define MOSFET_SENSORS_PIN 14 //pin to control the power to 
#define MOSFET_NOTECARD_PIN 4
//define GPIO_NUM_15 for rtc clock

#define myProductID "com.gmail.ouaibeer:datalogger_isska"
#define NOTECARD_I2C_ADDRESS 0x17

/* To connect the RTC with the TinyPICO
- Connect the VCC pin of the RTC to the 3.3V pin of the TinyPICO board.
- Connect the GND pin of the RTC to the GND pin of the TinyPICO board.
- Connect the SDA pin of the RTC to pin 21 (SDA) of the TinyPICO board.
- Connect the SCL pin of the RTC to pin 22 (SCL) of the TinyPICO board.
PIN 21 and 22 are used by default for the I2C protocol, we do not deed to specify them
*/

//SD card
#define SD_CS_PIN 5//Define CS pin for the SD card module
#define DATA_FILENAME "/data.csv"
#define CONFIG_FILENAME "/conf.txt"

//Global Variables which must be stored on the RTC memory (only about 4kB of storage available)
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool first_time = true;
RTC_DATA_ATTR bool data_succesfully_sent = true;
RTC_DATA_ATTR bool send_data_with_GSM = true;
RTC_DATA_ATTR int start_time; //I couldn't store something of type DateTime so I have to store the values in an array
RTC_DATA_ATTR int start_date_time[6]; //[year,month,day,hour,minute,second]

void setup(){ //The setup is recalled each time the micrcontroller wakes up from a deepsleep

  //Start communication protocol with external devices
  Serial.begin(115200);

  power_external_device();

  Wire.begin(); // initialize I2C bus 
  Wire.setClock(50000); // set I2C clock frequency to 50kHz which is rather slow but more reliable //change to lower value if cables get long

  if(first_time){ //sleep until rounded time, this snipet is only executed when the microcontroller is turned on for the first time

    tp.DotStar_SetPower(true); //This method turns on the Dotstar power switch, which enables the LED
    tp.DotStar_SetPixelColor(25, 25, 25 ); //Turn the LED white while it is not initialised

    //write initialize on the OLED display
    u8x8.begin();
    u8x8.setBusClock(50000); //Set the SCL at 50kHz since we can have long cables in some cases. Otherwise, the OLED set the frequency at 500kHz
    u8x8.setFont(u8x8_font_amstrad_cpc_extended_r);
    u8x8.setCursor(0, 0);
    u8x8.print("Init...");
    Serial.println("Init...");

    delay(2000); //wait to see the led

    //get stored values on SD card like deep_sleep_time, set_RTC etc.
    initialise_SD_card(); 

    //initialise the RTC clock
    rtc.begin();
    rtc.disable32K();
    if (SetRTC) set_RTC_with_GSM_time(); //also gives the number of bars of cellular network (0 to 4) 

    put_usefull_values_on_display();

    first_time=false;

    //enter deep_sleep until a rounded time
    get_next_rounded_time(); //this finds the next rounded time and stores it in the start_date_time array and the start_time variable

    deep_sleep_mode(0); //deep sleep until start time (next rounded time)
  }
  else{ //normal operation
    bootCount++; //to know how many times we have booted the  microcontroller
    Serial.print("***********Starting the card, Boot count: ");
    Serial.print(bootCount);
    Serial.println(" ***********************");
    
    //setup for the clock
    rtc.begin();
    rtc.disable32K();
    readRTC();//print current time

    initialise_SD_card(); //creates a header for the data.csv file and reads the values in the conf.txt file

    if (bootCount<=3){ //Turn the LED blue during first 3 measurements (this is only if you want to check that the logger works normally)
      tp.DotStar_SetPower(true);
      tp.DotStar_SetPixelColor(0, 0, 25 ); 
    }

    if (bootCount%50==0){ //after connexion failed we stop sending data over GSM because it uses battery for nothing. But after a while (here every 50 boots) we try to start again. remove the next to lines if you don't want to
      send_data_with_GSM = true; 
    }

    mesure_all_sensors(); // measure all sensors and stores all the measured values on the SD card 

    if (bootCount%MAX_MEASUREMENTS == 0 && send_data_with_GSM){ //after a number MAX_MEASUREMENTS of measurements, send the last few stored measurements over GSM
      send_data_overGSM();
    }

    deep_sleep_mode(deep_sleep_time);
  }
}


void loop(){ //No loop is used to spare energy
}

void put_usefull_values_on_display(){
 
  float batvolt = tp.GetBatteryVoltage();

  u8x8.display(); // Power on the OLED display
  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print("Batt: ");
  u8x8.print(batvolt);
  u8x8.print("V");

  u8x8.setCursor(0, 2);
  u8x8.print(readRTC());

  u8x8.setCursor(0, 4);
  u8x8.print("timestep: ");
  u8x8.print(deep_sleep_time);
  u8x8.print("s");

  sensor.mesure();   //measure all sensors to display the values
  String measured_values_str = sensor.serialPrint();
  u8x8.clear();
  u8x8.setCursor(0, 0);
  int line_count = 0;
  for (char const &c: measured_values_str) {
    if(c=='\n'){
      line_count++;
      if (line_count%6 ==0){
        delay(10000);
        u8x8.clear();
        u8x8.setCursor(0, 0);
      }
      else u8x8.println();
    }
    else u8x8.print(c);
  }
  delay(10000);
}

void get_next_rounded_time(){ //this finds the next rounded time and stores it in the start_date_time array and the start_time variable
  //if you chose 5, then it will start at :00, :05, :10 etc.
  int rounded_min = deep_sleep_time/60; //change this to the number of minutes to which you want to round. per default it will be the same as the sleeping time. e.g. if the sleeping time is 15min, it will only start at 00, :15, :30 etc. But for example if you want it to start every 10 minutes regardless of the sleeping time, write: int rounded_min = 10;
  DateTime current_time=rtc.now();
  int seconds_remaining = 60-current_time.second()%60; //number of seconds until the next rounded minute
  DateTime newtime = current_time+seconds_remaining; 
  if(newtime.minute()%rounded_min!=0){
    int minutes_remaining = rounded_min - newtime.minute()%rounded_min; //number of minutes until the next rounded time (in minutes)
    newtime = (newtime + 60*minutes_remaining);
  }
  start_time = newtime.unixtime();
  start_date_time[0]=newtime.year();
  start_date_time[1]=newtime.month();
  start_date_time[2]=newtime.day();
  start_date_time[3]=newtime.hour();
  start_date_time[4]=newtime.minute();
  start_date_time[5]=newtime.second();
  Serial.print("start time: ");
  Serial.println(start_time);
}


void power_external_device(){
  //Power the external clock

  pinMode(RTC_PIN, OUTPUT);     // RTC and status LED power via pin D7 (about 3 mA)
  digitalWrite(RTC_PIN, HIGH);  // Switch DS3231 RTC Power and Status LED on - Switched off in Deep Sleep mode

  //Pin to control the gate of MOSFET for notecard (turned off)
  pinMode(MOSFET_NOTECARD_PIN, OUTPUT);
  digitalWrite(MOSFET_NOTECARD_PIN, LOW);

  //Pin to control the gate of MOSFET for all the sensors (turned on)
  pinMode(MOSFET_SENSORS_PIN, OUTPUT);
  digitalWrite(MOSFET_SENSORS_PIN, HIGH);

  //something for SD card !!?! not sure if this is useful but it for sure insn't harmuful
  pinMode(MOSI, OUTPUT);
  digitalWrite(MOSI, HIGH);

}

void initialise_SD_card(){ //open SD card. Write header in the data.csv file. read values from the conf.txt file.

  bool SD_error = false;
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    SD_error = true;
  }
  else{
    File configFile = SD.open(CONFIG_FILENAME);
    if (configFile.available()) {
      deep_sleep_time = configFile.readStringUntil(';').toInt();
      while (configFile.peek() != '\n') //if there are any black space if the config file
        {
          configFile.seek(configFile.position() + 1);
        }
      configFile.seek(configFile.position() + 1);
      String str_SetRTC = configFile.readStringUntil(';');
      if (str_SetRTC=="0") SetRTC = false;
      else if (str_SetRTC=="1") SetRTC = true;
      else {
        Serial.println("problem by reading the SetRTC value from SD card");
        SD_error = true;
      }
      while (configFile.peek() != '\n') //if there are any black space if the config file
        {
          configFile.seek(configFile.position() + 1);
        }
      configFile.seek(configFile.position() + 1);
      String str_MAX_MEASUREMENT = configFile.readStringUntil(';');
      MAX_MEASUREMENTS = str_MAX_MEASUREMENT.toInt();
    }
    configFile.close();
    Serial.println("Values read from SD Card:");
    Serial.print("- Deep sleep time: ");
    Serial.println(deep_sleep_time);
    Serial.print("- Set the RTC clock with GSM: ");
    Serial.println(SetRTC);
    Serial.print("- Number of measurements to be sent at once: ");
    Serial.println(MAX_MEASUREMENTS);
  }

  if (first_time){ //first run
    u8x8.setCursor(0, 2);
    if (SD_error == true){
      u8x8.print("SD Failure !");
      tp.DotStar_SetPixelColor( 50, 0, 0 ); //set led to red
    }
    //write header in SD
    else {
      File logFile = SD.open(DATA_FILENAME, FILE_APPEND);
      String header = sensor.getFileHeader();
      logFile.println();
      logFile.println();
      logFile.println();
      logFile.print("ID;DateTime;VBatt;");
      logFile.println(header);
      logFile.close();

      // show on display and LED if values on SD card are OK
      u8x8.setCursor(0, 2);
      u8x8.print("Success !");
      tp.DotStar_SetPixelColor( 0, 50, 0 );
    }
    delay(1500); //in [ms] in order to see the LED
  }
  else{ //not first run -->OLED display is off but needs to be truned on if there is a SD error
    if (SD_error == true){
      u8x8.begin();
      u8x8.setBusClock(50000); //Set the SCL at 50kHz since we can have long cables in some cases. Otherwise, the OLED set the frequency at 500kHz
      u8x8.setFont(u8x8_font_amstrad_cpc_extended_r);
      u8x8.setCursor(0, 0);
      u8x8.print("SD Failure !");
      tp.DotStar_SetPower(true);
      tp.DotStar_SetPixelColor( 50, 0, 0 ); //set led to red
      delay(5000); //in [ms] in order to see the LED and display because there is an important and recurent error
    }
  }
}

void deep_sleep_mode(int sleeping_time){ //turnes off alomst everything and reboots the microconstroller after a given time in seceonds
  //turn off everything
  digitalWrite(MOSFET_SENSORS_PIN, LOW);
  digitalWrite(MOSFET_NOTECARD_PIN, LOW);
  digitalWrite(MOSI, LOW);
  tp.DotStar_SetPower(false);
  rtc.writeSqwPinMode(DS3231_OFF);
  rtc.clearAlarm(1);                // set alarm 1, 2 flag to false (so alarm 1, 2 didn't happen so far)
  rtc.clearAlarm(2);
  rtc.disableAlarm(2);  
  //enter deep sleep
  rtc.setAlarm1(start_time+bootCount*sleeping_time,DS3231_A1_Minute);  
  pinMode(GPIO_NUM_15, INPUT_PULLUP);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, 0);
  esp_deep_sleep_start();
}

String readRTC() { //print current time
  DateTime now = rtc.now();
  String time_str = String(now.day()) + "/" + String(now.month()) + " " + String(now.hour())+ ":" + String(now.minute()) + ":" + String(now.second());
  Serial.print("Current time: ");
  Serial.println(time_str);
  return time_str;
}

bool connect_notecard(){ //returns true if it was able to connect the notecard to GSM nework

  notecard.begin();
  J *req = notecard.newRequest("hub.set");
  JAddStringToObject(req, "product", myProductID);
  JAddBoolToObject(req, "unsecure", true);
  notecard.sendRequest(req);
  
  bool GSM_time_out = false;
  unsigned long start_time_connection = micros();
  String hubStatus_str;

  while(hubStatus_str != "connected (session open) {connected}" && !GSM_time_out){
    delay(500);
    J *ConnectionStatus = notecard.requestAndResponse(notecard.newRequest("hub.status"));
    if (ConnectionStatus != NULL){
      hubStatus_str = String(JGetString(ConnectionStatus,"status"));
      notecard.deleteResponse(ConnectionStatus);
      Serial.println(hubStatus_str);
    }
    //If connection is not made in less than 30 seconds ->Time out 
    unsigned long current_time = micros();
    if((current_time-start_time_connection)/1000000>120){ //attendre au max 2 min
      GSM_time_out = true;
      digitalWrite(MOSFET_NOTECARD_PIN, LOW);
      return false;
    }
    if(hubStatus_str == "connected (session open) {connected}"){
      Serial.println("The Hub is connected!");
      return true;
    }
  }
  return false;
}

void set_RTC_with_GSM_time(){ //gets the time from GSM and adjust the rtc clock with the swiss winter time (UTC+1)
  Serial.println("Try to use the notehub to request the GSM time ....");
  unsigned int GSM_time = getGSMtime();
  rtc.adjust(DateTime(GSM_time));  
  readRTC();
  sleep(3);
}

unsigned int getGSMtime(){
  u8x8.setCursor(0, 4);
  u8x8.print("Get GSM time...");
  digitalWrite(MOSFET_NOTECARD_PIN, HIGH); //turn on the notecard
  delay(200); //time for MOSFET to be totally turned on (probably way faster but its only once anyway)
  sensor.tcaselect(6);
  delay(100); //time for MUX to be totally turned on (probably way faster but its only once anyway)
  tp.DotStar_SetPixelColor(25, 0, 25 ); //LED Purple

  if(connect_notecard()){
    J *rsp = notecard.requestAndResponse(notecard.newRequest("card.time"));
    unsigned int recieved_time = (unsigned int)JGetNumber(rsp, "time");

    //show the numbers of bars of GSM network (0 to 4) like on a phone
    J *gsm_info = notecard.requestAndResponse(notecard.newRequest("card.wireless"));
    J *net_infos = JGetObjectItem(gsm_info,"net");
    int bars = JGetNumber(net_infos, "bars");
    Serial.print("Bars: ");
    Serial.println(bars);
    u8x8.setCursor(0, 6);
    u8x8.print("GSM Bars: ");
    u8x8.print(bars);

    tp.DotStar_SetPixelColor(0, 50, 0 ); //LED Green
    Serial.println("The RTC time is adjusted with the GSM time (GMT + 1)");
    return recieved_time+3600; //+3600 for GMT+1 (winter time in switzerland)
  }
  else{
    Serial.println("could not connect the Notecard, we use RTC time instead");
    digitalWrite(MOSFET_NOTECARD_PIN, LOW); //turn off the notecard
    u8x8.setCursor(0, 6);
    u8x8.print("Failed to connect");
    tp.DotStar_SetPixelColor(50, 0, 0 ); //LED Red
    return (unsigned int)rtc.now().unixtime();
  }
}

void save_data_in_SD(){ //give the measured values in a single string "ID;DateTime;VBatt;tempEXT;pressEXT;HumExt;..." eg 
  //measurement ID (bootcount)
  String data_message = (String) bootCount;

  char buffer [35] = "";
  DateTime starting_time_dt = DateTime(start_date_time[0],start_date_time[1],start_date_time[2],start_date_time[3],start_date_time[4],start_date_time[5]);
  DateTime curr_time = starting_time_dt +(bootCount-1)*deep_sleep_time;
  sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d", curr_time.day(), curr_time.month(), curr_time.year(), curr_time.hour(), curr_time.minute(), curr_time.second());
  data_message = data_message + ";"+ buffer + ";";

  //get battery voltage to estimate current battery
  float batvolt = tp.GetBatteryVoltage();
  data_message = data_message + (String)batvolt + ";";

  String sensor_data = sensor.getFileData(); 
  data_message = data_message + sensor_data;
  Serial.println(data_message);

  //write data in SD
  File logFile = SD.open(DATA_FILENAME, FILE_APPEND);
  logFile.print(data_message);
  logFile.println();
  logFile.close();
}

void mesure_all_sensors(){
  Serial.println("Start of the measurements");
  sensor.mesure(); // measures all sensors and stores the values in a struct sensor.mesures
  sensor.serialPrint(); //print all the sensor values in Serial monitor
  save_data_in_SD();
}


void send_data_overGSM(){ //send several measurements all at once over GSM

  Serial.println("We now try to send sevral measurements over GSM all at once");

  File myFile = SD.open(DATA_FILENAME, FILE_READ);


  int position_in_csv = myFile.size(); //get the position of the last character in the csv file

  //if data wasn't properly sent try again one more time. for example if MAX_MEASUREMENTS = 3 it tries to send the 3 last sored measurements, but if it fails once it tries next time to send the 6 last measurements
  int nb_lines_to_send =MAX_MEASUREMENTS; 
  if(data_succesfully_sent==false){
    nb_lines_to_send=2*MAX_MEASUREMENTS;
  }

  //find the position where the last few lines of the CSV begin but starting from the end of the CSV since the file might be very long
  //if there is some wierd behaviour with this function, check that you didn't forget a ";" in the Sensors::getFileData () function. I often forgot to write the last one
  for (int i = 0; i < nb_lines_to_send;i++){
    position_in_csv = position_in_csv -20; // avoid the last twenty characters since a line is certainly longer than 20 characters
    myFile.seek(position_in_csv); 
    while (myFile.peek() != '\n'){
      position_in_csv = position_in_csv - 1;
      myFile.seek(position_in_csv); // Go backwards until we detect the previous line separator
    }
  }
  myFile.seek(position_in_csv+1); //at this point we are at the beginning of the nb_lines_to_send last lines of the csv file

  //creat a matrix with the values to be sent
  String data_matrix[nb_lines_to_send][3+num_sensors]; //the "3 + ..." is there because we also store Bootcount,DateTime and Vbatt
  int counter=0;
  while (myFile.available()) {
    for (int i =0; i<3+num_sensors;i++){ //6 is for ID,Time,batVolt, +3sensors
      String element = myFile.readStringUntil(';');
      if (element.length()>0) data_matrix[counter][i] = element;
    }
    myFile.readStringUntil('\n');
    counter++;
  }

  //print the values which will be sent
  for (int i =0; i<nb_lines_to_send;i++) {
    Serial.print("line to be sent: ");
    for(int j=0;j<3+num_sensors;j++){        
      Serial.print(data_matrix[i][j]);
      Serial.print(";");
    }
    Serial.println();
  }

  /*data_matrix
      Bootcount DateTime Vbat sensor1 sensor2 ...
  meas1     x       x       x     x       x
  meas2     x       x       x     x       x
  meas3     x       x       x     x       x
  ...

  e.g. data_matrix[2][4] will get the value from sensor2 at meas3
  */

  //power the Notecard and connect the multiplexer with notecard
  digitalWrite(MOSFET_NOTECARD_PIN, HIGH); //turn on the notecard
  delay(200); //time for MOSFET to be totally turned on (probably way faster but its only once anyway)
  sensor.tcaselect(6);
  delay(100); //time for MUX to be totally turned on (probably way faster but its only once anyway)
  if(bootCount<=3) tp.DotStar_SetPixelColor(25, 0, 25 ); //LED pink while connecting to hub (to show everything is working on first round)
  //send the last few measurements over GSM
  if(connect_notecard()){ //connect the notecard
    Serial.println("Connected!");
    if(bootCount<=3) tp.DotStar_SetPixelColor(25, 25, 0 ); //LED yellow while sending data

    //create the Jason file to be sent

    //first create the body
    String sensor_names[num_sensors+1] = {"VBat","tempEXT","pressEXT","humEXT"}; //change this if you add more sensors!!!
    J *body = JCreateObject();
    J *data = JCreateObject();
    JAddItemToObject(body, "data", data);
    for(int i=0; i<num_sensors+1;i++){
      J *sensor = JAddArrayToObject(data,sensor_names[i].c_str());
      for(int j=0; j<nb_lines_to_send;j++){
        J *sample = JCreateObject();
        JAddItemToArray(sensor, sample);
        JAddStringToObject(sample, "value", data_matrix[j][i+2].c_str());

        int unix_time_stored = start_time +(bootCount-nb_lines_to_send+j)*deep_sleep_time -3600; //it's easier that way than to convert the string "yeart-month-day hour:minute:second" to unixtime. remark: for iotplotter, we need the UTC time, not UTC + 1 (winter time in switzerland). So here we take 3600 seconds away.
        JAddStringToObject(sample, "epoch", String(unix_time_stored).c_str());
      }
    }
    //print the Jason body
    char* tempString = JPrint(body);
    Serial.println("Print the body object ...");
    Serial.println(tempString);

    //create a request
    J *req = notecard.newRequest("note.add");
    JAddItemToObject(req, "body", body);
    JAddBoolToObject(req, "sync", true);

    //send Jason file and wait up until it is recieved
    notecard.sendRequest(req);
    unsigned long start_time_connection = micros();
    bool GSM_time_out = false;
    bool needToWait = true;
    while(needToWait==true && !GSM_time_out){
      delay(1000);
      J *SyncStatus = notecard.requestAndResponse(notecard.newRequest("hub.sync.status"));
      if (SyncStatus != NULL){
        String sync_status_str = String(JGetString(SyncStatus,"status"));
        needToWait = JGetBool(SyncStatus, "sync");
        notecard.deleteResponse(SyncStatus);
        Serial.print("need to wait: ");
        Serial.print(needToWait);
        Serial.print("  Sync status: ");
        Serial.println(sync_status_str);
      }
      //If connection is not made in less than 180 seconds ->Time out 
      unsigned long current_time = micros();
      if((current_time-start_time_connection)/1000000>180){ //attendre au max 3 min
        GSM_time_out = true;
        digitalWrite(MOSFET_NOTECARD_PIN, LOW);
        Serial.println("Time out to send the data although hub was connected!");
        if (!data_succesfully_sent) send_data_with_GSM = false; //if data not sent twice in a row, stop try to send data over gsm and only store it on SD
        data_succesfully_sent=false;
        break;
      }
      if(needToWait==0){
        Serial.println("The data is sent! (hopefully)");
        data_succesfully_sent=true;
        break;
      }
    }
  }
  else{
    Serial.println("could not connect the notecard");
    if (!data_succesfully_sent) send_data_with_GSM = false; //if data not sent twice in a row, stop try to send data over gsm and only store it on SD
    data_succesfully_sent=false;
  }
  myFile.close();
}
 








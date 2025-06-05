#include "pump.h"
#include <TinyPICO.h>
#include <PeakDetection.h>

#define PUMP_CONFIGFILE "/pump.txt"
#define PUMP_DATAFILE "/pumpdata.csv"
#define TEST_FILENAME "/test.csv"

// Constructeur

Pump::Pump() {

}



RTC_DATA_ATTR InjectionData Pump::data = {
  .heights = {0.0f},
  .boot_step = 0,
};


RTC_DATA_ATTR int doseTableSize = 0;
RTC_DATA_ATTR ClassDose doseTable[10];
RTC_DATA_ATTR int pumpActivationCount = 0; 
RTC_DATA_ATTR bool Inject = true;
RTC_DATA_ATTR int counter = 0;
float measuredWaterheight =  0;
// Fonction pour lire une réponse avec timeout
String Pump::readPumpResponse() {
  String response = "";
  const unsigned long timeout = 120000;  // 15 seconds timeout
  unsigned long startTime = millis();

  while (true) {
    while (Serial2.available() > 0) {
      char inchar = (char)Serial2.read();

      startTime = millis();  // Reset timer on receiving data

      if (inchar == '\r' || inchar == '\n') {
        response.trim();  // Clean up whitespace
        if (!response.isEmpty()) {
          return response;  // Return completed response
        }
      } 
      else {
        response += inchar;
      }
    }

    // Check for timeout
    if (millis() - startTime > timeout) {
      response = "Timeout";  
      return response;       // Return empty or handle accordingly
    }
  }
}



// Fonction pour mettre la pompe en veille
void Pump::pumpSleep() {
  Serial2.print("Sleep\r");  // Send Sleep command
  const unsigned long timeout = 5000;
  unsigned long startTime = millis();
  while (true) {  // Keep reading responses until we get *SL
    String response = readPumpResponse();
    if (!response.isEmpty()) {
      if (response.indexOf("*SL") != -1) {  // If response contains *SL, exit loop
        Serial.println("Pump response (sl): " + response);  // Debug output
        Serial.println("Pump is now in sleep mode.");
        return;
      }
      else if (response.indexOf("*ER") != -1){
        Serial.println("Pump could'nt fall asleep");
        handleError(response);
        return;
      }
    }
    if (millis() - startTime > timeout) {
      Serial.println( "Pump could'nt fall asleep due to lack of communication");  
      return;       // Return empty or handle accordingly
    }
  }
}



// Fonction pour gérer les erreurs de la pompe
bool Pump::handleError(String& response) {
  if (response == "*ER"){
    Serial.println("The pump encountered a problem");
    Serial2.print("X\r");  // Commande pour réinitialiser ou annuler l'opération
    latestErrorMessage = response;
    return true;
  }
  else if (response == "Timeout"){
    Serial.print("The pump does'nt answer\n");
    latestErrorMessage = response;
    Serial.print("latesterrormessage : ");
    Serial.println(latestErrorMessage);
    return true;
  }
  else{
    if (response != "*RS" && response != "*RE" && response != "*OK"){
      latestErrorMessage = response;
      int index = latestErrorMessage.indexOf(',');
      if (index != -1){
        latestErrorMessage = latestErrorMessage.substring(0, index);
      }
      
    }
    return false;
  }

}



// Fonction principale qui gère le cycle de la pompe
void Pump::sendCommand(const String& pumpcommand) {
  bool error = false;
  delay(1000);

  Serial2.print(pumpcommand + "\r");  // Send command

  while (true) {  // Keep reading until a valid response is received


    String response = readPumpResponse();  

    if (!response.isEmpty()) {
      if (response == "*OK" || response == "*RE" || response == "*RS") {
        continue;  // Ignore noise
      }
      else{
        Serial.println(response);
      }
      

      if (response.indexOf("DONE") != -1) {
        latestErrorMessage = "Injected";
        pumpActivationCount ++;
        break;
      } 
      else if (response.indexOf("*WA") != -1){
        latestErrorMessage = "*WA";
        Serial2.print(pumpcommand  + "\r");
      }
      else{
        error = handleError(response);
        if(error){
          break;
        }
        else{
          continue;
        }
      }
    }
  }
  pumpSleep();
} 



void Pump::displayConfiguration(U8X8 &u8x8) {
  Serial.printf("%-18s| %-18s| %-5s\n", "Lower class limit", "Upper class limit", "Dose");
  Serial.println("------------------|------------------|------");
  
  for (int i = 0; i < doseTableSize; i++) {
    Serial.printf("%-18d| %-18d| %-5d\n", doseTable[i].lower, doseTable[i].upper, doseTable[i].dose);
  }

  u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.println(" L | U | D | I ");
  u8x8.println("---|---|---|--");

  for (int i = 0; i < doseTableSize; i++) {

    if (i %6 ==0 && i>0){
      delay(5000);
      u8x8.clear();
      u8x8.setCursor(0, 0);
      u8x8.println(" L | U | D | I ");
      u8x8.println("---|---|---|--");
    }
    if (doseTable[i].lower < 10) u8x8.print("  ");
    else if (doseTable[i].lower < 100) u8x8.print(" ");
    u8x8.print(doseTable[i].lower);
    u8x8.print(" ");

    if (doseTable[i].upper < 10) u8x8.print("  ");
    else if (doseTable[i].upper < 100) u8x8.print(" ");
    u8x8.print(doseTable[i].upper);
    u8x8.print(" ");

    if (doseTable[i].dose < 10) u8x8.print("  ");
    else if (doseTable[i].dose < 100) u8x8.print(" ");
    u8x8.print(doseTable[i].dose);
    u8x8.print(" ");

    if (doseTable[i].numberOfInjections < 10) u8x8.print("  ");
    else if (doseTable[i].numberOfInjections < 100) u8x8.print(" ");
    u8x8.println(doseTable[i].numberOfInjections);

      }
  delay(5000);
}



void Pump::configure(int &time_step) {
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
  }
  else{
    File pumpconfigFile = SD.open(PUMP_CONFIGFILE);
    doseTableSize = 0;  
    for (int i = 0; i < 3; i++) {
      Serial2.print("L,1\r");
      delay(500);
      Serial2.print("L,0\r");
      delay(500);
    }
  
    Serial.println( "reading config file...");
    while (pumpconfigFile.available()) {
      String line = pumpconfigFile.readStringUntil('\n');
      line.trim();

      if (line.length() == 0) continue; // skip empty lines

      int dashPos = line.indexOf('-');
      int firstComma = line.indexOf(',');
      int secondComma = line.indexOf(',', firstComma + 1);

      if (dashPos != -1 && firstComma != -1 && secondComma != -1 && doseTableSize < 10) {
        int lower = line.substring(0, dashPos).toInt();
        int upper = line.substring(dashPos + 1, firstComma).toInt();
        int dose = line.substring(firstComma + 1, secondComma).toInt();
        int numberOfInjections = line.substring(secondComma + 1).toInt();

        doseTable[doseTableSize].lower = lower;
        doseTable[doseTableSize].upper = upper;
        doseTable[doseTableSize].dose = dose;
        doseTable[doseTableSize].numberOfInjections = numberOfInjections;
        doseTableSize++;
      }
    }
    pumpconfigFile.close();

    File pumpdataFile = SD.open(PUMP_DATAFILE, FILE_APPEND);
    pumpdataFile.print("ID; INJECTION STATUS; PUMP COUNT");
    pumpdataFile.println();
    pumpdataFile.close();
    
    sendCommand("D,150");
    data.boot_step = (int)((30.0 / (float)time_step) + 0.5); // 3600 seconds in one hour
    if (data.boot_step == 0) data.boot_step = 1; // just in case
  }
}

void Pump::save_in_SD(int &bootCount){
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
  }
  else{
    String pumpdata_message = String(bootCount) + ";" + pump.latestErrorMessage + ";" + String(pumpActivationCount) + ";";
    File pumpdataFile = SD.open(PUMP_DATAFILE, FILE_APPEND);
    pumpdataFile.print(pumpdata_message);
    pumpdataFile.println();
    pumpdataFile.close();
  }

}

void Pump::handleInjections2(int &bootCount, int &time_step) {
  delay(1000);
  for (int i = 0; i < 4; i++){
    data.heights[i] = data.heights [i + 1];
  }
  data.heights[4] = measuredWaterheight;
  
  float heightsMean = 0.0;
  for (int i = 0; i < 5; ++i) {
      heightsMean += data.heights[i]/5;
  }
  
  if(Inject){
    for (int i = 0; i < doseTableSize; i++) {
      if (heightsMean >= doseTable[i].lower && heightsMean < doseTable[i].upper && doseTable[i].numberOfInjections > 0) {
        String pumpCommand = "D," + String(doseTable[i].dose);
        Serial.println(i);
        Serial.println(doseTable[i].dose);
        sendCommand(pumpCommand);
        Inject = false;
        Serial.println(pumpCommand);
        doseTable[i].numberOfInjections --;
        break;
      }
    }
  }
  else{
    counter ++;
    if (counter >= (int)(10800/time_step +0.5) ){
      Inject = true;
      counter = 0;
    }
  }
  save_in_SD(bootCount);
  Serial.print(latestErrorMessage);
  latestErrorMessage = "*OFF";
  Serial.print(latestErrorMessage);
  delay(1000);
  return;
}




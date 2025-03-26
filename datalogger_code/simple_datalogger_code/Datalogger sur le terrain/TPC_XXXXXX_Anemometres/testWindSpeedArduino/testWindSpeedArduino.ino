#include <SoftwareSerial.h>
SoftwareSerial mySerial(22, 21); //Define the soft serial port, port 3 is TX, port 2 is RX,

uint8_t  Address = 0x10;


void setup()
{
  Serial.begin(115200);
  mySerial.begin(9600);
}

void loop()
{
  Serial.print(readWindSpeed(Address)); //read wind speed
  Serial.println("m/s");
  delay(1000);
}


size_t readN(uint8_t *buf, size_t len)
{
  size_t offset = 0, left = len;
  int16_t Tineout = 1500;
  uint8_t  *buffer = buf;
  long curr = millis();
  while (left) {
    if (mySerial.available()) {
      buffer[offset] = mySerial.read();
      offset++;
      left--;
    }
    if (millis() - curr > Tineout) {
      break;
    }
  }
  return offset;
}

float readWindSpeed(uint8_t Address)
{
  uint8_t Data[7] = {0};   //Store raw data packets returned by sensors
  uint8_t COM[8] = {0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};   //command to read wind speed
  boolean ret = false;   //Wind speed acquisition success flag
  float WindSpeed = 0;
  long curr = millis();
  long curr1 = curr;
  uint8_t ch = 0;
  COM[0] = Address;    //Refer to the communication protocol to complete the instruction package.

  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < 6; pos++)
  {
    crc ^= (uint16_t)COM[pos];
    for (int i = 8; i != 0; i--)
    {
      if ((crc & 0x0001) != 0)
      {
        crc >>= 1;
        crc ^= 0xA001;
      }
      else
      {
        crc >>= 1;
      }
    }
  }
  COM[6] = crc % 0x100;
  COM[7] = crc / 0x100;
  mySerial.write(COM, 8);  //Send command to read wind speed

  while (!ret) {
    if (millis() - curr > 1000) {
      WindSpeed = -1;    //If the wind speed has not been read for more than 1000 milliseconds, it will be considered a timeout and return -1.
      break;
    }

    if (millis() - curr1 > 100) {
      mySerial.write(COM, 8);  //If the last command to read the wind speed is issued for more than 100 milliseconds and the return command has not been received, the command to read the wind speed will be sent again
      curr1 = millis();
    }
    if (readN(&ch, 1) == 1) {
      if (ch == Address) {          //read and judge the HEAD
        Data[0] = ch;
        if (readN(&ch, 1) == 1) {
          if (ch == 0x03) {         //read and judge the HEAD
            Data[1] = ch;
            if (readN(&ch, 1) == 1) {
              if (ch == 0x02) {       //read and judge the HEAD
                Data[2] = ch;      
                if (readN(&Data[3], 4) == 4) {
                  uint16_t crc = 0xFFFF;
                  for (int pos = 0; pos < 5; pos++)
                  {
                    crc ^= (uint16_t)Data[pos];
                    for (int i = 8; i != 0; i--)
                    {
                      if ((crc & 0x0001) != 0)
                      {
                        crc >>= 1;
                        crc ^= 0xA001;
                      }
                      else
                      {
                        crc >>= 1;
                      }
                    }
                  }
                  crc = ((crc & 0x00ff) << 8) | ((crc & 0xff00) >> 8);
                  if (crc == (Data[5] * 256 + Data[6])) {  //CRC
                    ret = true;
                    WindSpeed = (Data[3] * 256 + Data[4]) / 10.00;  //calculate the windspeed
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return WindSpeed;
}
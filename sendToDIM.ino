// demo: CAN-BUS Shield, send data
// loovee@seeed.cc
#include <mcp_can.h>
#include <SPI.h>
#include <SD.h>
#include "SevSeg.h"
SevSeg sevseg;

/*SAMD core*/
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
#define SERIAL SerialUSB
#else
#define SERIAL Serial
#endif

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;
const int SPI_CS_SD = 4;
unsigned char flagRecv = 0;
unsigned long lineCnt = 0;
const int buttonPin = A0;
const int pullPin = A1;
int buttonState = 0;
bool bRun = false;
MCP_CAN CAN(SPI_CS_PIN); // Set CS pin

void setup()
{
  byte numDigits = 2;
  byte digitPins[] = {11, 10};
  byte segmentPins[] = {5, A3, A5, A2, 6, 7, A4};
  bool resistorsOnSegments = true;
  byte hardwareConfig = COMMON_ANODE;
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(100);
  //SERIAL.begin(115200);
  pinMode(buttonPin, INPUT);
  pinMode(pullPin, INPUT_PULLUP);
  while (CAN_OK != CAN.begin(CAN_125KBPS)) // init can bus : baudrate = 500k
  {
    //SERIAL.println("CAN BUS Shield init fail");
    //SERIAL.println(" Init CAN BUS Shield again");
    sevseg.setNumber(99);
    sevseg.refreshDisplay();
  }
  //SERIAL.println("CAN BUS Shield init ok!");
  attachInterrupt(0, MCP2515_ISR, FALLING); // start interrupt
  if (!SD.begin(4))
  {
    //Serial.println("SD initialization failed!");
    sevseg.setNumber(99);
    sevseg.refreshDisplay();
    while (1)
      ;
  }
  //Serial.println("SD initialization done.");
  sevseg.setNumber(11);
  sevseg.refreshDisplay();
  delay(200);
}
void MCP2515_ISR()
{
  flagRecv = 1;
}

unsigned char stmp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char *buf;
unsigned int len = 1;
String addr = "";
String dataLine = "";
unsigned long address = 0;
unsigned int tDelay = 0;
unsigned int startLoc = 0;
unsigned long tStart = 0;
unsigned long tEnd = 0;
void loop()
{
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW)
  {
    //Serial.println("Start pressed!");
    bRun = true;
  }
  while (bRun)
  {
    tStart = millis();
    sevseg.setNumber(00);
    sevseg.refreshDisplay();
    File logFile = SD.open("drive1.txt");
    //File logFile = SD.open("3A04004.txt");
    if (logFile.seek(lineCnt) && logFile)
    {
      String line = logFile.readStringUntil('\n');
      lineCnt = lineCnt + 1 + line.length();
      startLoc = line.indexOf(",");
      addr = line.substring(0, startLoc);
      address = (long)strtoul(addr.c_str(), 0, 16);
      //Serial.println(address,HEX);
      startLoc = startLoc + 2;
      dataLine = line.substring(startLoc, line.indexOf("]"));
      for (int i = 0; i < (sizeof(stmp) / sizeof(stmp[0])); i++)
      {
        int nextLoc = dataLine.indexOf(",");
        String temp = dataLine.substring(0, nextLoc);
        dataLine = dataLine.substring(nextLoc + 1, dataLine.length());
        sevseg.refreshDisplay();
        unsigned char val = strtoul(temp.c_str(), 0, 16);
        stmp[i] = val;
      }
      String stDelay = line.substring(line.indexOf("],") + 2, line.length());
      tDelay = stDelay.toInt();
      logFile.flush();
      logFile.close();
      tEnd = millis();
      delay(tDelay-(tEnd-tStart)); //causing glitches, works without it too...
      CAN.sendMsgBuf(address, 1, 8, stmp);
    }
    else 
    {
      logFile.flush();
      logFile.close();
      startLoc = 0;
      tStart = 0;
    } 
  }
}

// END FILE

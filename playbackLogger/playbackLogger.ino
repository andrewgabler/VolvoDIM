#include <mcp_can_dfs.h>
#include <mcp_can.h>
#include <SD.h>
#include "SevSeg.h"
SevSeg sevseg; 


const int SPI_CS_PIN = 9;
const int SPI_CS_SD = 4;
unsigned char flagRecv = 0;
MCP_CAN CAN(SPI_CS_PIN); // Set CS pin
const int buttonPin = A0; 
const int pullPin = A1; 
int buttonState = 0; 
bool bRun = false;

void setup()
{
  byte numDigits = 2;
    byte digitPins[] = {11,10};
    byte segmentPins[] = {5,A3,A5,A2,6,7,A4};
    bool resistorsOnSegments = true;
    byte hardwareConfig = COMMON_ANODE; 
    sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
    sevseg.setBrightness(100);
    //Serial.begin(115200);
    pinMode(buttonPin, INPUT);
    pinMode(pullPin,INPUT_PULLUP);
START_INIT:
    //if (CAN_OK == CAN.begin(CAN_500KBPS)) //OBD2 Pins 6 & 14
    if (CAN_OK != CAN.begin(CAN_125KBPS)) //OBD2 Pins 3 & 11
    {
        //Serial.println("CAN BUS Shield init fail");
        //Serial.println("Init CAN BUS Shield again");
        sevseg.setNumber(99);
        sevseg.refreshDisplay(); 
        delay(1000);
        goto START_INIT;
    }
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
unsigned long lastTime = millis();

void loop()
{
  buttonState = digitalRead(buttonPin);
  if(buttonState == LOW){
    //Serial.println("Start pressed!");
    bRun = true;
  } 
  while(bRun){
    sevseg.setNumber(00);
    sevseg.refreshDisplay();
    unsigned char len = 0;
    unsigned char buf[8];
    if (CAN_MSGAVAIL == CAN.checkReceive()) // check if data coming
    {
        File logFile = SD.open("sweeplg2.txt", FILE_WRITE);
        CAN.readMsgBuf(&len, buf); // read data,  len: data length, buf: data buf
        unsigned long canId = CAN.getCanId();
        //Serial.println(canId, HEX);
        logFile.print(canId, HEX);
        logFile.print(",");
        logFile.print("[");
        for (int i = 0; i < len; i++) // print the data
        {
            logFile.print(buf[i], HEX);
            if(i < len-1){
              logFile.print(",");
            }
        }
        logFile.print("]");
        logFile.print(",");
        logFile.print(millis()-lastTime); //print time since last message
        lastTime = millis();
        logFile.println();
        logFile.close();
    }
  }
}

#include <mcp_can.h>
#include <SPI.h>
#include <SD.h>
#include "SevSeg.h" //optional for seven segment display
SevSeg sevseg; //optional for seven segment display

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
const int buttonPin = A0; //optional button to start playback
const int pullPin = A1; //optional button to start playback
int buttonState = 0; //optional button to start playback
bool bRun = false;
MCP_CAN CAN(SPI_CS_PIN); // Set CS pin

void setup()
{
  /* option seven segment number for info without a computer attached  */
  byte numDigits = 2;
  byte digitPins[] = {11, 10};
  byte segmentPins[] = {5, A3, A5, A2, 6, 7, A4};
  bool resistorsOnSegments = true;
  byte hardwareConfig = COMMON_ANODE;
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(100);
  /* Delete the code above if not using 2 seven segment displays*/
  //SERIAL.begin(115200);
  pinMode(buttonPin, INPUT); //button
  pinMode(pullPin, INPUT_PULLUP); //button
  /* 2007 Volvo S60 R Low-Speed 125kbps High-Speed 500kbps */
  while (CAN_OK != CAN.begin(CAN_125KBPS)) // init can bus : baudrate = 125k
  {
    //SERIAL.println("CAN BUS Shield init fail");
    //SERIAL.println(" Init CAN BUS Shield again");
    sevseg.setNumber(99);//display
    sevseg.refreshDisplay();//display
  }
  //SERIAL.println("CAN BUS Shield init ok!");
  attachInterrupt(0, MCP2515_ISR, FALLING); // start interrupt
  if (!SD.begin(4))
  {
    //Serial.println("SD initialization failed!");
    sevseg.setNumber(99); //display
    sevseg.refreshDisplay(); //display
    while (1)
      ;
  }
  //Serial.println("SD initialization done.");
  sevseg.setNumber(11);//display
  sevseg.refreshDisplay();//display
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
  /* Delete if not using button */
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW)
  {
    //Serial.println("Start pressed!");
    bRun = true;
  }
  /* ^Delete if not using button^ */
  while (bRun)
  {
    /* This is my first *real* project using C++ where I didn't have a guide to follow.
    Code is using easy but slow solutions for splitting lines and storing/converting strings.
    Should be updated and improved to be more efficient but my knowledge is not there yet and it
    is working well enough. 
    Seems to take about 15ms to process and send a message which is 
    how long the CAN network takes in between messages.*/ 

    //Format of lines being read in 0xFFFFFF,[0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00],19
    //Message Id, Data, Time since last sent message.
    tStart = millis();
    sevseg.setNumber(00); //display
    sevseg.refreshDisplay(); //display
    File logFile = SD.open("drive1.txt");
    //File logFile = SD.open("3A04004.txt");
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
        sevseg.refreshDisplay();//display
        unsigned char val = strtoul(temp.c_str(), 0, 16);
        stmp[i] = val;
      }
      String stDelay = line.substring(line.indexOf("],") + 2, line.length());
      tDelay = stDelay.toInt();
      logFile.flush();
      logFile.close();
      tEnd = millis();
      //delay(tDelay-(tEnd-tStart)); //causing glitches, works with or without... 
      //timing will change with code improvemnts 
      CAN.sendMsgBuf(address, 1, 8, stmp);
  }
}

// END FILE

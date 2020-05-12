#include <mcp_can.h>
#include <SPI.h>
#include <SD.h>
#include <math.h>

/*SAMD core*/
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
#define SERIAL SerialUSB
#else
#define SERIAL Serial
#endif

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10-
//9 = Arduino UNO CAN Shield 
//const int SPI_CS_PIN = 9;
//3 = Arduino MKR CAN Shield
const int SPI_CS_PIN = 3;
const int SPI_CS_SD = 4;
unsigned char flagRecv = 0;
unsigned long lineCnt = 0;
MCP_CAN CAN(SPI_CS_PIN); // Set CS pin

void setup()
{
  SERIAL.begin(115200); //Enable and disable serial logging 
  while(!SerialUSB); //Serial monitor must be open for program to run.
  //Prevents messages from being skipped because the arduino passes them before the serial connection is initialized.
  /* 2007 Volvo S60 R Low-Speed 125kbps High-Speed 500kbps */
  while (CAN_OK != CAN.begin(CAN_125KBPS)) // init can bus : baudrate = 125k
  {
    SERIAL.println("CAN BUS Shield init fail");
    SERIAL.println(" Init CAN BUS Shield again");
  }
  SERIAL.println("CAN BUS Shield init ok!");
  attachInterrupt(0, MCP2515_ISR, FALLING); // start interrupt
  if (!SD.begin(4))
  {
    Serial.println("SD initialization failed!");
    while (1)
      ;
  }
  Serial.println("SD initialization done.");
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
  while (true)
  {
    /* This is my first *real* project using C++ where I didn't have a guide to follow.
    Code is using easy but slow solutions for splitting lines and storing/converting strings.
    Should be updated and improved to be more efficient but my knowledge is not there yet and it
    is working well enough. 
    Seems to take about 15ms to process and send a message which is 
    how long the CAN network takes in between messages.*/ 

    //Format of lines being read in 0xFFFFFF,[0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00],19
    //Message Id, Data, Time since last sent message.
     unsigned char tempBrake[8] = {0x00,0x00,0xB0,0x60,0x30,0x00,0x00,0x00};
    CAN.sendMsgBuf(0x3600008, 1, 8, tempBrake);
    tStart = millis();
    File logFile = SD.open("drive1.txt");
    //File logFile = SD.open("A10408.txt");
    if(logFile.seek(lineCnt)){
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
      if(address == 0x217FFC || address == 0x2803008 || address == 0x3C01428 ||
      address == 3600008 ||address == 0x381526C || address == 0xA10408|| 
      address == 0x1A0600A ||address == 0x2616CFC ||address == 0x2006428 || address == 0x1017FFC){
        CAN.sendMsgBuf(address, 1, 8, stmp);
        //Serial.println(address,HEX);
      }
     
    }
  }
}

// END FILE

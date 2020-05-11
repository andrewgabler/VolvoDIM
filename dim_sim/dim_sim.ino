#include <mcp_can.h>
#include <SPI.h>
#include <math.h>
#include <time.h>
/*SAMD core*/
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
  #define SERIAL SerialUSB
#else
  #define SERIAL Serial
#endif
// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
//9 = Arduino UNO CAN Shield 
//const int SPI_CS_PIN = 9;
//3 = Arduino MKR CAN Shield
const int SPI_CS_PIN = 3;
MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

int cnt = 0;
unsigned char stmp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long address;
unsigned long addrLi[9] = {0x217FFC,0x2803008,0x3C01428,0x381526C,0x3600008,0xA10408,0x2006428,0x1A0600A,0x2616CFC};
/*
 * addrLi[0] = Speed/KeepAlive
 * addrLi[1] = RPM/Backlights
 * addrLi[2] = Coolant/OutdoorTemp
 * addrLi[3] = Time/GasTank
 * addrLi[4] = Brakes Keep alive?
 * addrLi[5] = Blinker
 * addrLi[6] = Anti-Skid
 * addrLi[7] = Aibag Light
 * addrLi[8] = 4C Error
 */
 
 //Refer to Excel Sheets for info about data values.
unsigned char defaultData[9][8] = {
  {0x01,0x4B,0x00,0xD8,0xF0,0x58,0x00,0x00}, //Speed/KeepAlive , 0x217FFC
  {0xFF,0xE1,0xFF,0xF0,0xFF,0xCF,0x00,0x00}, //RPM/Backlights , 0x2803008
  {0x81,0x81,0x51,0x89,0x0D,0xDC,0x00,0x00}, //Coolant/OutdoorTemp , 0x3C01428 //broken right now
  {0x00,0x01,0x05,0xBC,0x05,0xA0,0x40,0x40}, //Time/GasTank , 0x381526C
  {0x00,0x00,0xB0,0x60,0x30,0x00,0x00,0x00}, //Brake system Keep alive , 0x3600008
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, //Blinker , 0xA10408
  {0x01,0xE3,0xE0,0x00,0x00,0x00,0x00,0x00}, //Anti-Skid , 0x2006428
  {0x00,0x00,0x00,0x00,0x00,0xBE,0x49,0x00}, //Aibag Light , 0x1A0600A
  {0x0B,0x42,0x00,0x00,0xFD,0x1F,0x00,0xFF}  //4C keep alive / prevent error , 0x2616CFC
  };

/*
 * Time is 0-1440
 * every mumber is a minute
 * 24 hours 
 * AM - 0- 719
 * PM - 720-1440
 */
void updateTime(int inputTime){
  int b4;
  for(int i=0;i<6;i++){
    if(inputTime >= (i*256) && inputTime < ((i+1)*256)){
        b4 = i;
    }
  }
  int b5 = inputTime - (b4*256);
  defaultData[3][4] = b4;
  defaultData[3][5] = b5;
}

/*
 * input time as 2 2 didgit numbers 12, 43 = 12:43, 4, 24 = 4:24
 * AM = 1 is AM
 * Am = 0 is PM
 * returns time in updateTime format 0 - 1440
 */
int clockToDecimal(int hour, int minute, int AM){
  if(AM){
    if(hour == 12){
      return (minute);
    } else {
      return ((hour*60) + minute);
    }
  } 
  return ((hour*60) + minute)+720;
}

/*
 * Initialization data from logs for SRS sytem
 */
void initSRS(){
    unsigned char temp[8] = {0xC0,0x0,0x0,0x0,0x0,0xBC,0xDB,0x80};
    CAN.sendMsgBuf(0x1A0600A, 1, 8, temp);
    delay(20);
    temp[0]=0x0;
    CAN.sendMsgBuf(0x1A0600A, 1, 8, temp);
    delay(20);
    temp[0]=0xC0;
    temp[6] = 0xC9;
    CAN.sendMsgBuf(0x1A0600A, 1, 8, temp);
    delay(20);
    temp[0]=0x80;
    CAN.sendMsgBuf(0x1A0600A, 1, 8, temp);
}

/*
 * Generates a value for the SRS message.
 * Couldn't detect a pattern from my logs on the meaning of the first byte.
 * First byte is decided semi-randomly and is selected based on occureance in the logs.
 */
void genSRS(long address, byte stmp[]){
  int randomNum = random(0,794);
  if(randomNum >=0 && randomNum < 180){
    stmp[0]= (char) 0x40;
  }
  if(randomNum >=180 && randomNum < 370){
    stmp[0]= (char) 0xC0;
  }
  if(randomNum >=370 && randomNum < 575){
    stmp[0]= (char) 0x0;
  }
  if(randomNum >=575 && randomNum < 794){
    stmp[0]= (char) 0x80;
  }
  CAN.sendMsgBuf(address, 1, 8, stmp);
  delay(20);
}

/*
 * Init 4C message
 */
 void init4C(){
  unsigned char temp[8] = {0x09,042,0x0,0x0,0x0,0x50,0x00,0x00};
    CAN.sendMsgBuf(0x2616CFC, 1, 8, temp);
    delay(20);
    CAN.sendMsgBuf(0x2616CFC, 1, 8, temp);
    delay(20);
    temp[0]=0x0B;
    CAN.sendMsgBuf(0x2616CFC, 1, 8, temp);
    delay(20);
    temp[0]=0x0B;
    CAN.sendMsgBuf(0x2616CFC, 1, 8, temp);
 }
 
void setup()
{
    SERIAL.begin(115200);
    randomSeed(analogRead(0));
    while (CAN_OK != CAN.begin(CAN_125KBPS))              // init can bus : baudrate = 500k
    {
        SERIAL.println("CAN BUS Shield init fail");
        SERIAL.println(" Init CAN BUS Shield again");
        delay(100);
    }
    SERIAL.println("CAN BUS Shield init ok!");
    updateTime(clockToDecimal(random(0,13),random(0,60),1));
    initSRS();
    init4C();
}


void loop()
{
    address = addrLi[cnt];
    for(int i=0;i<8;i++){
      stmp[i] = defaultData[cnt][i];
    }
    if(address == 0x1A0600A){
      genSRS(address,stmp);
    }
    else {
      SERIAL.println(address);
      CAN.sendMsgBuf(address, 1, 8, stmp);
      delay(20);  // send data per 20ms
    }
    cnt++;
    if(cnt==9){
      cnt=0;
    }
}

// END FILE

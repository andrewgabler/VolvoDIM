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
MCP_CAN CAN(SPI_CS_PIN); // Set CS pin

int cnt = 0;
int listLen = 10;
int carConCnt = 0;
int configCnt = 0;
unsigned char stmp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long address;
unsigned long addrLi[10] = {0x217FFC, 0x2803008, 0x3C01428, 0x381526C, 0x3600008, 0xA10408, 0x2006428, 0x1A0600A, 0x2616CFC, 0x1017FFC};
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
 * addrLi[9] = Car Config
 */

//Refer to Excel Sheets for info about data values.
unsigned char defaultData[10][8] = {
    {0x01, 0x4B, 0x00, 0xD8, 0xF0, 0x58, 0x00, 0x00}, //Speed/KeepAlive , 0x217FFC
    {0xFF, 0xE1, 0xFF, 0xF0, 0xFF, 0xCF, 0x00, 0x00}, //RPM/Backlights , 0x2803008
    {0xC0, 0x80, 0x51, 0xBE, 0x0D, 0xD4, 0x00, 0x00}, //Coolant/OutdoorTemp , 0x3C01428 //broken right now coolant doesnt set
    {0x00, 0x01, 0x05, 0xBC, 0x05, 0xA0, 0x40, 0x40}, //Time/GasTank , 0x381526C
    {0x00, 0x00, 0xB0, 0x60, 0x30, 0x00, 0x00, 0x00}, //Brake system Keep alive , 0x3600008
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //Blinker , 0xA10408
    {0x01, 0xE3, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00}, //Anti-Skid , 0x2006428
    {0x00, 0x00, 0x00, 0x00, 0x00, 0xBE, 0x49, 0x00}, //Aibag Light , 0x1A0600A
    {0x0B, 0x42, 0x00, 0x00, 0xFD, 0x1F, 0x00, 0xFF}, //4C keep alive / prevent error , 0x2616CFC
    {0x01, 0x0F, 0xF7, 0xFA, 0x00, 0x00, 0x00, 0xC0}  //Car Config default , 0x1017FFC
};

//Sets the data for what the car is equiped with.
//Taken from a 2007 M66 SR with Climate package, rear parking sensors, no nav.
unsigned char carConfigData[16][8] = {
    {0x10, 0x10, 0x01, 0x03, 0x02, 0x01, 0x00, 0x01},
    {0x11, 0x18, 0x05, 0x05, 0x02, 0x03, 0x01, 0x05},
    {0x12, 0x03, 0x02, 0x02, 0x01, 0x02, 0x05, 0x01},
    {0x13, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x04},
    {0x14, 0x04, 0x01, 0x02, 0x01, 0x02, 0x03, 0x11},
    {0x15, 0x15, 0x03, 0x02, 0x02, 0x01, 0x01, 0x01},
    {0x16, 0x02, 0x62, 0x02, 0x01, 0x02, 0x01, 0x02},
    {0x17, 0x01, 0x02, 0x02, 0x01, 0x03, 0x03, 0x01},
    {0x18, 0x01, 0x05, 0x03, 0x03, 0x12, 0x04, 0x04},
    {0x19, 0x11, 0x02, 0x10, 0x01, 0x05, 0x01, 0x02},
    {0x1A, 0x01, 0x02, 0x01, 0x04, 0x01, 0x01, 0x03},
    {0x1B, 0x01, 0x02, 0x10, 0x01, 0x01, 0x01, 0x01},
    {0x1C, 0x01, 0x04, 0x01, 0x03, 0x01, 0x06, 0x01},
    {0x1D, 0x04, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01},
    {0x1E, 0x01, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01},
    {0x1F, 0x01, 0x02, 0x05, 0x02, 0x02, 0x22, 0x01}
};
/*
 * Time is 0-1440
 * every mumber is a minute
 * 24 hours 
 * AM - 0- 719
 * PM - 720-1440
 */
void updateTime(int inputTime)
{
  int b4;
  for (int i = 0; i < 6; i++)
  {
    if (inputTime >= (i * 256) && inputTime < ((i + 1) * 256))
    {
      b4 = i;
    }
  }
  int b5 = inputTime - (b4 * 256);
  defaultData[3][4] = b4;
  defaultData[3][5] = b5;
}

/*
 * input time as 2 2 didgit numbers 12, 43 = 12:43, 4, 24 = 4:24
 * AM = 1 is AM
 * Am = 0 is PM
 * returns time in updateTime format 0 - 1440
 */
int clockToDecimal(int hour, int minute, int AM)
{
  if (AM)
  {
    if (hour == 12)
    {
      return (minute);
    }
    else
    {
      return ((hour * 60) + minute);
    }
  }
  return ((hour * 60) + minute) + 720;
}
/*
 * Initialization data from logs for SRS sytem
 */
void initSRS()
{
  unsigned char temp[8] = {0xC0, 0x0, 0x0, 0x0, 0x0, 0xBC, 0xDB, 0x80};
  unsigned long tempAddr = 0x1A0600A;
  CAN.sendMsgBuf(tempAddr, 1, 8, temp);
  delay(20);
  temp[0] = 0x0;
  CAN.sendMsgBuf(tempAddr, 1, 8, temp);
  delay(20);
  temp[0] = 0xC0;
  temp[6] = 0xC9;
  CAN.sendMsgBuf(tempAddr, 1, 8, temp);
  delay(20);
  temp[0] = 0x80;
  CAN.sendMsgBuf(tempAddr, 1, 8, temp);
}

/*
 * Generates a value for the SRS message.
 * Couldn't detect a pattern from my logs on the meaning of the first byte.
 * First byte is decided semi-randomly and is selected based on occureance in the logs.
 */
void genSRS(long address, byte stmp[])
{
  int randomNum = random(0, 794);
  if (randomNum >= 0 && randomNum < 180)
  {
    stmp[0] = (char)0x40;
  }
  else if (randomNum >= 180 && randomNum < 370)
  {
    stmp[0] = (char)0xC0;
  }
  else if (randomNum >= 370 && randomNum < 575)
  {
    stmp[0] = (char)0x0;
  }
  else if (randomNum >= 575 && randomNum < 794)
  {
    stmp[0] = (char)0x80;
  }
  CAN.sendMsgBuf(address, 1, 8, stmp);
  delay(20);
}

/*
 * Init 4C message
 */
void init4C()
{
  unsigned char temp[8] = {0x09, 042, 0x0, 0x0, 0x0, 0x50, 0x00, 0x00};
  CAN.sendMsgBuf(0x2616CFC, 1, 8, temp);
  delay(20);
  CAN.sendMsgBuf(0x2616CFC, 1, 8, temp);
  delay(20);
  temp[0] = 0x0B;
  CAN.sendMsgBuf(0x2616CFC, 1, 8, temp);
  delay(20);
  temp[0] = 0x0B;
  CAN.sendMsgBuf(0x2616CFC, 1, 8, temp);
}
/*
  * Generate the car config. May be mileage from CEM, not sure.
  * Sets last 2 bytes randomly based on random occurance in the logs.
  */
void genCC(long address, byte stmp[])
{
  int randomNum = random(0, 7);
  if (randomNum > 3)
  {
    stmp[6] = (char)0xFF;
    stmp[7] = (char)0xF3;
  }
  CAN.sendMsgBuf(address, 1, 8, stmp);
  delay(20);
}
/*
 * Generates random values for temp message
 */
 void genTemp(long address, byte stmp[]){
  int randomNum = random(0, 133);
  if (randomNum < 16)
  {
    stmp[0] = (char)0x00;
  } 
  else if(randomNum >= 16 && randomNum < 41)
  {
    stmp[0] = (char)0x40;
  }
  else if(randomNum >=41 && randomNum < 76)
  {
    stmp[0] = (char)0xC0;
  }
  else if(randomNum >=76 && randomNum < 133)
  {
    stmp[0] = (char)0x80;
  }
  if(randomNum < 76){
    stmp[1] = (char)0x80;
  } 
  else 
  {
    stmp[1] = (char)0x00;
  }
  CAN.sendMsgBuf(address, 1, 8, stmp);
  delay(20);
 }
 /*
  * Convert Celsius to Fahrenheit
  */
double celsToFahr(double temp){
  return ((temp * (9/5))+32);
}
 /*
  * Set the digital outdoor temperature with fahrenheit input 
  * (Will always display on dim in fahrenheit)
  */
void setTemp(int oTemp){
  if(!(oTemp < -49 || oTemp > 176))
  {
    if(oTemp > -50 && oTemp <= 32){
      defaultData[2][4] = 0x0D;
      defaultData[2][5] = ceil((oTemp+83)*2.21);
    }
    else if(oTemp > 32 && oTemp <= 146)
    {
      defaultData[2][4] = 0x0E;
      defaultData[2][5] = ceil((oTemp-33)*2.25);
    }
    else if(oTemp > 146 && oTemp < 177)
    {
      defaultData[2][4] = 0x0F;
      defaultData[2][5] = ceil((oTemp-147)*2.20);
    } 
  } 
  else 
  {
    //SERIAL.println("Temp out of range");
  }
  
}

void setup()
{
  SERIAL.begin(115200);
  //while(!SerialUSB); //Serial monitor must be open for program to run.
  //Prevents messages from being skipped because the arduino passes them before the serial connection is initialized.
  randomSeed(analogRead(0));
  while (CAN_OK != CAN.begin(CAN_125KBPS)) // init can bus : baudrate = 500k
  {
    SERIAL.println("CAN BUS Shield init fail");
    SERIAL.println("Init CAN BUS Shield again");
    delay(100);
  }
  SERIAL.println("CAN BUS Shield init ok!");
  updateTime(clockToDecimal(random(0, 13), random(0, 60), 1));
  setTemp(random(-49,177));
  initSRS();
  init4C();
}

void loop()
{
  address = addrLi[cnt];
  for (int i = 0; i < 8; i++)
  {
    stmp[i] = defaultData[cnt][i];
  }
  if (address == 0x1A0600A)
  {
    genSRS(address, stmp);
  }
  else if(address == 0x3C01428)
  {
    genTemp(address, stmp);
  }
  else if (address == 0x1017FFC)
  {
    if (carConCnt % 12 == 1)
    {
      for (int i = 0; i < 8; i++)
      {
        stmp[i] = carConfigData[configCnt][i];
      }
      configCnt++;
    }
    else
    {
      genCC(address, stmp);
    }
  }
  else
  {
    //SERIAL.println(address);
    CAN.sendMsgBuf(address, 1, 8, stmp);
    delay(20); // send data per 20ms
  }
  cnt++;
  if (cnt == listLen)
  {
    if (configCnt >= 16)
    {
      carConCnt = 0;
    }
    else
    {
      carConCnt++;
    }
    cnt = 0;
  }
}

// END FILE

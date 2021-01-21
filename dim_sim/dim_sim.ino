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
int genCnt = 0;
int cnt = 0;
int listLen = 10;
int carConCnt = 0;
int configCnt = 0;
int blinkerInterval = 0;
bool leftBlinker = false, rightBlinker = false;
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
  {0x01, 0x4B, 0x00, 0xD8, 0xF0, 0x58, 0x00, 0x00}, //0, Speed/KeepAlive , 0x217FFC
  {0xFF, 0xE1, 0xFF, 0xF0, 0xFF, 0xCF, 0x00, 0x00}, //1, RPM/Backlights , 0x2803008
  {0xC0, 0x80, 0x51, 0x89, 0x0D, 0xD4, 0x00, 0x00}, //2, Coolant/OutdoorTemp , 0x3C01428
  {0x00, 0x01, 0x05, 0xBC, 0x05, 0xA0, 0x40, 0x40}, //3, Time/GasTank , 0x381526C
  {0x00, 0x00, 0xB0, 0x60, 0x30, 0x00, 0x00, 0x00}, //4, Brake system Keep alive , 0x3600008
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //5, Blinker , 0xA10408
  {0x01, 0xE3, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00}, //6, Anti-Skid , 0x2006428
  {0x00, 0x00, 0x00, 0x00, 0x00, 0xBE, 0x49, 0x00}, //7, Aibag Light , 0x1A0600A
  {0x0B, 0x42, 0x00, 0x00, 0xFD, 0x1F, 0x00, 0xFF}, //8, 4C keep alive / prevent error , 0x2616CFC
  {0x01, 0x0F, 0xF7, 0xFA, 0x00, 0x00, 0x00, 0xC0}  //9, Car Config default , 0x1017FFC
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
  if(inputTime >=0 && inputTime <= 1440){
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
  else 
  {
    //SERIAL.println("Not a valid time");
  }
}

/*
 * input time as 2 2 didgit numbers 12, 43 = 12:43, 4, 24 = 4:24
 * AM = 1 is AM
 * Am = 0 is PM
 * returns time in updateTime format 0 - 1440
 */
int clockToDecimal(int hour, int minute, int AM)
{
  if((hour >=0 && hour <= 12) && (minute >=0 && minute < 60) && (AM == 1 || AM == 0)){
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
  else 
  {
    //SERIAL.println("Not a valid time");
    return 0;
  }
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
  randomNum = random(0, 105);
  if (randomNum < 7)
  {
    stmp[2] = (char)0x11;
  } 
  else if(randomNum >= 7 && randomNum < 15)
  {
    stmp[2] = (char)0x71;
  }
  else if(randomNum >=15 && randomNum < 25)
  {
    stmp[2] = (char)0x61;
  }
  else if(randomNum >=25 && randomNum < 60)
  {
    stmp[2] = (char)0x51;
  }
  else if(randomNum >=60&& randomNum < 105)
  {
    stmp[2] = (char)0x41;
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
void setOutdoorTemp(int oTemp){
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
/*
 * Sets the coolant gauge from low to high
 * Accepts value 0-100 low to high.
 */
void setCoolantGauge(int range){
  if(range >= 0 && range <= 55 ){
    defaultData[2][3] = range + 88;
  }
  else if(range > 55  && range <= 100 ){
    //upper limit scales very slowly.
    //This calculation makes it fit the 0-100 scale much better.
    defaultData[2][3] = ceil((range-55)*.333) + 173;
  }
  else
  {
    //SERIAL.println("Value out of range");
  }
}
/*
 * Sets the speed in mph
 */
 void setCarSpeed(int carSpeed){
  if(carSpeed >= 0 && carSpeed <= 160){
    if(carSpeed >= 0 && carSpeed <= 40){
      defaultData[0][5] = 0x58;
      defaultData[0][6] = round(carSpeed * 6.375);
    }
    else if(carSpeed > 40 && carSpeed <= 80)
    {
      defaultData[0][5] = 0x59;
      defaultData[0][6] = round(carSpeed * 6.375);
    }
    else if(carSpeed > 80 && carSpeed <= 120)
    {
      defaultData[0][5] = 0x5A;
      defaultData[0][6] = round(carSpeed * 6.375);
    }
    else if(carSpeed > 120 && carSpeed <= 160)
    {
      defaultData[0][5] = 0x5B;
      defaultData[0][6] = round(carSpeed * 6.375);
    }
  } 
  else 
  {
    //SERIAL.println("Speed out of range");
  }
 }
 /*
  * Set gas gauge level 0-100
  */
 void setGasLevel(int level){
  if(level >=0 && level <= 100){
    defaultData[3][6] = round(level * .64 );
    defaultData[3][7] = round(level * .64 );
  } 
  else 
  {
    //SERIAL.println("Gas level out of range");
  }
 }
/*
 * Set RPMs 502-8000
 * Will not match gauge precicely, but close as can be.
 * 501 and lower = 0x01 which is a no go.
 * Very weird system for rpm it counts 0-255 by increments of 32 and rolls over 
 */
void setRpm(int rpm){
  if(rpm > 501 && rpm <= 8000){
    defaultData[1][6] = floor((rpm*.031875)/32) * 32  + floor((rpm*.031875)/8);
  } 
  else 
  {
    //SERIAL.println("RPM out of range");
  }
  
}
/*
 * Set Overhead led brightness 0-256
 */
 void setOverheadBrightness(int value){
  if(value >= 0 && value <= 256){
    defaultData[1][2] = value;
  }
  else 
  {
    //SERIAL.println("Value out of range");
  }
 }
 /*
 * Set LCD brightness 0-256
 */
 void setLcdBrightness(int value){
  if(value >= 0 && value <= 256){
    defaultData[1][4] = round(value/32);
  }
  else 
  {
    //SERIAL.println("Value out of range");
  }
 }
 /*
 * Set brightness of all lights
 */
 void setTotalBrightness(int value){
  if(value >= 0 && value <= 256){
    setLcdBrightness(value);
    setOverheadBrightness(value);
  }
  else 
  {
    //SERIAL.println("Value out of range");
  }
 }
/*
   Set left blinker to ON or OFF
*/
void setLeftBlinker(bool state) {
  if (state) {
    defaultData[5][7] = 0x0A;
  } else {
    defaultData[5][7] = 0x0;
  }

  leftBlinker = state;
}
/*
   Set blink right to ON or OFF
*/
void setRightBlinker(bool state) {
  if (state) {
    defaultData[5][7] = 0x0C;
  } else {
    defaultData[5][7] = 0x0;
  }
  rightBlinker = state;
}
/*

*/
void genBlinking(long address, byte stmp[], bool isBlinking, int interval, int blinkSpeed) {
  int blinkRatio = 7;
  if (blinkSpeed == 0) {
    blinkRatio = 5;
  } else if (blinkSpeed == 2) {
    blinkRatio = 15;
  } else if (blinkSpeed == 3) {
    blinkRatio = 50;
  }
  if (isBlinking && (interval % blinkRatio == 0)) {
    int prevState = 0;
    if(defaultData[5][7] == 0x0A || defaultData[5][7] == 0x0C){
      prevState = 1;
    }
    //currently just leaves both on
    if (leftBlinker) {
      if (prevState == 1) {
        defaultData[5][7] = 0x0E;
      } else {
        defaultData[5][7] = 0x10;
      }
      stmp[7] = defaultData[5][7];
      CAN.sendMsgBuf(address, 1, 8, stmp);
    }
    //currently properly blinks the right blinker
    if (rightBlinker) {
      if (prevState == 1) {
        defaultData[5][7] = 0x08;
      } else {
        defaultData[5][7] = 0x0C;
      }
      stmp[7] = defaultData[5][7];
      CAN.sendMsgBuf(address, 1, 8, stmp);
    }
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
  initSRS();
  init4C();
  //updateTime(clockToDecimal(random(0, 13), random(0, 60), random(0,2)));
  //(random(-49,177));
  //setCoolantGauge(random(0,100));
  //setCarSpeed(random(0,160));
  //setGasLevel(random(0,100));
  //setRpm(random(502,8000));
  //setTotalBrightness(random(0,257));
  updateTime(clockToDecimal(1, 30, 1));
  setOutdoorTemp(72);
  setCoolantGauge(82);
  setCarSpeed(72);
  setGasLevel(90);
  setRpm(3400);
  setTotalBrightness(256);
  setLeftBlinker(true);
}
/*
 * Main message organization 
 */
void simDim(){
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
  else if (address == 0xA10408) {
    genBlinking(address, stmp, true, blinkerInterval, 1);
  }
  else
  {
    //SERIAL.println(address);
    CAN.sendMsgBuf(address, 1, 8, stmp);
    delay(20); // send data per 20ms
  }
  cnt++;
  blinkerInterval++;
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
  if (blinkerInterval >= 50) {
    blinkerInterval = 0;
  }
}

void loop()
{
  simDim();
}

// END FILE

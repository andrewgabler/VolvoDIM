// demo: CAN-BUS Shield, send data
// loovee@seeed.cc

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
int cnt = 0;
unsigned long addrLi[6] = {0x217FFC,0x2803008,0x3C01428,0x381526C,0x3600008,0xA10408};
/*
 * addrLi[0] = Speed/KeepAlive
 * addrLi[1] = RPM/Backlights
 * addrLi[2] = Coolant/OutdoorTemp
 * addrLi[3] = Time/GasTank
 * addrLi[4] = Brakes Keep alive?
 * addrLi[5] = Blinker
 */
 //Refer to Excel Sheets for info about data values.
unsigned char defaultData[6][8] = {
  {1,0x4B,0x0,0xD8,0xF0,88,0,0}, //Speed/KeepAlive , 0x217FFC
  {255,0xE1,255,0xF0,255,0xCF,0x0,0x0}, //RPM/Backlights , 0x2803008
  {0x81,0x81,33,150,13,220,0x0,0x0}, //Coolant/OutdoorTemp , 0x3C01428 //broken right now
  {0,1,5,0xBC,5,0xA0,64,64}, //Time/GasTank , 0x381526C
  {0x0,0x0,0xB0,60,30,0x0,0x0,0x0}, //Brakes Keep alive? , 0x3600008
  {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0} //Blinker , 0xA10408
  };
// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin


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


void setup()
{
    SERIAL.begin(115200);

    while (CAN_OK != CAN.begin(CAN_125KBPS))              // init can bus : baudrate = 500k
    {
        SERIAL.println("CAN BUS Shield init fail");
        SERIAL.println(" Init CAN BUS Shield again");
        delay(100);
    }
    SERIAL.println("CAN BUS Shield init ok!");
    updateTime(clockToDecimal(6,45,1));
}



unsigned char stmp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long address;
void loop()
{
    address = addrLi[cnt];
    for(int i=0;i<8;i++){
      stmp[i] = defaultData[cnt][i];
    }
    CAN.sendMsgBuf(address, 1, 8, stmp);
    delay(15);  // send data per 15ms
    cnt++;
    if(cnt==5){
      cnt=0;
    }
}

// END FILE

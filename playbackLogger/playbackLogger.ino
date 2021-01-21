#include <mcp_can_dfs.h>
#include <mcp_can.h>
#include <SD.h>


//9 = Arduino UNO CAN Shield 
//const int SPI_CS_PIN = 9;
//3 = Arduino MKR CAN Shield
const int SPI_CS_PIN = 3;
const int SPI_CS_SD = 4;
unsigned char flagRecv = 0;
MCP_CAN CAN(SPI_CS_PIN); // Set CS pin

void setup()
{
    Serial.begin(115200);
START_INIT:
    //if (CAN_OK == CAN.begin(CAN_500KBPS)) //OBD2 Pins 6 & 14
    if (CAN_OK != CAN.begin(CAN_125KBPS)) //OBD2 Pins 3 & 11
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println("Init CAN BUS Shield again");
        goto START_INIT;
    }
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
unsigned long lastTime = millis();

void loop()
{
    unsigned char len = 0;
    unsigned char buf[8];
    if (CAN_MSGAVAIL == CAN.checkReceive()) // check if data coming
    {
        //keep file name under 8 characters
        File logFile = SD.open("joystic2.txt", FILE_WRITE);
        CAN.readMsgBuf(&len, buf); // read data,  len: data length, buf: data buf
        unsigned long canId = CAN.getCanId();
        //Serial.println(canId, HEX);
        logFile.print(canId, HEX);
        logFile.print(",");
        logFile.print("[");
        for (int i = 0; i < len; i++) 
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

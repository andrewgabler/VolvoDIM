#include <SD.h>
#include "mcp2515_can.h"
#include <mcp_can.h>
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
#endif

//9 = Arduino UNO CAN Shield
//const int SPI_CS_PIN = 9;
//3 = Arduino MKR CAN Shield
const int SPI_CS_PIN = 3;
const int SPI_CS_SD = 4;
unsigned char flagRecv = 0;
mcp2515_can CAN(SPI_CS_PIN);  // Set CS pin
const int saveToSD = 0;

void setup() {
  Serial.begin(115200);  //Make sure to change your serial monitor to this rate
  while (!Serial)
    ;  //Wait for serial to init for next messages.
  Serial.println("Starting INIT");
START_INIT:
  /* 2007 Volvo S60 R Low-Speed 125kbps High-Speed 500kbps */
  /*2007 Volvo XC70 Low-Speed 125kbps High-Speed 500kbps */
  //if (CAN_OK == CAN.begin(CAN_500KBPS)) //OBD2 Pins 6 & 14 - High Speed Network
  if (CAN_OK != CAN.begin(CAN_125KBPS, MCP_16MHz))  //OBD2 Pins 3 & 11 - Low Speed Network
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println("Init CAN BUS Shield again");
    goto START_INIT;
  } else {
    Serial.println("Can-Bus Success");
  }
  attachInterrupt(0, MCP2515_ISR, FALLING);  // start interrupt
  if (saveToSD) {
    if (!SD.begin(4)) {
      Serial.println("SD initialization failed!");
      goto START_INIT;
    }
    Serial.println("SD initialization done.");
  }
  delay(200);
}

void MCP2515_ISR() {
  flagRecv = 1;
}
unsigned long lastTime = millis();

void loop() {
  unsigned char len = 0;
  unsigned char buf[8];
  if (CAN_MSGAVAIL == CAN.checkReceive())  // check if data coming
  {
    if (saveToSD) {
      //keep file name under 8 characters or it wont create a file
      File logFile = SD.open("srvrst.txt", FILE_WRITE);
      CAN.readMsgBuf(&len, buf);  // read data,  len: data length, buf: data buf
      unsigned long canId = CAN.getCanId();
      logFile.print(canId, HEX);
      logFile.print(",");
      logFile.print("[");
      for (int i = 0; i < len; i++) {
        logFile.print(buf[i], HEX);
        if (i < len - 1) {
          logFile.print(",");
        }
      }
      logFile.print("]");
      logFile.print(",");
      logFile.print(millis() - lastTime);  //print time since last message
      lastTime = millis();
      logFile.println();
      logFile.close();
    } else {
      CAN.readMsgBuf(&len, buf);  // read data,  len: data length, buf: data buf
      unsigned long canId = CAN.getCanId();
      Serial.println(canId, HEX);
      for (int i = 0; i < len; i++) {
        Serial.print(buf[i], HEX);
        if (i < len - 1) {
          Serial.print(",");
        }
      }
    }
  } else {
    Serial.println("No CAN data being read");
  }
}

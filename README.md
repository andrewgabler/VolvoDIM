# VolvoDIM
Arduino project to power a volvo DIM outside of the car.
Tested on a 2007 S60R DIM
Tested on a 2005 S80 DIM

# Tools needed 
### Arduino Uno or MKR
- Uno - https://www.amazon.com/gp/product/B008GRTSV6/
- MKR - https://www.amazon.com/gp/product/B07FYFF5YZ/
### Ardunio Can Shield
- Uno CAN Shield - https://www.amazon.com/gp/product/B076DSQFXH/
- MKR CAN Shield - https://www.amazon.com/gp/product/B07HNKCDCN/
### Arduino SD card shield (optional - for logging and replaying)
- UNO SD Card Shield - https://www.amazon.com/gp/product/B006LRR0IQ/
- MKR SD CARD Shield - https://www.amazon.com/gp/product/B07HNKCDCN/
- For MKR SD Shield 12v power as seen in youtube videos - https://www.amazon.com/gp/product/B074LK7G86/
### Misc
- SD Cards - https://www.amazon.com/gp/product/B08KSSX9PH/
- Breadboard jumper wires - https://www.amazon.com/gp/product/B07GD2BWPY/

# Libraries needed 
- CAn BUS Seede Studio https://www.arduino.cc/reference/en/libraries/can-bus-shield/
- SD
- SPI
- Math
- Time

# CANFileLogger
Watch a video on this here - https://www.youtube.com/watch?v=7YRgJ1NKBkg

This program can be used to log either the high or low speed networks of your volvo. 
Configure the code and change the CAN speed for which network you plan to log, then plug your arduinos CAN H and CAN L into pins 3 (high) and 11 (low) for the low speed network and 6 (high) and 14 (low) for the high speed network.

Be sure to edit the code and change the name of the text file that is generated. It must be 8 characters or short to work with the SD card library.

# CANLogReplayer
Watch a video on this here - https://www.youtube.com/watch?v=Jm3e2p2rOZw

This program can be used to "replay" the log from the created CANFileLogger text file on your DIM that is not connected to the car.
You need to modify the code and supply the name of the text file to read from and then connect the CAN H and CAN L along with 12 v + and - to your DIM.

In its current state this program works on and off. It may take some time for your log to begin replaying (usually under a minute).
It also will display the brake failure error message and the srs airbag message, this is because the timing of the messages being sent is not 100% accurate and the DIM thinks it is having communication issues with those modules.

This is fixed for the simulator implementation, but not on this file. If you need or want to replay your logs without error look into contributing to the project. The simple fix would be modifying the fix from DimSimulator and transforming it to work with this solution.

# DimSimulator
Watch a video on this here - https://www.youtube.com/watch?v=d4Ssp-XOB1Q

Currently this solution will allow you to simulate a number of the functions of the DIM. You need to connect your arduino to the DIMs CAN H and CAN L and the 12 v power + and -. 
Here follows a list of all of the functions that can currently be controlled on the DIM (in no particular order)
- Time
- Outdoor temperature
- RPM
- Speed
- Coolant / Engine temperature
- Gas Gauge
- Background lights 
- Above Lights
- Left, Right, and hazards blinkers

## Still needing implementation
- Orange warning triangle
- Red warning light
- Door Chime
- Auto transmission gear display
- All check engine lights
- Custom text in the lower left window
- Incrementing the milage (is possible)
- Trip counter being visually functional (not certain button will work)
- Future implementing with racing sim software ex SimHub

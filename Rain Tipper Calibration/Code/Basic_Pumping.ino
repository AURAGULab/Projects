//basically the same comments in I2C_to_UART.ino could be used in this code below 
#include<Wire.h>

int add = 0x67;

void setup()
{
//here we will tell the pump to maintain a constant flow rate of 10 mL/min for 5 min
  Wire.begin();
  Wire.beginTransmission(add);
  Wire.write(0x58);
  Wire.endTransmission();
  Wire.beginTransmission(add);  
  Wire.write(0x44);//D
  Wire.write(0x43);//C
  Wire.write(0x2C);//,
  Wire.write(0x35);//5
  Wire.write(0x30);//0
  Wire.write(0x2C);//,
  Wire.write(0x31);//1
Wire.endTransmission();
}

void loop() {}
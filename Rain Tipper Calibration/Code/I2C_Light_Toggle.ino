//basically the same comments in I2C_to_UART.ino could be used in this code below 
#include<Wire.h>

int add = 0x67;

void setup()
{
  Wire.begin();
}

void loop() 
{
  
  Wire.beginTransmission(add);
  Wire.write(0x4C);//L
  Wire.write(0x2C);//,
  Wire.write(0x30);//0
  Wire.endTransmission();
  
  delay(5000);//wait 5 sec
  
  Wire.beginTransmission(add);
  Wire.write(0x4C);//L
  Wire.write(0x2C);//,
  Wire.write(0x30);//0
  Wire.endTransmission();
    
  delay(5000);//wait 5 sec

}
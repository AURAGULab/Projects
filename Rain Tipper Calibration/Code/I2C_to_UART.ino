#include<Wire.h>  //this is the I2C library

int add = 0x67; //the address we will use

void setup()
{
  Serial.begin(9600); //this will allow us to print to the serial monitor
  Wire.begin(); //start running I2C library 
  delay(1000);  //wait for a second
  Wire.beginTransmission(add);  //start transmission
  //these writes send the character in ASCII to the pump 
  Wire.write(0x42);//B
  Wire.write(0x32);//a
  Wire.write(0x43);//u
  Wire.write(0x64);//d
  Wire.write(0x2C);//,
  Wire.write(0x31);//9
  Wire.write(0x30);//6
  Wire.write(0x34);//0
  Wire.write(0x30);//0
  Wire.endTransmission();  //end transmission
  Serial.println("done");  //this tells us in the serial monitor were done
}

void loop(){}
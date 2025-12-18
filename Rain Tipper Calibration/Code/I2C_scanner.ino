#include<Wire.h>

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  while(!Serial);
  Serial.println("Scanning!");
  
  for(int add = 0; add < 128; add++)
  {
    delay(25);
    Wire.beginTransmission(add);
    if (Wire.endTransmission() == 0){
      Serial.print("0x");
      Serial.println(add,HEX);
    }
  }
  Serial.println("done!!!");
}

void loop(){}

#include <Wire.h>
#include <SPI.h>

byte enableReg      = 0x00; //Enable register address
byte controlReg     = 0x01; //Control register address
byte IDReg          = 0x12; //ID register address
byte Ch0Reg         = 0x14; //Channel 0 Low Byte register address
byte Ch1LReg        = 0x16; //Channel 1 (IR) Low Byte register address
float SRDG_LEDOn  = 0;    //Sensor 90 Reading - LED On
float SRDG_LEDOff = 0;    //Sensor 90 Reading - LED Off
const int LED = 7;          //Output pin to control IR LED

void setup() 
{
  Serial.begin(9600);
  delay(500);
  Wire.begin();            //Start I2C comms
  delay(1000);
  pinMode(LED, OUTPUT);    //Output pin controls IR LED
  delay(1000);             //stall
  while(!Serial);
  for (int add = 0; add < 128; add++)
  {
    Wire.beginTransmission(add);
    if (Wire.endTransmission() == 0)
    {
      Serial.print("I2C Device found: ");
      Serial.println(add, HEX);
      delay(20);
    }
  }
  int device = configAndRead();  //Configure sensor; read its device ID which should be 0x50
  Serial.print("0x");
  Serial.println(device, HEX);
  delay(1000);
}

void loop() 
{ 
  //Measure Turbidity/Light Sensor
  //Turn ON LED
  digitalWrite(LED, HIGH);              //LED On
  delay(5000);                           //Wait a while...
  SRDG_LEDOn = readS();             //Read S90
  //Turn OFF LED
  digitalWrite(LED, LOW);               //LED Off
  delay(5000);                           //Wait a while...
  SRDG_LEDOff = readS();            //Read S90
  //Compute difference and display
  float deltaRDGOnOff = SRDG_LEDOn - SRDG_LEDOff;
  Serial.print(SRDG_LEDOn);
  Serial.print("\t");
  Serial.print(SRDG_LEDOff);
  Serial.print("\t");
  Serial.println(deltaRDGOnOff);
}

int configAndRead()
{
  Wire.beginTransmission(0x29);  //Device I2C address: 0x29
  Wire.write(0xA0 | enableReg);  //Enable register
  Wire.write(0x03);            
  Wire.endTransmission();

  Wire.beginTransmission(0x29);  //Device I2C address: 0x29
  Wire.write(0xA0 | controlReg); //Config/control register
  Wire.write(0X03);              //lOW gain, 400ms integration
  Wire.endTransmission();
  
  Wire.beginTransmission(0x29);  
  Wire.write(0xA0 | IDReg);      //Read Device ID: 0x50
  Wire.endTransmission();
  Wire.requestFrom(0x29, 1);     //Repeated start
  int deviceID0 = Wire.read();   //Should be 80 = 0x50
  Wire.endTransmission();
  return deviceID0;
}
float readS()
{
  int Ch0RDGSensor = 0;
  float Ch0RDGSensorAvg = 0;
  for (int ii = 0; ii < 50 ; ii++)
  {
    Wire.beginTransmission(0x29); 
    Wire.write(0xA0 | Ch0Reg);
    Wire.endTransmission();
    Wire.requestFrom(0x29, 4);  //Repeated start
    int null1   = Wire.read();
    int null2   = Wire.read();
    int lowCh0  = Wire.read();
    int highCh0 = Wire.read();
    Wire.endTransmission();
    Ch0RDGSensor = highCh0<<8 | lowCh0;
    Ch0RDGSensorAvg += Ch0RDGSensor;
    delay(100);
  }
  Ch0RDGSensorAvg = Ch0RDGSensorAvg/50.0;
  return Ch0RDGSensorAvg;
}

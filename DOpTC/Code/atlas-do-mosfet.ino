#include <ThingSpeak.h>
#include <ArduinoJson.h>
#include "Particle.h"
SYSTEM_MODE(AUTOMATIC);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

#include <Wire.h>;
#include <SPI.h>;

//Atlas universals
const int ORPadd = 0x62;
const int pHadd = 0x63;
const int Cadd = 0x64;
byte code = 0;  byte ii = 0;  byte charIn = 0;
int Delay = 2000;  //Delay 2 sec
int TD = 700;      //Tiny Delay 700 ms
char sensorData[20];
float data;

//DO sensor universals
#define mySerial Serial1
uint8_t Com[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x06, 0xC5, 0xC8};
int ctr = 0;
const int DO_Power = D7;
float mgL_DO = 0.0;

//FuelGauge fuel;
SystemSleepConfiguration config;    //Low power universal for Particle

//Functions
int calibrate(String command);
char buf[250];

void setup() 
{
  Particle.function("calibrate", calibrate);
  Serial.begin(9600);
  Wire.begin();
  mySerial.begin(4800); //comm rate default for DO
  pinMode(DO_Power, OUTPUT);
  digitalWrite(DO_Power, HIGH);
  //Set up Particle sleep for 10 minutes
  config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(10min);
  delay(Delay);
}

void loop() 
{   Particle.publish("Begin loop");    //tell when device starts loop and wakes up
    delay(8000); //wait 8 sec to fully wake up
    //DO Probe
    for (int ii = 0; ii < 8; ii++)
    { mySerial.write(Com[ii]); }
    unsigned long startTime = millis();
    while (mySerial.available()  < 17)
    {   delay(10);  }
    ctr = 0;
    uint8_t Data[20] = {0};
    while (mySerial.available() >0 && ctr <20)
    {   Data[ctr] = mySerial.read();
        ctr++;
    }
    //read and compute temp
    uint8_t intT = 0;
    intT = ( (Data[11]*16777216) + (Data[12]*65536) + (Data[13]*256) + Data[14] );
    float tempC = 0.0;
    memcpy(&tempC, &intT, sizeof(float));
    
    //read and compute mg/L DO
    uint8_t intDO = 0;
    intDO = ( (Data[7]*16777216) + (Data[8]*65536) + (Data[9]*256) + Data[10] );
    memcpy(&mgL_DO, &intDO, sizeof(float));
    delay(2000);
    
    int Timestamp = (int)Time.now();    //establish timestamp
    //Measure all and publish
    float ORP = measureEco(ORPadd);
    float pH  = measureEco(pHadd);
    float C   = measureEco(Cadd);
    delay(Delay);   //2 sec
    String payload = "DO: " + String(mgL_DO) + ", T: " + String(tempC) +  ", ORP: " + String(ORP) + ", pH: " + String(pH) + ", C: " + String(C);
    Particle.publish("sensor_data", payload, PRIVATE);
    delay(52000);       //wait 52 seconds to give time for function calls
    //delay(20000);       //delay 20 sec for testing speed
    //go to sleep for 10 min
    System.sleep(config);
}

int measureEco(int sensorAdd)
{
    Wire.beginTransmission(sensorAdd);
    Wire.write(82);     //R for read
    Wire.endTransmission();     
    delay(TD);  //700 ms
    //now get the info from the reading
    Wire.requestFrom(sensorAdd, 20, 1);
    code = Wire.read();
    for (int ii = 0; ii < 20; ii++)
    {   charIn = Wire.read();
        sensorData[ii] = charIn;
    }
    Wire.endTransmission();
    data = atof(sensorData);
    charIn = 0;
    for (int jj = 0; jj < 20; jj++)
    {   sensorData[jj] = 0;
    }
    return data;
}

int calibrate(String command)
{
    char input[20];
    command.toCharArray(input, sizeof(input));  //put contents of String into a character array
    uint8_t cal[] = {67,97,108,44};     //dec for Cal,
    //tokenize comma sep arguments in array using character pointers
    char* sensor = strtok(input, ",");
    char* value = strtok(NULL, ",");
    //safety check for valid inputs
    if (sensor == NULL || value == NULL)
    {   return -1;    }  //return error code
    
    if (sensor == "pH")
    {
        Wire.beginTransmission(pHadd);
        Wire.write(cal, sizeof(cal));
        if (value == "4")
        {   uint8_t low[] = {108, 111, 119, 44, 52};
            Wire.write(low, sizeof(low));
        }
        if (value == "7")
        {   uint8_t mid[] = {109, 105, 100, 44, 55};
            Wire.write(mid, sizeof(mid));
        }
         if (value == "10")
        {   uint8_t high[] = {104, 105, 103, 104, 44, 49, 48};
            Wire.write(high, sizeof(high));
        }
        Wire.endTransmission();
    }
    if (sensor == "ORP")
    {
        Wire.beginTransmission(ORPadd);
        Wire.write(cal, sizeof(cal));
        if (value == "225")
        {   uint8_t ORPcal[] = {50, 50, 53};
            Wire.write(ORPcal, sizeof(ORPcal));
        }
        Wire.endTransmission();
    }
    if (sensor == "Con")
    {   
        Wire.beginTransmission(Cadd);
        Wire.write(cal, sizeof(cal));
        if (value == "dry")
        {   uint8_t dry[] = {100, 114, 121};
            Wire.write(dry, sizeof(dry));
        }
        if (value == "12880")
        {   uint8_t low[] = {108, 111, 119, 44, 49, 50, 56, 56, 48};
            Wire.write(low, sizeof(low));
        }
        if (value == "80000")
        {   uint8_t high[] = {104, 105, 103, 104, 44, 56, 48, 48, 48, 48};
            Wire.write(high, sizeof(high));
        }
        Wire.endTransmission();
    }
    delay(1000);  
    //Now measure once and publish to confirm value
    float ORP = measureEco(ORPadd);
    float pH  = measureEco(pHadd);
    float C   = measureEco(Cadd);
    delay(TD);
    String payload = "ORP: " + String(ORP) + ", pH: " + String(pH) + ", C: " + String(C);
    Particle.publish("Post_Cal", payload, PRIVATE);
    return 1;
}

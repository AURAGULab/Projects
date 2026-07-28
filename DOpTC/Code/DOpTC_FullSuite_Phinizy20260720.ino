/* DOpTC Full Suite + Grafana Test
2026.07.20 14:27 start
Butler Creek field deployment
20min sleep, approx 1 mins of live activities, sensor reads within 5 sec
*/

#include "Particle.h"
#include "SPI.h"
#include "Wire.h"
#include <ThingSpeak.h>
#include <SdFat.h>


TCPClient client;
unsigned long myChannelNumber = 3415287; // DOpTC Full Suite + Grafana 
const char * myWriteAPIKey = "S7I56ELL5V1YJIL6"; /////////// CHECK SENSOR ADDY

SYSTEM_MODE(AUTOMATIC);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

// Low Power Universals:
// sleep settings, not actually putting the device to sleep; 
// tells HOW we want it to go to sleep
SystemSleepConfiguration config;

#define mySerial Serial1
uint8_t Com[8] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x06, 0xC5, 0xC8 };    
int ctr = 0;


// SD card initilization 
bool sdOK = false; // Keep track of SD initialization
const int sdCS = D5;
const char fileName[] = "doptc.csv"; // <<<<<<<<<< modify filename HERE (8 char long) <<<<<<<<<<<<<
SdFat SD;
File myFile;


// 12v battery voltage declare + read
float batteryVoltage = 0.0;

// Atlas Scientific sensors ORP, pH, Cond
const int orpAddr = 0x51; // <<<<<<<<<<<<<<<<<<<<< modify sensor addy here <<<<<<<<<<<<<<<<<<<<<<<<
const int phAddr  = 0x52;
const int conAddr = 0x53;

char orpData[20];         // define sensor buffer size
char phData[20];
char conData[20];

float orpNow = 0.0;
float phNow  = 0.0;
float conNow = 0.0;


// array of Atlas Scientific sensors (for sleep code)
const int atlasSensors[] =
{
    orpAddr,
    phAddr,
    conAddr
};


// battery voltage Universals:
const int batteryPin = A0;
const float R1 = 98600.0; // 100k resistor
const float R2 = 19470.0;  // 20k resistor
const float dividerRatio = (R1 + R2) / R2;
const int DO_Power = D7;
float mgL_DO = 0.0;
float tempC = 0.0;
char buf[512]; 
int timeDelay = 1000; // <<<<<<<<<<<<<<<<<<<<<<<<<< set pause duration <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< 

// Atlas Scientific sensors read function
float asRead(int sensorAddr, char sensorData[])
{
    byte dataIndex = 0;

    Wire.beginTransmission(sensorAddr);
    Wire.write('R');
    Wire.endTransmission();

    delay(timeDelay);

    Wire.requestFrom(sensorAddr, 20, true);

    byte responseCode = Wire.read();

    if (responseCode != 1)
    {
        memset(sensorData, 0, 20);
        return NAN;
    }

    while (Wire.available())
    {
        char sensorChar = Wire.read();

        sensorData[dataIndex++] = sensorChar;

        if (sensorChar == 0 || dataIndex >= 20)
        {
            break;
        }
    }

    float sensorValue = atof(sensorData);

    memset(sensorData, 0, 20);

    return sensorValue;
}

// Atlas Scientific sleep function
void asSleep(int sensorAddr)
{
    Wire.beginTransmission(sensorAddr);

    Wire.write('S');
    Wire.write('l');
    Wire.write('e');
    Wire.write('e');
    Wire.write('p');

    Wire.endTransmission();
    delay(timeDelay);
}


// func.SD Card
void writeToSD(const String &data)
{
    if (!sdOK)
    {
        Log.error("SD unavailable.");
        return;
    }

    myFile = SD.open(fileName, FILE_WRITE);

    if (myFile)
    {
        myFile.println(data);
        myFile.close();
        Log.info("Wrote to SD");
    }
    else
    {
        Log.error("Failed to open SD file");
    }
}

////////////////////////////////


// setup
void setup()
{
    // delay(10000); //give 10s to flash code if need be
    // Serial.begin(9600);     //for Particle CLI or serial monitor only
    mySerial.begin(4800);       //for the DO probe
    Wire.begin();
    ThingSpeak.begin(client);
    pinMode(DO_Power, OUTPUT);
    //digitalWrite(DO_Power, HIGH); //power gate open for DO
    
    //first paranthesis is calling the particle sleep mode and declaring WHICH 
    //sleep mode we want; after the period is how we want it to go INTO that sleep (eg duration), 
    //minutes = min hour = h; you can keep adding to the code on this line by adding a 
    //period and continuing the statment with "or"s for instances of sleeping
    config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(20min); // <<<<<<<<<<<<<< SET SLEEP TIME HERE <<<<<<<<<<<<<<<<<<<
 


// Initialize SD Card ONLY ONCE
sdOK = SD.begin(sdCS);

if (sdOK)
{
    Log.info("SD card initialized");

    myFile = SD.open(fileName, FILE_WRITE);

    if (myFile)
    {
        if (myFile.size() == 0)
        {
            myFile.println("Unix_Time,DO,ORP,pH,Temp,Conductivity,BatteryV");
            Log.info("SD header written");
        }

        myFile.close();
    }
    else
    {
        Log.error("Unable to open SD file.");
        sdOK = false;
    }
}
else
{
    Log.error("SD card initialization failed.");
}

    
    delay(timeDelay);
}

// loooooooooooooop
void loop()
{
    // delay(timeDelay);    //just because, no reason
    digitalWrite(DO_Power, LOW);
    // delay(timeDelay);    //give time to measure MOSFET
    digitalWrite(DO_Power, HIGH);

    delay(30000);   //safe to drop down to 29 seconds, set to 30s to conserve power for field deployment
// for DO probe:
    for (int ii = 0; ii < 8; ii++)
    {
        mySerial.write(Com[ii]);
    }
    
    
    unsigned long startTime = millis(); //polling start time

    // when data less than expected value, and interval under 1 sec, wait
        while (mySerial.available() < 17 && millis() - startTime < 1000)
    {
        delay(10);
    }
    
    ctr = 0;
    uint8_t Data[20] = {0};
    
    while (mySerial.available() > 0 && ctr < 20)
    {
        Data[ctr] = mySerial.read();
        ctr++;
    }
    
    // validate the packet
    if (ctr < 17)
    {
        Log.error("Incomplete DO packet");
        mgL_DO = 99.9;
        tempC = 99.9;
        Particle.publish("DO_ERROR", "Incomplete packet", PRIVATE);
    } 
    else
    {
    
    // grab DO in mg/L
        uint32_t intDO = 0;
        intDO = ( (Data[7]*16777216) + (Data[8]*65536) + (Data[9]*256) + Data[10] );
        memcpy(&mgL_DO, &intDO, sizeof(float));
    
    // grab Temperature from DO probe
        uint32_t intT = 0;
        intT = ( (Data[11]*16777216) + (Data[12]*65536) + (Data[13]*256) + Data[14] );
        memcpy(&tempC, &intT, sizeof(float));
        
        delay(timeDelay);
        
    //Particle.publish("DO & Temp measured", buf);
     }
	
// read the battery voltage:
    analogRead(batteryPin); //dummy poll to give RC circuit time to charge up
    delay(10);
    int batteryRawRead = analogRead(batteryPin);
// convert 12-bit ADC reading (0-4095) to voltage at A0
    float voltConvert = (batteryRawRead * 3.3) / 4095.0;
// calculate battery voltage
    batteryVoltage = voltConvert * dividerRatio;

    
// Atlas Scientific readings (redundent read)

    orpNow = asRead(orpAddr, orpData);
    delay(50);
    phNow  = asRead(phAddr, phData);
    delay(50);
    conNow = asRead(conAddr, conData);
    delay(50);
    delay(timeDelay);
    
// AS error reporting
    String errorMessage = "";

    if (isnan(orpNow))
    {
        errorMessage += "ORP ";
    }

    if (isnan(phNow))
    {
        errorMessage += "pH ";
    }

    if (isnan(conNow))
    {
        errorMessage += "CON ";
    }

    if (errorMessage.length() > 0)
    {
        Particle.publish("Sensor Failure", errorMessage, PRIVATE);
    }
    
// Atlas Scientific sensor data captured, put to sleep    
    for (int i = 0; i < 3; i++)
{
    asSleep(atlasSensors[i]);
}


// push to Particle console
    String payload ="DO:" + String(mgL_DO, 2) +
        		", ORP:" + String(orpNow, 2) +
        		", pH:" + String(phNow, 2) +
        		", Temp:" + String(tempC, 2) + 
        		", Cond:" + String(conNow, 2) +
                ", BattV:" + String(batteryVoltage, 2);
        
    Particle.publish("DOpTC:", payload, PRIVATE);
    delay(50);
        
// Grafana magical dust
    int Timestamp = (int)Time.now();

    
    snprintf(buf, sizeof(buf),
        "[{\"Device\":\"DOpTC_MSFT\",\"Place\":\"Phinizy1\",\"Count\":\"6\",\"time\":\"%d\"},"
        "{\"Meas0\":\"DO~%.2f\",\"Meas1\":\"ORP~%.2f\",\"Meas2\":\"pH~%.2f\","
        "\"Meas3\":\"Cond~%.2f\",\"Meas4\":\"Battery~%.2f\",\"Meas5\":\"Temp~%.2f\"}]",
        Timestamp,
        mgL_DO,
        orpNow,
        phNow,
        conNow,
        batteryVoltage,
        tempC);

Particle.publish("Pat_Pro", buf, PRIVATE);

    
    
// push to SD
    String sdData = String(Timestamp) + "," +
    		    String(mgL_DO, 2) + "," +
    		    String(orpNow, 2) + "," +
    		    String(phNow, 2) + "," +
    		    String(tempC, 2) + "," +
                String(conNow, 2) + "," +
                String(batteryVoltage, 2);
                writeToSD(sdData);
    
    
    
    
// push to ThingSpeak
    ThingSpeak.setField(1, mgL_DO);
    ThingSpeak.setField(2, orpNow);
    ThingSpeak.setField(3, phNow);
    ThingSpeak.setField(4, tempC);
    ThingSpeak.setField(5, conNow);
    ThingSpeak.setField(6, batteryVoltage);
    
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey); 


    delay(timeDelay);
    digitalWrite(DO_Power, LOW);
    
// put Particle to sleep for 20 minutes before starting the loop again
    System.sleep(config);
}

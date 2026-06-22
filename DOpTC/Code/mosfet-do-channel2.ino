#include "Particle.h"
SYSTEM_MODE(AUTOMATIC);
SerialLogHandler logHandler(LOG_LEVEL_INFO);
//Channel 2 - DO 4 ThingSpeak
#include <ThingSpeak.h>
TCPClient client;
unsigned long myChannelNumber =  3404326;
const char * myWriteAPIKey = "U967KW9ERCAICPYY";
//DO sensor universals
#define mySerial Serial1
uint8_t Com[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x06, 0xC5, 0xC8};
int ctr = 0;

//Battery Voltage Universals:
const int batteryPin = A0;
const float R1 = 1000000.0; // 1M resistor
const float R2 = 200000.0;  // 200k resistor
const float dividerRatio = (R1 + R2) / R2;
const int DO_Power = D7;    //MOSFET control pin
float mgL_DO = 0.0;
char buf[2500]; 
int timeDelay = 2000;

//Low Power univerals
SystemSleepConfiguration config; 

void setup() 
{
    delay(60000);
    Serial.begin(9600);
    mySerial.begin(4800);                                             //for comm with the DO probe, must match device baud rate. 4800 is default
    pinMode(DO_Power, OUTPUT);
    digitalWrite(DO_Power, HIGH);                                     //power gate open for DO
    config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(17min);    //first paranthesis is calling the particle sleep mode and declaring WHICH sleep mode we want; after the period is how we want it to go INTO that sleep (eg duration), minutes = min hour = h; you can keep adding to the code on this line by adding a period and continuing the statment with "or"s for instances of sleeping
    ThingSpeak.begin(client);
    Particle.variable("battery", "battLevel");
    delay(timeDelay);
}

void loop() 
{
    delay(8000); //just because, for no reason
    digitalWrite(DO_Power, LOW);
    delay(timeDelay); //give 1 sec to measure MOSFET w/ DMM
    digitalWrite(DO_Power, HIGH);
    delay(60000);
    //For DO probe:
    for (int ii = 0; ii < 8; ii++)
    {
        mySerial.write(Com[ii]);
    }
    
    
    unsigned long startTime = millis();
     while (mySerial.available() < 17 && millis() - startTime < 1000) //when data less than expected value, and interval under 1 sec, wait
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
    
//Read the Battery Voltage:
    int analogValue = analogRead(batteryPin);
    // Convert 12-bit ADC reading (0-4095) to voltage at A0
    float voltageAtPin = (analogValue * 3.3) / 4095.0;
    // Calculate battery voltage
    float batteryVoltage = voltageAtPin * dividerRatio;
// Analyze and compute Temperature, but repeat if the measurement is zero
    uint32_t intT = 0;
    intT = ( (Data[11]*16777216) + (Data[12]*65536) + (Data[13]*256) + Data[14] );
    float tempC = 0.0;
    memcpy(&tempC, &intT, sizeof(float));
    delay(timeDelay);
    Particle.publish("Temp was measured", buf);
    
    // Analyze and compute Percentage DO
    // uint32_t intDO = 0;
    // intDO = ( (Data[3]*16777216) + (Data[4]*65536) + (Data[5]*256) + Data[6] );
    // float percentDO = 0.0;
    // memcpy(&percentDO, &intDO, sizeof(float));
    // percentDO *= 100.0;
    // delay(timeDelay);
    // Particle.publish("%DO was measured", buf);
    
    // Analyze and compute mg/L DO
    uint32_t intDO = 0;
    intDO = ( (Data[7]*16777216) + (Data[8]*65536) + (Data[9]*256) + Data[10] );
    memcpy(&mgL_DO, &intDO, sizeof(float));
    delay(timeDelay);
    Particle.publish("DO was measured", buf);
    
    // Display DO and T to Particle if needed
    String payload = "DO: " + String(mgL_DO) + ", T: " + String(tempC) + ", V: " + String(batteryVoltage);
    Particle.publish("sensor_data", payload, PRIVATE);
    //Print to Grafana/Influx
    int Timestamp = (int)Time.now();
     //snprintf(buf, sizeof(buf), "[{\"Device\":\"DOpTC\",\"Place\":\"Test_Run\",\"Count\":\"3\",\"time\":\"%d\"},{\"Meas0\":\"DO~%.2f\",\"Meas1\":\"Temp~%.2f\",\"Meas2\":\"Battery~%.2f\"}]", Timestamp, mgL_DO, tempC, batteryVoltage);
    //Particle.publish("Pat_Pro", buf);
//Send data to ThingSpeak
    //ThingSpeak.setField(1,percentDO);
    ThingSpeak.setField(2, mgL_DO);
    ThingSpeak.setField(3, tempC);
    ThingSpeak.setField(4, batteryVoltage);
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    delay(104000);
    digitalWrite(DO_Power, LOW);
    //Put Particle to sleep for 17 minutes before starting the loop again
    System.sleep(config);
}
#include <SoftwareSerial.h>
const byte rxPin = 10;
const byte txPin = 11;

SoftwareSerial mySerial(rxPin, txPin); //rx of Arduino, tx of arduino 

String inputString = "";
String sensorString = "";
bool inputStringComplete = false;
bool sensorStringComplete = false;

void setup() 
{
  Serial.begin(9600);
  mySerial.begin(9600);
  inputString.reserve(10);
  sensorString.reserve(30);
  Serial.println("Starting loop...");
}

void loop() 
{
  if (inputStringComplete == true)   //Send commands to sensor board, reset our vars. // We talk to sensor
  {
    mySerial.print(inputString);
    mySerial.print('\r');
    inputString = "";
    inputStringComplete = false;
  }

  if (mySerial.available() > 0)     //Receive data from sensor. // Sensor trying to talk to us, read it 
  {
    char inChar = (char)mySerial.read();
    sensorString += inChar;
    if (inChar == '\r')
    {
      sensorStringComplete = true;
    }
  }

  if (sensorStringComplete == true) // Finished reading sensor, show us, reset vars
  {
    Serial.println(sensorString);
    sensorString = "";
    sensorStringComplete = false;
  }
}

void serialEvent()
{
  inputString = Serial.readStringUntil(13); // Read our command we want to send
  inputStringComplete = true;
}

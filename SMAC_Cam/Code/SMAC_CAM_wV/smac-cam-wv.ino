// This #include statement was automatically added by the Particle IDE.
#include <SdFat.h>


#include <SdFat.h>
// Include Particle Device OS APIs
#include "Particle.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);


#define myCam Serial1
CloudEvent cloudEvent;
char cmdReset[4] = {0x56, 0x00, 0x26, 0x00};          //Command and buffer for reseting the camera (currently not implemented)
char respReset[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

//Commands for taking picture///////////////////////////////////////////////////////////////////////////////////////////////

char cmdStopF[5] = {0x56, 0x00, 0x36, 0x01, 0x02};    //Command and buffer for stopping the capture frame
char respStopF[5];

char cmdCapt[5] = {0x56, 0x00, 0x36, 0x01, 0x00};     //Command and buffer for capturing one image
char respCapture[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

byte cmdImgL[5] = {0x56, 0x00, 0x34, 0x01, 0x00};     //Command and buffer for reading the image files length
byte respImgL[9];

//There is a resume frame command that should allow for repeated pictures without cyclying 
//the power this might be something to include
//For CLiC and low power this should not matter since power will naturally cycle in the process

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Commands for sending image file to the SD car******************************************************************************


//While loop stuff
bool flag = true;
bool butFlag = false;

//LED stuff
//int ledR = D6;      //Ready
//int ledP = D7;      //Processin
//int ledG = D8;      //Good
//button
int Relay = D5;    //Turns camera on an off
//SD card
SdFat SD;
//Ledger for logging when the device turns on
Ledger SMAC;
//Sleep config things
SystemSleepConfiguration config;
//SdFile File; 

// Functions Time 
bool goLoop = false;                                         //Will make the camera skip going to sleep
int StayOn(String x)                                        
{
    if(x == "1")
    {
        goLoop = true;
        return 1;
    }
    else
    {
        goLoop = false;
        return 0;
    }
}

float Battery_Check()
{
    char buf[50];
    pinMode(A5, INPUT);
    int anaReading = analogRead(A5);
    float Voltage = ((anaReading*3.3)/4095)*6.1;
    //snprintf(buf, sizeof(buf), "{\"Battery Voltage\":\%d\"}", Voltage);   //convert/save the data
    return Voltage;
}
// setup() runs once, when the device is first turned on
void setup()
{
  /*int timeOut = Time.minute();
  while(Particle.disconnected())
  {
      if((Time.minute()-timeOut) >=10)
      {
          //System.reset();
      }
      //wait to connect
  }*/
  pinMode(Relay, OUTPUT);                               //pin for the button that controls when a picture gets snapped
  Serial.begin(38400);                                   //good for debugging so it shall stay
  delay(5000);                                           //give it a chance to start up
  //Particle function that keeps sensor on for when it is being deployed
  Particle.function("Keep On", StayOn);
  Particle.variable("Caffeine", goLoop);
  //Particle Ledger for having a report of vitals 
  SMAC = Particle.ledger("smac-report");
  //SD card things
  //D6 for OG D4 for PCB
  if (!SD.begin(D4))                                     //Make sure SD card works
  {
    Serial.println("SD card failed");
    while (1)
    {
      Particle.publish("SDCARD","Help");
      delay(600000);
      config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(1h);
      System.sleep(config); 
      
      //Code name BLUE FLASH
      //digitalWrite(ledP, HIGH);                
     // delay(1000);
     // digitalWrite(ledP,LOW);
      //delay(1000);
      //be stuck here so things do
    }
  }
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  /////////////////////////////////////////////This part is taking the picture///////////////////////////////////////////////////////
  //turn on the camera
  
  delay(10000);
  digitalWrite(Relay,HIGH); 
  Particle.publish("Relay","is on?");
  delay(5000);
  
  myCam.begin(38400);                                    //Start the software Serial for camera
  /*
     For this you have to:
     1) stop the current capture frame
     2) capture the picture
     3) read the images data length
  */
  //Stop capture frame
  //send capture cmd
  /*
  digitalWrite(ledR,HIGH); //Ready to take picture
  digitalWrite(ledR,LOW);
  */
  
  for ( int ii = 0; ii < sizeof(cmdStopF); ii++)        //write the cmd for stopping the frome
  {
    myCam.write(cmdStopF[ii]);
  }
  //read response (look for 0x76)
  Particle.publish("cmd sent","sdsdsdsdsd");
  delay(100);
  while (flag)
  {
    respStopF[0] = myCam.read();
    if (respStopF[0] = 0x76)                            //Look for reply header
    {
      flag = false;                                     //once found exit loop
    }
  }
  Particle.publish("Ack","Recieved");
  flag = true;
  //Found the 76 so read the next 4 bytes
  for ( int jj = 1; jj < sizeof(respStopF); jj++)       //and read the remaining bytes
  {                                                     //for loop starts at 1
    respStopF[jj] = myCam.read();
  }
  //print the response
  Serial.println("Current frame stopped");
  for (int jj = 0; jj < sizeof(respStopF); jj++)
  {
    Serial.print(respStopF[jj], HEX);
    Serial.print(" ");
  }
  Serial.println();

  delay(2500);
  //Capture image
  //Send command for capture Should probably break these out into functions, that will come with the refined version (Tristan's job....)
  //Thanks Tristan :)
  for ( int ii = 0; ii < sizeof(cmdCapt); ii++)
  {
    myCam.write(cmdCapt[ii]);
  }
  
  //Look for ACK
  delay(100);
  while (flag)
  {
    respCapture[0] = myCam.read();
    if (respCapture[0] = 0x76)                          //Look for the 76 that starts the ack
    {
      flag = false;                                     //found it exit loop
    }
  }
  flag = true;                                          //turn flag true for next loop 
  for ( int jj = 1; jj < sizeof(respCapture); jj++)     //Now that we have the ACK read th packet
  {
    respCapture[jj] = myCam.read();                     //read the paket
  }

  Serial.println("Picture captured");                   //confirm picture is taken 
  //print ACK
  for (int jj = 0; jj < sizeof(respCapture); jj++)      //print the packet to the seirial monitor 
  {
    Serial.print(respCapture[jj], HEX);
    Serial.print(" ");
  }
  Serial.println();
  delay(2500);                                          //give camera a break, can probalby be reduced in time 
  
  //Get length of the image data
  for (int ii = 0; ii < sizeof(cmdImgL); ii++)          //send the get length command
  {
    myCam.write(cmdImgL[ii]);                               
  }
  delay(100);
  while (flag)                                          //Look for the 76 response 
  {
    respImgL[0] = myCam.read();                         
    if (respImgL[0] = 0x76)                             //76 found 
    {
      flag = false;                                     //exit loop
    }
  }
  flag = true;                                          //set true for next loop
  //Read whole response (perhaps add error chaecking later)
  for ( int jj = 1; jj < sizeof(respImgL); jj++)
  {
    respImgL[jj] = myCam.read();                        //read response packet
  }
  //Save High and low byte
  byte picLenHb = respImgL[7];                          //high byte of length
  byte picLenLb = respImgL[8];                          //low byte
  //Beefy masking inbound for total
  long totalB = (((picLenHb << 8) | picLenLb) & 0x0000FFFF);

  Serial.println("Image length");                       //serial print for comfirmation
  //print response
  for (int jj = 0; jj < sizeof(respImgL); jj++)         //print response
  {
    Serial.print(respImgL[jj], HEX);
    Serial.print(" ");
  }
  Serial.println();
  //print each byte and them combined bytes (for error checking)
  Serial.println(picLenHb, HEX);                        
  Serial.println(picLenLb, HEX);
  Serial.println(totalB, HEX);

 Particle.publish("cam","is good?");

  delay(2500); //wait 2.5 seconds
  

   /////////////////////////////////////////////////////////Send the image to an SD card/////////////////////////////////////////////////////
  /*
     So now that we have the image and the files length we are going to read the image over to an SD card
     I HAVE NO CLUE HOW TO DO THIS so this will be a lot of trial and error (that you hopefully won't see)
     Here's to loosing my sanity
  */
  
   /*
     So above I get the length of the stored image; However, in trying to use this for grabbing the image the length given never 
     lined up with the actual image length. I have left the code in and opperational in case it is figured out in the future 
     I believve that the meta data is the thing making the image not line up.
     
     Instead of using the image length I instead look for the 0xFFD8 at the begining of a jpg image and 0xFFD9 at the end. With this
     I am able to get a consistant image, but I loose the metadata (date and time) on the img file. 
     This is not a concern since particle will have the timestamps with each send
  */
  
  
  //Command and buffer for sending the image file over
  //needs to be passed Start address at bytes 8 and 9 (I am putting in 0x0000 assuming you want to start at the begining)
  //this value is iterated in the reading loop
  //and Image file lengths at bytes 12 and 13
  //the rest can be found on the datasheet
  Particle.publish("Processing img","dffdfdffssffswrwwrretette");
  digitalWrite(7, HIGH);
  byte cmdSendPics[16] = {0x56, 0x00, 0x32, 0x0C, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x0A};
  byte respSendPics[5];                                 //for reading ack from the chip
  byte imgBuf[32];                                      //hold image info before sending it to the sd card
  //For counting bytes read
  //long imgCount = 0;
  int bytes2Read = 32;
  //controls reading loop
  bool flagRead = true;                                 //for controlling the entire reading loop
  bool flagHead = true;                                 //for controlling reading the header
  bool flagTail = true;                                 //for controlling reading the tail of the image packet
  bool cmd = true;                                      //## for debugging delete later (controls when code loops)
  //for keeping up with where we are in image file
  //This will go into bytes 8 and 9
  long imgAddr = 0;                                     //for keeping track of the image address this will be masked into bits 8 and 9
  //initiate
  char filename[14];                                    //taken from adafruit library 
  char filenameRemove[14];                              //Will start overriding data
  strcpy(filename, "IMAGE000.JPG");                      //This makes a new img file everytime a picture is taken starting from IMAGE00 - 99
  strcpy(filenameRemove, "IMAGE000.JPG"); 
  for (int i = 0; i < 1000; i++) 
  {
    
    
    filename[5] = '0' + i/100;
    filename[6] = '0' + ((i/10)%10);
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename))
    {
        if(SD.exists("IMAGE999.JPG"))                   //If this file exist that means we are in overriding mode
        {
            int j = (i+1);                                 //so delete the next file 
            filenameRemove[5] = '0' + j/100;
            filenameRemove[6] = '0' + ((j/10)%10);
            filenameRemove[7] = '0' + j%10;
            SD.remove(filenameRemove);
        }
      break;
    }
    else if(i == 999)
    {
        SD.remove("IMAGE000.JPG");
        SD.remove("IMAGE001.JPG");                      //remove  to keep track of where in the override we are
        strcpy(filename, "IMAGE000.JPG");               //set the first one to be overriden
        break;
    }
  }
  //Open SD card file
  File imgFile = SD.open(filename, FILE_WRITE);         //create a file with the new name
  while (flagRead)
  { //send the command
    for (int ii = 0; ii < sizeof(cmdSendPics); ii++)    //send the command for recieving img data
    {
      myCam.write(cmdSendPics[ii]);
      Serial.print(cmdSendPics[ii], HEX);
      Serial.print(" ");

    }
    Serial.println();
    //lets do the response
    //it comprises of (Header: 5 bytes, Body: 32 bytes, Tail: 5 bytes)
    //look for header
    while (flagHead)
    {
      respSendPics[0] = myCam.read();
      Serial.println(respSendPics[0], HEX);
      if (respSendPics[0] == 0x76)                     //start of the packet
      {
        for (int ii = 1; ii < 5; ii++)                 // read the rest of the header
        {
          do
          {
          respSendPics[ii] = myCam.read();
          } while(respSendPics[ii] == 0xFF);
          Serial.print(" ");                          //could put bad info checking here
          Serial.print(respSendPics[ii], HEX);
        }
        Serial.println();
        flagHead = false;                             //get out of reding loop
      }
    }
    //now that the header is gone lets read the data
    //which is the next 32 bytes
    delay(5);                                         //needed delay
    for (int zz = 0; zz < 32; zz++)                   //we will now be reading the body (which is 32 bytes)
    {
      imgBuf[zz] = myCam.read();                      //read in image info bytes
      Serial.print(zz);                               //## for error checking delete later
      Serial.print(" : ");                            //## for error checking delete later
      Serial.print(imgBuf[zz], HEX);                  //## for error checking delete later
      Serial.print("|"); 
      if (imgBuf[zz] == 0xD9 && imgBuf[zz-1] == 0xFF) //0xFF 0xD9 signify the end of a jpeg
      {
        Serial.println("Found the end");              //## for error checking delete later
        flagRead = false;                             //allows us to exit the main reading loop
        flagTail = false;                             //ignore the tail
        bytes2Read = zz+1;                            //Will only write the new bits if loop ends early
      }
      delay(5);                                       //needed delay
    }
    Serial.println();
    //now look for the ending packet
    while (flagTail)                                  //We will now be reading the end of the img packet
    {
      respSendPics[0] = myCam.read();                 //read the UART line till we see 0x76 response
      //Serial.print("T");
      //Serial.println(respSendPics[0], HEX);         //##error checking delete later
      if (respSendPics[0] == 0x76)                    //start of the packet
      {

        for (int ii = 1; ii < 5; ii++)                // read the rest of the header
        {
          respSendPics[ii] = myCam.read();            //could put bad info checking here
        }
        flagTail = false;                             //get out of reding loop
      }
    }
    delay(5);
    // delay 500
    imgFile.write(imgBuf, bytes2Read);                //write to the SD card
    //bytes2Read = 0;     
    //reset to 0 so it does not overflow
    imgAddr += 32;                                    //add 32 to the img address since we just read 32 bytes
    //mask addr into the cmd array
    cmdSendPics[8] = ((imgAddr & 0xFF00) >> 8);       //High byte of address
    cmdSendPics[9] = (imgAddr & 0x00FF);              //low byte of address
    Serial.println(imgAddr);                          //##
    Serial.println(cmdSendPics[8],HEX);               //##
    Serial.println(cmdSendPics[9],HEX);               //##
    flagHead = true;                                  //Resets
    flagTail = true;                                  //Resets
   // myCam.flush();
   delay(100);
  }
  //some loop element goes



  imgFile.close();      //since we are done, close the file
  /*LED stuff not on rn maybe ask wesley for panel mounts?
  digitalWrite(ledP, LOW);
  digitalWrite(ledG, HIGH);
  */
  float volts = Battery_Check();
  delay(1000);
  digitalWrite(Relay, LOW);                         //Turn off power flowing to the camera to better help low power
  
  delay(10000);
  int run = 0;
  cloudEvent.name("frag0");
  cloudEvent.contentType(ContentType::JPEG);
  imgFile = SD.open(filename);
  //cloudEvent.loadData(myFile); //might do this later but I am not sure how the arguement works 
  while(imgFile.available())   //read the file on the sd card and save it to the cloud event 
  {
    cloudEvent.write(imgFile.read());
    //Serial.println(cloudEvent.size());
    cloudEvent.canPublish(cloudEvent.size());
    if(cloudEvent.size()/32 == 500)
    {
      
      run++;
      //Send the frag and check to see if it sent, then move on to the next fragment
      bool publishFlag = true;
      int tryAgain; //have ran into issues with setting to 0 and inst. on the same line 
      tryAgain = 0; //so I broke it into two
      while(publishFlag)
      {
      Particle.publish(cloudEvent);
      delay(15000);
        
        while(cloudEvent.status() == CloudEvent::SENDING)
        {
          Serial.println(cloudEvent.status());
          delay(1000);
        }
        if(cloudEvent.error() != 0)
        {
           Particle.publish("Frag failed to send");
           tryAgain++;
        }  
        else
        {
            publishFlag = false;    //we published it so we are good to leave the loop
        }
         delay(5000);
        if(tryAgain > 3) //try 4 times 
        {
            publishFlag = false;    //and get out
        }
      }
      //Serial.println(cloudEvent.size());
      cloudEvent.clear();
      if(run == 1)
      {
        Serial.println("Im here for frag 1");
      cloudEvent.name("frag1");
      }
      else if(run ==2)
      {
        cloudEvent.name("frag2");
      }
      else if(run ==3)
      {
        cloudEvent.name("frag3");
      }
      else{
        //System.reset();
      }
      cloudEvent.contentType(ContentType::JPEG);
    }
  }
  Particle.publish(cloudEvent);
  
  delay(10000);
  cloudEvent.clear();
  delay(1000);//allow the final cloud event to publish before going to bed
  imgFile.close();
  
  run = 0 ;
  
  Variant ledgerData;
  ledgerData.set("Voltage", volts); //volts
  SMAC.set(ledgerData);
  //Take a picture every 3 hours starting at 8 am and ending at 4 pm
  //Remeber this is in military time 
  
  /*
    I tried ot implament some dynamic time checking; However, I noticed that the sleep .duration did not like just integer values,
    until I reach a fis for this I am going back to the odd even check and the 16 hour sleep.
    I could do a very large switch statement but I do not think that we want that
  */
  if((Time.hour()>= 8) && (Time.hour() <=15)) //Its day time 
  {
      if((Time.hour() % 2) != 0) //check to see if the time is odd
      {
          config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(1h); //wait one hour
      }
      else //it must be even 
      {
          config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(2h); // wait two hours
      }

  }
  else // it is night time (big sleep inbound)
  {
      /*
      int sleepT;   //Sleep time 
      if (Time.hour() > 15) //This is for 3pm to 11:59
      {
          sleepT = (16-(Time.hour()%16)); 
      }
      else  //We are in 24h time so it loops back to 0
      {
          sleepT = (8 - Time.hour());
      }
      //Convert sleepT to microseconds
      sleepT = 3;
      */
      config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(16h);
  }
  if(!goLoop)
  {
    Particle.publish("Status", "Sleeping");
    delay(10000);
    System.sleep(config);  //Go to sleep
    System.reset();
  }
  //may need to add a command to restart the frame for now we will just cycle power 
  
  
  
}
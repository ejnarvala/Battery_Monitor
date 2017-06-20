#include "emailer.h"
#include <Wire.h>
#include "Adafruit_MCP9808.h"
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>


//<-----------------IP SETTINGS-------------------->
byte mac[] = { 0x8A, 0x7F, 0xA7, 0x2F, 0x8D, 0xE0 };  
IPAddress ip(30,30,30,90);
IPAddress gateway(30,30,30,254);
IPAddress subnet(255, 255, 255, 0);
//<---------------END IP SETTINGS------------------>




//<------------------VARIABLES--------------------->
extern EthernetClient client; //Used for Emailing/Texting
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808(); //Instantiate Temperature Sensor
//<------------------------------------------------>





void setup(){
  //Start Serial monitoring
  Serial.begin(9600);
  
  //Start SD card
  initialize_sd();
  sdRWTest(); //Uncomment to test SD card Read/Write
  
  //Start Ethernet Connection
  initialize_ethernet();
  
  //Start Temperature Sensor
  initialize_tempsensor();

  
}

void loop(){



}


void initialize_tempsensor(){
  while (!tempsensor.begin()) {
     Serial.println("Couldn't find MCP9808!\nTrying Again...");
     delay(1000);
  }
  Serial.println(F("Temperature Sensor Initialized"));
}



// Sets up DHCP, waits 25s to make sure its connected
//sets some pin outputs for sd card
void initialize_ethernet() 
{
  Serial.println("Initializing Ethernet");
  Ethernet.begin(mac, ip, gateway, gateway, subnet);
  delay(25000);    //Long enough to be fuly set up

  Serial.println("Ready");
}


//Sets up SD card
void initialize_sd() {
  pinMode(4, OUTPUT); //Needed to not conflict with ethernet
  digitalWrite(4, HIGH);
  if (!SD.begin(4)) { //Initializes sd card
    Serial.println(F("Card failed, or not present")); //Prints if SD not detected
    return; // don't do anything more:
  }
  Serial.println(F("SD card initialized."));
}


//Inputs char array pointer to file name on sd and
//String of what to append to that file;
void sdLog(const char* fileName, String stringToWrite) {
  File myFile = SD.open(fileName, FILE_WRITE);
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to ");
    Serial.print(fileName);
    Serial.print("...");
    myFile.println(stringToWrite);
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.print("error opening ");
    Serial.println(fileName);
  }
}



//Run this test to create and remove a text file
//on the SD card to test Read/Write functionality
void sdRWTest(void){
  File myFile;
  if (SD.exists("example.txt")) {
    Serial.println("example.txt exists.");
  } else {
    Serial.println("example.txt doesn't exist.");
  }

  // open a new file and immediately close it:
  Serial.println("Creating example.txt...");
  myFile = SD.open("example.txt", FILE_WRITE);
  myFile.close();

  // Check to see if the file exists:
  if (SD.exists("example.txt")) {
    Serial.println("example.txt exists.");
  } else {
    Serial.println("example.txt doesn't exist.");
  }

  // delete the file:
  Serial.println("Removing example.txt...");
  SD.remove("example.txt");

  if (SD.exists("example.txt")) {
    Serial.println("example.txt exists.");
  } else {
    Serial.println("example.txt doesn't exist.");
  }
}





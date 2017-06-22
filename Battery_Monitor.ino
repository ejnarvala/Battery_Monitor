/* ************************************************************************
 * ***              Battery Lab Temperature/Gas Monitor                 ***
 * ************************************************************************
 * Built using parts of:
 * Super Graphing Data Logger by Everett Robinson, December 2016 http://everettsprojects.com
 * Email client sketch by SurferTim, May 2015
 * 
 * 
 * 
 * Authors:
 * Ejnar Arechavala - github.com/ejnarvala
 * Serena Liu - github.com/serliu
 * 
 * 
 * 
 * This is a sketch which monitors the temperature and gas of four battery 
 * testing cabinets. The data can be read live on a local web page at the
 * arduino's ip address which also lists a history of logs.
 */




#include "emailer.h"
#include <Wire.h>
#include "Adafruit_MCP9808.h"
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>


//#define BUFSIZ 75 // How big our line buffer should be for sending the files over the ethernet.




//<-----------------IP SETTINGS-------------------->
byte mac[] = { 0x8A, 0x7F, 0xA7, 0x2F, 0x8D, 0xE0 };  
IPAddress ip(30,30,30,90);
IPAddress gateway(30,30,30,254);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(80);
//<---------------END IP SETTINGS------------------>




//<------------------VARIABLES--------------------->
//extern EthernetClient client; //Used for Emailing/Texting
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808(); //Instantiate Temperature Sensor
float temp_c;
String filename;
int filenum;
int loopcount = 0;
//<------------------------------------------------>





void setup(){
  //Start Serial monitoring
  Serial.begin(9600);
  
  //Start SD card
  initialize_sd();
  //sdRWTest(); //Uncomment to test SD card Read/Write
  
  //Start Ethernet Connection
  initialize_ethernet();
  
  //Start Temperature Sensor
  initialize_tempsensor();




  //Temporary data logging on SD which just saves data as next available integer everytime program restarts
  File myFile = SD.open("data/0.txt", FILE_WRITE); //should always be there but overwrite anyways
  File workingDir = SD.open("/data/");
  filenum = 1;
  while(SD.exists("data/" + (String) filenum + ".txt")){
    filenum++;
  }
  filename = "data/" + (String) filenum + ".txt";
  File workingFile = SD.open(filename);
  Serial.println("New File: " + filename + " Created!");
  workingFile.close();

}

void loop(){
  if(++loopcount == 500){
    sdLog(filename, (String) temp_c); //save to SD log
    loopcount = 0;
  }
  
  temp_c = tempsensor.readTempC(); //get temperature reading
  
  
  char clientline[BUFSIZ];
  int index = 0;
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    boolean current_line_is_blank = true;
    
    // reset the input buffer
    index = 0;
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        // If it isn't a new line, add the character to the buffer
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          // are we too big for the buffer? start tossing out data
          if (index >= BUFSIZ) 
            index = BUFSIZ -1;
          
          // continue to read more data!
          continue;
        }
        
        // got a \n or \r new line, which means the string is done
        clientline[index] = 0;
        
        // Print it out for debugging
        Serial.println(clientline);
        
        // Look for substring such as a request to get the root file
        if (strstr(clientline, "GET / ") != 0) {
          // send a standard http response header
          HtmlHeaderOK(client);
          // print all the data files, use a helper to keep it clean
          client.println(F("<style> h1 {font-size: 42px} h2 { font-size: 24px} html {background: #e6e9e9; height 100%; background-image: linear-gradient(270deg, rgb(230, 233, 233) 0%, rgb(216, 221, 221) 100%); -webkit-font-smoothing: antialiased;} body { height: 100%; background: #fff; box-shadow: 0 0 2px rgba(0, 0, 0, 0.06); color: #545454; font-family: \"Helvetica Neue\", Helvetica, Arial, sans-serif; font-size: 42px; text-align: center; line-height: 1.5; margin: 0 auto; max-width: 800px; padding: 2em 2em 4em;} li { list-style-type: none; font-size: 18px; font-family: \"Helvetica Neue\", Helvetica, Arial, sans-serif;} </style>"));
          client.println(F("<h1>Battery Lab Temperature and Gas Monitor<h1>"));
          client.print("<h1>Temperature: "); client.print(temp_c);  client.print("&degC<h1>");
          client.println(F("<h2>View data for the week of (dd-mm-yy):</h2>"));
          ListFiles(client);
        }
        else if (strstr(clientline, "GET /") != 0) {
          // this time no space after the /, so a sub-file!
          char *filename;
          filename = strtok(clientline + 5, "?"); // look after the "GET /" (5 chars) but before
          // the "?" if a data file has been specified. A little trick, look for the " HTTP/1.1"
          // string and turn the first character of the substring into a 0 to clear it out.
          (strstr(clientline, " HTTP"))[0] = 0;
          // print the file we want
          Serial.println(filename);
          File file = SD.open(filename,FILE_READ);
          if (!file) {
            HtmlHeader404(client);
            break;
          }
          
          Serial.println("Opened!");
          HtmlHeaderOK(client);
          while(file.available()) {
            int num_bytes_read;
            uint8_t byte_buffer[32];
            num_bytes_read=file.read(byte_buffer,32);
            client.write(byte_buffer,num_bytes_read);
          }
          file.close();
        }
        else {
          // everything else is a 404
          HtmlHeader404(client);
        }
        break;
      }
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
  }
  
  

}





void initialize_tempsensor(){
  while (!tempsensor.begin()) {
     Serial.println(F("Couldn't find MCP9808!\nTrying Again..."));
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
  server.begin();
  Serial.println("DHCP configured!");
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


//Inputs char array pointer to file name on sd and
//String of what to append to that file;
void sdLog(String fileName, String stringToWrite) {
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




// A function that takes care of the listing of files for the
// main page one sees when they first connect to the arduino.
// it only lists the files in the /data/ folder. Make sure this
// exists on your SD card.
void ListFiles(EthernetClient client) {

  File workingDir = SD.open("/data/");
  
  client.println("<ul>");
  
    while(true) {
      File entry =  workingDir.openNextFile();
      if (! entry) {
        break;
      }
      client.print("<li><a href=\"/HC.htm?file=");
      client.print(entry.name());
      client.print("\">");
      client.print(entry.name());
      client.println("</a></li>");
      entry.close();
    }
  client.println("</ul>");
  workingDir.close();
}





//-------------------RAM SAVING HTML HEADERS----------------------
// Strings stored in flash mem for the Html Header (saves ram)
const char HeaderOK_0[] PROGMEM = "HTTP/1.1 200 OK";            //
const char HeaderOK_1[] PROGMEM = "Content-Type: text/html";    //
const char HeaderOK_2[] PROGMEM = "Connection: keep-alive";     // the connection will be closed after completion of the response
const char HeaderOK_3[] PROGMEM = "Refresh: 5";                 // refresh the page automatically every 5 sec
const char HeaderOK_4[] PROGMEM = "";                           //

// A table of pointers to the flash memory strings for the header
const char* const HeaderOK_table[] PROGMEM = {   
  HeaderOK_0,
  HeaderOK_1,
  HeaderOK_2,
  HeaderOK_3,
  HeaderOK_4
};

// A function for reasy printing of the headers  
void HtmlHeaderOK(EthernetClient client) {
  
    char buffer[30]; //A character array to hold the strings from the flash mem
    
    for (int i = 0; i < 5; i++) {
      strcpy_P(buffer, (char*)pgm_read_word(&(HeaderOK_table[i]))); 
      client.println( buffer );
    }
} 
  
  
// Strings stored in flash mem for the Html 404 Header
const char Header404_0[] PROGMEM = "HTTP/1.1 404 Not Found";     //
const char Header404_1[] PROGMEM = "Content-Type: text/html";    //
const char Header404_2[] PROGMEM = "";                           //
const char Header404_3[] PROGMEM = "<h2>File Not Found!</h2>"; 

// A table of pointers to the flash memory strings for the header
const char* const Header404_table[] PROGMEM = {   
  Header404_0,
  Header404_1,
  Header404_2,
  Header404_3
};

// Easy peasy 404 header function
void HtmlHeader404(EthernetClient client) {
  
    char buffer[30]; //A character array to hold the strings from the flash mem
    
    for (int i = 0; i < 4; i++) {
      strcpy_P(buffer, (char*)pgm_read_word(&(Header404_table[i]))); 
      client.println( buffer );
    }
} 



//<-------------Included Libraries---------------->
#include <Wire.h>
#include "Adafruit_MCP9808.h"
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <EEPROM.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
//<----------------------------------------------->


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
 * testing cabinets. Data is logged on the SD card and live aata can be read 
 * local web page at the arduino's ip address which also lists a history of logs.
 * If temperatures or gas thresholds are surpassed, an email or text can be sent to
 * alert you.
 */

//List emails here that will recieve notifications
//Remeber to update how many emails there are
//<---------------EMAIL SETTINGS------------------>
const String sender_email = "your_email@email.com"
const String emails[] = {"recipient2@email.com", "recipient1@email.com"};
const int emails_length = 2; //number of emails in array to make things easier

//<----------------------------------------------->


//<-------------STATIC DEFINIITONS----------------->
#define UPPER_TEMP_THRESH 28
#define LOWER_TEMP_THRESH 18
#define UPPER_GAS_THRESH 400
#define LOWER_GAS_THRESH 100
#define MEASURE_INTERVAL_NORMAL 30000
#define MEASURE_INTERVAL_EMERGENCY
#define DAYS_BETWEEN_NEW_LOG 1
#define ADDR_COUNT 1 //address for where day count will be stored in EEPROM
#define ADDR_DAY 0 //address for where day number " "
#define HIGH_FREQ 700
#define LOW_FREQ 200

//<------------------------------------------------>



//<-----------------IP SETTINGS-------------------->
byte mac[] = { 0x8A, 0x7F, 0xA7, 0x2F, 0x8D, 0xE0 };  
IPAddress ip(30,30,30,90);
IPAddress gateway(30,30,30,254);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(80);
//char smtpserver[] = "mail.smtp2go.com";
IPAddress smtpserver (207,58,142,213); //equivalent but supposed to work better
int port = 80; //only open port on this network
EthernetClient client;
//<---------------END IP SETTINGS------------------>



//<------------------LOGGING VARIABLES--------------------->
String file_name;
int filenum;
tmElements_t tm;
unsigned long lastIntervalTime = 0;
long measure_interval = MEASURE_INTERVAL_NORMAL; //Time between measurements
//<------------------------------------------------>


//<------------------SENSOR VARIABLES-------------->
// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor0 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor1 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor2 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor3 = Adafruit_MCP9808();
String blank  = "";
String tab = "     ";
const int buzzerPin0 = 15;
const int buzzerPin1 = 16;
const int buzzerPin2 = 17;
const int buzzerPin3 = 18;
const int doorPin0 = 3;
const int doorPin1 = 7; 
const int doorPin2 = 5;
const int doorPin3 = 6;
bool *doors = (bool*) malloc (4 * sizeof(bool));
float *temps = (float*) malloc (4* sizeof(float));
bool emergency_mode = false;
//<------------------------------------------------>




//<-------------------FUNCTIONS-------------------->
//void playSound(int cNum, int frequency)
//void getTemps(float temps*)
//void initialize_ethernet()
//void initialize_sd(void);
//void initialize_tempsensor(void)
//void checkDoors(bool *doors)
//void send_email(String message) //Sends email to everyone on string arrary 'emails'
//void sdLog(File filename, String message)
//<------------------------------------------------>




void setup(){

  //Start Serial monitoring
  Serial.begin(9600);

  //Begin Pins
  //Buzzer Pins 0-3
  pinMode(buzzerPin0, OUTPUT);
  pinMode(buzzerPin1, OUTPUT);
  pinMode(buzzerPin2, OUTPUT);
  pinMode(buzzerPin3, OUTPUT);

  //Door pins 0-3, set them closed by default(high)
  pinMode(doorPin0, INPUT_PULLUP); 
  digitalWrite(doorPin0, HIGH);
  pinMode(doorPin1, INPUT_PULLUP); 
  digitalWrite(doorPin1, HIGH);
  pinMode(doorPin2, INPUT_PULLUP); 
  digitalWrite(doorPin2, HIGH);
  pinMode(doorPin3, INPUT_PULLUP); 
  digitalWrite(doorPin3, HIGH);

  
  //Start SD card
  initialize_sd();
  //sdRWTest(); //Uncomment to test SD card Read/Write

  
  //Start Ethernet Connection
  initialize_ethernet();

  
  //Start Temperature Sensor
  initialize_tempsensor();


  RTC.read(tm);
  Serial.println("Today is " + twoDigitString(tm.Month) + "-" + twoDigitString(tm.Day) + "-" + (String) tmYearToCalendar(tm.Year));
  getTemps(temps);
  Serial.println("Initial Temperatures: " + (String) temps[0] + " " + (String) temps[1] + " " + (String) temps[2] + " " + (String) temps[3]);
}

void loop(){
  //Buzz if any door is open, play corresponding sound
  checkDoors(doors);

  //Check if its time to take a new measurement
  if((millis()%lastIntervalTime) >= measure_interval){ //if its time, get new measuremnt and record it
    
    //Check if a day has passed to create a new log
    RTC.read(tm);
    Serial.println("Today is " + twoDigitString(tm.Month) + "-" + twoDigitString(tm.Day) + "-" + (String) tmYearToCalendar(tm.Year));
  
    //If new day happnes, new log will be started
    //file_name = "data/" + twoDigitString(tm.Month) + "-" + twoDigitString(tm.Day) + "-" + ((String)tmYearToCalendar(tm.Year)).substring(2) + ".CSV"; //update file name with new date
  
    //This code can be used to wait for multiple days to pass. Uncomment below and comment out line above to use;
    
    if(EEPROM.read(ADDR_DAY) != tm.Day){
      Serial.println((String)EEPROM.read(ADDR_DAY) + " != " + (String)tm.Day + " || NEW DAY");
      EEPROM.write(ADDR_DAY, tm.Day); //overwrite EEPROM day
      EEPROM.write(ADDR_COUNT, EEPROM.read(ADDR_COUNT) + 1); //increment count by one
    }
    if number of days to make a new log has occured, make a new log
    if(EEPROM.read(ADDR_COUNT) >= DAYS_BETWEEN_NEW_LOG){
      EEPROM.write(ADDR_COUNT, 0); //reset count
      file_name = "data/" + twoDigitString(tm.Month) + "-" + twoDigitString(tm.Day) + "-" + ((String)tmYearToCalendar(tm.Year)).substring(2) + ".CSV";
    }
    
    getTemps(temps);
    sdLog(file_name, twoDigitString(tm.Month - 1) + '-' + twoDigitString(tm.Day) + '-' + (String)tmYearToCalendar(tm.Year) +  ' ' + twoDigitString(tm.Hour) + ':' + twoDigitString(tm.Minute) + ':' + twoDigitString(tm.Second)
    + ',' + temps[0] + ',' + temps[1] + ',' + temps[2] + ',' + temps[3]);
    //If any sensors are out of bounds, send an email
    for (int n = 0; n < 4; n++){
      if (temps[n] > UPPER_TEMP_THRESH || temps[n] < LOWER_TEMP_THRESH){
        //send_email("The temperature is: " + (String) temps[n] + " which is out of your temperature threshold limits (" + (String) LOWER_TEMP_THRESH + "-" + (String) UPPER_TEMP_THRESH + ").");
        playSound(n+1 ,HIGH_FREQ);
        emergency_mode = true;
      }
    }
    if(emergency_mode){
      Serial.println("EMERGENCY MODE ACTIVATED; MEASUREMENT INTERVAL CHANGED TO 5 SECONDS");
      measure_interval = 5000;
      emergency_mode = false;
     }else{
      measure_interval = MEASURE_INTERVAL_NORMAL;
    }
    lastIntervalTime = millis(); //update time since last interval
  }
    
//<---------------------WEB PAGE CODE-------------------------------------------------->
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


          //Styling to make the website look pretty
          client.print(F("<!DOCTYPE html><html><style> #green{background:limegreen;} #red{background:red;} th{font-weight:bold;} th,td {padding: 12px; color: #fff; text-align: center; border: 0px none;} table{color: #fff; border: 1px solid #ddd; border-collapse: collapse; width: auto;}"));
          client.print(F("h1 {font-size: 42px; color: #fff;} h2{color: #fff; font-size: 32px;} html {background: #ddd; height: 100%;} ul{columns: 5}"));
          client.print(F("body { color: #fff; height: 100%; background: #3498db; box-shadow: 0 0 2px rgba(0, 0, 0, 0.06); color: #545454; font-family: \"Helvetica Neue\", Helvetica, Arial, sans-serif; text-align: center; line-height: 1.5; margin: 0 auto; max-width: 900px; padding: 2em 2em 4em;} li { list-style-type: none; font-size: 18px; font-family: \"Helvetica Neue\", Helvetica, Arial, sans-serif;} </style>"));
          client.print(F("<head><title>Arduino Battery Lab Monitor</title></head><body>"));
          client.print(F("<h1>Arduino Battery Lab Monitor</h1>"));
          client.print(F("<table style=\"width:100%\"><tr><th></th><th>Cabinet 1</th><th>Cabinet 2</th><th>Cabinet 3</th><th>Cabinet 4</th></tr>"));
          client.print(F("<tr><th>Temperature</th>"));
          getTemps(temps); //Update Temperature readings
          checkDoors(doors); //Check again if doors are open
          for (int i = 0; i < 4; i++){
            if (temps[i] > UPPER_TEMP_THRESH || temps[i] < LOWER_TEMP_THRESH){
              client.print(F("<td id=\"red\">"));
            } 
            else {
              client.print(F("<td id=\"green\">"));
            }
            client.print((String)temps[i] + "</td>");
          }
          client.print(F("</tr><tr><th>Door</th>")); 
          for (int i = 0; i < 4; i++){
            if (doors[i]){
              client.print("<td id=\"red\">Open</td>");
            } 
            else {
              client.print(F("<td id=\"green\">Closed</td>"));
            }
          }
          client.print(F("</tr></table>"));
          client.print(F("<h2>View data for the week of (mm-dd-yy):</h2>"));
          ListFiles(client);
          client.print(F("</body></html>"));
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
          //HtmlHeaderOK(client);
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html"));
          client.println("");
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
  
  
  //Serial.println("New Loop");
}





void initialize_tempsensor(){
  if (!tempsensor0.begin(0x18)) {
    Serial.println("Couldn't find Sensor 0");
  }
  if (!tempsensor1.begin(0x19)) {
    Serial.println("Couldn't find Sensor 1");
  }
    if (!tempsensor2.begin(0x1A)) {
    Serial.println("Couldn't find Sensor 2");
  }
    if (!tempsensor3.begin(0x1C)) {
    Serial.println("Couldn't find Sensor 3");
  }
  Serial.println(F("Temperature Sensors Initialized"));
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
  while (!SD.begin(4)) { //Initializes sd card
    Serial.println(F("Card failed, or not present")); //Prints if SD not detected
    //return; // don't do anything more:
    delay(1000);
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


void playSound(int cNum, int frequency) {
  int buzz;
  switch (cNum){
    case 1: buzz = buzzerPin0; break;
    case 2: buzz = buzzerPin1; break;
    case 3: buzz = buzzerPin2; break;
    case 4: buzz = buzzerPin3; break;
  }
  for(int k = 0; k<cNum; k++) {
    tone(buzz, 250, 400);
    delay(100);
    tone(buzz, 0, 400);
    delay(100);
  }
}



void getTemps(float *temps) {
  temps[0] = tempsensor0.readTempC();
  temps[1] = tempsensor1.readTempC();
  temps[2] = tempsensor2.readTempC();
  temps[3] = tempsensor3.readTempC();
 }




void checkDoors(bool *doors){
  if(digitalRead(doorPin0) == HIGH){
    playSound(1, LOW_FREQ);
    doors[0] = true;
    //Serial.println("DOOR 1 is OPEN");
  } else {
      doors[0] = false;
    }
  if(digitalRead(doorPin1) == HIGH){
    playSound(2, LOW_FREQ);
    doors[1] = true;
    //Serial.println("DOOR 2 is OPEN");
  } else {
      doors[1] = false;
  }
  if(digitalRead(doorPin2) == HIGH){
    playSound(3, LOW_FREQ);
    doors[2] = true;
    //Serial.println("DOOR 3 is OPEN");
  } else {
      doors[2] = false;
  }
  if(digitalRead(doorPin3) == HIGH){
    playSound(4, LOW_FREQ);
    doors[3] = true;
    //Serial.println("DOOR 4 is OPEN");
  } else {
      doors[3] = false;
  }
}

String twoDigitString(int num){
  if(num < 10){
    return '0' + (String) num;
  }
  else{
    return (String) num;
  }
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
//-------------------------------------------------------------------


//-------------------EMAIL FUNCTIONS---------------------------------


void send_email(String message)
{
  for(int i = 0; i < emails_length; i++){
    if(sendEmail(message, emails[i])) Serial.println(F("Email sent"));
    else Serial.println(F("Email failed"));
  }
}
 
byte sendEmail(String message, String to_email)
{
  byte thisByte = 0;
  byte respCode;
 
  if(client.connect(smtpserver,port)) {
    Serial.println(F("connected"));
  } else {
    Serial.println(F("connection failed"));
    return 0;
  }
 
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending hello"));
// replace 1.2.3.4 with your Arduino's ip
  client.println("EHLO 30.30.30.90");
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending auth login"));
  client.println("auth login");
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending User"));
// Change to your base64 encoded user
  client.println("YmF0bGFibGVu");
 
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending Password"));
// change to your base64 encoded password
  client.println("YkB0bEBibDNu");
 
  if(!eRcv()) return 0;
 
// change to your email address (sender)
  Serial.println(F("Sending From"));
  client.println("MAIL From: <" + sender_email + ">");
  if(!eRcv()) return 0;
 
// change to recipient address
  Serial.println(F("Sending To"));
  String rcpt_1 = "RCPT To: <" + to_email + ">";
  client.println(rcpt_1);
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending DATA"));
  client.println("DATA");
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending email"));
 
// change to recipient address
  String rcpt_2 = "To: You <" + to_email + ">";
  client.println(rcpt_2);
 
// change to your address
  client.println("From: Me <" + sender_email + ">");
 
  client.println("Subject: Arduino Battery Lab Monitor Update\r\n");
 
  client.println(message);
 
  client.println(".");
 
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending QUIT"));
  client.println("QUIT");
  if(!eRcv()) return 0;
 
  client.stop();
 
  Serial.println(F("disconnected"));
 
  return 1;
}
 
byte eRcv()
{
  byte respCode;
  byte thisByte;
  int loopCount = 0;
 
  while(!client.available()) {
    delay(1);
    loopCount++;
 
    // if nothing received for 10 seconds, timeout
    if(loopCount > 10000) {
      client.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }
 
  respCode = client.peek();
 
  while(client.available())
  {  
    thisByte = client.read();    
    Serial.write(thisByte);
  }
 
  if(respCode >= '4')
  {
    efail();
    return 0;  
  }
 
  return 1;
}
 
 
void efail()
{
  byte thisByte = 0;
  int loopCount = 0;
 
  client.println(F("QUIT"));
 
  while(!client.available()) {
    delay(1);
    loopCount++;
 
    // if nothing received for 10 seconds, timeout
    if(loopCount > 10000) {
      client.stop();
      Serial.println(F("\r\nTimeout"));
      return;
    }
  }
 
  while(client.available())
  {  
    thisByte = client.read();    
    Serial.write(thisByte);
  }
 
  client.stop();
 
  Serial.println(F("disconnected"));
}

//-------------------------------------------------------------------





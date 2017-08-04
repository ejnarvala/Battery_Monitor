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
const String sender_email = "";
const String encoded_user = ""; //encode here and http://base64-encoder-online.waraxe.us/
const String encoded_password = "";
const String emails[] = {"", "", ""};
const int emails_length = 3; //number of emails in array to make things easier
//<----------------------------------------------->


//<-------------STATIC DEFINIITONS----------------->
#define UPPER_TEMP_THRESH_ROOMTEMP 29
#define UPPER_TEMP_THRESH_HIGHTEMP 48
#define UPPER_GAS_THRESH 400
#define MEASURE_INTERVAL_NORMAL 30000
#define MEASURE_INTERVAL_EMERGENCY 5000
#define DAYS_BETWEEN_NEW_LOG 1
#define ADDR_COUNT 1 //address for where day count will be stored in EEPROM
#define ADDR_DAY 0 //address for where day number ''
#define HIGH_FREQ 700
#define LOW_FREQ 200
#define NUM_SENSORS 4
#define NUM_SENSORS_HIGHTEMP 1
#define NUM_DOORS 4
#define NUM_LOG_DAYS_TO_STORE 365
#define DOOR_OPEN_DELAY 5000
//<------------------------------------------------>



//<-----------------IP SETTINGS-------------------->
byte mac[] = { 0x2A, 0x7F, 0xA3, 0x2F, 0x8D, 0xE0 };  
IPAddress ip(30,30,30,90);
IPAddress gateway(30,30,30,254);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(80);
char smtpserver[] = "mail.smtp2go.com"; //for emails
int port = 80; //only open port on this network
EthernetClient client;
//<---------------END IP SETTINGS------------------>



//<------------------VARIABLES--------------------->
String file_name;
int filenum;
tmElements_t tm;
unsigned long lastIntervalTime = 0;
unsigned long lastEmailTime = 0;
long measure_interval = MEASURE_INTERVAL_NORMAL; //Time between measurements
long email_interval = 300000; //time between emails/texts sent in ms
String str2log = "";
String emailstr = "";
char timestamp[30];
File webFile;
//<------------------------------------------------>


//<------------------SENSOR VARIABLES-------------->
// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 *sensors = (Adafruit_MCP9808*)malloc(NUM_SENSORS*sizeof(Adafruit_MCP9808));
int *sensor_addresses = (int*) malloc (NUM_SENSORS*sizeof(int));
const int buzzerPins[NUM_DOORS] = {22,24,26,28};
const int doorPins[NUM_DOORS] = {3,5,6,7};
const int smokerPins[NUM_SENSORS] = {0,1,2,3};
float *smokers = (float*) malloc (NUM_SENSORS * sizeof(float));
bool *doors = (bool*) malloc (NUM_DOORS * sizeof(bool));
float *temps = (float*) malloc (NUM_SENSORS* sizeof(float));
bool *doorWasOpen = (bool*) malloc (NUM_DOORS * sizeof(bool));
long *doorOpenTimes = (long *) malloc (NUM_DOORS * sizeof(long));
bool emergency_mode = false;
//<------------------------------------------------>

void setup(){

  //Start Serial monitoring
  Serial.begin(9600);

  
  //Begin Pins
  //Buzzer Pins 0-3
  //Door pins 0-3, set them closed by default(high)
  for(int i = 0; i < NUM_DOORS; i++){
    pinMode(buzzerPins[i],OUTPUT);
    pinMode(doorPins[i], INPUT_PULLUP); 
    digitalWrite(doorPins[i], HIGH);
    doorOpenTimes[i] = 0;
    doorWasOpen[i] = false;
  }
 
  
  //Start SD card
  initialize_sd();
//  sdRWTest(); //Uncomment to test SD card Read/Write

  //Start Temperature Sensor
  initialize_tempsensor(sensors, sensor_addresses);

  
  //Start Ethernet Connection
  initialize_ethernet();


  RTC.read(tm);
  SdFile::dateTimeCallback(dateTime);
  sprintf(timestamp, "%02d:%02d:%02d %2d/%2d/%2d \n", tm.Hour,tm.Minute,tm.Second,tm.Month,tm.Day,tm.Year+1970);
  Serial.println(timestamp);

  
  getTemps(temps, sensors);
  Serial.print("Initial Temperatures:");
  for(int i = 0; i < NUM_SENSORS; i++){
    Serial.print(" " + (String) temps[i]);
  }
  Serial.println("");
  //set filename upon restart
  file_name = "data/" + twoDigitString(tm.Month) + "-" + twoDigitString(tm.Day) + "-" + ((String)tmYearToCalendar(tm.Year)).substring(2) + ".CSV";

}

void loop(){
  //Buzz if any door is open, play corresponding sound
  checkDoors(doors);

  //Check if its time to take a new measurement
  if((millis()%lastIntervalTime) >= measure_interval){ //if its time, get new measuremnt and record it
    
    //Check if a day has passed to create a new log
    RTC.read(tm);
    Serial.println("Time is " + twoDigitString(tm.Month) + "-" + twoDigitString(tm.Day) + "-" + (String) tmYearToCalendar(tm.Year));
  
    //If new day happnes, new log will be started
    //file_name = "data/" + twoDigitString(tm.Month) + "-" + twoDigitString(tm.Day) + "-" + ((String)tmYearToCalendar(tm.Year)).substring(2) + ".CSV"; //update file name with new date
  
    //This code can be used to wait for multiple days to pass. Uncomment below and comment out line above to use;
    //Serial.println("EERPOM Day is " + (String)EEPROM.read(ADDR_DAY) + " tm.Day is " + (String) tm.Day);
    if(EEPROM.read(ADDR_DAY) != tm.Day){
      Serial.println((String)EEPROM.read(ADDR_DAY) + " != " + (String)tm.Day + " || NEW DAY");
      EEPROM.write(ADDR_DAY, tm.Day); //overwrite EEPROM day
      EEPROM.write(ADDR_COUNT, EEPROM.read(ADDR_COUNT) + 1); //increment count by one
    }
    //if number of days to make a new log has occured, make a new log
    if(EEPROM.read(ADDR_COUNT) >= DAYS_BETWEEN_NEW_LOG){
      EEPROM.write(ADDR_COUNT, 0); //reset count
      file_name = "data/" + twoDigitString(tm.Month) + "-" + twoDigitString(tm.Day) + "-" + ((String)tmYearToCalendar(tm.Year)).substring(2) + ".CSV";
    }
    
    getTemps(temps, sensors);
    checkSmokers(smokers);

    
    str2log = twoDigitString(tm.Month) + '-' + twoDigitString(tm.Day) + '-' + (String)tmYearToCalendar(tm.Year) +  ' ' + twoDigitString(tm.Hour) + ':' + twoDigitString(tm.Minute) + ':' + twoDigitString(tm.Second);
    for(int i = 0; i < NUM_SENSORS; i++){
      str2log += (',' + (String) temps[i]); 
    }
    for(int i = 0; i < NUM_SENSORS; i++){
      if(i == 0){
        str2log +=",GAS=";
      }else{
        str2log +=";";
      }
      str2log += (String) smokers[i]; 
    }
    sdLog(file_name, str2log);
    //Serial.println("Writing: "+ str2log);
    //If any sensors are out of bounds, send an email
    emailstr="";
    for (int n = 0; n < (NUM_SENSORS - NUM_SENSORS_HIGHTEMP); n++){
      if (temps[n] > UPPER_TEMP_THRESH_ROOMTEMP){
        emailstr += "Cabinet " + (String) n + " [Room Temperature Cabinet] has  a Temperature of " + (String) temps[n]+" and a gas level of " + (String) smokers[n] + " PPM.\n";
        playSound(n ,HIGH_FREQ);
        emergency_mode = true;
      }
    }
    for (int n = (NUM_SENSORS - NUM_SENSORS_HIGHTEMP); n < NUM_SENSORS; n++){
      if (temps[n] > UPPER_TEMP_THRESH_HIGHTEMP){
        emailstr += "Cabinet " + (String) n + " [High Temperature Cabinet] has  a Temperature of " + (String) temps[n]+" and a gas level of " + (String) smokers[n] + " PPM.\n";
        playSound(n ,HIGH_FREQ);
        emergency_mode = true;
      }
    }
    if (emailstr != ""){
        if((millis()%lastEmailTime) >= email_interval || lastEmailTime == 0){
          send_email(emailstr);
          lastEmailTime = millis();
        }
    }
    if(emergency_mode){
      Serial.println("EMERGENCY MODE ACTIVATED; MEASUREMENT INTERVAL CHANGED TO 5 SECONDS");
      measure_interval = MEASURE_INTERVAL_EMERGENCY;
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


          //Load top half from file
          webFile = SD.open("excerpt.htm");
          while(webFile.available()){
            client.write(webFile.read());
          }
          webFile.close();
          client.print(F("<head><title>Arduino Battery Lab Monitor</title></head><body>"));
          client.print(F("<h1>Battery Lab Monitor</h1>"));
          client.print(F("<div id=\"live_table\"></div>"));

          client.print(F("<br/><iframe src=\"\" frameBorder=\"0\" style=\"display:none;\"id=\"graphframe\"></iframe><br/>"));
          ListFiles(client);
          client.print(F("</body></html>"));
        }
        else if (strstr(clientline, "GET /json")!=0){ //used for live update data
          client.println(F("HTTP/1.1 200 OK"));
          client.println("Access-Control-Allow-Origin: *");
          client.println(F("Content-Type: application/json;charset=utf-8"));
//          client.println(F("Connection: close"));
          client.println(F(""));
          getTemps(temps, sensors);
          checkDoors(doors);
          checkSmokers(smokers);
          RTC.read(tm);
          //formatting into json
          client.print("{");
          client.print("\"date\": ");
          client.print("\"" +twoDigitString(tm.Month) + '-' + twoDigitString(tm.Day) + '-' + (String)tmYearToCalendar(tm.Year) +  ' ' + twoDigitString(tm.Hour) + ':' + twoDigitString(tm.Minute) + ':' + twoDigitString(tm.Second)+"\"");
          client.print(",\"temps\":[");
          for(int i = 0; i < NUM_SENSORS; i++){
            client.print(temps[i]);
            if( i != (NUM_SENSORS-1) ){
              client.print(",");
            }
          }
          client.print("],\"doors\":[");
          for(int i = 0; i < NUM_DOORS; i++){
            client.print(doors[i]);
            if( i!= (NUM_DOORS-1) ){
              client.print(",");
            }
          }
          client.print("],\"gas\":[");
          for(int i = 0; i < NUM_SENSORS; i++){
            client.print(smokers[i]);
            if( i!= (NUM_SENSORS-1) ){
              client.print(",");
            }
          }
          client.println("]}");
//          client.stop();
          break;
       } else if (strstr(clientline, "GET /") != 0) {
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






void initialize_tempsensor(Adafruit_MCP9808 *sensors, int *sensor_addresses){
  Serial.println("Initializing Temp Sensors");
  for(int i = 0; i < NUM_SENSORS; i++){
    sensor_addresses[i] = 24 + i;
    sensors[i] = Adafruit_MCP9808();
    if(!sensors[i].begin(sensor_addresses[i])){
      Serial.println("Couldn't find Sensor " + (String) i);
    } else{
      Serial.println("Sensor: " + (String) i + " Initialized");
    }
  }
  Serial.println(F("Temperature Sensors Initialized"));
}



// Sets up DHCP, waits 25s to make sure its connected
//sets some pin outputs for sd card
void initialize_ethernet() 
{
  Serial.println("Initializing Ethernet");
  Ethernet.begin(mac, ip, gateway, gateway, subnet);
  server.begin();
  //delay(10000);
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());
}

// call back for file timestamps
void dateTime(uint16_t* date, uint16_t* time) {
  RTC.read(tm);
  sprintf(timestamp, "%02d:%02d:%02d %2d/%2d/%2d \n", tm.Hour,tm.Minute,tm.Second,tm.Month,tm.Day,tm.Year +1970);
  Serial.println(timestamp);
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(tm.Year+1970, tm.Month, tm.Day);
  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(tm.Hour, tm.Minute, tm.Second);
}

//Sets up SD card
void initialize_sd() {
  pinMode(10, OUTPUT); //Needed to not conflict with ethernet
  digitalWrite(10, HIGH);
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
  SdFile::dateTimeCallback(dateTime);
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
  int filecount = 0;
  client.print("<table><tr><th colspan=\"5\">Data Logs</th></tr><tr>");
    while(true) {
      File entry =  workingDir.openNextFile();
      if (! entry) {
        break;
      }
      if ((filecount % 5 == 0) && (filecount != 0)){
        client.print("</tr><tr>");
      }
      client.print("<td><a href=\"#\" onclick=\"return sendframe('HC.htm?file=");
//      client.print("<td><a href=\"/HC.htm?file=");
      client.print(entry.name());
//      client.print("\">");
      client.print("');\">");
      client.print(entry.name());
      client.println("</a></td>");
      entry.close();
      filecount++;
    }
  client.println("</tr></table>");
  //Serial.println("Number of files: " + (String) filecount);
  if(filecount > NUM_LOG_DAYS_TO_STORE){
    removeOldestLog();
  }
  workingDir.close();
}

void removeOldestLog(){
  File workingDir = SD.open("/data/");
  File entry = workingDir.openNextFile();
  SD.remove("/data/" + (String) entry.name());
  Serial.println("Oldest Log Removed");
}

void playSound(int cNum, int frequency) {
  int buzz = buzzerPins[cNum];
  for(int k = 0; k < (cNum + 1); k++) {
    tone(buzz, 550, 400);
    delay(100);
    tone(buzz, 0, 400);
    delay(100);
  }
}



void getTemps(float *temps, Adafruit_MCP9808 *sensors) {
  for(int i = 0; i < NUM_SENSORS; i++){
    temps[i] = sensors[i].readTempC();
  }
 }

void checkSmokers(float *smokers){
  for(int i = 0; i < NUM_SENSORS; i++){
    smokers[i] = analogRead(smokerPins[i]);
  }

}


void checkDoors(bool *doors){
  for(int i = 0; i < NUM_DOORS; i++){
    if((digitalRead(doorPins[i]) == HIGH) && (doorWasOpen[i] == false)){
      doorOpenTimes[i] = millis();
      doorWasOpen[i] = true;
      //Serial.println("Door Opened");
      //Serial.println(doorOpenTimes[i]);
    }
    if(digitalRead(doorPins[i]) == LOW){
      doorWasOpen[i] = false;
      doors[i] = false;
      //Serial.println("Door is closed");
    }else{
      doors[i] = true;
      //Serial.println("Door is open");
      if (millis() % doorOpenTimes[i] >= DOOR_OPEN_DELAY){
        playSound(i, LOW_FREQ);
        //Serial.println("DOOR " + (String) i + " is OPEN");
      }
    }




    
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
//const char HeaderOK_3[] PROGMEM = "Refresh: ";                 // refresh the page automatically every 5 sec
const char HeaderOK_3[] PROGMEM = "";                           //

// A table of pointers to the flash memory strings for the header
const char* const HeaderOK_table[] PROGMEM = {   
  HeaderOK_0,
  HeaderOK_1,
  HeaderOK_2,
  HeaderOK_3
};

// A function for reasy printing of the headers  
void HtmlHeaderOK(EthernetClient client) {
  
    char buffer[30]; //A character array to hold the strings from the flash mem
    
    for (int i = 0; i < 4; i++) {
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
  client.println(encoded_user);
 
  if(!eRcv()) return 0;
 
  Serial.println(F("Sending Password"));
// change to your base64 encoded password
  client.println(encoded_password);
 
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





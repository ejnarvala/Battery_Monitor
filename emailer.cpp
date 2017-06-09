/*
   Email client sketch for IDE v1.0.5 and w5100/w5200
   Posted 7 May 2015 by SurferTim
*/
#include "emailer.h"
#include "Arduino.h"
#include <SPI.h>
#include <Ethernet.h>


// this must be unique
//byte mac[] = { 0x8A, 0x7F, 0xA7, 0x2F, 0x8D, 0xE0 };  
//IPAddress ip(30,30,30,90);
//IPAddress gateway(30,30,30,254);
//IPAddress subnet(255, 255, 255, 0);



//char server[] = "mail.smtp2go.com";
IPAddress server (207,58,142,213); //equivalent but supposed to work better
int port = 80; //only open port on this network


EthernetClient client;


//void initialize_ethernet()
//{
//  Serial.println("Initializing Ethernet");
//  Ethernet.begin(mac, ip, gateway, gateway, subnet);
//  delay(30000);
//  pinMode(4, OUTPUT);
//  digitalWrite(4, HIGH);
//  Serial.println("Ready");
//}






void send_email(String message, String to_email)
{

  if(sendEmail(message, to_email)) Serial.println(F("Email sent"));
  else Serial.println(F("Email failed"));
}
 
byte sendEmail(String message, String to_email)
{
  byte thisByte = 0;
  byte respCode;
 
  if(client.connect(server,port)) {
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
  client.println("MAIL From: <batlablen@gmail.com>");
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
  client.println("From: Me <batlablen@gmail.com>");
 
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

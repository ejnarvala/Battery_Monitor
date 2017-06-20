#include "emailer.h"
#include <Wire.h>
#include "Adafruit_MCP9808.h"
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>


byte mac[] = { 0x8A, 0x7F, 0xA7, 0x2F, 0x8D, 0xE0 };  
IPAddress ip(30,30,30,90);
IPAddress gateway(30,30,30,254);
IPAddress subnet(255, 255, 255, 0);

extern EthernetClient client;
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();


void setup(){
  Serial.begin(9600);
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);
  initialize_ethernet();
  //tempsensor.begin();
  //Serial.println("Press 'e' to send email");
}

void loop(){
  byte inChar;
  inChar = Serial.read();
  if(inChar == 'e')
  {
    String message = "Emailer?";
    String to_email = "aboss@lenovo.com";
    Serial.println("Please Wait, Attmepting to Send");
    send_email(message, to_email);
  }


}





void initialize_ethernet()
{
  Serial.println("Initializing Ethernet");
  Ethernet.begin(mac, ip, gateway, gateway, subnet);
  delay(30000);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  Serial.println("Ready");
}

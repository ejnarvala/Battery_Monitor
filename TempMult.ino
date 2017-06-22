
/**************************************************************************/
/*!
This is a demo for the Adafruit MCP9808 breakout
----> http://www.adafruit.com/products/1782
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!
*/
/**************************************************************************/

#include <Wire.h>
#include "Adafruit_MCP9808.h"

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor0 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor1 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor2 = Adafruit_MCP9808();
Adafruit_MCP9808 tempsensor3 = Adafruit_MCP9808();
String blank  = "";
String tab = "     ";

void initialize();

void setup() {
  Serial.begin(9600);
  Serial.println("MCP9808 demo");
  
  // Make sure the sensor is found, you can also pass in a different i2c
  // address with tempsensor.begin(0x19) for example
initialize();
}

void loop() {
  //Serial.println("wake up MCP9808.... "); // wake up MSP9808 - power consumption ~200 mikro Ampere
  //tempsensor.wake();   // wake up, ready to read!

  // Read and print out the temperature, then convert to *F
  Serial.print(tab + "Sensor 0: " + tempsensor0.readTempC() + "*C");

    Serial.print(tab + "Sensor 1: " + tempsensor1.readTempC()+ "*C");

       Serial.print(tab + "Sensor 2: " + tempsensor2.readTempC()+ "*C");
       
            Serial.print(tab + "Sensor 3: " + tempsensor3.readTempC()+ "*C" );
          
            Serial.println("");
  //Serial.println("Shutdown MCP9808.... ");
  //tempsensor.shutdown(); // shutdown MSP9808 - power consumption ~0.1 mikro Ampere
  
  delay(1000);
}

void initialize() {
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
}


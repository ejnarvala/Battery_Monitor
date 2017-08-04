# Arduino Battery Lab Temperature/Gas/Door Monitor

This program monitors the temperatures, gases, and doors of four separate cabinets where batteries are tested.
Each cabinet contains a door switch, temperature probe, buzzer, and gas probe which are read by an Arduino.
If a cabinet door is opened, the arduino will sound the corresponding buzzer.
The Arduino hosts a locally acessible website which refreshes automatically displaying live temperature, gas, and door readings.
The sensor data is also logged locally on the arduino's SD card and can be viewed on a graph or downloaded through the website.
If temperature or gas values exceed their respective thresholds, an email is sent to the designated recipients and an emergency mode is activated
where probing times speed up.


## Getting Started

Starting is simple, just clone the github directory locally and open the Battery_Monitor.ino file in the Arduino IDE. Import the librarys listed below. Modify the values to your specifications and load it to your Arduino.
On the SD card that will go on the Arduino, in the root directory, copy the HC.htm file and then create an empty folder titled "data".
Insert the SD card and connect an ethernet cable and restart the Arduino. Monitor the serial for feedback.

### Prerequisites

Hardware:
* Arduino Mega
* Arduino Ethernet Shield (W5100)
* ChronoDot 2.1 - DS3231
* MCP9808 Temperature Sensors (4)
* Piezo Buzzers (4)
* 3M Magnetic Door Sensors (4)

Software:
* Arduino IDE

## Built With

* [DS3231RTC Library](https://github.com/JChristensen/DS3232RTC) - Chronodot calibration and library
* [Wiznet Library](https://github.com/Wiznet/WIZ_Ethernet_Library) - Ethernet Libary
* [Adafruit MCP9808 Library](https://github.com/adafruit/Adafruit_MCP9808_Library) - Used to acces Temperature Sensors


## Authors

* **Ejnar Arechavala** - *Software Development* - [Github](https://github.com/ejnarvala)
* **Serina Liu** - *Hardware Development* - [Github](https://github.com/serliu)

## Acknowledgments

* [everettsprojects](http://everettsprojects.com)


# Screenshot
![screenshot](./screenshot.PNH)

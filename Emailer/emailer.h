#ifndef emailer_h
#define emailer_h

#include "Arduino.h"

//extern void initialize_ethernet();
extern void send_email(String message, String to_email);
byte sendEmail(String message, String to_email);
byte eRcv();
void efail();

#endif


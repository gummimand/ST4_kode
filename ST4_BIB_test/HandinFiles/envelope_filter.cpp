#include <Arduino.h>

#include <M5Stack.h>
#include "MPU9250.h"


#define LOWBYTE 0
#define HIGHBYTE 1

#define FILTER_LENGTH2 2

double xe[FILTER_LENGTH2+1] = {0,0,0};
double ye[FILTER_LENGTH2+1] = {0,0,0};
double be[FILTER_LENGTH2 + 1] = {0.0036216815, 0.0072433630, 0.0036216815};
double ae[FILTER_LENGTH2 + 1] = {1.0000000000, -1.8226949252, 0.8371816513};


bool current_byte = LOWBYTE;
int16_t ye_org, ye_filt;
void serialEvent();


int16_t floor_and_convert(double value)
{
  if (value > 0) // positiv
  {
      return (int16_t)(value + 0.5);
  }
  else // negativ
  {
      return (int16_t)(value - 0.5);
  }
}

int16_t iir_filter(int16_t value)
{
    xe[0] =  (double) (value); // Read received sample and perform tyepecast

    ye[0] = be[0]*xe[0];                   // Run IIR filter for first element

    for(int i = 1;i <= FILTER_LENGTH2;i++)   // Run IIR filter for all other elements
    {
        ye[0] += be[i]*xe[i] - ae[i]*ye[i];
    }

    for(int i = FILTER_LENGTH2-1;i >= 0;i--) // Roll xe and ye arrayes in order to hold old sample inputs and outputs
    {
        xe[i+1] = xe[i];
        ye[i+1] = ye[i];
    }

    return floor_and_convert(ye[0]);     // fixe rounding issues;
}


void setup()
{
  Serial.begin(115200);                 // initialize serial:
}

void loop()
{
  serialEvent();
}

void serialEvent()
{
  while (Serial.available())
  {
    if (current_byte == LOWBYTE)
    {
      ye_org = Serial.read();                      // get the new lowbyete
      current_byte = HIGHBYTE;
    }
    else {
      ye_org += (int16_t)(Serial.read()<<8);       // get the new highbyete
      current_byte = LOWBYTE;
      if(ye_org < 0)
        ye_org*=-1;
      ye_filt = iir_filter(ye_org);                 // filter new value
      Serial.write(ye_filt&0xeFF);                  // send low byete first
      Serial.write(ye_filt>>8);                    // send high byete second (this is called littleindian)
    }
  }
}

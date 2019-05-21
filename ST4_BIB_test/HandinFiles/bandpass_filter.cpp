#include <Arduino.h>

#include <M5Stack.h>
#include "MPU9250.h"


#define LOWBYTE 0
#define HIGHBYTE 1

#define FILTER_LENGTH 4

double x[FILTER_LENGTH+1] = {0,0,0,0,0};
double y[FILTER_LENGTH+1] = {0,0,0,0,0};
double b[FILTER_LENGTH + 1] = {0.3705549358, 0.0000000000, -0.7411098715, 0.0000000000, 0.3705549358};
double a[FILTER_LENGTH + 1] = {1.0000000000, -1.5634341687, 0.4548488847, -0.0723023604, 0.1870198984};

//double b[FILTER_LENGTH + 1] = {0.0388885939, 0.0000000000, -0.0777771878, 0.0000000000, 0.0388885939};//200Hz
//double a[FILTER_LENGTH + 1] = {1.0000000000, -3.3572161919, 4.2435012079, -2.4116992993, 0.5254553067};//200Hz


bool current_byte = LOWBYTE;
int16_t y_org, y_filt;
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
    x[0] =  (double) (value); // Read received sample and perform typecast

    y[0] = b[0]*x[0];                   // Run IIR filter for first element

    for(int i = 1;i <= FILTER_LENGTH;i++)   // Run IIR filter for all other elements
    {
        y[0] += b[i]*x[i] - a[i]*y[i];
    }

    for(int i = FILTER_LENGTH-1;i >= 0;i--) // Roll x and y arrays in order to hold old sample inputs and outputs
    {
        x[i+1] = x[i];
        y[i+1] = y[i];
    }

    return floor_and_convert(y[0]);     // fix rounding issues;
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
      y_org = Serial.read();                      // get the new lowbyte
      current_byte = HIGHBYTE;
    }
    else {
      y_org += (int16_t)(Serial.read()<<8);       // get the new highbyte
      current_byte = LOWBYTE;

      y_filt = iir_filter(y_org);                 // filter new value
      Serial.write(y_filt&0xFF);                  // send low byte first
      Serial.write(y_filt>>8);                    // send high byte second (this is called littleindian)
    }
  }
}

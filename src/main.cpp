#include <mbed.h>

Ticker sampleTicker;
AnalogIn sensorPin(PTB0);     // ADC pin
AnalogOut scopePin(PTE30);    // DAC pin
DigitalOut timetestPin(PTC1); // digital pin

volatile float v;

const float alpha = 0.15;
volatile float vLag;

unsigned long counter = 0;
float offset;
float rollingAvg;
volatile float vNorm;

float vMin, vMax;
volatile float vDisc;


// sampling interrupt handler function
void sampler()
{
  timetestPin = 1;
  v = sensorPin.read();
  timetestPin = 0;

  counter++;
  if (v < vMin)
  {
    vMin = v;
  }
  if (v > vMax)
  {
    vMax = v;
  }

  timetestPin = 1;
  // apply first-order lag filter
  vLag = (alpha) * v + (1-alpha) * vLag;
  timetestPin = 0;

  timetestPin = 1;
  // apply rolling average normalisation
  rollingAvg = (rollingAvg + vLag) / counter;
  vNorm = vLag + offset - rollingAvg;
  timetestPin = 0;
}


int main()
{
  timetestPin = 0;
  sampleTicker.attach(&sampler, 4200us); // 240 Hz sampling frequency

  while(1)
  {  
    scopePin.write(vNorm);
  }

  return 0;
}

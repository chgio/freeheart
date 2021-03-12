#include <mbed.h>
#include <max7219.h>
#include <dsp.h>
#include <array>
#include <deque>

std::deque<float> edge;
std::array<float, size> processing;
std::array<float, size> processed;

Ticker sampleTicker;
AnalogIn sensorPin(PTB0); // ADC pin
AnalogOut testPin(PTE30); // DAC pin

// TODO verify consistency of front/back queuing with dsp iterators

// sampling interrupt handler function
void sampler()
{
  float v = sensorPin.read();

  // this is responsible for enforcing the size of the queue
  edge.push_back(v);
  if (edge.size() >= size)
  {
    edge.pop_front();
  }
}


int main()
{
  sampleTicker.attach(&sampler, 420us); // 240 Hz sampling frequency

  while(1)
  {  
    // check if the data is ready to be processed and displayed
    if (edge.size() == size)
    {
      std::copy(edge.begin(), edge.end(), processing.begin());

      firstOrderLag(processing, 0.025);
      rollingAvgNorm(processing);

      testPin.write(edge[size-1]);

      std::copy(processing.begin(), processing.end(), processed.begin());
    }

    // otherwise show some filler display
    else
    {
      // filler here
    }
  }

  return 0;
}

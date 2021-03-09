#include <mbed.h>
#include <deque>

const int size = 8;
const int hsize = size/2;

std::deque<volatile float> edge;
float* processing[size];

Ticker sampleTicker;
AnalogIn sensorPin; // ADC pin

// sampling interrupt handler function
void sampler()
{
  float v = sensorPin.read();

  // this is responsible for enforcing the size of the deque
  edge.push_front(v);
  if (edge.size() > size)
  {
    edge.pop_back();
  }
}


void firstOrderLag(float *queue, float alpha)
{
  // the initial Y is the first item in the "trailing" half-queue
  float yLast = queue[hsize];

  // iterate through the "head" half-queue
  for (int i=hsize-1; i>=0; i--)
  {
    // apply first-order lag processing to each *unprocessed* item based on the
    // previous *processed* item
    float x = queue[i];
    float yCur = (alpha) * x + (1-alpha) * yLast;
  }
}


void rollingAvgNorm(float *queue)
{
  float offset; // TODO what offset?
  float avgs[hsize];
  
  // iterate through the "head" half-queue a first time to compute the rolling 
  // averages up to each item
  for (int i=hsize-1; i>=0; i--)
  {
    float y = 0;

    // this is quadratic? if it takes too long we can try a more "rolling" 
    // approach: at each step we subtract x/hsize for the exiting item and add 
    // x/hsize for the entering item
    for (int j=i+hsize; j>i; j--)
    {
      float x = queue[i];
      y += x / hsize;
    }

    avgs[i] = y;
  }

  // iterate through the "head" half-queue again to offset all items and 
  // subtract the respective rolling averages from each item
  for (int i=hsize-1; i>=0; i--)
  {
    queue[i] = queue[i] + offset - avgs[i];
  }
}


int main()
{
  // put your setup code here, to run once:

  sampleTicker.attach(&sampler, 420us); // 240 Hz sampling frequency

  while(1)
  {
    // put your main code here, to run repeatedly:

    // check if the data is ready to be processed and displayed
    if (edge.size() == size) {
      std::copy(edge.cbegin(), edge.cend(), &processing);

      firstOrderLag(*processing, 0.025);
      rollingAvgNorm(*processing);
    }
    // otherwise show some filler display
    else {
      // filler here
    }
  }
}

#include <mbed.h>
#include <array>
#include <deque>
#include <dsp.h>
#include <tsi_sensor.h>
#include <max7219.h>

std::deque<float> edge;
std::array<float, size> processing;
std::array<int, size> processed;

char buffer[64] = {0};
BufferedSerial serial(USBTX, USBRX);

Ticker sampleTicker;
AnalogIn sensorPin(PTB0);     // ADC pin
AnalogOut scopePin(PTE30);    // DAC pin
DigitalOut timetestPin(PTC1); // digital pin
TSIAnalogSlider slider(PTB16, PTB17, 16);
Max7219 display(PTD2, NC, PTD1, PTD0);


// TODO verify consistency of front/back queuing with dsp iterators

// sampling interrupt handler function
void sampler()
{
  timetestPin = 1;
  float v = sensorPin.read();
  timetestPin = 0;
  printf("read:\t%f\n",
    v
  );
  
  // this is responsible for enforcing the size of the queue
  if (edge.size() >= size)
  {
    edge.pop_front();
  }
  edge.push_back(v);
}


int main()
{
  sampleTicker.attach(&sampler, 4200us); // 240 Hz sampling frequency
  
  serial.set_baud(9600);
  serial.set_format(
    8,
    BufferedSerial::None,
    1
  );

  max7219_configuration_t cfg = {
    .device_number = 1,
    .decode_mode = 0,
    .intensity = Max7219::MAX7219_INTENSITY_8,
    .scan_limit = Max7219::MAX7219_SCAN_8
  };
  display.init_display(cfg);
  display.enable_display();
  display.set_display_test();
  wait_us(10);
  display.clear_display_test();

  while(1)
  {  
    // check if the data is ready to be processed and displayed
    if (edge.size() == size)
    {
      std::copy(edge.begin(), edge.end(), edge.begin());

      firstOrderLag(processing, 0.025);
      rollingAvgNorm(processing);

      float v = edge[size-1];
      scopePin.write(v);
      printf("write:\t%f\n",
        v
      );

      std::copy(processing.begin(), processing.end(), processed.begin());

      float brightness = slider.readDistance();
      printf("slider:\t%f\n",
        brightness
      );

      cfg.intensity = brightness;
      display.init_display(cfg);
    }

    // otherwise show some filler display
    else
    {
      // filler here
    }
  }

  return 0;
}

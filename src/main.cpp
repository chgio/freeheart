#include <mbed.h>
#include <array>
#include <nonstd/ring_span.hpp>
#include <freeheart/dsp.h>
#include <tsi_sensor.h>
#include <max7219.h>

std::array<volatile float, size> edge;
nonstd::ring_span<volatile float> edgeRing(edge.begin(), edge.end());
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
  volatile float v = sensorPin.read();
  timetestPin = 0;
  
  printf("read:\t%f\n",
    v
  );

  edgeRing.push_back(v);
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
    if (edgeRing.size() == size)
    {
      std::copy(edgeRing.begin(), edgeRing.end(), processing.begin());

      firstOrderLag(processing, 0.025);
      rollingAvgNorm(processing);

      float v = edgeRing[size-1];
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

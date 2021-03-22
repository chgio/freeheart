#include <mbed.h>
#include <numeric>
#include <array>
#include <tsi_sensor.h>
#include <max7219.h>

char buffer[64] = {0};
BufferedSerial serial(USBTX, USBRX);

Ticker sampleTicker;
AnalogIn sensorPin(PTB0);     // ADC pin
AnalogOut scopePin(PTE30);    // DAC pin
DigitalOut timetestPin(PTC1); // digital pin
TSIAnalogSlider slider(PTB16, PTB17, 16);
Max7219 display(PTD2, NC, PTD1, PTD0);

volatile float v;

const float alpha = 0.025;
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

  printf("read:\t%f\n",
    v
  );
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
  vNorm = vNorm + offset - rollingAvg;
  timetestPin = 0;

  timetestPin = 1;
  // compute 8-step discretisation ladder
  float qsize = (vMin-vMax)/8;
  std::array<float, 7> qhigherbounds;
  for (int i=0; i<=7; i++)
  {
    qhigherbounds[i] = vMin + (i+2)*qsize;
  }

  // apply 8-step discretisation
  vDisc = 0;
  for (int j=0; j<8; j++)
  {
    if (vNorm <= qhigherbounds[j])
    {
      vDisc = j+1;
      break;
    }
  }
  timetestPin = 0;
}


int main()
{
  timetestPin = 0;
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
    scopePin.write(vNorm);
    printf("write:\t%f\n",
      v
    );

    float brightness = slider.readDistance();
    printf("slider:\t%f\n",
      brightness
    );

    cfg.intensity = brightness;
    display.init_display(cfg);
  }

  return 0;
}

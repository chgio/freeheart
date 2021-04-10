#include <mbed.h>
#include <array>
#include <nonstd/ring_span.hpp>
#include <freeheart/dsp.hpp>
#include <freeheart/displaymaps.hpp>
#include <tsi_sensor.h>
#include <max7219.h>

std::array<volatile float, size> edge;
nonstd::ring_span<volatile float> edgeRing(edge.begin(), edge.end());
std::array<float, size> processing;
std::array<int, size> processed;

Ticker sampleTicker;
AnalogIn sensorPin(PTB0);     // ADC pin
AnalogOut scopePin(PTE30);    // DAC pin
TSIAnalogSlider slider(PTB16, PTB17, 16);
Max7219 display(PTD2, NC, PTD1, PTD0);


// sampling interrupt handler function
void sampler()
{
    volatile float v = sensorPin.read();
    edgeRing.push_back(v);
}


int main()
{
    sampleTicker.attach(&sampler, 4200us); // 240 Hz sampling frequency

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

    while (1)
    {
        // check if the data is ready to be processed and displayed
        if (edgeRing.size() == size)
        {
            std::copy(edgeRing.begin(), edgeRing.end(), processing.begin());

            firstOrderLag(processing, 0.15);
            rollingAvgNorm(processing, 0);
            aggregate(processing);

            float v = edgeRing[size - 1];
            scopePin.write(v);

            std::copy(processing.begin(), processing.end(), processed.begin());

            float bpmv = bpm(processing, 200, 0.3, 0.7);

            float brightness = slider.readDistance();
            printf("slider:\t%f\n",
                   brightness);

            cfg.intensity = brightness;
            display.init_display(cfg);
            display.enable_display();

            for (int i=0; i<size; i++)
            {
                int d = processed[size-8+i];
                display.write_digit(1, i, barmap.at(d));
            }
        }

        // otherwise show some filler display
        else
        {
            display.set_display_test();
        }
    }

    return 0;
}

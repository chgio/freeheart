#include <array>
#include <vector>
#include <numeric>

const int size = 960; // multiple of 8
const int tsize = 32;
const int hsize = size - tsize;
const int osize = (int)size / 8;


void firstOrderLag(std::array<float, size> queue, float alpha)
{
    // iterate through the "head" sub-queue
    for (int i=tsize; i<size; i++)
    {
        // apply first-order lag filtering to each *unprocessed* item based on the
        // previous *processed* item
        float x = queue[i];
        float y = queue[i - 1];
        queue[i] = (alpha) * x + (1-alpha) * y;
    }
}


void rollingAvgNorm(std::array<float, size> queue, float offset)
{
    std::array<float, size> avgs;

    // iterate through the "head" sub-queue a first time to compute the rolling
    // averages up to each item
    for (int i=tsize; i<size; i++)
    {
        float y = 0;

        for (int j=i-tsize; j<i; j++)
        {
            float x = queue[j];
            y += x / tsize;
        }

        avgs[i] = y;
    }

    // iterate through the "head" sub-queue again to offset all items and
    // subtract the respective rolling averages from each item
    for (int i=tsize; i<size; i++)
    {
        queue[i] = queue[i] + offset - avgs[i];
    }
}


void aggregate(std::array<float, size> queue)
{
    std::pair<float*, float*> qminmax = std::minmax_element(queue.begin(), queue.end());

    float qmin = *qminmax.first;
    float qmax = *qminmax.second;
    float delta = (qmax - qmin) / 7;

    for (int i=tsize; i<size; i++)
    {
        queue[i] = static_cast<int>((queue[i] - qmin)) / static_cast<int>(delta);
    }
}


float bpm(std::array<float, size> queue, float sample_freq, float rmin, float rmax)
{
    std::pair<float*, float*> qminmax = std::minmax_element(queue.begin(), queue.end());

    float qmin = *qminmax.first;
    float qmax = *qminmax.second;
    float delta = qmax - qmin;
    float tmin = qmin + rmin * delta;
    float tmax = qmin + rmax * delta;

    std::vector<float> pulseTs;
    int ilast = 0;
    bool above = false;

    for (int i=tsize; i<size; i++)
    {
        if (!above && queue[i] >= tmax)
        {
            pulseTs.push_back(i-ilast);
            ilast = i;
            above = true;
        }
        else if (above && queue[i] <= tmin)
        {
            above = false;
        }
    }

    float avgT = std::accumulate(pulseTs.begin(), pulseTs.end(), 0) / pulseTs.size();
    return 60 / avgT;
}

#include <array>
#include <numeric>

const int size = 16; // multiple of 8
const int bsize = (int) size/2;
const int osize = (int) size/8;


void firstOrderLag(std::array<float, size> queue, float alpha)
{
  // the initial Y is the first item in the "trailing" half-queue
  float yLast = queue[bsize-1];

  // iterate through the "head" half-queue
  for (int i=bsize; i<size; i++)
  {
    // apply first-order lag processing to each *unprocessed* item based on the
    // previous *processed* item
    float x = queue[i];
    queue[i] = (alpha) * x + (1-alpha) * yLast;
  }
}


void rollingAvgNorm(std::array<float, size> queue)
{
  float offset; // TODO what offset?
  std::array<float, size> avgs;
  
  // iterate through the "head" half-queue a first time to compute the rolling 
  // averages up to each item
  for (int i=bsize; i<size; i++)
  {
    float y = 0;

    // this is quadratic? if it takes too long we can try a more "rolling" 
    // approach: at each step we subtract x/bsize for the exiting item and add 
    // x/bsize for the entering item
    for (int j=i-bsize; j<i; j++)
    {
      float x = queue[i];
      y += x / bsize;
    }

    avgs[i] = y;
  }

  // iterate through the "head" half-queue again to offset all items and 
  // subtract the respective rolling averages from each item
  for (int i=bsize; i<size; i++)
  {
    queue[i] = queue[i] + offset - avgs[i];
  }
}


void aggregate(std::array<float, size> queue)
{
  const auto [mine, maxe] = std::minmax_element(queue.begin(), queue.end());

  // TODO fix this ??
  float qsize = (maxe-mine)/8;
  std::array<float, 7> qhigherbounds;
  for (int i=0; i<=7; i++)
  {
    qhigherbounds[i] = *mine + (i+2)*qsize;
  }

  for (int i=bsize; i<size; i++)
  {
    float aggh = std::accumulate(queue.begin(), queue.end(), 0.0f) / osize;
    int aggv;

    for (int j=0; j<8; j++)
    {
      if (aggh <= qhigherbounds[j])
      {
        aggv = j+1;
        break;
      }
    }
  }
}

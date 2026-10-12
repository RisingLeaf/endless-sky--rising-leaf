// Taken and modified from https://github.com/weigert/SimpleTectonics/blob/master/README.md which is under the MIT
// License

#include "poisson.h"

#include "math/graphics_math/gm.h"

#include <functional>
#include <random>
#include <vector>

namespace poisson
{
  thread_local std::mt19937                   gen;
  thread_local std::uniform_real_distribution dis(0.0f, 1.0f);

  constexpr static float uniform(const float a, const float b)
  {
    return dis(gen) * (a - b) + b;
  }
} // namespace poisson

// Single point at position towards p with radius R
bool poisson::disc(std::vector<gm::vec2> &set, const float R, const gm::vec2 &p, const gm::vec2 &a, const gm::vec2 &b)
{
  if(set.empty())
  {
    set.push_back(p);
    return true;
  }

  const float length = R / std::sqrt(2.0f);
  const int   NX     = static_cast<int>(std::ceil((b[0] - a[0]) / length));
  const int   NY     = static_cast<int>(std::ceil((b[1] - a[1]) / length));
  auto        grid   = std::vector<size_t>(NX * NY, 0);

  const auto AddGrid = [&](const gm::vec2 &s, const size_t val)
  {
    const gm::vec2 s_a = s - a;
    const gm::vec2 b_a = b - a;

    gm::ivec2 ind              = gm::ivec2(static_cast<float>(NX) * s_a[0] / b_a[0], static_cast<float>(NY) * s_a[1] / b_a[1]);
    grid[ind[0] * NY + ind[1]] = val;
  };

  // Set is Not Empty - Fill Grid
  for(size_t i = 0; i < set.size(); i++)
    AddGrid(set[i], i + 1);

  // Find the Nearest Guy - Attempt to Place Nearby
  size_t near = 0;
  float  dist = gm::length(a - b);
  for(unsigned int i = 0; i < set.size(); i++)
  {
    if(gm::length(set[i] - p) < dist)
    {
      dist = gm::length(set[i] - p);
      near = i;
    }
  }

  // Generate Nearby Point
  int tries = 16;
  while(tries > 0)
  {
    tries--;

    const float nr = uniform(R, 2 * R);
    const float nt = uniform(0, 2 * 3.14159265);

    gm::vec2 new_pos = set[near] + nr * gm::vec2(std::cos(nt), std::sin(nt));
    if(gm::any(gm::less_than(new_pos, gm::vec2(a))) || gm::any(gm::greater_than_equals(new_pos, gm::vec2(b))))
      continue;

    const gm::vec2 s_a = new_pos - a;
    const gm::vec2 b_a = b - a;

    const auto ind = gm::ivec2(static_cast<float>(NX) * s_a[0] / b_a[0], static_cast<float>(NY) * s_a[1] / b_a[1]);

    bool free = true;
    for(int i = ind[0] - 2; i <= ind[0] + 2 && free; i++)
    {
      for(int j = ind[1] - 2; j <= ind[1] + 2 && free; j++)
      {

        if(i < 0 || i >= NX || j < 0 || j >= NY)
          continue;

        // Non-Zero Entry,and Too Close
        if(grid[i * NY + j] > 0)
        {
          if(gm::length(set[grid[i * NY + j] - 1] - new_pos) < R)
            free = false;
        }
      }
    }

    if(free)
    {
      set.push_back(new_pos);
      AddGrid(set.back(), set.size());
      return true;
    }
  }
  return false;
}

// Sample Set of Points in Region
void poisson::disc(std::vector<gm::vec2> &set, size_t k, const gm::vec2 &a, const gm::vec2 &b, const asl::uint64 seed)
{
  gen                = std::mt19937(seed);
  const float r      = std::sqrt(gm::dot(b - a, b - a) / 3.14159265f / static_cast<float>(k));
  const float length = r / std::sqrt(2.f); // Edge Length
  const int   NX     = static_cast<int>(std::ceil((b[0] - a[0]) / length));
  const int   NY     = static_cast<int>(std::ceil((b[1] - a[1]) / length));
  auto        grid   = std::vector<size_t>(NX * NY, 0);

  // Grid Addition Function
  const auto AddGrid = [&](const gm::vec2 &s, const size_t val)
  {
    const gm::vec2 s_a = s - a;
    const gm::vec2 b_a = b - a;

    const auto ind = gm::ivec2(static_cast<float>(NX) * s_a[0] / b_a[0], static_cast<float>(NY) * s_a[1] / b_a[1]);

    grid[ind[0] * NY + ind[1]] = val;
  };

  // Setup Grid
  if(set.empty())
  {
    set.push_back(0.5f * (a + b)); // Center
    AddGrid(set.back(), set.size());
    k--;
  }
  else
  {
    for(unsigned int i = 0; i < set.size(); i++)
      AddGrid(set[i], i + 1);
  }

  // Now Generate New Samples by Iterating over the Set
  size_t tries = 0;

  while(k > 0)
  {
    if(tries > set.size())
      break;

    // Sample Uniformly from Set
    const int n = static_cast<int>(uniform(0, static_cast<float>(set.size())));

    // Generate Sample Surrounding It
    const float nr = uniform(r, 2 * r);
    const float nt = uniform(0, 2 * 3.14159265);

    // New Sample Position, Grid Index
    gm::vec2 new_pos = set[n] + nr * gm::vec2(std::cos(nt), std::sin(nt));
    if(gm::any(gm::less_than(new_pos, gm::vec2(a))) || gm::any(gm::greater_than_equals(new_pos, gm::vec2(b))))
      continue;


    const gm::vec2 s_a = new_pos - a;
    const gm::vec2 b_a = b - a;

    const auto ind = gm::ivec2(static_cast<float>(NX) * s_a[0] / b_a[0], static_cast<float>(NY) * s_a[1] / b_a[1]);


    bool free = true;
    for(int i = ind[0] - 2; i <= ind[0] + 2 && free; i++)
    {
      for(int j = ind[1] - 2; j <= ind[1] + 2 && free; j++)
      {

        if(i < 0 || i >= NX || j < 0 || j >= NY)
          continue;

        // Non-Zero Entry,and Too Close
        if(grid[i * NY + j] > 0)
        {
          if(gm::length(set[grid[i * NY + j] - 1] - new_pos) < r)
            free = false;
        }
      }
    }

    if(!free)
    {
      tries++;
      continue;
    }

    // Place Point
    set.push_back(new_pos);
    AddGrid(set.back(), set.size());
    tries = 0;
    k--;
  }
}

// Those functions could probably be improved. Not sure how right now. Recursiveness?

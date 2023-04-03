#pragma once
#include <numeric>

constexpr uint32_t tick_size = 15;//ms
constexpr float dt = tick_size * 0.001f;

static inline uint32_t time_to_tick(uint32_t time)
{
  return time / tick_size;
}

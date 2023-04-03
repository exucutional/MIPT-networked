#pragma once

struct Snapshot
{
  float x;
  float y;
  float ori;
  uint32_t time;
};

struct InputSnapshot
{
  float thr;
  float steer;
  uint32_t time;
};

#pragma once
#include "mathUtils.h"
#include "bitstream.h"
#include <cassert>

float step(float lo, float hi, int num_bits)
{
  int range = (1 << num_bits) - 1;
  return (hi - lo) / range;
}

template<typename T>
T pack_float(float v, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1;//std::numeric_limits<T>::max();
  return range * ((clamp(v, lo, hi) - lo) / (hi - lo));
}

template<typename T>
float unpack_float(T c, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1;//std::numeric_limits<T>::max();
  return float(c) / range * (hi - lo) + lo;
}

template<typename T, int num_bits>
struct PackedFloat
{
  T packedVal;

  PackedFloat(float v, float lo, float hi) { pack(v, lo, hi); }
  PackedFloat(T compressed_val) : packedVal(compressed_val) {}

  void pack(float v, float lo, float hi) { packedVal = pack_float<T>(v, lo, hi, num_bits); }
  float unpack(float lo, float hi) { return unpack_float<T>(packedVal, lo, hi, num_bits); }
};

template<typename T, int num_bits1, int num_bits2>
struct PackedVec2
{
  T packedVal;
  class float2
  {
    float val[2];
   public:
    float& operator [](int idx)
    {
      return val[idx];
    }
  };
  PackedVec2(float v1, float v2, float lo, float hi) { pack(v1, v2, lo, hi); }
  PackedVec2(T compressed_val) : packedVal(compressed_val) {}

  void pack(float v1, float v2, float lo, float hi)
  {
    const auto packedVal1 = pack_float<T>(v1, lo, hi, num_bits1);
    const auto packedVal2 = pack_float<T>(v2, lo, hi, num_bits2);
    packedVal = packedVal1 << num_bits2 | packedVal2;
  }
  float2 unpack(float lo, float hi)
  {
    float2 vec;
    vec[0] = unpack_float<T>(packedVal >> num_bits2, lo, hi, num_bits1);
    vec[1] = unpack_float<T>(packedVal & ((1 << num_bits2) - 1), lo, hi, num_bits2);
    return vec;
  }
};

template<typename T, int num_bits1, int num_bits2, int num_bits3>
struct PackedVec3
{
  T packedVal;
  class float3
  {
    float val[3];
   public:
    float& operator [](int idx)
    {
      return val[idx];
    }
  };
  PackedVec3(float v1, float v2, float v3, float lo, float hi) { pack(v1, v2, v3, lo, hi); }
  PackedVec3(T compressed_val) : packedVal(compressed_val) {}

  void pack(float v1, float v2, float v3, float lo, float hi)
  {
    const auto packedVal1 = pack_float<T>(v1, lo, hi, num_bits1);
    const auto packedVal2 = pack_float<T>(v2, lo, hi, num_bits2);
    const auto packedVal3 = pack_float<T>(v3, lo, hi, num_bits3);
    packedVal = (packedVal1 << (num_bits2 + num_bits3)) | (packedVal2 << num_bits3) | packedVal3;
  }
  float3 unpack(float lo, float hi)
  {
    float3 vec;
    vec[0] = unpack_float<T>(packedVal >> (num_bits2 + num_bits3), lo, hi, num_bits1);
    vec[1] = unpack_float<T>((packedVal >> num_bits3) & ((1 << num_bits2) - 1), lo, hi, num_bits2);
    vec[2] = unpack_float<T>(packedVal & ((1 << num_bits3) - 1), lo, hi, num_bits3);
    return vec;
  }
};

typedef PackedFloat<uint8_t, 4> float4bitsQuantized;

void unit_test()
{
  PackedFloat<uint8_t, 4> floatPacked(0.3f, -1.0f, 1.0f);
  auto unpackedFloat = floatPacked.unpack(-1.0f, 1.0f);
  assert(abs(unpackedFloat - 0.3f) <= step(-1.0f, 1.0f, 4));

  PackedVec2<uint32_t, 10, 15> vec2(40.0f, -10.0f, -50.0f, 50.0f);
  auto unpackedVec2 = vec2.unpack(-50.0f, 50.0f);
  assert(abs(unpackedVec2[0] - 40.0f) <= step(-50.0f, 50.0f, 10));
  assert(abs(unpackedVec2[1] - -10.0f) <= step(-50.0f, 50.0f, 15));

  PackedVec3<uint16_t, 4, 5, 6> vec3(0.5f, 0.9f, -0.3f, -1.0f, 1.0f);
  auto unpackedVec3 = vec3.unpack(-1.0f, 1.0f);
  assert(abs(unpackedVec3[0] - 0.5f) <= step(-1.0f, 1.0f, 4));
  assert(abs(unpackedVec3[1] - 0.9f) <= step(-1.0f, 1.0f, 5));
  assert(abs(unpackedVec3[2] - -0.3f) <= step(-1.0f, 1.0f, 6));

  byte mem[1024];
  auto writeBitstream = Bitstream(mem, 1024 * sizeof(byte));
  auto readBitstream = Bitstream(mem, 1024 * sizeof(byte));
  writeBitstream.writePackedUint32(127);
  writeBitstream.writePackedUint32(12545);
  writeBitstream.writePackedUint32(16383);
  writeBitstream.writePackedUint32(16384);
  assert(writeBitstream.cur_size_ == sizeof(uint8_t) + 2 * sizeof(uint16_t) + sizeof(uint32_t));
  assert(readBitstream.readPackedUint32() == 127);
  assert(readBitstream.readPackedUint32() == 12545);
  assert(readBitstream.readPackedUint32() == 16383);
  assert(readBitstream.readPackedUint32() == 16384);
  assert(readBitstream.cur_size_ == sizeof(uint8_t) + 2 * sizeof(uint16_t) + sizeof(uint32_t));
}

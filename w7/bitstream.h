#pragma once
#include <cassert>
#include <numeric>

using byte = uint8_t;

extern void unit_test();

class Bitstream
{
public:
  explicit Bitstream(byte* ptr, size_t size) : ptr_(ptr), cur_size_(0), max_size_(size) {}
  friend void unit_test();
  template <typename T>
  void write(const T& value);

  template <typename T, typename... Ts>
  void write(const T& val, const Ts&... rest);

  template <typename T>
  void read(T& val);

  uint32_t readPackedUint32();
  void writePackedUint32(uint32_t value);

  template <typename T, typename... Ts>
  void read(T& val, Ts&... rest);
private:
  byte* ptr_;
  size_t cur_size_;
  size_t max_size_;
};

template <typename T>
void Bitstream::write(const T& value)
{
  cur_size_ += sizeof(T);
  assert(cur_size_ <= max_size_);
  memcpy(ptr_, &value, sizeof(T));
  ptr_ += sizeof(T);
}

template <typename T, typename... Ts>
void Bitstream::write(const T& val, const Ts&... rest)
{
  write(val);
  (write(rest), ...);
}

template <typename T>
void Bitstream::read(T& value)
{
  cur_size_ += sizeof(T);
  assert(cur_size_ <= max_size_);
  memcpy(&value, ptr_, sizeof(T));
  ptr_ += sizeof(T);
}

template <typename T, typename... Ts>
void Bitstream::read(T& val, Ts&... rest)
{
  read(val);
  (read(rest), ...);
}

uint32_t Bitstream::readPackedUint32()
{
  uint32_t* data = reinterpret_cast<uint32_t*>(ptr_);
  uint32_t value = 0;
  if ((*data & 0x1) == 0)       // uint8_t
  {
    cur_size_ += sizeof(uint8_t);
    assert(cur_size_ <= max_size_);
    value = (*data & 0xFF) >> 1;
    ptr_ += sizeof(uint8_t);
  }
  else if ((*data & 0x3) == 1)  // uint16_t
  {
    cur_size_ += sizeof(uint16_t);
    assert(cur_size_ <= max_size_);
    value = (*data & 0xFFFF) >> 2;
    ptr_ += sizeof(uint16_t);
  }
  else                          // uint32_t
  {
    cur_size_ += sizeof(uint32_t);
    assert(cur_size_ <= max_size_);
    value = (*data) >> 2;
    ptr_ += sizeof(uint32_t);
  }
  return value;
}

void Bitstream::writePackedUint32(uint32_t value)
{
  if (value <= 0x7F)
  {
    cur_size_ += sizeof(uint8_t);
    assert(cur_size_ <= max_size_);
    uint8_t* data = reinterpret_cast<uint8_t*>(ptr_);
    *data = (value << 1) & 0xFE;
    ptr_ += sizeof(uint8_t);
  }
  else if (value <= 0x3FFF)
  {
    cur_size_ += sizeof(uint16_t);
    assert(cur_size_ <= max_size_);
    uint16_t* data = reinterpret_cast<uint16_t*>(ptr_);
    *data = (value << 2) & 0xFFFC | 0x1;
    ptr_ += sizeof(uint16_t);
  }
  else if (value <= 0x3FFF'FFFF)
  {
    cur_size_ += sizeof(uint32_t);
    assert(cur_size_ <= max_size_);
    uint32_t* data = reinterpret_cast<uint32_t*>(ptr_);
    *data = (value << 2) & 0xFFFF'FFFC | 0x3;
    ptr_ += sizeof(uint32_t);
  }
  assert(true);
}

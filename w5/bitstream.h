#pragma once
#include <cassert>
namespace hw5
{

using byte = uint8_t;

class Bitstream
{
public:
  explicit Bitstream(byte* ptr, size_t size) : ptr_(ptr), cur_size_(0), max_size_(size) {}

  template <typename T>
  void write(const T& value);

  template <typename T, typename... Ts>
  void write(const T& val, const Ts&... rest);

  template <typename T>
  void read(T &val);

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

} // namespace hw4
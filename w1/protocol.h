#pragma once
#include <cstdint>
#include <sys/types.h>
#include <sys/select.h>
#include <netdb.h>
#include <cassert>
#include <ctime>

enum class MessageType : uint32_t
{
  Handshake,
  Data,
  KeepAlive,
};

using mid_t = uint32_t; 
using mtime_t = std::time_t;
using msize_t = uint32_t;

constexpr mid_t INVALID_ID = -1;
constexpr uint32_t KEPP_ALIVE = 10; // seconds

struct Message
{
  MessageType type;
  msize_t size;
};

struct HandshakeMessage
{
  MessageType type;
  // Size of payload
  msize_t size;
  // Server give each client personal id 
  mid_t id;
  // Synchronization time between client and server
  mtime_t time;
  mtime_t keepAliveTime;
};

struct DefaultMessage
{
  MessageType type;
  msize_t size;
  // Sender id (0 for server)
  mid_t id;
};

struct KeepAliveMessage
{
  MessageType type;
  msize_t size;
  // Client id
  mid_t id;
};

template <typename T>
ssize_t send_message(int fd_, T *msg_, const void *payload_, addrinfo *addrInfo_)
{
  constexpr size_t buf_size = 1000;
  static char buffer[buf_size];
  memset(buffer, 0, buf_size);
  assert(msg_);
  assert(addrInfo_);
  assert(buf_size > sizeof(T) + msg_->size);
  memcpy(buffer, static_cast<void*>(msg_), sizeof(T));
  if (payload_)
  {
    memcpy(buffer + sizeof(T), payload_, msg_->size);
  }
  return sendto(fd_, static_cast<void*>(buffer), sizeof(T) + msg_->size, 0,
    addrInfo_->ai_addr, addrInfo_->ai_addrlen);
}

void handshake(int fd_, mid_t id, addrinfo *addrInfo_);

const Message* get_message(int fd_, fd_set *readSet);

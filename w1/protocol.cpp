#include <iostream>
#include <cstring>
#include "protocol.h"

void handshake(int fd_, mid_t id, addrinfo *addrInfo_)
{
  HandshakeMessage msg
  {
    .type = MessageType::Handshake,
    .size = 0,
    .id = id,
    .time = std::time(nullptr),
    .keepAliveTime = KEPP_ALIVE/2,
  };
  ssize_t res = send_message(fd_, &msg, nullptr, addrInfo_);
  if (res == -1)
    std::cout << strerror(errno) << std::endl;
}

const Message* get_message(int fd_, fd_set *readSet)
{
  if (FD_ISSET(fd_, readSet))
  {
    constexpr size_t buf_size = 1000;
    static char buffer[buf_size];
    memset(buffer, 0, buf_size);

    ssize_t numBytes = recvfrom(fd_, buffer, buf_size - 1, 0, nullptr, nullptr);
    if (numBytes > 0)
    {
      Message* msg = reinterpret_cast<Message*>(buffer);
      return msg;
    }
  }
  return nullptr;
}

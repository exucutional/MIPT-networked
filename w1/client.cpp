#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <ctime>
#include <chrono>
#include "socket_tools.h"
#include "protocol.h"

constexpr uint32_t sendPeriod = 2; // seconds

int main(int argc, const char **argv)
{
  const char *sport = "2022";
  const char *rport = "2023";

  addrinfo resAddrInfo;
  int sfd = create_dgram_socket("localhost", sport, &resAddrInfo);
  int rfd = create_dgram_socket(nullptr, rport, nullptr);

  if (sfd == -1 || rfd == -1)
  {
    printf("Cannot create a socket\n");
    return 1;
  }

  // Start connection with server by requesting personal id
  mid_t myId = INVALID_ID;
  handshake(sfd, myId, &resAddrInfo);

  auto sendTimerNow = std::chrono::high_resolution_clock::now();
  auto keepAliveNow = std::chrono::high_resolution_clock::now();

  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(rfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(rfd + 1, &readSet, NULL, NULL, &timeout);

    // Send message with text "Hello" every sendPeriod seconds
    auto time = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>
      (time - sendTimerNow).count() > sendPeriod)
    {
      sendTimerNow = time;
      std::string payload = "Hello";
      DefaultMessage message
      {
        .type = MessageType::Data,
        .size = static_cast<msize_t>(payload.size()),
        .id = myId,
      };
      std::cout << "Sending message: " << payload << "\n";
      ssize_t res = send_message(sfd, &message, payload.c_str(), &resAddrInfo);
      if (res == -1)
        std::cout << strerror(errno) << std::endl;
    }

    //Send keep alive every KEEP_ALIVE/2 seconds
    if (std::chrono::duration_cast<std::chrono::seconds>
      (time - keepAliveNow).count() > KEPP_ALIVE/2)
    {
      keepAliveNow = time;
      KeepAliveMessage message
      {
        .type = MessageType::KeepAlive,
        .size = 0,
        .id = myId,
      };
      ssize_t res = send_message(sfd, &message, nullptr, &resAddrInfo);
      if (res == -1)
        std::cout << strerror(errno) << std::endl;
    }

    // Listen answers to handshake and messages from server
    if (FD_ISSET(rfd, &readSet))
    {
      const Message* msg = get_message(rfd, &readSet);
      if (msg && msg->type == MessageType::Handshake)
      {
        const HandshakeMessage *hmsg = reinterpret_cast<const HandshakeMessage*>(msg);
        printf("Handshake from server. My id is %d\n", hmsg->id);
        myId = hmsg->id;
      }
      if (msg && msg->type == MessageType::Data)
      {
        const DefaultMessage *dmsg = reinterpret_cast<const DefaultMessage*>(msg);
        printf("Message from server: %s\n",
          reinterpret_cast<const char*>(dmsg) + sizeof(DefaultMessage));
      }
    }

  }
  return 0;
}

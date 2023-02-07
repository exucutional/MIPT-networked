#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <chrono>
#include "socket_tools.h"
#include "protocol.h"


int main(int argc, const char **argv)
{
  const char *sport = "2023";
  const char *rport = "2022";

  addrinfo resAddrInfo;
  int rfd = create_dgram_socket(nullptr, rport, nullptr);
  int sfd = create_dgram_socket("localhost", sport, &resAddrInfo);

  if (sfd == -1 || rfd == -1)
    return 1;
  printf("listening!\n");

  // Remember clients and their last keep alive time
  std::unordered_map<mid_t, std::chrono::high_resolution_clock::time_point> clients;

  int newClientId = 1;

  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(rfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(rfd + 1, &readSet, NULL, NULL, &timeout);
    const Message* msg = get_message(rfd, &readSet);

    // Listen messages and verify clients
    if (msg && msg->type == MessageType::Data)
    {
      const DefaultMessage* dmsg = reinterpret_cast<const DefaultMessage*>(msg);
      printf("Message from client %d: %s\n",
        dmsg->id, reinterpret_cast<const char*>(dmsg) + sizeof(DefaultMessage));

      std::string answer = "OK";
      auto clientIt = clients.find(dmsg->id);
      if (clientIt == std::end(clients))
      {
        answer = "You did not open connection";
      }
      else
      {
        auto time = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>
          (time - clientIt->second).count() > KEPP_ALIVE)
        {
          answer = "Your session is ended";
        }
      }
      DefaultMessage message
      {
        .type = MessageType::Data,
        .size = static_cast<msize_t>(answer.size()),
        .id = 0u,
      };

      ssize_t res = send_message(sfd, &message, answer.c_str(), &resAddrInfo);
      if (res == -1)
        std::cout << strerror(errno) << std::endl;
    }

    // Open 'connection' for new clients
    if (msg && msg->type == MessageType::Handshake)
    {
      const HandshakeMessage *hmsg = reinterpret_cast<const HandshakeMessage*>(msg);
      printf("Handshake from new client %d\n", newClientId);
      clients[newClientId] = std::chrono::high_resolution_clock::now();
      handshake(sfd, newClientId++, &resAddrInfo);
    }

    // Update connections if they are still active
    if (msg && msg->type == MessageType::KeepAlive)
    {
      const HandshakeMessage *kmsg = reinterpret_cast<const HandshakeMessage*>(msg);
      auto clientIt = clients.find(kmsg->id);
      if (clientIt != std::end(clients))
        clientIt->second = std::chrono::high_resolution_clock::now();

      printf("KeepAlive from client %d\n", kmsg->id);
    }

  }
  return 0;
}

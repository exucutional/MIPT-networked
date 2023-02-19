#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include "lobby.h"
#include "utility.h"

Lobby::Lobby()
{
  clients.reserve(100);
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10887;

  host = enet_host_create(&address, 32, 2, 0, 0);

  if (!host)
  {
    printf("Cannot create ENet host\n");
  }
  enet_address_set_host(&server, "localhost");
  server.port = 10888;;
}

void Lobby::sendSession(const LobbyClient& client)
{
  char buffer[100];
  int n = sprintf(buffer, "Server: %u:%hu", server.host, server.port);
  send_reliable_packet(buffer, client.peer, n);
}

void Lobby::startSession()
{
  isSessionStarted = true;
  printf("Starting session\n");
  for (const auto& client : clients)
  {
    sendSession(client);
  }
}

void Lobby::listen()
{
  while (true)
  {
    ENetEvent event;
    while (enet_host_service(host, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
      {
        printf("Connection with %x:%u established\n", event.peer->address.host,
          event.peer->address.port);
        clients.emplace_back(event.peer);
        if (isSessionStarted)
          sendSession(clients.back());
        break;
      }
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s' from %x:%u\n", event.packet->data,
          event.peer->address.host, event.peer->address.port);
        if (!isSessionStarted && event.packet->data
          && strncmp("start", reinterpret_cast<char*>(event.packet->data),
            strlen("start")) == 0)
        {
          for (const auto& client : clients)
          {
            if (client.peer->address.host == event.peer->address.host
              && client.peer->address.port == event.peer->address.port)
            {
              startSession();
              break;
            }
          }
        }
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      }
    }
  }
}

Lobby::~Lobby()
{
  enet_host_destroy(host);
  atexit(enet_deinitialize);
}

int main(int argc, const char **argv)
{
  Lobby lobby;
  lobby.listen();
  return 0;
}

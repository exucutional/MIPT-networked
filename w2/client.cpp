#include <enet/enet.h>
#include <iostream>
#include <future>
#include <chrono>
#include "utility.h"
#include "client.h"

Client::Client()
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
  }

  host = enet_host_create(nullptr, 2, 2, 0, 0);
  if (!host)
  {
    printf("Cannot create ENet host\n");
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10887;

  lobbyPeer = enet_host_connect(host, &address, 2, 0);
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
  }
  asyncInput = std::async(std::launch::async, &Client::input, this);
}

Client::~Client()
{
  enet_host_destroy(host);
  atexit(enet_deinitialize);
}

std::string Client::input()
{
  std::string res;
  std::getline(std::cin, res);
  return res;
}

void Client::sendRandom()
{
  std::srand(std::time(nullptr));
  auto curTime = std::time(nullptr) - std::rand();
  char buffer[100];
  auto timeStr = std::asctime(std::localtime(&curTime));
  int n = sprintf(buffer, "Time: %s", timeStr);
  send_unreliable_packet(buffer, serverPeer, n - 1);
}

void Client::listen()
{
  uint32_t timeStart = enet_time_get();
  uint32_t lastRandomSendTime = timeStart;
  while (true)
  {
    ENetEvent event;
    while (enet_host_service(host, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        isLobbyConnected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
      {
        printf("Packet received '%s' from %x:%u\n", event.packet->data,
          event.peer->address.host, event.peer->address.port);
        char* msg = reinterpret_cast<char*>(event.packet->data);
        if (msg && std::strncmp("Server: ", msg, strlen("Server: ")) == 0)
        {
          ENetAddress address;
          sscanf(msg, "Server: %u:%hu", &address.host, &address.port);
          serverPeer = enet_host_connect(host, &address, 2, 0);
          isServerConnected = true;
        };
        enet_packet_destroy(event.packet);
        break;
      }
      default:
        break;
      };
    }
    if (isLobbyConnected)
    {
      std::future_status status = asyncInput.wait_for((std::chrono::milliseconds(1)));
      if (status == std::future_status::ready)
      {
        std::string inputStr = asyncInput.get();
        send_reliable_packet(inputStr.c_str(), lobbyPeer, inputStr.length());
        asyncInput = std::async(std::launch::async, &Client::input, this);
      }
    }
    if (isServerConnected)
    {
      uint32_t curTime = enet_time_get();
      if (curTime - lastRandomSendTime > 10000)
      {
        lastRandomSendTime = curTime;
        sendRandom();
      }
    }
  }
}

int main(int argc, const char **argv)
{
  Client client;
  client.listen();
  return 0;
}

#include <iostream>
#include <sstream>
#include <ctime>
#include "server.h"
#include "utility.h"

static std::string genName(uint32_t seed)
{
  return "Player" + std::to_string(seed);
}

Server::Server()
{
  players.reserve(100);
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10888;

  host = enet_host_create(&address, 32, 2, 0, 0);

  if (!host)
  {
    printf("Cannot create ENet host\n");
  }
}

void Server::sendNewPlayer()
{
  const Player& newPlayer = players.back();
  for (size_t i = 0; i < players.size() - 1; ++i)
  {
    char buffer[100]; 
    int n = sprintf(buffer, "New player. Id: %d. Name: %s",
      newPlayer.id, newPlayer.name.c_str());
    send_reliable_packet(buffer, players[i].peer, n);
  }
  char buffer[100]; 
  int n = sprintf(buffer, "Your Id: %d. Name: %s",
    newPlayer.id, newPlayer.name.c_str());
}

void Server::sendPlayersList(const Player& player)
{
  std::stringstream ss;
  ss << "Player list\n";
  for (size_t i = 0; i < players.size(); ++i)
  {
    ss << "Id: " << players[i].id << " Name: " << players[i].name << "\n";
  }
  std::string s = ss.str();
  send_reliable_packet(s.c_str(), player.peer, s.length()-1);
}

void Server::sendRandom()
{
  for (auto& player : players)
  {
    std::srand(std::time(nullptr));
    auto curTime = std::time(nullptr) - std::rand();
    char buffer[100];
    auto timeStr = std::asctime(std::localtime(&curTime));
    int n = sprintf(buffer, "Time: %s", timeStr);
    send_unreliable_packet(buffer, player.peer, n - 1);
  }
}

void Server::sendPing()
{
  char buffer[100];
  for (auto& player : players)
  {
    for (auto& player2 : players)
    {
      int n = sprintf(buffer, "Id: %d. Ping: %d", player2.id, player2.peer->roundTripTime);
      send_unreliable_packet(buffer, player.peer, n);
    }
  }
}

void Server::listen()
{
  uint32_t timeStart = enet_time_get();
  uint32_t lastPingTime = timeStart;
  uint32_t lastRandomTime = timeStart;
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

        players.emplace_back(event.peer, players.size(), genName(players.size()));
        sendNewPlayer();
        sendPlayersList(players.back());
        break;
      }
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s' from %x:%u\n", event.packet->data,
          event.peer->address.host, event.peer->address.port);
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      }
    }
    uint32_t curTime = enet_time_get();
    if (curTime - lastPingTime > 1000)
    {
      lastPingTime = curTime;
      sendPing();
    }
    if (curTime - lastRandomTime > 10000)
    {
      lastRandomTime = curTime;
      sendRandom();
    }
  }
}

Server::~Server()
{
  enet_host_destroy(host);
  atexit(enet_deinitialize);
}

int main(int argc, const char **argv)
{
  Server server;
  server.listen();
  return 0;
}

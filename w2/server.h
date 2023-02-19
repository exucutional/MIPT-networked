#pragma once
#include <enet/enet.h>
#include <vector>
#include <string>

typedef ENetPeer player_peer_t;
typedef uint32_t player_id_t;
typedef std::string player_name_t;

struct Player
{
  Player(player_peer_t* p, player_id_t id, player_name_t name)
    : peer(p), id(id), name(name) {}
  player_peer_t* peer;
  player_id_t id;
  player_name_t name;
};

struct Server
{
  Server();
  Server(const Server& l) = delete;
  Server(Server&& l) = delete;
  Server& operator=(const Server& l) = delete;
  Server& operator=(Server&& l) = delete;
  ~Server();
  void listen();
  void sendPing();
  void sendRandom();
  void sendPlayersList(const Player& player);
  void sendNewPlayer();
private:
  std::vector<Player> players;
  ENetHost* host = nullptr;
};

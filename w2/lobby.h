#pragma once
#include <vector>
#include <enet/enet.h>

typedef ENetPeer client_peer_t;

struct LobbyClient
{
  LobbyClient(client_peer_t* p) : peer(p) {}
  client_peer_t* peer;
};

struct Lobby
{
  Lobby();
  Lobby(const Lobby& l) = delete;
  Lobby(Lobby&& l) = delete;
  Lobby& operator=(const Lobby& l) = delete;
  Lobby& operator=(Lobby&& l) = delete;
  ~Lobby();
  void listen();
  void startSession();
  void sendSession(const LobbyClient& client);
private:
  std::vector<LobbyClient> clients;
  bool isSessionStarted = false;
  ENetHost* host = nullptr;
  ENetAddress server;
};

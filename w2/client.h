#pragma once
#include <enet/enet.h>
#include <string>
#include <future>

struct Client
{
  Client();
  Client(const Client& l) = delete;
  Client(Client&& l) = delete;
  Client& operator=(const Client& l) = delete;
  Client& operator=(Client&& l) = delete;
  ~Client();
  void listen();
  void sendRandom();
  std::string input();
private:
  ENetHost* host;
  ENetPeer* lobbyPeer;
  ENetPeer *serverPeer;
  bool isLobbyConnected = false;
  bool isServerConnected = false;
  std::future<std::string> asyncInput;
};

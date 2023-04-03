#pragma once

#include <enet/enet.h>
#include <vector>
#include <map>
#include "entity.h"

class Server
{
public:
  int listen();
private:
  ENetHost* server_;
  uint32_t last_update_ = 0;
  std::map<uint16_t, Entity> entities_ = {};
  std::map<uint16_t, ENetPeer*> controlledMap_ = {};
  void on_join_(ENetPeer* peer, ENetHost* host, uint32_t tick);
  void on_leave_(ENetPeer* peer, ENetHost* host);
  void on_input_(ENetPacket* packet);
  void on_snapshot_(ENetPacket* packet);
  void simulate_(uint32_t time);
  Entity create_random_entity_();
};

#pragma once

#include <enet/enet.h>
#include <vector>
#include <map>
#include "entity.h"

class Server
{
public:
  int listen();
  static std::array<float, 2> generate_rand_location(int w, int h);
private:
  size_t ai_count_ = 5u;
  std::vector<Target> ai_targets_;
  ENetHost* server_;
  uint32_t last_update_ = 0;
  std::vector<Entity> entities_;
  std::map<uint16_t, ENetPeer*> controlledMap_;
  void on_join_(ENetPacket* packet, ENetPeer* peer, ENetHost* host);
  void on_state_(ENetPacket* packet);
  Entity create_random_entity_();
  void ai_init_();
  void update_ai_();
  void send_snapshots_();
  void collision_update_();
  static bool is_collide_(const Entity& e1, const Entity& e2);
};
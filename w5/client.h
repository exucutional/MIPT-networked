#pragma once
#include <unordered_map>
#include <deque>
#include <enet/enet.h>
#include "entity.h" 
#include "snapshot.h"

constexpr auto interpolation_offset_ = 200u;//ms

class Client
{
public:
  int listen();
private:
  uint16_t my_eid_ = invalid_entity;
  ENetHost* client_;
  ENetPeer* server_peer_;
  std::unordered_map<uint16_t, Entity> entities_ = {};
  std::unordered_map<uint16_t, Snapshot> last_snapshot_ = {};
  std::unordered_map<uint16_t, std::deque<Snapshot>> snapshots_ = {};
  std::vector<InputSnapshot> inputs_ = {};
  std::vector<Snapshot> my_snapshots_ = {};
  void on_new_entity_packet_(ENetPacket* packet);
  void on_set_controlled_entity_(ENetPacket* packet);
  void on_snapshot_(ENetPacket* packet);
  void interpolate_();
  void simulate_me_(const InputSnapshot& input, uint32_t tick);
  void restore_me_(uint32_t fromTick, uint32_t toTick);
  void clear_input_(size_t threshold);
  void on_leave_(ENetPacket* packet);
};

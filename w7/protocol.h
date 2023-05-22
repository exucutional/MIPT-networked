#pragma once
#include <enet/enet.h>
#include <cstdint>
#include "entity.h"

enum MessageType : uint8_t
{
  E_CLIENT_TO_SERVER_JOIN = 0,
  E_SERVER_TO_CLIENT_NEW_ENTITY,
  E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY,
  E_CLIENT_TO_SERVER_INPUT,
  E_SERVER_TO_CLIENT_INPUT_ACK,
  E_SERVER_TO_CLIENT_SNAPSHOT,
  E_CLIENT_TO_SERVER_SNAPSHOT_ACK
};

using delta_t = uint16_t;

struct DeltaHeader
{
  uint16_t eid;
  uint32_t id;
  uint16_t deltaId;
  delta_t deltaMask;
};

struct Snapshot
{
  uint16_t xPacked;
  uint16_t yPacked;
  uint8_t oriPacked;
};

struct Input
{
  uint8_t thrSteerPacked;
};

void send_join(ENetPeer *peer);
void send_new_entity(ENetPeer *peer, const Entity &ent);
void send_set_controlled_entity(ENetPeer *peer, uint16_t eid);
void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float steer, uint32_t id);
void send_entity_input_ack(ENetPeer* peer, uint16_t eid, uint32_t id);
void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori, uint32_t id);
void send_snapshot_ack(ENetPeer* peer, uint16_t eid, uint32_t id);


MessageType get_packet_type(ENetPacket *packet);

template <typename T>
delta_t getDelta(const T &ref, const T &s);

template <typename T>
void compressDelta(uint8_t* data, delta_t deltaMask, const T &s);

template <typename T>
void uncompressDelta(uint8_t* data, delta_t deltaMask, T& d);

uint32_t deltaSize(delta_t deltaMask);

void deserialize_new_entity(ENetPacket *packet, Entity &ent);
void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid);
void deserialize_entity_input(ENetPacket *packet, ENetPeer* peer, uint16_t &eid, float &thr, float &steer);
void deserialize_entity_input_ack(ENetPacket* packet);
void deserialize_snapshot(ENetPacket *packet, ENetPeer* peer, uint16_t &eid, float &x, float &y, float &ori);
void deserialize_snapshot_ack(ENetPacket* packet, ENetPeer* peer);

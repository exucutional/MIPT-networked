#include "protocol.h"
#include "bitstream.h"
#include <cstring> // memcpy

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(MessageType), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_leave(ENetPeer* peer, uint16_t eid)
{
  ENetPacket* packet = enet_packet_create(nullptr, sizeof(MessageType) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  auto bitstream = hw5::Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_SERVER_TO_CLIENT_LEAVE, eid);
  enet_peer_send(peer, 0, packet);
}


void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(MessageType) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);

  auto bitstream = hw5::Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_SERVER_TO_CLIENT_NEW_ENTITY, ent);
  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(MessageType) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);

  auto bitstream = hw5::Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY, eid);
  enet_peer_send(peer, 0, packet);
}

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float steer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(MessageType) + sizeof(uint16_t) +
                                                   2 * sizeof(float),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);

  auto bitstream = hw5::Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_CLIENT_TO_SERVER_INPUT, eid, thr, steer);
  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, MessageType msg, uint16_t eid, float x, float y, float ori, uint32_t tick)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(MessageType) + sizeof(uint16_t) +
                                                   3 * sizeof(float) + sizeof(uint32_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);

  auto bitstream = hw5::Bitstream(packet->data, packet->dataLength);
  bitstream.write(msg, eid, x, y, ori, tick);
  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  auto bitstream = hw5::Bitstream(packet->data, packet->dataLength);
  MessageType msg;
  bitstream.read(msg, ent);
}

void deserialize_leave(ENetPacket* packet, uint16_t& eid)
{
  auto bitstream = hw5::Bitstream(packet->data, packet->dataLength);
  MessageType msg;
  bitstream.read(msg, eid);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  auto bitstream = hw5::Bitstream(packet->data, packet->dataLength);
  MessageType msg;
  bitstream.read(msg, eid);
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer)
{
  auto bitstream = hw5::Bitstream(packet->data, packet->dataLength);
  MessageType msg;
  bitstream.read(msg, eid, thr, steer);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori, uint32_t & tick)
{
  auto bitstream = hw5::Bitstream(packet->data, packet->dataLength);
  MessageType msg;
  bitstream.read(msg, eid, x, y, ori, tick);
}


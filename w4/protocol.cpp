#include "protocol.h"
#include <cstring> // memcpy
#include "bitstream.h"

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  auto bitstream = hw4::Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_CLIENT_TO_SERVER_JOIN);

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  auto bitstream = hw4::Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_SERVER_TO_CLIENT_NEW_ENTITY, ent);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  auto bitstream = hw4::Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY, eid);

  enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y, size_t size)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   2 * sizeof(float) + sizeof(size_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  auto bitstream = hw4::Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_CLIENT_TO_SERVER_STATE, eid, x, y, size);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, size_t size)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   2 * sizeof(float) + sizeof(size_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  auto bitstream = hw4::Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_SERVER_TO_CLIENT_SNAPSHOT, eid, x, y, size);

  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  auto bitstream = hw4::Bitstream(packet->data, packet->dataLength);
  uint8_t head;
  bitstream.read(head, ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  auto bitstream = hw4::Bitstream(packet->data, packet->dataLength);
  uint8_t head;
  bitstream.read(head, eid);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y, size_t &size)
{
  auto bitstream = hw4::Bitstream(packet->data, packet->dataLength);
  uint8_t head;
  bitstream.read(head, eid, x, y, size);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, size_t &size)
{
  auto bitstream = hw4::Bitstream(packet->data, packet->dataLength);
  uint8_t head;
  bitstream.read(head, eid, x, y, size);
}


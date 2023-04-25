#include "protocol.h"
#include "quantisation.h"
#include <cstring> // memcpy
#include <iostream>

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);

  auto bitstream = Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_SERVER_TO_CLIENT_NEW_ENTITY, ent);
  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);

  auto bitstream = Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  bitstream.writePackedUint32(eid);
  enet_peer_send(peer, 0, packet);
}

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(uint8_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);

  float4bitsQuantized thrPacked(thr, -1.f, 1.f);
  float4bitsQuantized oriPacked(ori, -1.f, 1.f);
  uint8_t thrSteerPacked = (thrPacked.packedVal << 4) | oriPacked.packedVal;
  auto bitstream = Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_CLIENT_TO_SERVER_INPUT);
  bitstream.writePackedUint32(eid);
  bitstream.write(thrSteerPacked);
  enet_peer_send(peer, 1, packet);
}

typedef PackedFloat<uint16_t, 11> PositionXQuantized;
typedef PackedFloat<uint16_t, 10> PositionYQuantized;

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(uint16_t) +
                                                   sizeof(uint16_t) +
                                                   sizeof(uint8_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);

  PositionXQuantized xPacked(x, -16, 16);
  PositionYQuantized yPacked(y, -8, 8);
  uint8_t oriPacked = pack_float<uint8_t>(ori, -PI, PI, 8);
  auto bitstream = Bitstream(packet->data, packet->dataLength);
  bitstream.write(E_SERVER_TO_CLIENT_SNAPSHOT);
  bitstream.writePackedUint32(eid);
  bitstream.write(xPacked.packedVal, yPacked.packedVal, oriPacked);
  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  auto bitstream = Bitstream(packet->data, packet->dataLength);
  uint8_t head;
  bitstream.read(head, ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  uint8_t head;
  auto bitstream = Bitstream(packet->data, packet->dataLength);
  bitstream.read(head);
  eid = bitstream.readPackedUint32();
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer)
{
  uint8_t head;
  auto bitstream = Bitstream(packet->data, packet->dataLength);
  bitstream.read(head);
  eid = bitstream.readPackedUint32();
  uint8_t thrSteerPacked;
  bitstream.read(thrSteerPacked);
  static uint8_t neutralPackedValue = pack_float<uint8_t>(0.f, -1.f, 1.f, 4);
  static uint8_t nominalPackedValue = pack_float<uint8_t>(1.f, 0.f, 1.2f, 4);
  float4bitsQuantized thrPacked(thrSteerPacked >> 4);
  float4bitsQuantized steerPacked(thrSteerPacked & 0x0f);
  thr = thrPacked.packedVal == neutralPackedValue ? 0.f : thrPacked.unpack(-1.f, 1.f);
  steer = steerPacked.packedVal == neutralPackedValue ? 0.f : steerPacked.unpack(-1.f, 1.f);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori)
{
  auto bitstream = Bitstream(packet->data, packet->dataLength);
  uint8_t head;
  bitstream.read(head);
  eid = bitstream.readPackedUint32();
  uint16_t xPacked;
  uint16_t yPacked;
  uint8_t oriPacked;
  bitstream.read(xPacked, yPacked, oriPacked);
  PositionXQuantized xPackedVal(xPacked);
  PositionYQuantized yPackedVal(yPacked);
  x = xPackedVal.unpack(-16, 16);
  y = yPackedVal.unpack(-8, 8);
  ori = unpack_float<uint8_t>(oriPacked, -PI, PI, 8);
}


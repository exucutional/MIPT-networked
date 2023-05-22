#include "protocol.h"
#include "quantisation.h"
#include <cstring> // memcpy
#include <iostream>
#include <unordered_map>
#include <stack>
#include <cassert>

// #define MDEBUG

#ifdef MDEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...) ;
#endif

template <typename T>
using PeerMap = std::unordered_map<ENetPeer*, T>;

template <typename T>
using EntityIdMap = std::unordered_map<uint16_t, T>;

template <typename T>
using FramePair = std::pair<uint32_t, T>;

static PeerMap<std::unordered_map<uint16_t, FramePair<Snapshot>>> snapshotRefs{};
static PeerMap<EntityIdMap<std::vector<FramePair<Snapshot>>>> snapshotHistory{};
static EntityIdMap<FramePair<Input>> inputRefs{};
static EntityIdMap<std::vector<FramePair<Input>>> inputHistory{};

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
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_NEW_ENTITY; ptr += sizeof(uint8_t);
  memcpy(ptr, &ent, sizeof(Entity)); ptr += sizeof(Entity);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);

  enet_peer_send(peer, 0, packet);
}

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float ori, uint32_t id)
{
  uint16_t deltaId = 0;
  DeltaHeader inputHeader = {
    .eid = eid,
    .id = id,
    .deltaId = 0,
    .deltaMask = 0xFFFF,
  };
  float4bitsQuantized thrPacked(thr, -1.f, 1.f);
  float4bitsQuantized oriPacked(ori, -1.f, 1.f);
  uint8_t thrSteerPacked = (thrPacked.packedVal << 4) | oriPacked.packedVal;
  Input input = {
    .thrSteerPacked = thrSteerPacked
  };
  if (inputRefs.find(eid) == inputRefs.end())
    inputRefs[eid] = {0, {0}};
  
  auto deltaMask = getDelta(inputRefs[eid].second, input);
  ENetPacket *packet = enet_packet_create(nullptr,
    sizeof(uint8_t) + sizeof(DeltaHeader) + deltaSize(deltaMask),
    ENET_PACKET_FLAG_UNSEQUENCED);

  uint8_t *ptr = packet->data;
  *ptr = E_CLIENT_TO_SERVER_INPUT; ptr += sizeof(uint8_t);
  inputHeader.deltaId = inputRefs[eid].first;
  inputHeader.deltaMask = deltaMask;
  memcpy(ptr, &inputHeader, sizeof(DeltaHeader)); ptr += sizeof(DeltaHeader);
  compressDelta(ptr, deltaMask, input);
  enet_peer_send(peer, 1, packet);
  if (inputHistory[eid].size() > 10000)
    inputHistory[eid].resize(1000);

  inputHistory[eid].push_back({id, input});
}

void send_entity_input_ack(ENetPeer* peer, uint16_t eid, uint32_t id)
{
  ENetPacket* packet = enet_packet_create(nullptr,
    sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t),
    ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t* ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_INPUT_ACK; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  memcpy(ptr, &id, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  enet_peer_send(peer, 1, packet);
}

typedef PackedFloat<uint16_t, 11> PositionXQuantized;
typedef PackedFloat<uint16_t, 10> PositionYQuantized;

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori, uint32_t id)
{
  DeltaHeader snapshotHeader = {
    .eid = eid,
    .id = id,
    .deltaId = 0,
    .deltaMask = 0xFFFF,
  };
  if (snapshotRefs[peer].find(eid) == snapshotRefs[peer].end())
    snapshotRefs[peer][eid] = {0, {0, 0, 0}};

  PositionXQuantized xPacked(x, -16, 16);
  PositionYQuantized yPacked(y, -8, 8);
  uint8_t oriPacked = pack_float<uint8_t>(ori, -PI, PI, 8);
  //printf("xPacked/unpacked %d %f\n", xPacked, x);
  Snapshot snapshot = {
    .xPacked = xPacked.packedVal,
    .yPacked = yPacked.packedVal,
    .oriPacked = oriPacked,
  };

  auto deltaMask = getDelta(snapshotRefs[peer][eid].second, snapshot);
  ENetPacket* packet = enet_packet_create(nullptr,
    sizeof(uint8_t) + sizeof(DeltaHeader) + deltaSize(deltaMask),
    ENET_PACKET_FLAG_UNSEQUENCED);

  uint8_t* ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SNAPSHOT; ptr += sizeof(uint8_t);
  snapshotHeader.deltaId = snapshotRefs[peer][eid].first;
  snapshotHeader.deltaMask = deltaMask;
  memcpy(ptr, &snapshotHeader, sizeof(DeltaHeader)); ptr+= sizeof(DeltaHeader);
  compressDelta(ptr, deltaMask, snapshot);
  static uint64_t counter = 0;
  DEBUG_PRINTF("[%llu] Ref snapshot [%u] %u %u %u. ", counter++, snapshotHeader.deltaId, snapshotRefs[peer][eid].second.xPacked, snapshotRefs[peer][eid].second.yPacked, snapshotRefs[peer][eid].second.oriPacked);
  DEBUG_PRINTF("Sending snapshot [%d] %u %u %u\n", snapshotHeader.id, snapshot.xPacked, snapshot.yPacked, snapshot.oriPacked);
  enet_peer_send(peer, 1, packet);
  if (snapshotHistory[peer][eid].empty() || snapshotHistory[peer][eid].back().first != id)
  {
    if (snapshotHistory[peer][eid].size() > 10000)
      snapshotHistory[peer][eid].resize(1000);

    snapshotHistory[peer][eid].push_back({id, snapshot});
  }
}

void send_snapshot_ack(ENetPeer* peer, uint16_t eid, uint32_t id)
{
  ENetPacket* packet = enet_packet_create(nullptr,
    sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t),
    ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t* ptr = packet->data;
  *ptr = E_CLIENT_TO_SERVER_SNAPSHOT_ACK; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  memcpy(ptr, &id, sizeof(uint32_t)); ptr += sizeof(uint32_t);
  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  ent = *(Entity*)(ptr); ptr += sizeof(Entity);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
}

void deserialize_entity_input(ENetPacket *packet, ENetPeer* peer, uint16_t &eid, float &thr, float &steer)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  DeltaHeader inputHeader = *reinterpret_cast<DeltaHeader*>(ptr); ptr += sizeof(DeltaHeader);
  eid = inputHeader.eid;
  if (inputRefs.find(eid) == inputRefs.end())
    inputRefs[eid] = {0, {0}};
  
  auto input = inputRefs[eid].second;
  if (inputHeader.deltaId == 0)
    input = {0};

  if (inputHeader.deltaId == inputRefs[eid].first || inputHeader.deltaId == 0)
  {
    uncompressDelta(ptr, inputHeader.deltaMask, input);
    inputRefs[eid] = {inputHeader.id, input};
  }

  static uint8_t neutralPackedValue = pack_float<uint8_t>(0.f, -1.f, 1.f, 4);
  static uint8_t nominalPackedValue = pack_float<uint8_t>(1.f, 0.f, 1.2f, 4);
  float4bitsQuantized thrPacked(input.thrSteerPacked >> 4);
  float4bitsQuantized steerPacked(input.thrSteerPacked & 0x0f);
  thr = thrPacked.packedVal == neutralPackedValue ? 0.f : thrPacked.unpack(-1.f, 1.f);
  steer = steerPacked.packedVal == neutralPackedValue ? 0.f : steerPacked.unpack(-1.f, 1.f);
  send_entity_input_ack(peer, eid, inputRefs[eid].first);
}

void deserialize_entity_input_ack(ENetPacket* packet)
{
  uint8_t* ptr = packet->data; ptr += sizeof(uint8_t);
  uint16_t eid = *reinterpret_cast<uint16_t*>(ptr); ptr += sizeof(uint16_t);
  uint32_t id = *reinterpret_cast<uint32_t*>(ptr); ptr += sizeof(uint32_t);
  if (inputHistory.find(eid) == inputHistory.end())
    return;

  auto s = std::find_if(inputHistory[eid].begin(), inputHistory[eid].end(), [id](auto& val) {return val.first == id; });
  if (s == inputHistory[eid].end())
  {
    inputHistory[eid].clear();
    inputRefs[eid] = {0, {0}};
    return;
  }
  inputHistory[eid].erase(inputHistory[eid].begin(), s);
  inputRefs[eid] = inputHistory[eid].front();
}

void deserialize_snapshot(ENetPacket *packet, ENetPeer* peer, uint16_t &eid, float &x, float &y, float &ori)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  DeltaHeader snapshotHeader = *reinterpret_cast<DeltaHeader*>(ptr); ptr += sizeof(DeltaHeader);
  eid = snapshotHeader.eid;
  if (snapshotRefs[peer].find(eid) == snapshotRefs[peer].end())
    snapshotRefs[peer][eid] = { 0, {0, 0, 0} };

  auto snapshot = snapshotRefs[peer][eid].second;
  if (snapshotHeader.deltaId == 0)
    snapshot = {0, 0, 0};
  
  static uint64_t counter = 0;
  DEBUG_PRINTF("[%llu] Ref snapshot [%u] %u %u %u. ", counter++, snapshotRefs[peer][eid].first, snapshot.xPacked, snapshot.yPacked, snapshot.oriPacked);
  DEBUG_PRINTF("Mask %u. ", snapshotHeader.deltaMask);

  if (snapshotHeader.deltaId == 0 || snapshotHeader.deltaId == snapshotRefs[peer][eid].first)
  {
    uncompressDelta(ptr, snapshotHeader.deltaMask, snapshot);
    snapshotRefs[peer][eid] = {snapshotHeader.id, snapshot};
  }
  else
  {
    DEBUG_PRINTF("Mismatch in reference. Ignoring. ");
  }
  DEBUG_PRINTF("Deserialize snapshot [%u] %u %u %u\n", snapshotHeader.id, snapshot.xPacked, snapshot.yPacked, snapshot.oriPacked);
  PositionXQuantized xPackedVal(snapshot.xPacked);
  PositionYQuantized yPackedVal(snapshot.yPacked);
  x = xPackedVal.unpack(-16, 16);
  y = yPackedVal.unpack(-8, 8);
  ori = unpack_float<uint8_t>(snapshot.oriPacked, -PI, PI, 8);
  send_snapshot_ack(peer, eid, snapshotRefs[peer][eid].first);
}

void deserialize_snapshot_ack(ENetPacket* packet, ENetPeer* peer)
{
  uint8_t* ptr = packet->data; ptr += sizeof(uint8_t);
  uint16_t eid = *reinterpret_cast<uint16_t*>(ptr); ptr += sizeof(uint16_t);
  uint32_t id = *reinterpret_cast<uint32_t*>(ptr); ptr += sizeof(uint32_t);
  assert(snapshotHistory[peer].find(eid) != snapshotHistory[peer].end());
  auto s = std::find_if(snapshotHistory[peer][eid].begin(), snapshotHistory[peer][eid].end(), [id](auto& val) {return val.first == id;});
  if (s == snapshotHistory[peer][eid].end())
  {
    snapshotHistory[peer][eid].clear();
    snapshotRefs[peer][eid] = {0, {0, 0, 0}};
    return;
  }

  snapshotHistory[peer][eid].erase(snapshotHistory[peer][eid].begin(), s);
  snapshotRefs[peer][eid] = snapshotHistory[peer][eid].front();
}

// Delta compression
//
template <typename T>
delta_t getDelta(const T& ref, const T& s)
{
  delta_t delta = 0;
  auto ref_p = reinterpret_cast<const uint8_t*>(&ref);
  auto s_p = reinterpret_cast<const uint8_t*>(&s);
  std::stack<uint8_t> stack;
  for (int i = 0; i < sizeof(T); ++i)
  {
    if (*ref_p != *s_p)
      stack.push(1);
    else
      stack.push(0);

    s_p += sizeof(uint8_t);
    ref_p += sizeof(uint8_t);
  }
  while (!stack.empty())
  {
    delta = delta << 1 | stack.top();
    stack.pop();
  }

  return delta;
}

template <typename T>
void compressDelta(uint8_t* data, delta_t deltaMask, const T& s)
{
  auto s_p = reinterpret_cast<const uint8_t*>(&s);
  for (int i = 0; i < sizeof(T); ++i)
  {
    if ((deltaMask & 0x1) == 1)
    {
      memcpy(data, s_p, sizeof(uint8_t));
      data += sizeof(uint8_t);
    }
    s_p += sizeof(uint8_t);
    deltaMask >>= 1;
  }
}

template <typename T>
void uncompressDelta(uint8_t* data, delta_t deltaMask, T& d)
{
  auto d_p = reinterpret_cast<uint8_t*>(&d);
  for (int i = 0; i < sizeof(T); ++i)
  {
    if ((deltaMask & 0x1) == 1)
    {
      memcpy(d_p, data, sizeof(uint8_t));
      data += sizeof(uint8_t);
    }
    deltaMask >>= 1;
    d_p += sizeof(uint8_t);
  }
}

uint32_t deltaSize(delta_t deltaMask)
{
  uint32_t size = 0;
  for (int i = 0; i < sizeof(delta_t) * 8; ++i)
  {
    if ((deltaMask & 0x1) == 1)
      size += 1;

    deltaMask >>= 1;
  }
  return size;
}

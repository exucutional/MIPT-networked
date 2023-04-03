#include <enet/enet.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include "entity.h"
#include "protocol.h"
#include "mathUtils.h"
#include "server.h"
#include "tick.h"

Entity Server::create_random_entity_()
{
  // find max eid
  uint16_t maxEid = entities_.empty() ? 0 : entities_[0].eid;
  for (const auto &[eid, e] : entities_)
    maxEid = std::max(maxEid, eid);
  uint16_t newEid = maxEid + 1;
  uint32_t color = 0x000000ff +
    ((rand() % 255) << 8) +
    ((rand() % 255) << 16) +
    ((rand() % 255) << 24);
  float x = (rand() % 4) * 5.f;
  float y = (rand() % 4) * 5.f;
  Entity ent = { color, x, y, 0.f, (rand() / RAND_MAX) * 3.141592654f, 0.f, 0.f, newEid };
  return ent;
}

void Server::on_join_(ENetPeer *peer, ENetHost *host, uint32_t tick)
{
  // send all entities
  for (const auto& [eid, e] : entities_)
    send_new_entity(peer, e);

  Entity ent = create_random_entity_();
  ent.tick = tick;
  entities_[ent.eid] = ent;

  controlledMap_[ent.eid] = peer;

  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    if (host->peers[i].channels != nullptr)
      send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, ent.eid);
}

void Server::on_leave_(ENetPeer* peer, ENetHost* host)
{
  for (auto& [eid, cpeer] : controlledMap_)
  {
    if (cpeer == peer)
    {
      entities_.erase(eid);
      for (size_t i = 0; i < host->peerCount; ++i)
        for (size_t i = 0; i < host->peerCount; ++i)
          if (host->peers[i].channels != nullptr)
            send_leave(&host->peers[i], eid);
    }
  }
}

void Server::on_input_(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float thr = 0.f; float steer = 0.f;
  deserialize_entity_input(packet, eid, thr, steer);
  auto &e = entities_[eid];
  e.thr = thr;
  e.steer = steer;
}

void Server::on_snapshot_(ENetPacket* packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float ori = 0.f; uint32_t tick = 0;
  deserialize_snapshot(packet, eid, x, y, ori, tick);
  auto& e = entities_[eid];
  e.x = x;
  e.y = y;
  e.ori = ori;
}

void Server::simulate_(uint32_t time)
{
  for (auto& [eid, e] : entities_)
  {
    for (; e.tick < time_to_tick(time); ++e.tick)
      simulate_entity(e, dt);
 
    for (size_t i = 0; i < server_->peerCount; ++i)
    {
      ENetPeer* peer = &server_->peers[i];
      if (peer->channels != nullptr)
        send_snapshot(peer, E_SERVER_TO_CLIENT_SNAPSHOT, e.eid, e.x, e.y, e.ori, e.tick);
    }
  }
}

int Server::listen()
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  server_ = enet_host_create(&address, 32, 2, 0, 0);

  if (!server_)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  while (true)
  {
    uint32_t curTime = enet_time_get();
    ENetEvent event;
    while (enet_host_service(server_, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join_(event.peer, server_, time_to_tick(curTime));
            break;
          case E_CLIENT_TO_SERVER_INPUT:
            on_input_(event.packet);
            break;
          case E_CLIENT_TO_SERVER_SNAPSHOT:
            on_snapshot_(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        printf("Disconnect with %x:%u\n", event.peer->address.host, event.peer->address.port);
        on_leave_(event.peer, server_);
        break;
      default:
        break;
      };
    }
    simulate_(curTime);
    std::this_thread::sleep_for(std::chrono::microseconds(10000));
  }

  enet_host_destroy(server_);

  atexit(enet_deinitialize);
  return 0;
}

int main(int argc, const char** argv)
{
  Server server;
  return server.listen();
}

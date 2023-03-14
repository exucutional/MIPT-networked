#include <ctime>
#include <array>
#include "protocol.h"
#include "server.h"

void Server::on_join_(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  srand(time(NULL));
  // send all entities
  for (const Entity &ent : entities_)
    send_new_entity(peer, ent);

  auto ent = create_random_entity_();
  entities_.push_back(ent);

  controlledMap_[ent.eid] = peer;

  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, ent.eid);
}

void Server::on_state_(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; size_t size = 0;
  deserialize_entity_state(packet, eid, x, y, size);
  for (Entity &e : entities_)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
      e.size = size;
    }
}

int Server::listen()
{
  ai_init_();
  last_update_ = enet_time_get();
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
            on_join_(event.packet, event.peer, server_);
            break;
          case E_CLIENT_TO_SERVER_STATE:
            on_state_(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    update_ai_();
    collision_update_();
    send_snapshots_();
    //usleep(100000);
  }

  enet_host_destroy(server_);

  atexit(enet_deinitialize);
  return 0;
}

Entity Server::create_random_entity_()
{
  uint32_t color = 0x000000ff +
    ((rand() % 255) << 8)     +
    ((rand() % 255) << 16)    +
    ((rand() % 255) << 24);
  auto xy = generate_rand_location(1920 / 2, 1080 / 2);
  uint16_t size = std::max(rand() % 25, 1);
  uint16_t maxEid = entities_.empty() ? invalid_entity : entities_[0].eid;
  for (const Entity& e : entities_)
    maxEid = std::max(maxEid, e.eid);

  uint16_t newEid = maxEid + 1;
  Entity ent = { color, xy[0], xy[1], size, newEid};
  return ent;
}

void Server::ai_init_()
{
  entities_.reserve(ai_count_);
  for (size_t i = 0; i < ai_count_; ++i)
  {
    entities_.emplace_back(create_random_entity_());
    auto xy = generate_rand_location(1920 / 2, 1080 / 2);
    ai_targets_.emplace_back(Target{ xy[0], xy[1]});
  }
}

void Server::send_snapshots_()
{
  for (const Entity& e : entities_)
    for (size_t i = 0; i < server_->peerCount; ++i)
    {
      ENetPeer* peer = &server_->peers[i];
      if (controlledMap_[e.eid] != peer && peer->channels != nullptr)
        send_snapshot(peer, e.eid, e.x, e.y, e.size);
    }
}

void Server::update_ai_()
{
  uint32_t curTime = enet_time_get();
  for (size_t i = 0; i < ai_count_; ++i)
  {
    auto& entity = entities_[i];
    auto& target = ai_targets_[i];
    float dir[2] = {target.x - entity.x, target.y - entity.y};
    float l2 = dir[0] * dir[0] + dir[1] * dir[1];
    float l = sqrt(l2);
    dir[0] /= l * 5.0f;
    dir[1] /= l * 5.0f;
    float delta[2] = { dir[0] * (curTime - last_update_) , dir[1] * (curTime - last_update_) };
    if (abs(entity.x + delta[0] - target.x) > 1 && abs(entity.y + delta[1] - target.x) > 1)
    {
      entity.x += delta[0];
      entity.y += delta[1];
    }
    else
    {
      auto xy = generate_rand_location(1920 / 2, 1080 / 2);
      target = { xy[0], xy[1] };
    }
  }
  last_update_ = curTime;
}

std::array<float, 2> Server::generate_rand_location(int w, int h)
{
  return { rand() % w - w / 2.0f, rand() % h - h / 2.0f };
}

void Server::collision_update_()
{
  for (Entity& e1 : entities_)
    for (Entity& e2 : entities_)
    {
      if (e1.eid != e2.eid && is_collide_(e1, e2))
      {
        if (e1.size > e2.size)
        {
          e1.eat(e2);
        }
        else
        {
          e2.eat(e1);
        }
        if (controlledMap_[e1.eid] != nullptr)
          send_snapshot(controlledMap_[e1.eid], e1.eid, e1.x, e1.y, e1.size);
        if (controlledMap_[e2.eid] != nullptr)
          send_snapshot(controlledMap_[e2.eid], e2.eid, e2.x, e2.y, e2.size);
      }
    }
}

bool Server::is_collide_(const Entity& e1, const Entity& e2)
{
  return abs(e1.x - e2.x) < std::max(e1.size, e2.size) && abs(e1.y - e2.y) < (e1.size + e2.size) / 2;
}

void Entity::eat(Entity& e)
{
  size += e.size / 2;
  e.size = std::max(rand() % 25, 1);
  auto xy = Server::generate_rand_location(1920 / 2, 1080 / 2);
  e.x = xy[0];
  e.y = xy[1];
}

int main(int argc, const char** argv)
{
  Server server;
  return server.listen();
}

// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>

#include <vector>
#include "entity.h"
#include "protocol.h"
#include "client.h"
#include "tick.h"

void Client::on_new_entity_packet_(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  if (entities_.find(newEntity.eid) == std::end(entities_))
  {
    entities_[newEntity.eid] = std::move(newEntity);
    last_snapshot_[newEntity.eid] = {newEntity.x, newEntity.y, newEntity.ori, enet_time_get()};
    snapshots_[newEntity.eid] = std::deque<Snapshot>();
  }
}

void Client::on_set_controlled_entity_(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_eid_);
}

void Client::restore_me_(uint32_t fromTick, uint32_t toTick)
{
  auto iterErase = inputs_.begin();
  for (auto iter = inputs_.begin(); (iter < std::end(inputs_) && iter->time < toTick); ++iter)
  {
    if (iter->time >= fromTick)
    {
      if (iter + 1 < std::end(inputs_))
        simulate_me_(*iter, (iter + 1)->time - iter->time);
      else
        simulate_me_(*iter, toTick - iter->time);
    }
    else
    {
      iterErase = iter;
    }
  }

  inputs_.erase(inputs_.begin(), iterErase);
}

void Client::on_snapshot_(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float ori = 0.f; uint32_t tick = 0;
  deserialize_snapshot(packet, eid, x, y, ori, tick);
  //std::cout << "Snapshot. id: " << eid << " x: " << x << " y " << y << " ori " << ori << " tick " << tick << "\n";
  if (eid != my_eid_)
  {
    if (snapshots_.find(eid) != std::end(snapshots_))
      snapshots_[eid].push_back({x, y, ori,
        enet_time_get() + interpolation_offset_});
  }
  else
  {
    auto snapshotsN = my_snapshots_.size();
    size_t i = 0;
    for (; i < snapshotsN; ++i)
    {
      auto &s = my_snapshots_[i];
      if (s.time > tick)
        break;

      if (s.time == tick && (s.x != x || s.y != y || s.ori != ori))
      {
        auto &me = entities_[my_eid_];
        me.x = x;
        me.y = y;
        me.ori = ori;
        uint32_t oldTick = me.tick;
        me.tick = tick;
        restore_me_(tick, oldTick);
        i = snapshotsN;
        break;
      }
    }
    my_snapshots_.erase(my_snapshots_.begin(), my_snapshots_.begin() + i);
  }
}

void Client::interpolate_()
{
  for (auto &[eid, s1] : last_snapshot_)
  {
    if (eid == my_eid_)
      continue;

    auto now = enet_time_get();
    while (!snapshots_[eid].empty())
    {
      auto &s2 = snapshots_[eid].front();
      if (now >= s2.time)
      {
        last_snapshot_[eid] = std::move(s2);
        snapshots_[eid].pop_front();
      }
      else
      {
        auto now_delta = now - s1.time;
        auto interval = s2.time - s1.time;
        double t = (double)now_delta / interval;
        auto &e = entities_[eid];
        e.x = (1.0 - t) * s1.x + t * s2.x;
        e.y = (1.0 - t) * s1.y + t * s2.y;
        e.ori = (1.0 - t) * s1.ori + t * s2.ori;
        break;
      }
    }
  }
}

void Client::simulate_me_(const InputSnapshot& input, uint32_t tick)
{
  auto &me = entities_[my_eid_];
  me.thr = input.thr;
  me.steer = input.steer;
  for (uint32_t t = 0; t < tick; ++t)
  {
    simulate_entity(me, dt);
    me.tick++;
    my_snapshots_.push_back({me.x, me.y, me.ori, me.tick});
  }

}

void Client::clear_input_(size_t threshold)
{
  int toDelete = inputs_.size() - threshold;
  if (toDelete > 0)
    inputs_.erase(std::begin(inputs_), std::begin(inputs_) + toDelete);
}

void Client::on_leave_(ENetPacket* packet)
{
  uint16_t eid = 0;
  deserialize_leave(packet, eid);
  entities_.erase(eid);
  last_snapshot_.erase(eid);
  snapshots_.erase(eid);
}

int Client::listen()
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  client_ = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client_)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10131;

  server_peer_ = enet_host_connect(client_, &address, 2, 0);
  if (!server_peer_)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 600;
  int height = 600;

  InitWindow(width, height, "w5 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 10.f;


  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  bool connected = false;
  uint32_t lastTime = enet_time_get();
  while (!WindowShouldClose())
  {
    float dt = GetFrameTime();
    ENetEvent event;
    uint32_t curTime = enet_time_get();
    while (enet_host_service(client_, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(server_peer_);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet_(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity_(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot_(event.packet);
          break;
        case E_SERVER_TO_CLIENT_LEAVE:
          on_leave_(event.packet);
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    if (my_eid_ != invalid_entity)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      auto &me = entities_[my_eid_];
      if (entities_.find(my_eid_) != std::end(entities_))
      {
        // Update
        float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
        float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);

        auto oldTick = me.tick;
        auto input = InputSnapshot{thr, steer, me.tick};
        
        simulate_me_(input, time_to_tick(curTime - lastTime));
        lastTime += time_to_tick(curTime - lastTime) * tick_size;
        if (oldTick != me.tick)
        {
          inputs_.push_back(std::move(input));
          // Send
          send_entity_input(server_peer_, my_eid_, thr, steer);
        }
      }
    }
    interpolate_();
    clear_input_(100);
    BeginDrawing();
      ClearBackground(GRAY);
      BeginMode2D(camera);
        for (auto const &[eid, e] : entities_)
        {
          const Rectangle rect = {e.x, e.y, 3.f, 1.f};
          DrawRectanglePro(rect, {0.f, 0.5f}, e.ori * 180.f / PI, GetColor(e.color));
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();
  enet_peer_disconnect_now(server_peer_, 0);
  return 0;
}

int main(int argc, const char** argv)
{
  auto client = Client();
  return client.listen();
}

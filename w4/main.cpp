#include <functional>
#include <algorithm> // min/max
#include "raylib.h"
#include <enet/enet.h>

#include <vector>
#include "entity.h"
#include "protocol.h"


static std::unordered_map<uint16_t, Entity> entities;
static uint16_t my_eid = invalid_entity;

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  if (entities.find(newEntity.eid) == std::end(entities))
    entities[newEntity.eid] = std::move(newEntity);

  printf("new entity\n");
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_eid);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; size_t size = 0;
  deserialize_snapshot(packet, eid, x, y, size);
  if (entities.find(eid) != std::end(entities))
  {
    auto& e = entities[eid];
    e.x = x;
    e.y = y;
    e.size = size;
  }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "127.0.0.1");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 1920/2;
  int height = 1080/2;
  InitWindow(width, height, "w6 AI MIPT");

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
  camera.zoom = 1.f;

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  bool connected = false;
  while (!WindowShouldClose())
  {
    float dt = GetFrameTime();
    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    if (my_eid != invalid_entity)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      if (entities.find(my_eid) != std::end(entities))
      {
        auto& e = entities[my_eid];
        // Update
        e.x += ((left ? -dt : 0.f) + (right ? +dt : 0.f)) * 100.f;
        e.y += ((up ? -dt : 0.f) + (down ? +dt : 0.f)) * 100.f;

        // Send
        send_entity_state(serverPeer, my_eid, e.x, e.y, e.size);
      }
    }


    BeginDrawing();
      ClearBackground(GRAY);
      BeginMode2D(camera);
        for (auto& it : entities)
        {
          auto& e = it.second;
          //const Rectangle rect = {e.x, e.y, 10.f, 10.f};
          DrawCircle(e.x, e.y, e.size, GetColor(e.color));
          //DrawRectangleRec(rect, GetColor(e.color));
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();

  return 0;
}

#include "utility.h"

void send_reliable_packet(const char* msg, ENetPeer *peer, int len)
{
  ENetPacket *packet = enet_packet_create(msg, len, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_unreliable_packet(const char* msg, ENetPeer *peer, int len)
{
  ENetPacket *packet = enet_packet_create(msg, len, ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

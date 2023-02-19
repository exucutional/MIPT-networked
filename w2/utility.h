#pragma once
#include <enet/enet.h>
#include <cstring>

void send_reliable_packet(const char* msg, ENetPeer *peer, int len);
void send_unreliable_packet(const char* msg, ENetPeer *peer, int len);

#pragma once
// The port that both client and server will use for discovery
#define LISTEN_PORT 34567

// The reply that the server will send to the client scan
// Note that numeric types should be set to network byte order
typedef struct
{
	char hostname[1024];
	enet_uint16 port;
} ServerInfo;

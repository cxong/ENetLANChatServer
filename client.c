#include <stdio.h>

#include <enet/enet.h>


ENetHost *host;
#define CONNECTION_WAIT_MS 5000

void send_string(ENetPeer *peer, char *s)
{
	ENetPacket *packet = enet_packet_create(
		s, strlen(s) + 1, ENET_PACKET_FLAG_RELIABLE);
	if (enet_peer_send(peer, 0, packet) < 0)
	{
		fprintf(stderr, "Error when sending packet\n");
	}
}

int main(int argc, char *argv[])
{
	// Start client
	if (enet_initialize() != 0)
	{
		fprintf(stderr, "An error occurred while initializing ENet\n");
		return 1;
	}
	host = enet_host_create(NULL, 1, 2, 0, 0);
	if (host == NULL)
	{
		fprintf(stderr, "Failed to open ENet host\n");
		return 1;
	}
	// TODO: find server on LAN
	ENetAddress addr;
	enet_address_set_host(&addr, "127.0.0.1");
	addr.port = 34567;
	char buf[256];
	enet_address_get_host_ip(&addr, buf, sizeof buf);
	ENetPeer *peer = enet_host_connect(host, &addr, 2, 0);
	if (peer == NULL)
	{
		fprintf(stderr, "Failed to connect to server on %s:%d\n", buf, addr.port);
		return 1;
	}
	// Wait for connection to succeed
	ENetEvent event;
	if (enet_host_service(host, &event, CONNECTION_WAIT_MS) > 0 &&
		event.type == ENET_EVENT_TYPE_CONNECT)
	{
		printf("ENet client connected to %s:%d\n", buf, addr.port);
	}
	else
	{
		fprintf(stderr, "connection failed\n");
		return 1;
	}

	// Send a greeting
	send_string(peer, "Hello, my name is Inigo Montoya");
	enet_host_service(host, &event, 0);

	// TODO: event/input loop

	// Shut down client
	printf("Client closing\n");
	enet_peer_disconnect_now(peer, 0);
	enet_host_destroy(host);
	enet_deinitialize();
	return 0;
}

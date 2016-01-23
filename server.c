#include <stdio.h>
#include <signal.h>

#include <enet/enet.h>


// Simple LAN chat server
// Clients can send simple string messages to the server, which simply
// gets broadcast to all connected clients.


ENetHost *host;
#define MAX_CLIENTS 16

// Stop server on interrupt
volatile sig_atomic_t stop = 0;
void sigint_handle(int signum)
{
	stop = 1;
}

#ifdef _WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

void send_string(ENetHost *host, char *s)
{
	ENetPacket *packet = enet_packet_create(
		s, strlen(s) + 1, ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(host, 0, packet);
}


int main(int argc, char *argv[])
{
	signal(SIGINT, sigint_handle);

	// Start server
	if (enet_initialize() != 0)
	{
		fprintf(stderr, "An error occurred while initializing ENet\n");
		return 1;
	}
	ENetAddress addr;
	addr.host = ENET_HOST_ANY;
	// TODO: select random available port
	addr.port = 34567;
	host = enet_host_create(&addr, MAX_CLIENTS, 2, 0, 0);
	if (host == NULL)
	{
		fprintf(stderr, "Failed to open ENet host\n");
		return 1;
	}
	printf("ENet host started on port %d (press ctrl-C to exit)\n", addr.port);

	// Loop and process events
	int check;
	do
	{
		ENetEvent event;
		check = enet_host_service(host, &event, 0);
		if (check > 0)
		{
			char buf[256];
			// Whenever a client connects or disconnects, broadcast a message
			// Whenever a client says something, broadcast it including
			// which client it was from
			switch (event.type)
			{
				case ENET_EVENT_TYPE_CONNECT:
					sprintf(buf, "New client connected: id %d", event.peer->incomingPeerID);
					send_string(host, buf);
					printf("%s\n", buf);
					break;
				case ENET_EVENT_TYPE_RECEIVE:
					sprintf(buf, "Client %d says: %s", event.peer->incomingPeerID, event.packet->data);
					send_string(host, buf);
					printf("%s\n", buf);
					break;
				case ENET_EVENT_TYPE_DISCONNECT:
					sprintf(buf, "Client %d disconnected", event.peer->incomingPeerID);
					send_string(host, buf);
					printf("%s\n", buf);
					break;
				default:
					break;
			}
		}
		else if (check < 0)
		{
			fprintf(stderr, "Error servicing host\n");
		}
		Sleep(1);
	}
	while (!stop && check >= 0);

	// Shut down server
	printf("Server closing\n");
	enet_host_destroy(host);
	enet_deinitialize();
	return 0;
}

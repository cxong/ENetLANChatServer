#include <stdbool.h>
#include <stdio.h>
#include <signal.h>

#include <enet/enet.h>
#include "common.h"


// Simple LAN chat server
// Clients can send simple string messages to the server, which simply
// gets broadcast to all connected clients.


#ifdef _WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif
volatile sig_atomic_t stop = 0;
void sigint_handle(int signum);
typedef struct
{
	// The chat server host
	ENetHost *host;
	// The socket for listening and responding to client scans
	ENetSocket listen;
} ENetLANServer;
bool start_server(ENetLANServer *server);
void send_string(ENetHost *host, char *s);
void stop_server(ENetLANServer *server);
#define MAX_CLIENTS 16


int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	// Stop server on interrupt
	signal(SIGINT, sigint_handle);

	// Start server
	ENetLANServer server;
	if (!start_server(&server))
	{
		return 1;
	}

	// Loop and process events
	int check;
	do
	{
		// Check our listening socket for scanning clients
		ENetAddress recvaddr;
		char buf[256];
		ENetBuffer recvbuf;
		recvbuf.data = buf;
		recvbuf.dataLength = sizeof buf;
		const int recvlen = enet_socket_receive(server.listen, &recvaddr, &recvbuf, 1);
		if (recvlen > 0)
		{
			char addrbuf[256];
			enet_address_get_host_ip(&recvaddr, addrbuf, sizeof addrbuf);
			printf("Listen port: received (%d) from %s:%d\n",
				*(int *)recvbuf.data, addrbuf, recvaddr.port);
			// Reply to scanner client
			// TODO: send the port of the server host
			int data = 12345;
			recvbuf.data = &data;
			recvbuf.dataLength = sizeof data;
			if (enet_socket_send(server.listen, &recvaddr, &recvbuf, 1) != (int)recvbuf.dataLength)
			{
				fprintf(stderr, "Failed to reply to scanner\n");
				return NULL;
			}
		}

		ENetEvent event;
		check = enet_host_service(server.host, &event, 0);
		if (check > 0)
		{
			// Whenever a client connects or disconnects, broadcast a message
			// Whenever a client says something, broadcast it including
			// which client it was from
			switch (event.type)
			{
				case ENET_EVENT_TYPE_CONNECT:
					sprintf(buf, "New client connected: id %d", event.peer->incomingPeerID);
					send_string(server.host, buf);
					printf("%s\n", buf);
					break;
				case ENET_EVENT_TYPE_RECEIVE:
					sprintf(buf, "Client %d says: %s", event.peer->incomingPeerID, event.packet->data);
					send_string(server.host, buf);
					printf("%s\n", buf);
					break;
				case ENET_EVENT_TYPE_DISCONNECT:
					sprintf(buf, "Client %d disconnected", event.peer->incomingPeerID);
					send_string(server.host, buf);
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
		// Sleep a bit so we don't consume 100% CPU
		Sleep(1);
	}
	while (!stop && check >= 0);

	// Shut down server
	stop_server(&server);
	return 0;
}

void sigint_handle(int signum)
{
	if (signum == SIGINT)
	{
		stop = 1;
	}
}

bool start_server(ENetLANServer *server)
{
	// Start server
	if (enet_initialize() != 0)
	{
		fprintf(stderr, "An error occurred while initializing ENet\n");
		return false;
	}

	// Start listening socket
	server->listen = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
	ENetAddress listenaddr;
	listenaddr.host = ENET_HOST_ANY;
	listenaddr.port = LISTEN_PORT;
	if (enet_socket_bind(server->listen, &listenaddr) != 0)
	{
		fprintf(stderr, "Failed to bind listen socket\n");
		return false;
	}
	if (enet_socket_get_address(server->listen, &listenaddr) != 0)
	{
		fprintf(stderr, "Cannot get listen socket address\n");
		return false;
	}
	printf("Listening for scans on port %d\n", listenaddr.port);

	ENetAddress addr;
	addr.host = ENET_HOST_ANY;
	// This selects random available port
	addr.port = 0;
	server->host = enet_host_create(&addr, MAX_CLIENTS, 2, 0, 0);
	if (server->host == NULL)
	{
		fprintf(stderr, "Failed to open ENet host\n");
		return false;
	}
	printf("ENet host started on port %d (press ctrl-C to exit)\n",
		server->host->address.port);

	return true;
}

void send_string(ENetHost *host, char *s)
{
	ENetPacket *packet = enet_packet_create(
		s, strlen(s) + 1, ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(host, 0, packet);
}

void stop_server(ENetLANServer *server)
{
	printf("Server closing\n");
	if (enet_socket_shutdown(server->listen, ENET_SOCKET_SHUTDOWN_READ_WRITE) != 0)
	{
		fprintf(stderr, "Failed to shutdown listen socket\n");
	}
	enet_socket_destroy(server->listen);
	enet_host_destroy(server->host);
	enet_deinitialize();
}

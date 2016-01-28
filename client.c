#include <stdbool.h>
#include <stdio.h>

#include <enet/enet.h>
#include "common.h"
#include "rlutil.h"


// Simple LAN chat client
// The client sends string messages to the server, and the server passes
// them on to all clients

ENetHost *start_client(ENetPeer **peer, const int timeout_seconds);
void send_string(ENetPeer *peer, char *s);
void stop_client(ENetHost *host, ENetPeer *peer);
#define CONNECTION_WAIT_MS 5000


int main(int argc, char *argv[])
{
	// TODO: optionally connect directly to IP
	(void)argc;
	(void)argv;
	// Start client
	ENetPeer *peer;
	ENetHost *host = start_client(&peer, 5);
	if (host == NULL)
	{
		return 1;
	}

	// Send a greeting
	send_string(peer, "Hello, my name is Inigo Montoya");

	char buf[256];
	memset(buf, 0, sizeof buf);
	int check;
	do
	{
		Sleep(1);
		const int k = nb_getch();
		if (k == KEY_ENTER || k == '\r')
		{
			// Exit the client on text command
			if (strcmp(buf, "quit") == 0 || strcmp(buf, "exit") == 0)
			{
				break;
			}
			// If we have something to say, say it to the server
			if (strlen(buf) > 0)
			{
				send_string(peer, buf);
			}
			memset(buf, 0, sizeof buf);
			printf("\n");
		}
		else if (k > 0 && strlen(buf) < 255)
		{
			// Hold on to our message until we press enter
			buf[strlen(buf)] = (char)k;
			printf("%c", k);
		}

		// Check for new messages from the server; if there are any
		// just print them out
		ENetEvent event;
		check = enet_host_service(host, &event, 0);
		if (check > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_RECEIVE:
				printf("%s\n", event.packet->data);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				printf("Lost connection with server\n");
				check = -1;
				break;
			default:
				break;
			}
		}
		else if (check < 0)
		{
			fprintf(stderr, "Error servicing host\n");
		}
	}
	while (check >= 0);

	// Shut down client
	stop_client(host, peer);
	return 0;
}

ENetHost *start_client(ENetPeer **peer, const int timeout_seconds)
{
	if (enet_initialize() != 0)
	{
		fprintf(stderr, "An error occurred while initializing ENet\n");
		return NULL;
	}

	// Scan for server on LAN
	ENetSocket scanner = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
	if (scanner == ENET_SOCKET_NULL)
	{
		fprintf(stderr, "Failed to create socket\n");
		return NULL;
	}
	if (enet_socket_set_option(scanner, ENET_SOCKOPT_BROADCAST, 1) != 0)
	{
		fprintf(stderr, "Failed to enable broadcast socket\n");
		return NULL;
	}
	ENetAddress scanaddr;
	scanaddr.host = ENET_HOST_BROADCAST;
	scanaddr.port = LISTEN_PORT;
	// Send a dummy payload
	char data = 42;
	ENetBuffer sendbuf;
	sendbuf.data = &data;
	sendbuf.dataLength = 1;
	if (enet_socket_send(scanner, &scanaddr, &sendbuf, 1) != (int)sendbuf.dataLength)
	{
		fprintf(stderr, "Failed to scan for LAN server\n");
		return NULL;
	}
	// Wait for the reply, which will give us the server address
	ENetAddress addr;
	ServerInfo sinfo;
	bool found = false;
	for (int i = 0; i < timeout_seconds; i++)
	{
		printf("Scanning for server...\n");
		ENetSocketSet set;
		ENET_SOCKETSET_EMPTY(set);
		ENET_SOCKETSET_ADD(set, scanner);
		if (enet_socketset_select(scanner, &set, NULL, 0) > 0)
		{
			ENetBuffer recvbuf;
			recvbuf.data = &sinfo;
			recvbuf.dataLength = sizeof sinfo;
			const int recvlen = enet_socket_receive(scanner, &addr, &recvbuf, 1);
			if (recvlen > 0)
			{
				if (recvlen != sizeof(ServerInfo))
				{
					fprintf(stderr, "Unexpected reply from scan\n");
					return NULL;
				}
				addr.port = sinfo.port;
				char buf[256];
				enet_address_get_host_ip(&addr, buf, sizeof buf);
				printf("Found server '%s' at %s:%d\n", sinfo.hostname, buf, addr.port);
				found = true;
				break;
			}
		}
		Sleep(1000);
	}
	if (enet_socket_shutdown(scanner, ENET_SOCKET_SHUTDOWN_READ_WRITE) != 0)
	{
		fprintf(stderr, "Failed to shutdown listen socket\n");
		return NULL;
	}
	enet_socket_destroy(scanner);
	if (!found)
	{
		fprintf(stderr, "Server not found\n");
		return NULL;
	}

	ENetHost *host = enet_host_create(NULL, 1, 2, 0, 0);
	if (host == NULL)
	{
		fprintf(stderr, "Failed to open ENet host\n");
		return NULL;
	}
	char buf[256];
	enet_address_get_host_ip(&addr, buf, sizeof buf);
	*peer = enet_host_connect(host, &addr, 2, 0);
	if (*peer == NULL)
	{
		fprintf(stderr, "Failed to connect to server on %s:%d\n", buf, addr.port);
		return NULL;
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
		return NULL;
	}

	return host;
}

void send_string(ENetPeer *peer, char *s)
{
	ENetPacket *packet = enet_packet_create(
		s, strlen(s) + 1, ENET_PACKET_FLAG_RELIABLE);
	if (enet_peer_send(peer, 0, packet) < 0)
	{
		fprintf(stderr, "Error when sending packet\n");
	}
}

void stop_client(ENetHost *host, ENetPeer *peer)
{
	printf("Client closing\n");
	enet_peer_disconnect_now(peer, 0);
	enet_host_destroy(host);
	enet_deinitialize();
}

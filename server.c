#include <stdio.h>
#include <signal.h>

#include <enet/enet.h>


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
			switch (event.type)
			{
				case ENET_EVENT_TYPE_CONNECT:
					enet_address_get_host_ip(&event.peer->address, buf, sizeof buf);
					printf("New connection from %s\n", buf);
					break;
				case ENET_EVENT_TYPE_RECEIVE:
					printf("Received data '%s'\n", event.packet->data);
					break;
				case ENET_EVENT_TYPE_DISCONNECT:
					enet_address_get_host_ip(&event.peer->address, buf, sizeof buf);
					printf("Client disconnected %s\n", buf);
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

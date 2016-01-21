# ENet LAN Chat Server
A simple LAN chat server/client using ENet. Demonstrates hosting a server that
can be automatically discovered by clients running on the same LAN.

The basic concept is:
- The server runs the chat service on one port, using ENet host
- The server listens to a second, designated port for UDP broadcast messages
- Clients broadcast UDP to 255.255.255.255, which will be sent to all hosts on
  the same LAN
- The server responds with something (possibly the port of the chat service)
- The client now knows the presence of this server. Other servers on the same
  LAN will all respond the same way, letting the client know of them
- The client can connect to one of the servers as an ENet peer

This can be used to implement zero-conf LAN services like games.

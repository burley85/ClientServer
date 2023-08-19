#ifndef NETWORKING_H
#define NETWORKING_H

#include <WinSock2.h>

WSADATA init_winsock();
SOCKET init_server_socket();
SOCKET connect_to_API(char* API_ip_addr, int API_port_num);
void bind_socket(SOCKET server_socket_desc, char* ip_addr, int port_num);
void listen_on_socket(SOCKET server_socket_desc);
SOCKET accept_connection(SOCKET server_socket_desc);

#endif
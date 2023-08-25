#ifndef HANDLERS_H
#define HANDLERS_H

#include <WinSock2.h>

void handle_request(SOCKET API_socket_desc, SOCKET client_socket_desc, char* request);

#endif
#ifndef RESPONSES_H
#define RESPONSES_H

#include <WinSock2.h>

#include "db_structs.h"

int send_all(SOCKET client_socket_desc, const char* buffer, int buffer_len, int flags);
int send_page(SOCKET client_socket_desc, char* filepath, char* content_type, char* response_code);
// Returns session token
char* create_session(User *user);
void send_302(SOCKET client_socket_desc, char* redirect, char *token);
void send_404(SOCKET client_socket_desc);
void send_501(SOCKET client_socket_desc);
void send_obj_json(SOCKET client_desc, char* obj_json);

#endif
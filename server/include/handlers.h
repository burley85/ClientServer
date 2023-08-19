#ifndef HANDLERS_H
#define HANDLERS_H

#include <WinSock2.h>

void handle_api_request(SOCKET client_socket_desc, char* request);
void handle_get(SOCKET client_socket_desc, SOCKET API_socket_desc, char* request);
//If user was created successfully send 201 Created
void handle_register_request(SOCKET client_socket_desc, void* dbObj);
//Send 302 Found if user was logged in successfully
//Send 401 Unauthorized if login failed
void handle_login_request(SOCKET client_socket_desc, void* dbObj);
void handle_logout_request(SOCKET client_socket_desc, char* request);
//Returns a pointer to a dynamically allocated database struct
void handle_post(SOCKET client_socket_desc, SOCKET API_socket_desc, char* request);

#endif
#include "networking.h"
#include "Logger.h"

WSADATA init_winsock() {
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        print_error("WSAStartup() failed");
        exit(1);
    }

    print_debug("WSAStartup() success");

    return wsaData;
}

SOCKET init_server_socket() {
    SOCKET server_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_desc == INVALID_SOCKET) {
        print_error("socket() failed");
        if (WSACleanup()) {
            print_error("WSACleanup() also failed");
        }
        exit(1);
    }
    print_debug("Socket created");

    return server_socket_desc;
}

SOCKET connect_to_API(char* API_ip_addr, int API_port_num) {
    print_debug("Connecting to API at %s:%d", API_ip_addr, API_port_num);

    SOCKET API_server_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (API_server_desc == INVALID_SOCKET) {
        print_error("socket() failed");
        if (WSACleanup()) {
            print_error("WSACleanup() also failed");
        }
        exit(1);
    }
    print_debug("Socket created");


    SOCKADDR_IN server_addr;
  
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(API_port_num); 
    server_addr.sin_addr.s_addr = inet_addr(API_ip_addr);
  
    if(connect(API_server_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR){
        print_error("Failed to connect to API server");
        if (WSACleanup()) {
            print_error("WSACleanup() also failed");
        }
        exit(1);
    }

    print_debug("Connected to API server");

    return API_server_desc;
}

void bind_socket(SOCKET server_socket_desc, char* ip_addr, int port_num) {
    SOCKADDR_IN server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);
    if (bind(server_socket_desc, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        print_error("bind() failed");
        if (closesocket(server_socket_desc)) {
            print_error("closesocket() also failed");
        }
        if (WSACleanup()) {
            print_error("WSACleanup() also failed");
        }
        exit(1);
    }
    print_debug("Socket bound");
}

void listen_on_socket(SOCKET server_socket_desc) {
    if (listen(server_socket_desc, 1) == SOCKET_ERROR) {
        print_error("listen() failed");
        if (closesocket(server_socket_desc)) {
            print_error("closesocket() also failed.");
        }
        if (WSACleanup()) {
            print_error("WSACleanup() also failed");
        }
        exit(1);
    }
    print_debug("Listening for connection...");
}

SOCKET accept_connection(SOCKET server_socket_desc) {
    SOCKET client_socket_desc;
    SOCKADDR_IN client_addr;
    client_socket_desc = accept(server_socket_desc, NULL, NULL);
    if (client_socket_desc == INVALID_SOCKET) {
        print_error("accept() error");
        if (closesocket(server_socket_desc)) {
            print_error("closesocket() also failed");
        }
        if (WSACleanup()) {
            print_error("WSACleanup() also failed");
        }
        exit(1);
    }
    print_debug("Connection accepted");
    return client_socket_desc;
}

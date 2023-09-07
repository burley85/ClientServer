#include <WinSock2.h>
#include <stdio.h>

#include "CommandLineArgs.h"
#include "Logger.h"
#include "db_structs.h"
#include "handlers.h"
#include "networking.h"
#include "responses.h"


int port_num = 8080;                    // Default port number--can be set with -p
char* ip_addr = "192.168.1.232";        // Default IP address with -i param

int API_port_num = 8081;                // Default port number--can be set with -ap
char* API_ip_addr = "192.168.1.232";    // Default IP address with -ai param

//Map of session tokens to users
#define MAX_SESSIONS 16
struct {
    char token[17];
    User* user;
} SessionMap[MAX_SESSIONS];
int max_sessions = MAX_SESSIONS; //TODO: SessionMap struct

void parse_argv(int argc, char** argv) {
    setup_logger(argc, argv);

    char* param;
    if ((param = check_param(argc, argv, "-i")) || (param = check_param(argc, argv, "-ip")))
        ip_addr = param;
    if ((param = check_param(argc, argv, "-ai")) || (param = check_param(argc, argv, "-apiip")))
        API_ip_addr = param;
    if ((param = check_param(argc, argv, "-p")) || (param = check_param(argc, argv, "-port"))) {
        port_num = atoi(param);
        if (port_num <= 0) {
            print_error("%d is not a valid port number", port_num);
            exit(1);
        }
    }
    if ((param = check_param(argc, argv, "-ap")) || (param = check_param(argc, argv, "-apiport"))) {
        API_port_num = atoi(param);
        if (API_port_num <= 0) {
            print_error("%d is not a valid port number", API_port_num);
            exit(1);
        }
    }
}

void serve_client(SOCKET client_socket_desc, SOCKET API_socket_desc){
    while(1){
        // Get client's request
        char client_request[1024] = "";
        print_debug("Getting client request...");

        int rval = recv(client_socket_desc, client_request, sizeof(client_request), 0);
        if (rval == SOCKET_ERROR) {
            print_error("recv() failed");
            return;
        }
        else if (rval == 0) {
            print_debug("Client disconnected");
            return;
        }
        print_debug("Client request: %s", client_request);
        handle_request(API_socket_desc, client_socket_desc, client_request);
    }
}

int main(int argc, char** argv) {
    parse_argv(argc, argv);

    print_debug("Starting server with address 'http://%s:%d'...", ip_addr, port_num);
    printf("Starting server with address 'http://%s:%d'...\n", ip_addr, port_num);

    // Initialize WinSock
    WSADATA wsaData = init_winsock();

    // Create socket
    SOCKET server_socket_desc = init_server_socket();
    SOCKET API_server_desc = connect_to_API(API_ip_addr, API_port_num);

    // Bind socket
    bind_socket(server_socket_desc, ip_addr, port_num);

    SOCKET client_socket_desc;
    while (1) {
        // Listen for incoming connections
        listen_on_socket(server_socket_desc);

        // Accept socket
        client_socket_desc = accept_connection(server_socket_desc);

        serve_client(client_socket_desc, API_server_desc);

        closesocket(client_socket_desc);
    }

    // Close socket
    if (closesocket(client_socket_desc)) {
        print_error("closesocket() failed.\n");
    }

    // Cleanup WinSock
    if (WSACleanup()) {
        print_error("WSACleanup() failed.\n");
    }

    return 0;
}
#include <WinSock2.h>
#include <stdio.h>

#include "CommandLineArgs.h"
#include "Logger.h"

int port_num = 8080;                    // Default port number--can be set with -p
char* ip_addr = "192.168.1.232";        // Default IP address with -i param

int API_port_num = 8081;                // Default port number--can be set with -ap
char* API_ip_addr = "192.168.1.232";    // Default IP address with -ai param

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

SOCKET connect_to_API() {
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

void bind_socket(SOCKET server_socket_desc) {
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

int send_page(SOCKET client_socket_desc, char* filepath, char* content_type) {
    // Open file
    FILE* fp = fopen(filepath, "r");
    if (fp == NULL) {
        print_warning("Could not open file '%s'", filepath);
        return 0;
    }

    // Get file size
    fseek(fp, 0L, SEEK_END);
    int file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    // Send header
    char* header_format =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s; charset=UTF-8\r\n"
        "Content-Length: %d\r\n"
        "\r\n";
    char header[128] = "";
    sprintf(header, header_format, content_type, file_size);
    print_debug("Sending header: %s", header);
    send(client_socket_desc, header, strlen(header), 0);

    // Send file
    char buffer[256] = "";
    while (fgets(buffer, sizeof(buffer) - 2, fp) != NULL) {
        // Replace newline with \r\n
        if (buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\r';
            buffer[strlen(buffer)] = '\n';
        }

        int buffer_len = strlen(buffer);
        int bytes_sent = 0;
        // print_debug("Sending '%s'...", buffer);

        while (bytes_sent < buffer_len) {
            int rval = send(client_socket_desc, buffer + bytes_sent, buffer_len - bytes_sent, 0);
            bytes_sent += rval;
            if (rval == SOCKET_ERROR) {
                print_error("send() failed");
                fclose(fp);
                return 0;
            }
        }
        memset(buffer, 0, 256);
    }
    fclose(fp);
    send(client_socket_desc, buffer, 256, 0);
    print_debug("Sent file '%s'", filepath);
    return 1;
}

void send_404(SOCKET client_socket_desc) {
    const char* error_msg =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n";
    if (send(client_socket_desc, error_msg, strlen(error_msg), 0) == SOCKET_ERROR) {
        print_error("Failed to send 404 Not Found");
    }
}

void send_501(SOCKET client_socket_desc) {
    const char* error_msg =
        "HTTP/1.1 501 Not Implemented\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n";
    if (send(client_socket_desc, error_msg, strlen(error_msg), 0) == SOCKET_ERROR) {
        print_error("Failed to send 501 Not Implemented");
    }
}

void send_post_to_api(SOCKET API_socket_desc, char* request) {
    //Remove headers from request
    char* request_body = strstr(request, "\r\n\r\n") + 4;

    print_debug("Sending POST to API: %s", request_body);
    if (send(API_socket_desc, request_body, strlen(request_body), 0) == SOCKET_ERROR) {
        print_error("Failed to send POST to API server");
    }
}

void serve_client(SOCKET client_socket_desc, SOCKET API_socket_desc) {
    while (1) {
        // Get client's response
        char client_response[1024] = "";
        print_debug("Getting client request...");

        int rval = recv(client_socket_desc, client_response, sizeof(client_response), 0);
        if (rval == SOCKET_ERROR) {
            print_error("recv() failed");
            return;
        }
        else if (rval == 0) {
            print_debug("Client disconnected");
            return;
        }

        print_debug("Client request: %s", client_response);

        if (strncmp(client_response, "GET", 3) == 0) {
            char* file_name;
            // Check if client sent GET request for root
            if (strstr(client_response, "GET / HTTP/1.1") == client_response) {
                print_debug("Client sent GET request for root");
                file_name = "index.html";
            }
            else {
                file_name = malloc(256);
                sscanf(client_response, "GET /%s HTTP/1.1", file_name);
                print_debug("Client sent GET request for '%s'", file_name);
            }

            if (send_page(client_socket_desc, file_name, "text/html")) {
                print_debug("Sent page '%s'", file_name);
            }
            else {
                print_debug("Could not send page '%s'", file_name);
                send_404(client_socket_desc);
                return;
            }
        }

        //Check if client sent POST request
        else if (strncmp(client_response, "POST", 4) == 0) {
            print_debug("Client sent POST request");
            send_post_to_api(API_socket_desc, client_response);
        }

        // Send 501 Not Implemented for anything else
        else {
            print_debug("Sending 501 Not Implemented");
            send_501(client_socket_desc);
            return;
        }
    }
}

int main(int argc, char** argv) {
    parse_argv(argc, argv);

    print_debug("Starting server with address 'http://%s:%d'...", ip_addr, port_num);

    // Initialize WinSock
    WSADATA wsaData = init_winsock();

    // Create socket
    SOCKET server_socket_desc = init_server_socket();
    SOCKET API_server_desc = connect_to_API();

    // Bind socket
    bind_socket(server_socket_desc);

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
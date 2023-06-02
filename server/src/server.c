#include <stdio.h>
#include <WinSock2.h>

#include "Logger.h"
#include "CommandLineArgs.h"

int port_num = 8080; //Default port number--can be set with -p param
char* ip_addr = "192.168.1.232"; //Default IP address with -i param

void parse_argv(int argc, char **argv){
    setup_logger(argc, argv);

    char* param;
    if((param = check_param(argc, argv, "-i")) || (param = check_param(argc, argv, "-ip"))) ip_addr = param;
    if((param = check_param(argc, argv, "-p")) || (param = check_param(argc, argv, "-port"))){
        port_num = atoi(param);
        if(port_num <= 0){
            print_error("%d is not a valid port number", port_num);
            exit(1);
        }
    }
}

WSADATA init_winsock(){
    WSADATA wsaData;

    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
        print_error("WSAStartup() failed");
        exit(1);
    }

    print_debug("WSAStartup() success");

    return wsaData;
}

SOCKET init_server_socket(){
    SOCKET server_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket_desc == INVALID_SOCKET){
        print_error("socket() failed");
        if(WSACleanup()){
            print_error("WSACleanup() also failed");
        }
        exit(1);
    }
    print_debug("Socket created");

    return server_socket_desc;
}

void bind_socket(SOCKET server_socket_desc){
    SOCKADDR_IN server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);
    if(bind(server_socket_desc, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR){
        print_error("bind() failed");
        if(closesocket(server_socket_desc)){
            print_error("closesocket() also failed");
        }
        if(WSACleanup()){
            print_error("WSACleanup() also failed");
        }
        exit(1);
    }
    print_debug("Socket bound");

}

void listen_on_socket(SOCKET server_socket_desc){
    if(listen(server_socket_desc, 1) == SOCKET_ERROR){
        print_error("listen() failed");
        if(closesocket(server_socket_desc)){
            print_error("closesocket() also failed.");
        }
        if(WSACleanup()){
            print_error("WSACleanup() also failed");
        }
        exit(1);
    }
    print_debug("Listening for connection...");
}

SOCKET accept_connection(SOCKET server_socket_desc){
    SOCKET client_socket_desc;
    SOCKADDR_IN client_addr;
    client_socket_desc = accept(server_socket_desc, NULL, NULL);
    if(client_socket_desc == INVALID_SOCKET){
        print_error("accept() error");
        if(closesocket(server_socket_desc)){
            print_error("closesocket() also failed");
        }
        if(WSACleanup()){
            print_error("WSACleanup() also failed");
        }
        exit(1);  
    }
    print_debug("Connection accepted");
    return client_socket_desc;
}

void send_page(SOCKET client_socket_desc, char* filepath){
    //Open file
    FILE* fp = fopen(filepath, "r");
    if(fp == NULL){
        print_error("Could not open file '%s'", filepath);
        return;
    }

    //Get file size
    fseek(fp, 0L, SEEK_END);
    int file_size = ftell(fp) + 1;
    fseek(fp, 0L, SEEK_SET);
    
    //Send header
    char* header_format = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: %d\r\n\r\n";
    char header[128] = "";
    sprintf(header, header_format, file_size);
    print_debug("Sending header: %s", header);
    send(client_socket_desc, header, strlen(header), 0);

    //Send file
    char buffer[256] = "";
    while(fgets(buffer, sizeof(buffer), fp) != NULL){
        int bytes_sent = 0;
        print_debug("Sending '%s'...", buffer);
        while(bytes_sent < strlen(buffer)){
            bytes_sent += send(client_socket_desc, buffer + bytes_sent, strlen(buffer) - bytes_sent + 1, 0);
        }
        memset(buffer, 0, strlen(buffer));
    }
    fclose(fp);
    print_debug("Sent file '%s'", filepath);
    //send(client_socket_desc, " ", 20, 0);
}

int main(int argc, char **argv){
    parse_argv(argc, argv);

    print_debug("Starting server with address 'http://%s:%d'...", ip_addr, port_num);

    //Initialize WinSock
    WSADATA wsaData = init_winsock();

    //Create socket
    SOCKET server_socket_desc = init_server_socket();   

    //Bind socket
    bind_socket(server_socket_desc);

    SOCKET client_socket_desc;
    while(1){
        //Listen for incoming connections
        listen_on_socket(server_socket_desc);

        //Accept socket
        client_socket_desc = accept_connection(server_socket_desc);
        
        //Get client's response
        char client_response[1024] = "";
        print_debug("Getting client response after connecting...");

        int rval = recv(client_socket_desc, client_response, sizeof(client_response), 0);
        if(rval > 0){
            print_debug("Client response after connecting: %s\n", client_response);
        }
        print_debug("rcv() returned %d", rval);


        //Send web page with header to client
        send_page(client_socket_desc, "index.html");

        // //Get client's response
        // memset(client_response, 0, sizeof(client_response));
        // if(recv(client_socket_desc, client_response, sizeof(client_response), 0) > 0){
        //     print_debug("Client response after sending header: %s\n", client_response);
        // }


        //Get client's response
        client_response[1024];
        memset(client_response, 0, sizeof(client_response));
        print_debug("Getting client response after connecting...");
        if(recv(client_socket_desc, client_response, sizeof(client_response), 0) > 0){
            print_debug("Client response: %s\n", client_response);
        }
        print_debug("rcv() returned %d", rval);

    }

    //Close socket
    if(closesocket(client_socket_desc)){
        print_error("closesocket() failed.\n");
    }

    //Cleanup WinSock
    if(WSACleanup()){
        print_error("WSACleanup() failed.\n");
    }

    return 0;
}
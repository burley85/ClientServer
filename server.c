#include <WinSock2.h>
#include <stdio.h>

#define DEBUG 1 //Debug mode

int default_port = 1234; //Default port number
char default_ip[] = "192.168.1.232"; //Default IP address

//Set client's header to "Hello World!"
int main(int argc, char **argv){

    if(DEBUG) printf("DEBUG: Starting server with address 'http://%s:%d'...\n", default_ip, default_port);

    //Initialize WinSock
    WSADATA wsaData;

    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
        printf("ERROR: WSAStartup() failed.\n");
        return 1;
    }
    
    if(DEBUG) printf("DEBUG: WSAStartup() success.\n");

    //Create socket
    SOCKET server_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket_desc == INVALID_SOCKET){
        printf("ERROR: socket() failed.\n");
        if(WSACleanup()){
            printf("ERROR: WSACleanup() also failed.\n");
        }
        return 1;
    }
    if(DEBUG) printf("DEBUG: Socket created.\n");

    //Bind socket
    SOCKADDR_IN server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(default_port);
    server_addr.sin_addr.s_addr = inet_addr(default_ip);
    if(bind(server_socket_desc, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR){
        printf("ERROR: bind() failed.\n");
        if(closesocket(server_socket_desc)){
            printf("ERROR: closesocket() also failed.\n");
        }
        if(WSACleanup()){
            printf("ERROR: WSACleanup() also failed.\n");
        }
        return 1;
    }
    if(DEBUG) printf("DEBUG: Socket bound. Listening for connection...\n");

    //Listen for incoming connections
    if(listen(server_socket_desc, 1) == SOCKET_ERROR){
        printf("ERROR: listen() failed.\n");
        if(closesocket(server_socket_desc)){
            printf("ERROR: closesocket() also failed.\n");
        }
        if(WSACleanup()){
            printf("ERROR: WSACleanup() also failed.\n");
        }
        return 1;
    }
    if(DEBUG) printf("DEBUG: Connecting...\n");

    //Accept socket
    SOCKET client_socket_desc;
    SOCKADDR_IN client_addr;
    int szClntAddr = sizeof(client_addr);
    client_socket_desc = accept(server_socket_desc, NULL, NULL);
    if(client_socket_desc == INVALID_SOCKET){
        printf("accept() error!");
        if(closesocket(server_socket_desc)){
            printf("ERROR: closesocket() also failed.\n");
        }
        if(WSACleanup()){
            printf("ERROR: WSACleanup() also failed.\n");
        }
        return 1;    
    }
    if(DEBUG) printf("DEBUG: Connection accepted.\n");

    //Send web page to client

    //Send web page with header to client 
    const char *responseHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    const char *webpageContent = "<html><body><h1>Welcome to the Server!</h1></body></html>";

    send(client_socket_desc, responseHeader, strlen(responseHeader), 0);
    send(client_socket_desc, webpageContent, strlen(webpageContent), 0);

    //Get client's response
    char client_response[1024];
    recv(client_socket_desc, client_response, sizeof(client_response), 0);
    if(DEBUG) printf("DEBUG: Client response: %s\n", client_response);

    //Close socket
    if(closesocket(client_socket_desc)){
        printf("ERROR: closesocket() failed.\n");
    }

    //Cleanup WinSock
    if(WSACleanup()){
        printf("ERROR: WSACleanup() failed.\n");
    }

    return 0;
}
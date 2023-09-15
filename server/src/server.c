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

//Returns the client's request as a dynamically allocated string
//The returned string may also contain part of the response body
char* get_client_request_header(SOCKET client_socket_desc, int* bytes_read){
    int response_max_length = 256;
    float length_factor = 1.5;

    char* response = malloc(response_max_length);
    if(response == NULL){
        print_error("Failed to allocate memory for response");
        return NULL;
    }
    memset(response, 0, response_max_length);

    //Receive the response until "\r\n\r\n" is found
    while(strstr(response, "\r\n\r\n") == NULL){
        //Receive the response
        int rval = recv(client_socket_desc, response + *bytes_read, response_max_length - *bytes_read, 0);
        if (rval == SOCKET_ERROR) {
            print_error("recv() failed");
            free(response);
            return NULL;
        }
        else if (rval == 0) {
            print_debug("Client disconnected");
            free(response);
            return "\0";
        }
        *bytes_read += rval;

        //Reallocate memory if necessary
        if(*bytes_read == response_max_length){
            print_debug("Reallocating memory for response header. Increasing size from %d to %d", response_max_length, (int) (response_max_length * length_factor));
            char* temp = realloc(response, response_max_length * length_factor);
            if(temp == NULL){
                print_error("Failed to reallocate memory for response");
                free(response);
                return NULL;
            }
            response = temp;
            //Set all of the new memory to 0
            memset(response + response_max_length, 0, (response_max_length * length_factor) - response_max_length);
            response_max_length *= length_factor;
        }
    }
    print_debug("Client request header: %s", response);
    return response;
}

char* get_client_request(SOCKET client_socket_desc){
    int bytes_read = 0;
    char* request_header = get_client_request_header(client_socket_desc, &bytes_read);
    
    //Realloc the response to include the body
    char* body_start = strstr(request_header, "\r\n\r\n") + 4;
    int header_length = body_start - request_header;
    char* content_length_header = strstr(request_header, "Content-Length: ");
    if(content_length_header == NULL){
        print_warning("Content-Length header not found"); //TODO: Send Content-Length required response
        return request_header;
    }
    int content_length = atoi(content_length_header + 16);
    int response_length = header_length + content_length;
    char* response = realloc(request_header, response_length + 1);
    if(response == NULL){
        print_error("Failed to allocate memory for response");
        free(request_header);
        return NULL;
    }

    while(bytes_read < response_length){
        int rval = recv(client_socket_desc, response + bytes_read, response_length - bytes_read, 0);
        if (rval == SOCKET_ERROR) {
            print_error("recv() failed");
            free(response);
            return NULL;
        }
        else if (rval == 0) {
            print_debug("Client disconnected");
            free(response);
            return "\0";
        }
        bytes_read += rval;
    }

    response[response_length] = '\0';
    print_debug("Client request body: %s", strstr(response, "\r\n\r\n") + 4);
    return response;
}

void serve_client(SOCKET client_socket_desc, SOCKET API_socket_desc){
    while(1){
        // Get client's request
        char* client_request = get_client_request(client_socket_desc);
        
        if(client_request == NULL || client_request[0] == '\0') return;
 
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
#include <WinSock2.h>
#include <stdio.h>

#include "CommandLineArgs.h"
#include "Logger.h"
#include "db_structs.h"

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

int send_all(SOCKET client_socket_desc, const char* buffer, int buffer_len, int flags) {
    int bytes_sent = 0;
    while (bytes_sent < buffer_len) {
        int rval = send(client_socket_desc, buffer + bytes_sent, buffer_len - bytes_sent, flags);
        bytes_sent += rval;
        if (rval == SOCKET_ERROR) {
            print_error("send() failed");
            return 0;
        }
    }
    return 1;
}

int send_page(SOCKET client_socket_desc, char* filepath, char* content_type, char* response_code) {
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
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s; charset=UTF-8\r\n"
        "Content-Length: %d\r\n"
        "\r\n";
    char header[128] = "";
    sprintf(header, header_format, response_code, content_type, file_size);
    print_debug("Sending header: %s", header);
    if(!send_all(client_socket_desc, header, strlen(header), 0)){
        print_error("Failed to send header for file '%s'", filepath);
        fclose(fp);
        return 0;
    }

    // Send file
    char buffer[256] = "";
    while (fgets(buffer, sizeof(buffer) - 2, fp) != NULL) {
        // Replace newline with \r\n
        if (buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\r';
            buffer[strlen(buffer)] = '\n';
        }

        if(!send_all(client_socket_desc, buffer, strlen(buffer), 0)){
            print_error("Failed to send file '%s'", filepath);
            fclose(fp);
            return 0;
        }

        memset(buffer, 0, 256);
    }
    fclose(fp);
    print_debug("Sent file '%s'", filepath);
    return 1;
}

// Returns session token
char* create_session(User *user){
    //Create string of random characters
    char token[17] = "";
    for(int i = 0; i < 16; i++){
        char charSet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        token[i] = charSet[rand() % (sizeof(charSet) - 1)];
    }
    token[16] = '\0';

    //Add token and user to session map
    int i;
    for(i = 0; i < MAX_SESSIONS; i++){
        if(SessionMap[i].user == NULL){
            strcpy(SessionMap[i].token, token);
            SessionMap[i].user = malloc(sizeof(User));
            *(SessionMap[i].user) = *user;
            print_debug("Added token %s and user %s to session map at index %d", SessionMap[i].token, userToStr(*(SessionMap[i].user)), i);
            break;
        }
    }
    if(i == MAX_SESSIONS) print_error("Session map is full");
    return SessionMap[i].token;
}

void send_302(SOCKET client_socket_desc, char* redirect, char *token) {
    char *set_cookie_params = "Path=/; HttpOnly";
    if(token == NULL){
        token = "deleted";
        set_cookie_params = "Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT";
    }
    char* header_format =
        "HTTP/1.1 302 Found\r\n"
        "Location: %s\r\n"
        "Set-Cookie: token=%s; %s\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    char header[128] = "";
    sprintf(header, header_format, redirect, token, set_cookie_params);
    print_debug("Sending header: %s", header);
    if(!send_all(client_socket_desc, header, strlen(header), 0)){
        print_error("Failed to send 302 Found");
    }
}

void send_404(SOCKET client_socket_desc) {
    const char* error_msg =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n";
    if (!send_all(client_socket_desc, error_msg, strlen(error_msg), 0)) {
        print_error("Failed to send 404 Not Found");
    }
}

void send_501(SOCKET client_socket_desc) {
    const char* error_msg =
        "HTTP/1.1 501 Not Implemented\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n";
    if (!send_all(client_socket_desc, error_msg, strlen(error_msg), 0)) {
        print_error("Failed to send 501 Not Implemented");
    }
}

void send_obj_json(SOCKET client_desc, char* obj_json) {
    char* header_format =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json; charset=UTF-8\r\n"
        "Content-Length: %d\r\n"
        "\r\n";
    char header[128] = "";
    sprintf(header, header_format, strlen(obj_json));
    
    print_debug("Sending header: %s", header);
    if(!send_all(client_desc, header, strlen(header), 0)){
        print_error("Failed to send header for object '%s'", obj_json);
        return;
    }

    if(!send_all(client_desc, obj_json, strlen(obj_json), 0)){
        print_error("Failed to send object '%s'", obj_json);
        return;
    }
}

void handle_api_request(SOCKET client_socket_desc, char* request){
    char* request_obj = strstr(request, "GET /api/") + 9;
    char* session_token = strstr(request, "token=") + 6;

    if(strstr(request_obj, "user HTTP/1.1") == request_obj){
        print_debug("Client sent GET request for user with token %s", session_token);
        for(int i = 0; i < MAX_SESSIONS; i++){
            printf("Comparing %s to %s\n", SessionMap[i].token, session_token);
            if(!strncmp(SessionMap[i].token, session_token, 16)){
                print_debug("Found user with token %s", session_token);
                char* userStr = userToStr(*(SessionMap[i].user));
                send_obj_json(client_socket_desc, userStr);
                free(userStr);
                return;
            }
        }
        print_warning("Could not find user with token %16s", session_token);
    }

}

void handle_get(SOCKET client_socket_desc, SOCKET API_socket_desc, char* request){
    char* root = "login.html";

    // Check if client sent GET request for root
    if (strstr(request, "GET / HTTP/1.1") == request) {
        print_debug("Client sent GET request for root");
        send_page(client_socket_desc, root, "text/html", "200 OK");
        return;
    }
    
    // Check if client set API request
    if (strstr(request, "GET /api") == request) {
        print_debug("Client sent API request");
        handle_api_request(client_socket_desc, request);
        return;
    }

    char* file_name = malloc(256);
    sscanf(request, "GET /%s HTTP/1.1", file_name);
    print_debug("Client sent GET request for '%s'", file_name);

    if (!send_page(client_socket_desc, file_name, "text/html", "200 OK")) {
        print_debug("Could not send page '%s'", file_name);
        send_404(client_socket_desc);
    }
}

//If user was created successfully send 201 Created
void handle_register_request(SOCKET client_socket_desc, void* dbObj){
    User* u = (User*) dbObj;

    if(u != NULL) send_page(client_socket_desc, "login.html", "text/html", "201 Created");
    
    else send_page(client_socket_desc, "register.html", "text/html", "409 Conflict");
    
}

//Send 302 Found if user was logged in successfully
//Send 401 Unauthorized if login failed
void handle_login_request(SOCKET client_socket_desc, void* dbObj){
    User* u = (User*) dbObj;

    if(u != NULL){
        char* token = create_session(u);
        send_302(client_socket_desc, "home.html", token);
    }
    else {
        send_page(client_socket_desc, "login.html", "text/html", "401 Unauthorized");
    }
}

void handle_logout_request(SOCKET client_socket_desc, char* request){
    char* session_token = strstr(request, "token=") + 6;
        for(int i = 0; i < MAX_SESSIONS; i++){
            if(!strncmp(SessionMap[i].token, session_token, 16)){
                print_debug("Found user with token %s", session_token);
                free(SessionMap[i].user);
                SessionMap[i].user = NULL;
                memset(SessionMap[i].token, 0, 17);
            }
        }
        print_warning("Could not find user with token %s", session_token);

    send_302(client_socket_desc, "login.html", NULL);
}

//Returns a pointer to a dynamically allocated database struct
void handle_post(SOCKET client_socket_desc, SOCKET API_socket_desc, char* request) {
    //Remove headers from request
    char* request_body = strstr(request, "\r\n\r\n") + 4;

    char* formType = strstr(request_body, "type=") + 5;

    if(!strncmp("logout", formType, 6)){
        handle_logout_request(client_socket_desc, request);
        return;
    }

    print_debug("Sending POST to API: %s", request_body);
    if (send_all(API_socket_desc, request_body, strlen(request_body), 0) == SOCKET_ERROR) {
        print_error("Failed to send POST to API server");
        return;
    }


    // Get API response
    char API_response[1024] = "";
    if(recv(API_socket_desc, API_response, sizeof(API_response), 0) == SOCKET_ERROR){
        print_error("Failed to receive API response");
        return;
    }
    print_debug("Received API response: %s\n", API_response);

    // Parse API response into database struct
    void* dbObj = strToDatabaseObject(API_response);

    if(!strncmp(formType, "register", 8)) handle_register_request(client_socket_desc, dbObj);
    if(!strncmp(formType, "login", 5)) handle_login_request(client_socket_desc, dbObj);

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

        if (strncmp(client_response, "GET", 3) == 0)
            handle_get(client_socket_desc, API_socket_desc, client_response);
        
        //Check if client sent POST request
        else if (strncmp(client_response, "POST", 4) == 0)
            handle_post(client_socket_desc, API_socket_desc, client_response);

        // Send 501 Not Implemented for anything else
        else send_501(client_socket_desc);
        
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
#include <stdio.h>

#include "handlers.h"
#include "db_structs.h"
#include "responses.h"
#include "Logger.h"

extern int max_sessions;
extern struct {
    char token[17];
    User* user;
} SessionMap[];

void handle_api_request(SOCKET client_socket_desc, char* request){
    char* request_obj = strstr(request, "GET /api/") + 9;
    char* session_token = strstr(request, "token=") + 6;

    if(strstr(request_obj, "user HTTP/1.1") == request_obj){
        print_debug("Client sent GET request for user with token %s", session_token);
        for(int i = 0; i < max_sessions; i++){
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
    for(i = 0; i < max_sessions; i++){
        if(SessionMap[i].user == NULL){
            strcpy(SessionMap[i].token, token);
            SessionMap[i].user = malloc(sizeof(User));
            *(SessionMap[i].user) = *user;
            print_debug("Added token %s and user %s to session map at index %d", SessionMap[i].token, userToStr(*(SessionMap[i].user)), i);
            break;
        }
    }
    if(i == max_sessions) print_error("Session map is full");
    return SessionMap[i].token;
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
        for(int i = 0; i < max_sessions; i++){
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
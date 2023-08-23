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

void handle_api_request(SOCKET API_socket_desc, SOCKET client_socket_desc, char* request){
    char* request_obj_ptr = strstr(request, "GET /api/");
    char request_obj[32];
    memset(request_obj, 0, sizeof(request_obj));
    sscanf(request_obj_ptr, "GET /api/%31s", request_obj);

    char* token_ptr = strstr(request, "token=");
    char session_token[17];
    memset(session_token, 0, sizeof(session_token));
    sscanf(token_ptr, "token=%16s", session_token);

    print_debug("User with token %16s sent api request for %s", session_token, request_obj);
    //Find the user using the session token
    User* user = NULL;
    int i;
    for(i = 0; i < max_sessions; i++){
        if(!strncmp(SessionMap[i].token, session_token, 16)){
            print_debug("Found user with token %16s", session_token);
            user = SessionMap[i].user;
            break;
        }
    }
    if(i == max_sessions){
        print_warning("Could not find user with token %16s", session_token);
        send_404(client_socket_desc);
        return;
    }

    //Send user object if request is for user
    if(!strcmp(request_obj, "user")
        //strstr(request_obj_ptr, "GET /api/user HTTP") == request_obj_ptr
        ){
        char* userStr = userToStr(*(SessionMap[i].user));
        send_obj_json(client_socket_desc, userStr);
        free(userStr);
        return;
    }
    //Otherwise, create API server request
    else{
        if(!strcmp("channels", request_obj)){
            char* api_request_format = "type=%s&username=%s";
            char api_request[256] = "";
            sprintf(api_request, api_request_format, request_obj, user->username);
            print_debug("Sending API request: %s", api_request);
            if (!send_all(API_socket_desc, api_request, strlen(api_request), 0))
                    print_error("Failed to send API request");
        }
        char api_response[1024] = "";
        if(recv(API_socket_desc, api_response, sizeof(api_response), 0) == SOCKET_ERROR){
            print_error("Failed to receive API response");
            return;
        }
        print_debug("Received API response: %s\n", api_response);
        char* channel_list = strstr(api_response, "DatabaseObjectList = ") + 21;
        //Replace all ' with " unless preceded by '\'
        for(int i = 0; i < strlen(channel_list); i++){
            if(channel_list[i] == '\'' && channel_list[i-1] != '\\'){
                channel_list[i] = '\"';
            }
        }
        send_obj_json(client_socket_desc, channel_list);
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
        handle_api_request(API_socket_desc, client_socket_desc, request);
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

void handle_create_channel_request(SOCKET client_socket_desc, void* dbObj){
    Channel* c = (Channel*) dbObj;

    if(c != NULL) send_page(client_socket_desc, "home.html", "text/html", "201 Created");
    
    else send_page(client_socket_desc, "createChannel.html", "text/html", "400 Bad Request");
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

    // Append username to request body
    char* session_token = strstr(request, "token=");
    if(session_token != NULL){
        session_token += 6;
        char* new_request = request_body;
        int i;
        for(i = 0; i < max_sessions; i++){
            if(!strncmp(SessionMap[i].token, session_token, 16)){
                char *username = SessionMap[i].user->username;
                new_request = malloc(strlen(username) + strlen(request_body) + strlen("&username=") + 1);
                sprintf(new_request, "%s&username=%s", request_body, username);
                print_debug("Found user with token %s", session_token);
                break;
            }
            else{
                print_debug("User with token %s does not match %s", session_token, SessionMap[i].token);
            }
        }
        if(i == max_sessions) print_warning("Could not find user with token %s", session_token);
        
        print_debug("Sending POST to API: %s", new_request);
        if(!send_all(API_socket_desc, new_request, strlen(new_request), 0)){
            print_error("Failed to send POST to API server");
            return;
        }
        free(new_request);
    }
    else{
        print_debug("Sending POST to API: %s", request_body);
        if(!send_all(API_socket_desc, request_body, strlen(request_body), 0)){
            print_error("Failed to send POST to API server");
            return;
        }
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
    if(!strncmp(formType, "createChannel", 13)) handle_create_channel_request(client_socket_desc, dbObj);
}
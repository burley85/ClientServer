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


User* get_session(char* request){
    char* token_ptr = strstr(request, "Cookie: token=");
    if(token_ptr == NULL) return NULL;

    char session_token[17];
    memset(session_token, 0, sizeof(session_token));
    sscanf(token_ptr, "Cookie: token=%16s", session_token);

    int i;
    for(i = 0; i < max_sessions; i++){
        if(!strncmp(SessionMap[i].token, session_token, 16)){
            print_debug("Found user with token %16s", session_token);
            return SessionMap[i].user;
        }
    }
    print_warning("Could not find user with token %16s", session_token);
    return NULL;
}

//Receive from API
char* API_recv(SOCKET API_socket_desc){
    //API response will look like "<Length of response>\n<Response>"
    //Read length of API response
    char length_buffer[16] = "";
    if(recv(API_socket_desc, length_buffer, sizeof(length_buffer) - 1, 0) == SOCKET_ERROR){
        print_error("Failed to receive API response length");
        return NULL;
    }
    char* response_start = NULL;
    long len = strtol(length_buffer, &response_start, 10);
    
    //response_start should point to '\n'
    if(*response_start != '\n' || len == LONG_MAX || len == LONG_MIN){
        print_error("Failed to receive valid API response length");
        return NULL;
    }
    response_start++;

    char* response = malloc(len + 1);
    memset(response, 0, len + 1);
    sprintf(response, "%s", response_start);
    
    int bytes_read = strlen(response_start);
    while(bytes_read < len){
        if(recv(API_socket_desc, response + bytes_read, len - bytes_read, 0) == SOCKET_ERROR){
            print_error("Failed to receive API response");
            free(response);
            return NULL;
        }
        bytes_read += strlen(response + bytes_read);
    }

    //Replace all ' with " unless preceded by '\'
    for(int i = 1; i < len + 1; i++){
        if(response[i] == '\'' && response[i - 1] != '\\' && i > 0){
            response[i] = '\"';
        }
    }

    return response;
}

//Create API request from HTTP request
char* create_api_request(char* http_request){
    struct {
        char* http_cmd;
        char* api_cmd;
    } cmd_map[] = {
        {"POST", "create"},
        {"GET", "read"},
        {"PATCH", "update"},
        {"DELETE", "delete"}
    };
    char http_cmd[8] = "";
    char obj[32] = "";
    char parameters[512] = "";
    int parameters_len = 0;

    sscanf(http_request, "%7s /api/%[a-zA-Z]31s", http_cmd, obj);

    //Choose API command based on HTTP command
    char* api_cmd = NULL;
    for(int i = 0; i < 4; i++){
        if(!strcmp(cmd_map[i].http_cmd, http_cmd)){
            api_cmd = cmd_map[i].api_cmd;
            break;
        }
    }
    if(api_cmd == NULL){
        print_warning("Received HTTP request with unknown command: %s", http_cmd);
        return NULL;
    }

    //Append URI query parameters to the end of the API request
    char* query_ptr = strstr(http_request, "?");
    char* end_of_first_line = strstr(http_request, " HTTP/1.1\r\n");
    if(query_ptr != NULL && query_ptr < end_of_first_line){
        query_ptr += 1;
        int query_len = end_of_first_line - query_ptr;
        sprintf(parameters + parameters_len, "&%.*s", query_len, query_ptr);
        parameters_len += strlen(parameters + parameters_len);
    }

    //Append request body to the end of the API request, unless it is a GET or DELETE request
    if(strcmp(http_cmd, "GET") && strcmp(http_cmd, "DELETE")){
        char* request_body = strstr(http_request, "\r\n\r\n");
        if(request_body != NULL){
            sprintf(parameters + parameters_len, "&%s", request_body + 4);
            parameters_len += strlen(parameters + parameters_len);
        }
    }
        
    //If request is for a User and no parameters were specified, add user_id parameter, based on session token
    if(!strcmp(obj, "User") && parameters_len == 0){
        User* user = get_session(http_request);
        if(user != NULL) {
            sprintf(parameters + parameters_len, "&id=%d", user->id);
            parameters_len = strlen(parameters);
        }
    }

    char* api_request = malloc(600);
    sprintf(api_request, "cmd=%s&obj=%s%s", api_cmd, obj, parameters);

    return api_request;
}

//Forward HTTP request to API server and send API response to client
void handle_api_get_request(SOCKET API_socket_desc, SOCKET client_socket_desc, char* request){    
    char* api_request = create_api_request(request);

    print_debug("Sending API request: %s", api_request);
    if (!send_all(API_socket_desc, api_request, strlen(api_request), 0)){
        print_error("Failed to send API request");
        send_500(client_socket_desc);
        return;
    }

    char* api_response = API_recv(API_socket_desc);
    if(api_response == NULL){
        send_500(client_socket_desc);
        return;
    }

    print_debug("Received API response: %s\n", api_response);

    if(!strcmp(api_response, "None")) send_400(client_socket_desc);
    else{
        //Look for the start of the JSON object
        char* json_start = strstr(api_response, "{");
        send_obj_json(client_socket_desc, json_start);
    }
    free(api_response);
}

//Forward HTTP request to API server and redirect client to appropriate page
void handle_api_post_request(SOCKET API_socket_desc, SOCKET client_socket_desc, char* request){
    char* obj = strstr(request, "/api/") + 5;
    char* api_request = create_api_request(request);

    print_debug("Sending API request: %s", api_request);
    if (!send_all(API_socket_desc, api_request, strlen(api_request), 0)){
        free(api_request);
        print_error("Failed to send API request");
        send_500(client_socket_desc);
        return;
    }
    
    free(api_request);

    char* api_response = API_recv(API_socket_desc);
    if(api_response == NULL){
        send_500(client_socket_desc);
        return;
    }
    print_debug("Received API response: %s\n", api_response);
    //TODO: FIX THIS

    if(strcmp(api_response, "None")){
        //Send 201 Created
        //Send 303 See Other where the Location header is based on what object was created
        send_201(client_socket_desc, strstr(api_response, "{"));
        // if(!strncmp(obj, "User", 4)) send_303(client_socket_desc, "../login.html");
        // else if(strncmp(obj, "Channel", 7)) send_303(client_socket_desc, "../home.html");
        // else send_303(client_socket_desc, "../home.html");
        //TODO: handle other types of objects
    }
    
    else send_400(client_socket_desc);

    free(api_response);
}

void handle_get(SOCKET client_socket_desc, SOCKET API_socket_desc, char* request){
    char* root = "login.html";

    // Check if client sent GET request for root
    if (strstr(request, "GET / HTTP/1.1") == request) {
        print_debug("Client sent GET request for root");
        send_page(client_socket_desc, root, "text/html", "200 OK");
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
//Send 400 Bad Request if login failed
void handle_login_request(SOCKET API_socket_desc, SOCKET client_socket_desc, char* request){
    char *http_request_body = strstr(request, "\r\n\r\n");
    if(http_request_body == NULL){
        print_error("Could not find request body");
        send_400(client_socket_desc);
        return;
    }
    
    char api_request[128] = "";
    sprintf(api_request, "cmd=read&obj=User&%s", http_request_body + 4);
    if(!send_all(API_socket_desc, api_request, strlen(api_request), 0)){
        print_error("Failed to send API request");
        send_500(client_socket_desc);
        return;
    }

    char* api_response = API_recv(API_socket_desc);

    if(api_response == NULL){
        send_500(client_socket_desc);
        return;
    }

    print_debug("Received API response: %s\n", api_response);

    User* u = (User*) strToDatabaseObject(api_response);

    if(u == NULL){
        print_warning("Invalid username or password");
        send_page(client_socket_desc, "login.html", "text/html", "401 Unauthorized");
    }
    else{
        char* token = create_session(u);
        send_302(client_socket_desc, "home.html", token);
    }
    free(api_response);
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

void handle_request(SOCKET API_socket_desc, SOCKET client_socket_desc, char* request){
    if(!strncmp(request, "GET /api/", 9))
        handle_api_get_request(API_socket_desc, client_socket_desc, request);
    else if(!strncmp(request, "POST /api/", 10))
        handle_api_post_request(API_socket_desc, client_socket_desc, request);
    else if(!strncmp(request, "GET /", 5))
        handle_get(client_socket_desc, API_socket_desc, request);
    else if(!strncmp(request, "POST /login", 6))
        handle_login_request(API_socket_desc, client_socket_desc, request);
    else if(!strncmp(request, "POST /logout", 6))
        handle_logout_request(client_socket_desc, request);
    else send_400(client_socket_desc);
}
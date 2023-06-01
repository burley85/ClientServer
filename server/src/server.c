#include <stdio.h>
#include <WinSock2.h>

#include "server_output.h"

//If DEBUG and WARNING, print DEBUG message to log, WARNING message to stderr
//If !DEBUG and WARNING, print WARNING message to log
//DEBUG and !WARNING is silly
int debug = 0; //Default debug flag--can be set with --d flag
int warnings = 1; //Default warnings flag--can be set with --w flag

char* log_file = "log.txt"; //Default log file name--can be set with -l param; "" for output to stdout

int port_num = 8080; //Default port number--can be set with -p param
char* ip_addr = "192.168.1.232"; //Default IP address with -i param

int check_flag(int argc, char **argv, char* look_for){
    for(int i = 0; i < argc; i++){
        if(strcmp(argv[i], look_for) == 0){
            return 1;
        }
    }
    return 0;

}

char* check_param(int argc, char **argv, char* look_for){
    for(int i = 0; i < argc; i++){
        if(strcmp(argv[i], look_for) == 0){
            if(i + 1 >= argc){
                print_warning(stderr, "No parameter for '%s', returning \"\"\n", look_for);
                return "";
            }
            return argv[i + 1];
        }
    }
    return NULL;

}

void parse_argv(int argc, char **argv){
    //Check for flags
    debug = debug || check_flag(argc, argv, "--d") || check_flag(argc, argv, "--debug");
    warnings = warnings || check_flag(argc, argv, "--w") || check_flag(argc, argv, "--warnings");

    //Check for params
    char* param;
    if((param = check_param(argc, argv, "-l")) || (param = check_param(argc, argv, "-logfile"))) log_file = param;
    if((param = check_param(argc, argv, "-i")) || (param = check_param(argc, argv, "-ip"))) ip_addr = param;
    if((param = check_param(argc, argv, "-p")) || (param = check_param(argc, argv, "-port"))){
        port_num = atoi(param);
        if(port_num <= 0){
            print_error("%d is not a valid port number", port_num);
            exit(1);
        }
    }
}

FILE* setup_debug_output(){
    FILE* debug_file = NULL;

    if(debug){
        printf("Debugging enabled\n");
        if(!strcmp(log_file, "stdout") || !strcmp(log_file, "")){
            debug_file = stdout;
        }
        else if(!strcmp(log_file, "stderr")){
            debug_file = stderr;
        }
        else{
            debug_file = fopen(log_file, "w");
            if(debug_file == NULL){
                print_error("Failed to open log file '%s'", log_file);
                exit(1);
            }
        }

        setbuf(debug_file, NULL);
    }

    return debug_file;
}

FILE* setup_warning_output(){
    FILE *warning_file = NULL;

    if(warnings){
        if(debug) warning_file = stderr;
        else{
            if(!strcmp(log_file, "stderr") || !strcmp(log_file, "")){
                warning_file = stderr;
            }
            else if(!strcmp(log_file, "stdin")){
                warning_file = stdin;
            }
            else{
                printf("Opening\n");

                warning_file = fopen(log_file, "w");
                if(warning_file == NULL){
                    print_error("Failed to open log file '%s'", log_file);
                    exit(1);
                }
            }
        }

        setbuf(warning_file, NULL);
    }

    return warning_file;
}

int main(int argc, char **argv){
    printf("Starting server...\n");
    parse_argv(argc, argv);

    FILE *debug_file = setup_debug_output();
    FILE *warning_file = setup_warning_output();


    print_debug(debug_file, "Starting server with address 'http://%s:%d'...", ip_addr, port_num);
    print_warning(warning_file, "Starting server with address 'http://%s:%d'...", ip_addr, port_num);

    //Initialize WinSock
    WSADATA wsaData;

    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
        print_error("WSAStartup() failed");
        return 1;
    }
    
    print_debug(debug_file, "WSAStartup() success");

    //Create socket
    SOCKET server_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket_desc == INVALID_SOCKET){
        print_error("socket() failed");
        if(WSACleanup()){
            print_error("WSACleanup() also failed");
        }
        return 1;
    }
    print_debug(debug_file, "Socket created");

    //Bind socket
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
        return 1;
    }
    print_debug(debug_file, "Socket bound");
    
    SOCKET client_socket_desc;
    while(1){
        print_debug(debug_file, "Listening for connection...");

        //Listen for incoming connections
        if(listen(server_socket_desc, 1) == SOCKET_ERROR){
            print_error("listen() failed");
            if(closesocket(server_socket_desc)){
                print_error("closesocket() also failed.");
            }
            if(WSACleanup()){
                print_error("WSACleanup() also failed");
            }
            return 1;
        }
        print_debug(debug_file, "Connecting...");

        //Accept socket
        SOCKADDR_IN client_addr;
        int szClntAddr = sizeof(client_addr);
        client_socket_desc = accept(server_socket_desc, NULL, NULL);
        if(client_socket_desc == INVALID_SOCKET){
            print_error("accept() error");
            if(closesocket(server_socket_desc)){
                print_error("closesocket() also failed");
            }
            if(WSACleanup()){
                print_error("WSACleanup() also failed");
            }
            return 1;    
        }
        print_debug(debug_file, "Connection accepted");
        
        //Get client's response
        char client_response[1024] = "";
        print_debug(debug_file, "Getting client response after connecting...");

        int rval = recv(client_socket_desc, client_response, sizeof(client_response), 0);
        if(rval > 0){
            print_debug(debug_file, "Client response after connecting: %s\n", client_response);
        }
        print_debug(debug_file, "rcv() returned %d", rval);


        //Send web page with header to client 
        const char *webpageContent = "<html><body><h1>Welcome to the Server!</h1></body></html>";
        const char *responseHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: 57\r\n\r\n";

        print_debug(debug_file, "Sending header:\n%s\n", responseHeader);
        int bytes_sent = send(client_socket_desc, responseHeader, strlen(responseHeader), 0);
        print_debug(debug_file, "%d/%d bytes sent", bytes_sent, strlen(responseHeader));

        // //Get client's response
        // memset(client_response, 0, sizeof(client_response));
        // if(recv(client_socket_desc, client_response, sizeof(client_response), 0) > 0){
        //     print_debug(debug_file, "Client response after sending header: %s\n", client_response);
        // }

        print_debug(debug_file, "Sending webpage:\n%s\n", webpageContent);
        bytes_sent = send(client_socket_desc, webpageContent, strlen(webpageContent), 0);
        print_debug(debug_file, "%d/%d bytes sent", bytes_sent, strlen(webpageContent));

        //Get client's response
        client_response[1024];
        memset(client_response, 0, sizeof(client_response));
        print_debug(debug_file, "Getting client response after connecting...");
        if(recv(client_socket_desc, client_response, sizeof(client_response), 0) > 0){
            print_debug(debug_file, "Client response: %s\n", client_response);
        }
        print_debug(debug_file, "rcv() returned %d", rval);

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
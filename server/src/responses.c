#include <stdio.h>
#include <WinSock2.h>

#include "responses.h"
#include "Logger.h"

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
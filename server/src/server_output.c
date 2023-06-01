#include "server_output.h"
#include <stdarg.h>
#include <WinSock2.h>

extern int debug;
extern int warnings;

void print_debug(FILE* debug_file, char* formatted_message, ...){
    if(!debug) return;

    va_list args;
    va_start(args, formatted_message);

    fprintf(debug_file, "DEBUG: ");
    vfprintf(debug_file, formatted_message, args);
    fprintf(debug_file, "\n");

    va_end(args);
}

void print_warning(FILE* warning_file, char* formatted_message, ...){
    if(!warnings) return;

    va_list args;
    va_start(args, formatted_message);

    fprintf(warning_file, "WARNING: ");
    vfprintf(warning_file, formatted_message, args);
    fprintf(warning_file, "\n");

    va_end(args);
}

void print_error(char* formatted_message, ...){
    va_list args;
    va_start(args, formatted_message);

    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, formatted_message, args);
    if(WSAGetLastError() != 0){
        fprintf(stderr, ". WSA ERROR CODE: %d", WSAGetLastError());
        WSASetLastError(0);
    }
    fprintf(stderr, "\n");

    va_end(args);
}

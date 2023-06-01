#include <stdarg.h>
#include <WinSock2.h>

#include "Logger.h"
#include "CommandLineArgs.h"

static FILE *log;
static int log_level;

void setup_logger(int argc, char **argv){
    //Set log level
    log_level = DEFAULT_LOG_LEVEL;
    if(check_flag(argc, argv, "--w") || check_flag(argc, argv, "--warnings")) log_level = WARNING_LOG_LEVEL;
    if(check_flag(argc, argv, "--d") || check_flag(argc, argv, "--debug")) log_level = DEBUG_LOG_LEVEL;

    //Set log filepath
    char* log_filepath = check_param(argc, argv, "-l");
    if(log_filepath == NULL) log_filepath = check_param(argc, argv, "-logfile");
    if(log_filepath == NULL) log_filepath = DEFAULT_LOG_FILE;

    if(!strcmp(log_filepath, "stdout") || !strcmp(log_filepath, "")){
        log = stdout;
    }
    else if(!strcmp(log_filepath, "stderr")){
        log = stderr;
    }
    else{
        log = fopen(log_filepath, "w");
        if(log == NULL){
            print_error("Failed to open log file '%s'", log_filepath);
            exit(1);
        }
    }

}

void print_debug(char* formatted_message, ...){
    if(log_level < DEBUG_LOG_LEVEL) return;

    va_list args;
    va_start(args, formatted_message);

    fprintf(log, "DEBUG: ");
    vfprintf(log, formatted_message, args);
    fprintf(log, "\n");

    va_end(args);

    fflush(log);
}

void print_warning(char* formatted_message, ...){
    if(log_level < WARNING_LOG_LEVEL) return;

    va_list args;
    va_start(args, formatted_message);

    //Print to stderr
    fprintf(stderr, "WARNING: ");
    vfprintf(stderr, formatted_message, args);
    fprintf(stderr, "\n");

    //Print to log file
    if(log != stderr && log != stdout){
        fprintf(log, "WARNING: ");
        vfprintf(log, formatted_message, args);
        fprintf(log, "\n");
    }
    va_end(args);

    fflush(log);
}

void print_error(char* formatted_message, ...){
    va_list args;
    va_start(args, formatted_message);

    //Print to stderr
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, formatted_message, args);
    if(WSAGetLastError() != 0){
        fprintf(stderr, ". WSA ERROR CODE: %d", WSAGetLastError());
        WSASetLastError(0);
    }
    fprintf(stderr, "\n");

    //Print to log file
    if(log != stderr && log != stdout){
        fprintf(log, "ERROR: ");
        vfprintf(log, formatted_message, args);
        if(WSAGetLastError() != 0){
            fprintf(log, ". WSA ERROR CODE: %d", WSAGetLastError());
            WSASetLastError(0);
        }
        fprintf(log, "\n");
    }

    va_end(args);

    fflush(log);
}

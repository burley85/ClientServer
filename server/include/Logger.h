#ifndef SERVER_OUTPUT_H_
#define SERVER_OUTPUT_H_

#include <stdio.h>

#define DEFAULT_LOG_LEVEL 0
#define WARNING_LOG_LEVEL 1
#define DEBUG_LOG_LEVEL 2
#define DEFAULT_LOG_FILE "log.txt"

void setup_logger(int argc, char **argv);

void print_debug(char* formatted_message, ...);

void print_warning(char* formatted_message, ...);

void print_error(char* formatted_message, ...);



#endif
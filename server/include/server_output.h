#ifndef SERVER_OUTPUT_H_
#define SERVER_OUTPUT_H_

#include <stdio.h>

void print_debug(FILE* debug_file, char* formatted_message, ...);

void print_warning(FILE* warning_file, char* formatted_message, ...);

void print_error(char* formatted_message, ...);



#endif
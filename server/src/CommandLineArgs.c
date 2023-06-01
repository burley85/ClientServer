#include <stdio.h>

#include "CommandLineArgs.h"

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
                print_warning("No parameter for '%s', returning \"\"\n", look_for);
                return "";
            }
            return argv[i + 1];
        }
    }
    return NULL;
}

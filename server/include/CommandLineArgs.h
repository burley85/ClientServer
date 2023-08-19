#ifndef COMMANDLINEARGS_H
#define COMMANDLINEARGS_H

#include <string.h>

#include "Logger.h"


int check_flag(int argc, char **argv, char* look_for);

char* check_param(int argc, char **argv, char* look_for);

#endif
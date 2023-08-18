#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "db_structs.h"
#include "Logger.h"

//Searches a json string for a key and returns the string value
//If value is NULL, it will be malloced and returned
//If value is not NULL, it will be filled with the value
char* searchJsonForStr(char* json, char* key, char *value){
    char* look_for = malloc(strlen(key) + 5);
    sprintf(look_for, "'%s': ", key);

    print_debug("Looking for %s in %s", look_for, json);

    char* start = strstr(json, look_for) + strlen(look_for);
    char quote = *start; //Can be ' or "
    start++;

    //Look for the next non escaped quote
    char* end = start;
    while((*end != quote && *end != '\0') || *(end - 1) == '\\'){
        end++;
    }

    if(value == NULL) value = malloc(end - start + 1);
    strncpy(value, start, end - start);
    value[end - start] = '\0';

    print_debug("Found \"%s\": \"%s\"", key, value);


    free(look_for);
    return value;
}

//Searches a json string for a key and returns the integer value
//If value is NULL, it will be malloced and returned
//If value is not NULL, it will be filled with the value
int searchJsonForInt(char* json, char* key){
    char* look_for = malloc(strlen(key) + 5);
    sprintf(look_for, "'%s': ", key);

    print_debug("Looking for %s in %s", look_for, json);

    char* start = strstr(json, look_for) + strlen(look_for);
    return strtol(start, NULL, 10);
}

//Converts a string to a dynamically allocated database object
//Returns NULL if the string is "None"
void* strToDatabaseObject(char* str){
    if(strncmp(str, "None", 4) == 0) return NULL;

    User* obj = malloc(sizeof(User));
    *obj = strToUser(str);
    printf(userToStr(*obj));

    return obj;
}

User strToUser(char *str){
    User user;

    print_debug("Converting %s to user", str);

    searchJsonForStr(str, "username", user.username);
    searchJsonForStr(str, "pword", user.pword);
    searchJsonForStr(str, "email", user.email);
    searchJsonForStr(str, "fname", user.fname);
    searchJsonForStr(str, "lname", user.lname);
    user.id = searchJsonForInt(str, "id");

    return user;
}

char* userToStr(User user){
    char* str = malloc(512);
    sprintf(str, "{\"username\": \"%s\", \"pword\": \"%s\", \"email\": \"%s\", \"fname\": \"%s\", \"lname\": \"%s\", \"id\": \"%d\"}", user.username, user.pword, user.email, user.fname, user.lname, user.id);
    return str;
}

Channel strToChannel(char *str){
    Channel channel;


    return channel;
}

Membership strToMembership(char *str){
    Membership membership;
    

    return membership;
}

Message strToMessage(char *str){
    Message message;


    return message;
}
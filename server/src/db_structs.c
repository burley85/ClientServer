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
    sprintf(str,
            "{\"username\": \"%s\", \"pword\": \"%s\", "
            "\"email\": \"%s\", \"fname\": \"%s\", "
            "\"lname\": \"%s\", \"id\": \"%d\"}",
            user.username, user.pword,
            user.email, user.fname,
            user.lname, user.id);
    return str;
}

Channel strToChannel(char *str){
    Channel channel;

    print_debug("Converting %s to channel", str);

    searchJsonForStr(str, "channel_name", channel.channel_name);
    channel.id = searchJsonForInt(str, "id");

    return channel;
}

char* channelToStr(Channel channel){
    char* str = malloc(512);
    sprintf(str,
            "{\"channel_name\": \"%s\", \"id\": \"%d\"}",
            channel.channel_name, channel.id);
    return str;
}

Membership strToMembership(char *str){
    Membership membership;
    
    print_debug("Converting %s to membership", str);
    
    membership.channel_id = searchJsonForInt(str, "channel_id");
    membership.user_id = searchJsonForInt(str, "user_id");
    membership.perm_flags = searchJsonForInt(str, "perm_flags");

    return membership;
}

char* membershipToStr(Membership membership){
    char* str = malloc(512);
    sprintf(str,
            "{\"channel_id\": \"%d\", \"user_id\": \"%d\", "
            "\"perm_flags\": \"%d\"}",
            membership.channel_id, membership.user_id,
            membership.perm_flags);
    return str;
}

Message strToMessage(char *str){
    Message message;


    return message;
}

char* messageToStr(Message message){
    return NULL;
}

Invitation strToInvitation(char *str){
    Invitation invitation;

    print_debug("Converting %s to invitation", str);

    invitation.sender_id = searchJsonForInt(str, "sender_id");
    invitation.receiver_id = searchJsonForInt(str, "receiver_id");
    invitation.channel_id = searchJsonForInt(str, "channel_id");
    
    return invitation;
}

char* invitationToStr(Invitation invitation){
    char* str = malloc(512);
    sprintf(str,
            "{\"sender_id\": \"%d\", \"receiver_id\": \"%d\", "
            "\"channel_id\": \"%d\", }",
            invitation.sender_id, invitation.receiver_id,
            invitation.channel_id);
    return str;
}

//Converts a string to a dynamically allocated database object
//Returns NULL if the string is "None"
void* strToDatabaseObject(char* str){
    if(strncmp(str, "None", 4) == 0) return NULL;

    if(strstr(str, "User = ") == str){
        str += 7;
        User *user = malloc(sizeof(User));
        *user = strToUser(str);
        char *userStr = userToStr(*user);
        print_debug("Converted %s to user: %s", str, userStr);
        free(userStr);
        return user;
    }

    if(strstr(str, "Channel = ") == str){
        str += 10;
        Channel *channel = malloc(sizeof(Channel));
        *channel = strToChannel(str);
        char* channelStr = channelToStr(*channel);
        print_debug("Converted %s to channel: %s", str, channelStr);
        free(channelStr);
        return channel;
    }

    if(strstr(str, "Membership = ") == str){
        str += 13;
        Membership *membership = malloc(sizeof(Membership));
        *membership = strToMembership(str);
        char* membershipStr = membershipToStr(*membership);
        print_debug("Converted %s to membership: %s", str, membershipStr);
        free(membershipStr);
        return membership;
    }

    if(strstr(str, "Message = ") == str){
        str += 10;
        Message *message = malloc(sizeof(Message));
        *message = strToMessage(str);
        char* messageStr = messageToStr(*message);
        print_debug("Converted %s to message: %s", str, messageStr);
        free(messageStr);
        return message;
    }

    print_warning("Could not convert %s to database object", str);
    return NULL;
}

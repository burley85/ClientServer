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

    char* start = strstr(json, look_for);
    if(start == NULL){
        sprintf(look_for, "\"%s\": ", key);
        start = strstr(json, look_for);
        if(start == NULL){
            print_warning("Could not find %s in %s", look_for, json);
            free(look_for);
            return NULL;
        }
    }
    start += strlen(look_for);
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
int searchJsonForInt(char* json, char* key){
    char* look_for = malloc(strlen(key) + 5);
    sprintf(look_for, "'%s': ", key);

    print_debug("Looking for %s in %s", look_for, json);

    char* start = strstr(json, look_for);
    if(start == NULL){
        sprintf(look_for, "\"%s\": ", key);
        start = strstr(json, look_for);
        if(start == NULL){
            print_warning("Could not find %s in %s", look_for, json);
            free(look_for);
            return -1;
        }
    }
    start += strlen(look_for);

    return strtol(start, NULL, 10);
}

User* strToUser(char *str){
    User* user = malloc(sizeof(User));
    print_debug("Converting %s to user", str);

    user->id = searchJsonForInt(str, "id");

    if(searchJsonForStr(str, "username", user->username) == NULL ||
            searchJsonForStr(str, "email", user->email) == NULL ||
            searchJsonForStr(str, "fname", user->fname) == NULL ||
            searchJsonForStr(str, "lname", user->lname) == NULL ||
            user->id == -1){
        print_warning("Could not convert %s to user", str);
        free(user);
        return NULL;            
    }

    char* userStr = userToStr(*user);
    print_debug("Converted %s to user: %s", str, userStr);
    free(userStr);

    return user;
}

char* userToStr(User user){
    char* str = malloc(512);
    sprintf(str,
            "{\"username\": \"%s\", \"email\": \"%s\", "
            "\"fname\": \"%s\", \"lname\": \"%s\", "
            "\"id\": \"%d\"}",
            user.username, user.email,
            user.fname, user.lname,
            user.id);
    return str;
}

Channel* strToChannel(char *str){
    Channel* channel = malloc(sizeof(Channel));
    print_debug("Converting %s to channel", str);

    channel->id = searchJsonForInt(str, "id");

    if(searchJsonForStr(str, "channel_name", channel->channel_name) == NULL ||
            channel->id == -1){
        print_warning("Could not convert %s to channel", str);
        free(channel);
        return NULL;
    }

    char* channelStr = channelToStr(*channel);
    print_debug("Converted %s to channel: %s", str, channelStr);
    free(channelStr);

    return channel;
}

char* channelToStr(Channel channel){
    char* str = malloc(512);
    sprintf(str,
            "{\"channel_name\": \"%s\", \"id\": \"%d\"}",
            channel.channel_name, channel.id);
    return str;
}

Membership* strToMembership(char *str){
    Membership* membership = malloc(sizeof(Membership));
    print_debug("Converting %s to membership", str);
    
    membership->channel_id = searchJsonForInt(str, "channel_id");
    membership->user_id = searchJsonForInt(str, "user_id");
    membership->perm_flags = searchJsonForInt(str, "perm_flags");

    if(membership->channel_id == -1 || membership->user_id == -1 ||
            membership->perm_flags == -1){
        print_warning("Could not convert %s to membership", str);
        free(membership);
        return NULL;
    }

    char* membershipStr = membershipToStr(*membership);
    print_debug("Converted %s to membership: %s", str, membershipStr);
    free(membershipStr);

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

Message* strToMessage(char *str){
    Message* message = NULL;


    return message;
}

char* messageToStr(Message message){
    return NULL;
}

Invitation* strToInvitation(char *str){
    Invitation* invitation = malloc(sizeof(Invitation));
    print_debug("Converting %s to invitation", str);

    invitation->id = searchJsonForInt(str, "id");
    invitation->sender_id = searchJsonForInt(str, "sender_id");
    invitation->receiver_id = searchJsonForInt(str, "receiver_id");
    invitation->channel_id = searchJsonForInt(str, "channel_id");
    
    if(invitation->id == -1 || invitation->sender_id == -1 ||
            invitation->receiver_id == -1 || invitation->channel_id == -1){
        print_warning("Could not convert %s to invitation", str);
        free(invitation);
        return NULL;
    }

    char* invitationStr = invitationToStr(*invitation);
    print_debug("Converted %s to invitation: %s", str, invitationStr);
    free(invitationStr);

    return invitation;
}

char* invitationToStr(Invitation invitation){
    char* str = malloc(512);
    sprintf(str,
            "{\"id\": \"%d\", \"sender_id\": \"%d\", "
            "\"receiver_id\": \"%d\", \"channel_id\": \"%d\"}",
            invitation.sender_id, invitation.receiver_id,
            invitation.channel_id);
    return str;
}

//Converts a string to a dynamically allocated database object
//Returns NULL if the string is "None"
void* strToDatabaseObject(char* str){
    if(strncmp(str, "None", 4) == 0) return NULL;

    char* start = "User =";
    if(strstr(str, start) == str) return strToUser(str + strlen(start));

    start = "Channel =";
    if(strstr(str, start) == str) return strToChannel(str + strlen(start)); 

    start = "Membership =";
    if(strstr(str, start) == str) return strToMembership(str + strlen(start));

    start = "Message =";
    if(strstr(str, start) == str) return strToMessage(str + strlen(start));

    start = "Invitation =";
    if(strstr(str, start) == str) return strToInvitation(str + strlen(start));

    print_warning("Could not convert %s to database object", str);

    return NULL;
}

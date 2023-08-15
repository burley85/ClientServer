typedef struct {
    int id;
    char username[32];
    char pword[33];
    char email[64];
    char fname[32];
    char lname[32];
} User;

typedef struct {
    int id;
    char channel_name[32];
} Channel;

typedef struct {
    int channel_id;
    int user_id;
    char perm_flags;
} Membership;

typedef struct {
    int id;
    char message_time[20];
    int sender_id;
    char message_text[256];
    void* DirectOrGroupMessage;
} Message;

typedef struct {
    Message* message;
    int receiver_id;
} DirectMessage;

typedef struct {
    Message* message;
    int channel_id;
} GroupMessage;

void* strToDatabaseObject(char* str);

char* userToStr(User user);

static User strToUser(char *str);
static Channel strToChannel(char *str);
static Membership strToMembership(char *str);
static Message strToMessage(char *str);
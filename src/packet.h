#pragma once

#define NAME_MAX 16
#define MSG_MAX 256
#define TIME_MAX 9
#define CPKTSIZE sizeof(c_pkt)

enum class action:int{
    CON = 1,
    MSG = 2,
    EXT = 3,
};

typedef struct client_packet{
    action type;
    char uname [NAME_MAX];
    char time  [TIME_MAX];
    char msg   [MSG_MAX];
} c_pkt;

void serialize(c_pkt* msgPacket, char *data);
void deserialize(char *data, c_pkt* msgPacket);

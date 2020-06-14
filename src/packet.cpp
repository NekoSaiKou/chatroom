#include "packet.h"

/**
 * Serialize a message struct in order to send to server
 * @param msgPacket - c_pkt pointer, the message sturct from client to server
 * @param data      - char pointer , serialized data
 */
void serialize(c_pkt* msgPacket, char *data){
    action *q = (action*)data;    
    *q = msgPacket->type;
    ++q;

    char *p = (char*)q;
    // Packet username
    int i = 0;
    while (i < NAME_MAX){
        *p = msgPacket->uname[i];
        ++p;
        ++i;
    }
    // Packet time
    i = 0;
    while (i < TIME_MAX){
        *p = msgPacket->time[i];
        ++p;
        ++i;
    }
    // Packet message
    i = 0;
    while (i < MSG_MAX){
        *p = msgPacket->msg[i];
        ++p;
        ++i;
    }
}

/**
 * Deserialize a message struct
 * @param data      - char pointer     , serialized data
 * @param msgPacket - c_pkt pointer, deserialized data in c_pkt form
 */
void deserialize(char *data, c_pkt* msgPacket){
    action *q = (action*)data;
    msgPacket->type = *q;
    ++q;

    char *p = (char*)q;
    // Decode username
    int i = 0;
    while (i < NAME_MAX){
        msgPacket->uname[i] = *p;
        ++p;
        ++i;
    }
    // Decode time
    i = 0;
    while (i < TIME_MAX){
        msgPacket->time[i] = *p;
        ++p;
        ++i;
    }
    // Decode msg
    i = 0;
    while (i < MSG_MAX){
        msgPacket->msg[i] = *p;
        ++p;
        ++i;
    }
}

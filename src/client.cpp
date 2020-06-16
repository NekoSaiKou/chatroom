// C
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// c++
#include <string>
#include <thread>
using namespace std;

// Custom
#include "packet.h"

#define SERVER_PORT 8080

/**
 * Serialize the message packet and send to target socket
 * @param client - int         , the socket file descripter connect to server
 * @param type   - enum action , what is this packet used for
 * @param msg    - string      , the message to send
 * @param name   - string      , the user name
 */
void send_msg(int client, action type, string msg, string name){
    c_pkt *msg_packet = new c_pkt;
    msg_packet->type = type;
    strncpy(msg_packet->uname, name.c_str(), NAME_MAX);
    msg_packet->uname[sizeof(msg_packet->uname) - 1] = 0;
    strncpy(msg_packet->time, "", TIME_MAX);
    msg_packet->time[sizeof(msg_packet->time) - 1] = 0;
    strncpy(msg_packet->msg, msg.c_str(), MSG_MAX);
    msg_packet->msg[sizeof(msg_packet->msg) - 1] = 0;

    char data[CPKTSIZE];
    serialize(msg_packet, data);
    int send_num = send(client, data, CPKTSIZE, 0);
}

/**
 * Receive message in background
 * @param client - int, the socket file descripter connect to server
 * @note  SOCK_STREAM are full-duplex byte streams --from socket manpage
 */
void recv_msg(int client){
    while (true){
        char buf[1024] = {0}; // Server recv buffer
        int n_buf;            // Number of received bytes
        if ((n_buf = recv(client, buf, sizeof(buf), 0)) <= 0){
            if (n_buf == 0){
                fprintf(stderr, "[chatroom] Connection lost\n");
            }
            else{
                perror("[chatroom] receive error\n");
            }
            break;
        }
        else{
            c_pkt *msg_packet = new c_pkt;
            deserialize(buf, msg_packet);
            switch (msg_packet->type){
            case action::CON:
                printf("%s | [chatroom] Welcome <%s>\n", msg_packet->time, msg_packet->uname);
                break;
            case action::MSG:
                printf("%s | [%s] %s\n", msg_packet->time, msg_packet->uname, msg_packet->msg);
                break;
            case action::EXT:
                printf("%s | [chatroom] Goodbye, %s\n", msg_packet->time, msg_packet->uname);
                break;
            default:
                fprintf(stderr, "[chatroom] Message decode error\n");
                break;
            }
        }
    }
}

/**
 * Get user input
 * @param buf    - char*, the char buffer for store user input
 */
int user_input(char *buf){
    int ch;
    int i = 0;
    while ((ch = getchar()) != '\n'){
        buf[i] = ch;
        i++;
    }
    buf[i] = '\0';
    return i;
}

int main(int argc, char *argv[]){
    /** 
     * Check server arguments
     * argv[1] --> IPaddress to server
     * argv[2] --> The name of client
    */
    string host;
    string username;
    if (argc != 3){
        fprintf(stderr, "[chatroom] Error: Usage: %s Server-IP Username\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    else{
        struct sockaddr_in sa;
        int result;
        if ((result = inet_pton(AF_INET, argv[1], &(sa.sin_addr))) <= 0){
            fprintf(stderr, "[chatroom] Target server address invalid format\n");
            exit(EXIT_FAILURE);
        }
        else if (strlen(argv[2]) > NAME_MAX){
            fprintf(stderr, "[chatroom] Username should less then %d character\n", NAME_MAX);
            exit(EXIT_FAILURE);
        }
        else{
            printf("[chatroom] Target host address %s, user name is %s\n", argv[1], argv[2]);
            host = argv[1];
            username = argv[2];
        }
    }

    // Start connection

    // 1. Initialze the server socket info
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;                               // IPv4
    serverAddr.sin_port = htons(SERVER_PORT);                      // Port
    serverAddr.sin_addr.s_addr = inet_addr(host.c_str());          // Address
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero); // Clear data

    // 2. Connect to the server
    int clientSocket;
    socklen_t addr_size = sizeof serverAddr;
    if ((clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("[chatroom] Socket creation failed");
        exit(EXIT_FAILURE);
    }
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, addr_size) < 0){
        perror("[chatroom] Host connection failed");
        exit(EXIT_FAILURE);
    }

    // 3. Send name to server
    send_msg(clientSocket, action::CON, "", username);

    // Start a background thread to listen message from server
    thread listener;
    listener = thread(recv_msg, clientSocket);

    // Start to capture user input
    while (true){
        char input_buf[1024] = {0}; // Input buffer
        int length = user_input(input_buf);
        send_msg(clientSocket, action::MSG, input_buf, username);
    }

    listener.join();

    close(clientSocket);

    return 0;
}

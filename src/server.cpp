// C
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
// c++
#include <chrono>
#include <iomanip>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
using namespace std;

// Custom
#include "packet.h"

#define PORT 8080

// lock client socket file descripter list and maximum fd
mutex client_Mutex; 	// Protect com_conn and max_fd
mutex msg_queue_Mutex;	// Protect shared message queue

/**
 * Get current datetime string
 * @retval current time - string, current datetime string
 */ 
string get_datetime(){
    auto now = chrono::system_clock::now();
    auto in_time_t = chrono::system_clock::to_time_t(now);

    stringstream ss;
    ss << put_time(localtime(&in_time_t), "%H:%M:%S");
    return ss.str();
}

/**
 * Receive message from each client in background
 * @param com_conn  - pointer of fd_set      , a pointer point to a socket fd_set which contains all client sockets.
 * @param max_fd    - pointer of int         , a pointer point to a int which indicates maximum fd of socket.
 * @param client    - int                    , the socket file descripter come from client
 * @param msg_queue - pointer of packet queue, a pointer point to where the packets are queued
 * @note Each client has one corresponding thread executing this listener
 * @note This thread is detach from the main thread, the thread will close itself if server is closed.
 */
void client_listener(fd_set *com_conn, int *max_fd, int client, queue<c_pkt> *msg_queue){
	signal(SIGPIPE, SIG_IGN);
	char uname [NAME_MAX] = {0};
	while(true){
		char buf[1024] = {0};	// Server recv buffer
		int  n_buf;				// Numver of received bytes
		c_pkt *msg_packet = new c_pkt;
		if ((n_buf=recv(client, buf, sizeof(buf), 0)) <= 0 ){
			if (n_buf == 0){
				fprintf(stderr, "[server] Connection lost\n");
			}
			else{
				perror("[server] receive error");
			}
			// Remove client and update broadcast list and max_fd
			client_Mutex.lock();
			FD_CLR(client, com_conn);
			if (client == *max_fd){
				*max_fd = *max_fd - 1;
			}
			client_Mutex.unlock();
			close(client);

			// Set msg packet with leaving message
			msg_packet->type = action::EXT;
			strncpy(msg_packet->uname, uname, NAME_MAX);
			msg_packet->uname[sizeof(msg_packet->uname) - 1] = 0;
			strncpy(msg_packet->msg, "", MSG_MAX);
			msg_packet->msg[sizeof(msg_packet->msg) - 1] = 0;

			// Push to broadcast waiting queue
			msg_queue_Mutex.lock();
			msg_queue->push(*msg_packet);
			msg_queue_Mutex.unlock();
			break;
		}
		else{
			// Set msg packet with received message
			deserialize(buf, msg_packet);

			// Regisetr username to this thread
			if (msg_packet->type == action::CON){
				strncpy(uname, msg_packet->uname, NAME_MAX);
			}
			// Push to broadcast waiting queue
			msg_queue_Mutex.lock();
			msg_queue->push(*msg_packet);
			msg_queue_Mutex.unlock();
		}
    }
}

/**
 * Broadcast new message to all client in background.
 * @param com_conn  - pointer of fd_set      , a pointer point to a socket fd_set which contains all client sockets.
 * @param max_fd    - pointer of int         , a pointer point to a int which indicates maximum fd of socket.
 * @param msg_queue - pointer of packet queue, a pointer point to where the packets are queued
 */
void bcast(fd_set *com_conn, int *max_fd, queue<c_pkt> *msg_queue){
	int max_target;
	fd_set client_set;
	FD_ZERO(&client_set);
	while(true){
		// Update broadcast list and max_fd
		client_Mutex.lock();
		client_set = *com_conn;
		max_target = *max_fd;
		client_Mutex.unlock();

		// Lock and check if queue full
		c_pkt tmp_packet;
		msg_queue_Mutex.lock();
		if (msg_queue->size()>0){
			tmp_packet = msg_queue->front();
			msg_queue->pop();
			msg_queue_Mutex.unlock();

			// Add timestamp and print server message
			c_pkt *msg_packet = &tmp_packet;
			string current_time = get_datetime();
			strncpy(msg_packet->time, current_time.c_str(), TIME_MAX);
			msg_packet->time[sizeof(msg_packet->time) - 1] = 0;
			switch(msg_packet->type){
                case action::CON:
					printf("[server] <%s> join the chatroom \n", msg_packet->uname);
                    break;
                case action::MSG:
					printf("[server] %s > %s\n", msg_packet->uname, msg_packet->msg);
                    break;
                case action::EXT:
                    printf("[server] <%s> left.\n", msg_packet->uname);
                    break;
                default:
                    fprintf(stderr, "[server] Message decode error\n");
                    break;
            }

			// Broadcast to all connected clients
			char data[CPKTSIZE];
    		serialize(msg_packet, data);
			for(int i=0; i<= max_target; i++){
				if(FD_ISSET(i, &client_set)){
					int ret = send(i, data, CPKTSIZE, 0);
					if (ret == -1 && errno == EPIPE){
						perror("send");
					}
				}
			}
		}
		else{
			msg_queue_Mutex.unlock();
		}
	}
}

int main() {
	// Create socket for server
	int server_socket;		// Server socket
	if ((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) { 
		perror("[server] socket create failed"); 
		exit(EXIT_FAILURE); 
	}
	// Prevent port already in use
	int opt = 1; 
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) { 
		perror("[server] Set sockopt failed"); 
		exit(EXIT_FAILURE);
	}
	// Bind server
	struct sockaddr_in serverAddr;
	socklen_t addr_size = sizeof serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(PORT);
	if (bind(server_socket, (struct sockaddr *)&serverAddr, addr_size)<0) {
		perror("[server] Bind failed");
		exit(EXIT_FAILURE);
	}
	// listen
	if (listen(server_socket, 10) == -1) {
		perror("[server] Listen");
		exit(EXIT_FAILURE);
	}
	// Add server into master select list
	fd_set master, read_fds, com_conn;
	FD_ZERO(&master);		// For this main thread, cannot be modified by select()
	FD_ZERO(&read_fds);		// For this main thread, checking new connection
	FD_ZERO(&com_conn);		// For client handler and broadcaster, new connection will be added by main thread
	FD_SET(server_socket, &master);
	int max_fd = server_socket;

	// Message queue
	queue<c_pkt> msg_queue;

	// Client struct
	struct sockaddr_in clientAddr; 				// client address, either ipv4 or ipv6
	socklen_t clientlen = sizeof clientAddr;	// Client address length

	// Start broadcast service
	thread bcaster;
	bcaster = thread(bcast, &com_conn, &max_fd, &msg_queue);

	// Main loop
	while(true){
		read_fds = master;
		struct timeval tv = {0,50000};

		// Check readable
		if (select(server_socket+1, &read_fds, NULL, NULL, &tv) == -1){
			perror("[server] Select reader error");
			exit(EXIT_FAILURE);
		}
		if (FD_ISSET(server_socket, &read_fds)){
			// Server is readable means new connection
			int new_client = 0;								// file descripter for new client
			new_client = accept(server_socket, (struct sockaddr *)&clientAddr, &clientlen);
			if (new_client < 0){
				perror("[server] Accept new connection failed");
			}
			else{
				thread listener;
				listener = thread(client_listener, &com_conn, &max_fd, new_client, &msg_queue);
				listener.detach();

				// Modify broadcast list and max_fd
				client_Mutex.lock();
				FD_SET(new_client, &com_conn);
				if (new_client > max_fd){
					max_fd = new_client;
				}
				client_Mutex.unlock();
			}
		}
	}
} 
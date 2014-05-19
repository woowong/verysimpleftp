#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define USER_MAX 10
#define COMMAND_SIZE 16

int connectSocket(int port);
void* server_thread(void *args);

int main (int argc, char *argv[])
{
	int serv_sock;
	int sock;

	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;
	pthread_t thread;

	if (argc < 2) {
		printf("Usage : %s <Port> \n", argv[0]);
		return 0;
	}
	else {
		
		serv_sock = connectSocket(atoi(argv[1]));
		
		for(;;)
		{
			clnt_addr_size = sizeof(clnt_addr);
			printf("before accept\n");
			sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
			printf("after accept\n");
			if(sock==-1) {
				printf("accept() error\n");
				exit(-1);
			}
			
			// register new sock num to clnt_sock,
			pthread_create(&thread, NULL, server_thread, (void*)sock);
		}
	}
}

void* server_thread (void *args)
{
	int clnt_sock = (int) args;
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char cmd[COMMAND_SIZE];
	char arg[BUFFER_SIZE];
	
	int is_logged = 0;

	// init connection
	sprintf(sendBuffer, "220 Service ready for new user. - woowong\n");
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
	
	for(;;)
	{
		recv(clnt_sock, readBuffer, sizeof(readBuffer)-1, 0);
		sscanf(readBuffer, "%s %s", cmd, arg);
		// USER recv
		if(!is_logged) {
			if(!strcmp(cmd, "USER")) 
				is_logged = ftp_user(clnt_sock, arg);
		}
		else {
		}
	}
}

// USER, PASS, return 1 is login success. 
int ftp_user(int clnt_sock, char *arg)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char cmd[COMMAND_SIZE];
	if(!strcmp(arg, "anonymous")) {
		sprintf(sendBuffer, "331 User name okay, need password.\n");
		send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
		// PASS recv
		recv(clnt_sock, readBuffer, sizeof(readBuffer)-1, 0);
		sscanf(readBuffer, "%s %*s", cmd);
		if(!strcmp(cmd, "PASS")) {
			// login success
			sprintf(sendBuffer, "230 User logged in, proceed.\n");
			send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
			return 1;
		}
		else {
			sprintf(sendBuffer, "530 Not logged in, proceed.\n");
			send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
		}
	}
	else {
		sprintf(sendBuffer, "530 Not logged in, proceed.\n");
		send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
	}
	return 0;
}
// Socket Connection
int connectSocket(int port)
{
	int sock;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printf ("socket() error\n");
		exit(-1);
	}
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(port);

	if(bind(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) {
		printf ("bind() error\n"); 
		exit(-1);
	}
	if (listen(sock, 5)== -1) {
		printf("listen() error\n");
		exit(-1);
	}

	return sock;
}

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

int ftp_user(int clnt_sock, char *arg);
void ftp_pwd(int clnt_sock);
void ftp_cwd(int clnt_sock, char *arg);

int serv_sock;

int main (int argc, char *argv[])
{
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
			sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
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
	int d_sock;

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
			if(!strcmp(cmd, "PWD"))
				ftp_pwd(clnt_sock);
			if(!strcmp(cmd, "PASV"))		
				d_sock = ftp_pasv(clnt_sock);
			if(!strcmp(cmd, "CWD"))
				ftp_cwd(clnt_sock, arg);
			if(!strcmp(cmd, "RETR"));		
			if(!strcmp(cmd, "STOR"));		
			if(!strcmp(cmd, "LIST"));		
			if(!strcmp(cmd, "REVRETR"));		
			if(!strcmp(cmd, "REVSTOR"))	;	
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

// PASV
int ftp_pasv(int clnt_sock)
{
	struct sockaddr_in serv_addr;
	int addr_size = sizeof(struct sockaddr_in);

	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char cmd[COMMAND_SIZE];
	int pasv_port, port0, port1;
	int d_sock;
	// random generate port number
	srand(time(NULL));
	pasv_port = 1024 + rand() % (65535 - 1024);
	port0 = pasv_port / 256;
	port1 = pasv_port % 256;
	// get server ip information
	getsockname(clnt_sock, (struct sockaddr *)&serv_addr, &addr_size);
	//printf("serv_sock : %d\n", serv_sock);
	//printf("%s : IP \n", inet_ntoa(serv_addr.sin_addr));
	printf("%d.%d.%d.%d\n",
			(int) (serv_addr.sin_addr.s_addr&0xFF),
			(int)((serv_addr.sin_addr.s_addr&0xFF00)>>8),
			(int)((serv_addr.sin_addr.s_addr&0xFF0000)>>16),
			(int)((serv_addr.sin_addr.s_addr&0xFF000000)>>24));
	/*	
	// create new socket for data path
	d_sock = connectSocket(pasv_port);
	sprintf(readBuffer, "227 Entering Passive Mode (%d, %d, %d, %d, %d, %d).\n");
	return d_sock;
	 */
}

// PWD
void ftp_pwd(int clnt_sock)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	getcwd(readBuffer, sizeof(readBuffer));
	sprintf(sendBuffer, "257 \"%s\"\n", readBuffer);
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
}

// CWD
void ftp_cwd(int clnt_sock, char *arg)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	if (!chdir(arg)) {
		sprintf(sendBuffer, "250 Requested file action okay, completed.\n");
		send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
	}
	else {
		sprintf(sendBuffer, "550 Requested action not taken.\n");
		send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
	}
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

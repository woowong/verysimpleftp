#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define COMMAND_SIZE 1024
#define BUFFER_SIZE 1024

// etc.
void cmdHandler(char *cmd);
// command function
void ftp_open(char *cmd);
// socket fucntion
int connectSocket(char *ip, int port);
	
int sock; // for command path socket

int main(int argc, char* argv[])
{
	char cmd[COMMAND_SIZE]; // buffer for Command input
	int is_connected = 0; // boolean variable for connection status

	if (argc == 3) {
		for (;;)
		{
			if (is_connected) {
				printf("ftp>");
				fgets(cmd, COMMAND_SIZE, stdin);
			}
			else {
				sprintf(cmd, "open %s %s", argv[1], argv[2]);
				is_connected = 1;
			}
			cmdHandler(cmd);
		}
	}
	else {
		printf("Usage : %s <Server IP> <Port>\n", argv[0]); 
	}
	return 0;
}

void cmdHandler(char *cmd)
{
	char cmdBuffer[COMMAND_SIZE];
	char *token;
	
	strcpy(cmdBuffer, cmd);
	token = strtok (cmdBuffer, " ");

	if ( !strcmp(token, "open") )
	{
		// open
		ftp_open(cmd);
	}
	else if ( !strcmp(token, "pwd") )
	{
		// PWD
	}
	else if ( !strcmp(token, "passive") )
	{
		// PASV
	}
	else if ( !strcmp(token, "cd") )
	{
		// CWD / CDUP
	}
	else if ( !strcmp(token, "get") )
	{
		// RETR <file>
	}
	else if ( !strcmp(token, "put") )
	{
		// STOR <file>
	}
	else if ( !strcmp(token, "ls") )
	{
		// LIST 
	}
	else if ( !strcmp(token, "revget") )
	{
		// REVRETR <file>
	}
	else if ( !strcmp(token, "revput") )
	{
		// REVSTOR <file>
	}
}

// open connection, USER, PASS command
void ftp_open(char *cmd)
{
	char ip[16], port[16];
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char input[BUFFER_SIZE];

	sscanf(cmd, "%*s %s %s", ip, port);

	// command socket?
	sock = connectSocket(ip, atoi(port));
	recv(sock, readBuffer, sizeof(readBuffer)-1, 0);
	printf("%s\n", readBuffer);

	// send USER command
	printf("Name : ");
	fgets(input, BUFFER_SIZE, stdin);
	sprintf(sendBuffer, "USER %s", input);
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	recv(sock, readBuffer, sizeof(readBuffer)-1, 0);
	printf("%s\n", readBuffer);

	// send PASS command
	printf("Password : ");
	fgets(input, BUFFER_SIZE, stdin);
	sprintf(sendBuffer, "PASS %s", input);
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	recv(sock, readBuffer, sizeof(readBuffer)-1, 0);
	printf("%s\n", readBuffer);
}

// Socket Connection
int connectSocket(char *ip, int port)
{
	int sock;
	struct sockaddr_in serv_addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printf ("socket() error\n");
		exit(-1);
	}
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(ip);
	serv_addr.sin_port=htons(port);

	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1) {
		printf ("connect() error\n");
		exit(-1);
	}
	return sock;
}












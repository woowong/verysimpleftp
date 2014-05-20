#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

#define COMMAND_SIZE 1024
#define BUFFER_SIZE 1024

// etc.
void cmdHandler(char *cmd);
// command function
void ftp_bye(char *cmd);
void ftp_open(char *cmd);
void ftp_pwd(char *cmd);
void ftp_cd(char *cmd);
void ftp_cdup(char *cmd);
void ftp_passive(char *cmd);
void get_passive(char* ip, int* d_port);
void ftp_get(char *cmd);
void ftp_revget(char *cmd);
void ftp_put(char *cmd);
void ftp_revput(char *cmd);
void ftp_list(char *cmd);
// socket fucntion
int connectSocket(char *ip, int port);
void recvMsg(int sock, char *readBuffer, int size);
	
int sock; // for command path socket
int d_sock; // data path socket
int is_passive=0; // bool for passive mode
int is_off=0;

int main(int argc, char* argv[])
{
	char cmd[COMMAND_SIZE]; // buffer for Command input
	int is_connected = 0; // boolean variable for connection status
	if (argc == 3) {
		for (;is_off==0;)
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
	// remove \n char from non-arg cmd.
	if (token[strlen(token)-1] == '\n')
		token[strlen(token)-1] = '\0';
	// switching command
	if ( !strcmp(token, "open") )
		ftp_open(cmd);
	else if ( !strcmp(token, "pwd") )
		ftp_pwd(cmd);
	else if ( !strcmp(token, "passive") )
		ftp_passive(cmd);
	else if ( !strcmp(token, "cd") )
		ftp_cd(cmd);
	else if ( !strcmp(token, "cdup") )
		ftp_cdup(cmd);
	else if ( !strcmp(token, "get") )
		ftp_get(cmd);
	else if ( !strcmp(token, "put") )
		ftp_put(cmd);
	else if ( !strcmp(token, "ls") )
		ftp_list(cmd);
	else if ( !strcmp(token, "revget") )
		ftp_revget(cmd);
	else if ( !strcmp(token, "revput") )
		ftp_revput(cmd);
	else if ( !strcmp(token, "bye") )
		ftp_bye(cmd);
}

void ftp_bye(char *cmd)
{
	char sendBuffer[BUFFER_SIZE];
	sprintf(sendBuffer, "QUIT\r\n");
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	close(sock);
	is_off = 1;
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
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);

	// send USER command
	printf("Name : ");
	fgets(input, BUFFER_SIZE, stdin);
	sprintf(sendBuffer, "USER %s", input);
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);

	// send PASS command
	printf("Password : ");
	fgets(input, BUFFER_SIZE, stdin);
	sprintf(sendBuffer, "PASS %s", input);
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);
}

// PWD command
void ftp_pwd(char *cmd)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	sprintf(sendBuffer, "PWD\r\n");
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	//recvMsg(sock, readBuffer, sizeof(readBuffer));
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);
}

// PASV command
void ftp_passive(char *cmd)
{
	if (!is_passive) {
		printf("Passive mode on.\n");
		is_passive = 1;
	}
	else {
		printf("Passive mode off.\n");
		is_passive = 0;
	}
}
// Passive mode setting
void get_passive(char* ip, int* d_port)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char tempBuffer[BUFFER_SIZE];
	int pasv_port[2];
	int pasv_ip[4];
	
	sprintf(sendBuffer, "PASV\r\n");
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);
	
	strcpy(tempBuffer, strchr(readBuffer, '(')+1);
	sscanf(tempBuffer, "%d,%d,%d,%d,%d,%d", 
			&pasv_ip[0], &pasv_ip[1], &pasv_ip[2], &pasv_ip[3], 
			&pasv_port[0], &pasv_port[1]); 

	sprintf(ip, "%d.%d.%d.%d", pasv_ip[0], pasv_ip[1], pasv_ip[2], pasv_ip[3]);
	*d_port = pasv_port[0]*256 + pasv_port[1];	
}

// CDUP command
void ftp_cdup(char *cmd)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char destBuffer[BUFFER_SIZE];

	sprintf(sendBuffer, "CDUP\r\n");
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);
}
// CWD command
void ftp_cd(char *cmd)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char destBuffer[BUFFER_SIZE];

	sscanf(cmd, "%*s %s", destBuffer);

	sprintf(sendBuffer, "CWD %s\r\n", destBuffer);
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);
}
// REVRETR command
void ftp_revget(char *cmd)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char nameBuffer[BUFFER_SIZE];
	char buffer;
	char d_ip[16];
	int d_port;
	int file_size;
	int read_byte, total_byte=0;
	int fd;
	// PASSIVE mode need
	if (!is_passive) {
		printf("You need to activate Passvie mode with PASV command.\n");
		return;
	}
	// get filename
	sscanf(cmd, "%*s %s", nameBuffer);
	printf("local: %s remote: %s\n", nameBuffer, nameBuffer);
	// make data path conneciton
	get_passive(d_ip, &d_port);
	d_sock = connectSocket(d_ip, d_port);
		
	sprintf(sendBuffer, "REVRETR %s\r\n", nameBuffer);
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);
	
	fd = open(nameBuffer, O_WRONLY | O_CREAT, 0755);

	while( (read_byte = read(d_sock, &buffer, 1)) > 0 )
		write(fd, &buffer, 1);
	close(fd);
	close(d_sock);
	
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);
}

// RETR command
void ftp_get(char *cmd)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char nameBuffer[BUFFER_SIZE];
	char fileBuffer[BUFFER_SIZE]; 
	char d_ip[16];
	char *token;
	int d_port;
	int file_size;
	int read_byte, total_byte=0;
	int fd;
	// PASSIVE mode need
	if (!is_passive) {
		printf("You need to activate Passvie mode with PASV command.\n");
		return;
	}
	// get filename
	sscanf(cmd, "%*s %s", nameBuffer);
	printf("local: %s remote: %s\n", nameBuffer, nameBuffer);
	// make data path conneciton
	get_passive(d_ip, &d_port);
	d_sock = connectSocket(d_ip, d_port);
		
	sprintf(sendBuffer, "RETR %s\r\n", nameBuffer);
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	
	// receive message wait 150
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	token = strtok(readBuffer, "\n");
	printf("%s\n", token);
	token = strtok(NULL, "\n");
	if(token == NULL) // wait 226
		recvMsg(sock, readBuffer, sizeof(readBuffer));
	else
		strcpy(readBuffer, token);
	// file transfer
	fd = open(nameBuffer, O_WRONLY | O_CREAT, 0755);
	memset(fileBuffer, 0, sizeof(fileBuffer));
	while ( (read_byte = recv(d_sock, fileBuffer, sizeof(fileBuffer), 0)) > 0)
		write(fd, fileBuffer, read_byte);
	close(fd);
	close(d_sock);
	// 	
	printf("%s\n", readBuffer);
}

// STOR command
void ftp_put(char *cmd)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char nameBuffer[BUFFER_SIZE];
	char fileBuffer[BUFFER_SIZE]; 
	char d_ip[16];
	char *token;
	int d_port;
	int file_size;
	int read_byte, total_byte=0;
	int fd;

	// PASSIVE mode need
	if (!is_passive) {
		printf("You need to activate Passvie mode with PASV command.\n");
		return;
	}
	// get filename
	sscanf(cmd, "%*s %s", nameBuffer);
	printf("local: %s remote: %s\n", nameBuffer, nameBuffer);
	// make data path conneciton
	get_passive(d_ip, &d_port);
	d_sock = connectSocket(d_ip, d_port);

	sprintf(sendBuffer, "STOR %s\r\n", nameBuffer);
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);

	fd = open(nameBuffer, O_RDONLY);
	while ( (read_byte = read(fd, fileBuffer, sizeof(fileBuffer)) ) > 0)
		write(d_sock, fileBuffer, read_byte);
	close(fd);
	close(d_sock);

	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);
}
// REVSTOR command
void ftp_revput(char *cmd)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char nameBuffer[BUFFER_SIZE];
	char d_ip[16];
	char buffer;
	int d_port;
	int file_size;
	int read_byte, total_byte=0;
	int fd;
	int i;
	// PASSIVE mode need
	if (!is_passive) {
		printf("You need to activate Passvie mode with PASV command.\n");
		return;
	}
	// get filename
	sscanf(cmd, "%*s %s", nameBuffer);
	printf("local: %s remote: %s\n", nameBuffer, nameBuffer);
	// make data path conneciton
	get_passive(d_ip, &d_port);
	d_sock = connectSocket(d_ip, d_port);
		
	sprintf(sendBuffer, "REVSTOR %s\r\n", nameBuffer);
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);
	
	fd = open(nameBuffer, O_RDONLY);
	file_size = lseek(fd, (off_t) 0, SEEK_END);
	for (i=file_size-1; i>=0 ; i--)
	{
		lseek(fd, (off_t) i, SEEK_SET);
		read(fd, &buffer, 1);
		write(d_sock, &buffer, 1);
	}
	close(fd);
	close(d_sock);

	recvMsg(sock, readBuffer, sizeof(readBuffer));
	printf("%s\n", readBuffer);
}

// LIST command
void ftp_list(char *cmd)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char listBuffer[BUFFER_SIZE*64];
	char d_ip[16];
	int d_port;
	char *token;

	// PASSIVE mode need
	if (!is_passive) {
		printf("You need to activate Passvie mode with PASV command.\n");
		return;
	}
	// make data path conneciton
	get_passive(d_ip, &d_port);
	d_sock = connectSocket(d_ip, d_port);
	sprintf(sendBuffer, "LIST\r\n");
	send(sock, sendBuffer, strlen(sendBuffer), 0);
	
	// receive message wait 150
	recvMsg(sock, readBuffer, sizeof(readBuffer));
	token = strtok(readBuffer, "\n");
	printf("%s\n", token);
	token = strtok(NULL, "\n");
	if(token == NULL) // wait 226
		recvMsg(sock, readBuffer, sizeof(readBuffer));
	else
		strcpy(readBuffer, token);
	
	// directory list
	recvMsg(d_sock, listBuffer, sizeof(listBuffer));
	printf("%s\n", listBuffer);

	printf("%s\n", readBuffer);

	close(d_sock);
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

// advanced recv() for add \0 char.
void recvMsg(int sock, char *readBuffer, int size)
{
	int len;
	len = recv(sock, readBuffer, size-1, 0);
	readBuffer[len] = '\0';
}







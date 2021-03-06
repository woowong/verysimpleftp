#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define USER_MAX 50
#define COMMAND_SIZE 16

int connectSocket(int port);
void* server_thread(void *args);

int ftp_user(int clnt_sock, char *arg);
int ftp_pasv(int cltk_sock);
void ftp_retr(int clnt_sock, char *arg, int d_sock);
void ftp_revretr(int clnt_sock, char *arg, int d_sock);
void ftp_stor(int clnt_sock, char *arg, int d_sock);
void ftp_revstor(int clnt_sock, char *arg, int d_sock);
void ftp_list(int clnt_sock, int d_sock);
void ftp_pwd(int clnt_sock);
void ftp_cwd(int clnt_sock, char *arg);
void ftp_cdup(int clnt_sock);

int serv_sock;
int sock_list[USER_MAX];
int sock_list_num=0;

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
				exit(-1);
			}
			// register new sock num to clnt_sock,
			sock_list[sock_list_num] = sock;
			pthread_create(&thread, NULL, server_thread, (void*)(&sock_list[sock_list_num]));
			sock_list_num++;
		}
	}
	return 0;
}

void* server_thread (void *args)
{
	int clnt_sock = *((int*) args);
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char cmd[COMMAND_SIZE];
	char arg[BUFFER_SIZE];
	int recv_byte;	
	int is_logged = 0;
	int d_sock;

	// init connection
	sprintf(sendBuffer, "220 Service ready for new user. - woowong\n");
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
	
	for(;;)
	{
		recv_byte = recv(clnt_sock, readBuffer, sizeof(readBuffer)-1, 0);
		if(!recv_byte) return;
		sscanf(readBuffer, "%s %s", cmd, arg);
		// USER recv
		if(!is_logged) {
			if(!strcmp(cmd, "USER")) 
				is_logged = ftp_user(clnt_sock, arg);
		}
		else {
			if(!strcmp(cmd, "SYST")|!strcmp(cmd, "FEAT")) {
				sprintf(sendBuffer, "215 NAME system type. \n");
				send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
			}
			else if(!strcmp(cmd, "TYPE")) {
				sprintf(sendBuffer, "200 Switching to Binary mode.\n");
				send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
			}
			else if(!strcmp(cmd, "PORT")) {
				sprintf(sendBuffer, "200 command okay.\n");
				send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
			}

			else if(!strcmp(cmd, "PWD"))
				ftp_pwd(clnt_sock);
			else if(!strcmp(cmd, "PASV"))		
				d_sock = ftp_pasv(clnt_sock);
			else if(!strcmp(cmd, "CWD"))
				ftp_cwd(clnt_sock, arg);
			else if(!strcmp(cmd, "CDUP"))
				ftp_cdup(clnt_sock);
			else if(!strcmp(cmd, "RETR"))		
				ftp_retr(clnt_sock, arg, d_sock);
			else if(!strcmp(cmd, "STOR"))		
				ftp_stor(clnt_sock, arg, d_sock);
			else if(!strcmp(cmd, "LIST"))		
				ftp_list(clnt_sock, d_sock);
			else if(!strcmp(cmd, "REVRETR"))
				ftp_revretr(clnt_sock, arg, d_sock);
			else if(!strcmp(cmd, "REVSTOR"))
				ftp_revstor(clnt_sock, arg, d_sock);
			else if(!strcmp(cmd, "QUIT"))
				break;
		}
	}
	close(clnt_sock);
	sock_list_num--;
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
	int ip[4];
	int d_sock;
	// random generate port number
	srand(time(NULL));
	pasv_port = 1024 + rand() % (65535 - 1024);
	port0 = pasv_port / 256;
	port1 = pasv_port % 256;
	// get server ip information
	getsockname(clnt_sock, (struct sockaddr *)&serv_addr, &addr_size);
	ip[0] = (int) (serv_addr.sin_addr.s_addr&0xFF);
	ip[1] =	(int)((serv_addr.sin_addr.s_addr&0xFF00)>>8);
	ip[2] =	(int)((serv_addr.sin_addr.s_addr&0xFF0000)>>16);
	ip[3] = (int)((serv_addr.sin_addr.s_addr&0xFF000000)>>24);
	// create new socket for data path
	d_sock = connectSocket(pasv_port);
	sprintf(sendBuffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\n", 
			ip[0], ip[1], ip[2], ip[3], port0, port1);
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
	return d_sock;
}
// STOR
void ftp_stor(int clnt_sock, char *arg, int d_sock)
{
	struct sockaddr_in clnt_addr;
	int clnt_addr_size = sizeof(clnt_addr);
	char readBuffer[BUFFER_SIZE];
	char fileBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char cmd[COMMAND_SIZE];
	int sock;
	// 150 send
	sprintf(sendBuffer, "150 File status okay; about to open data connection.\n");
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
	// file send
	sock = accept(d_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

	int fd = open(arg, O_WRONLY | O_CREAT, 0755);
	int read_byte;
	while ( (read_byte = read(sock, fileBuffer, sizeof(fileBuffer)) ) > 0)
		write(fd, fileBuffer, read_byte);
	close(fd);

	// 226
	close(sock);
	close(d_sock);
	memset(sendBuffer, 0, sizeof(sendBuffer));
	sprintf(sendBuffer, "226 Closing data connection.\n");
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);

}
// REVSTOR
void ftp_revstor(int clnt_sock, char *arg, int d_sock)
{
	struct sockaddr_in clnt_addr;
	int clnt_addr_size = sizeof(clnt_addr);
	char readBuffer[BUFFER_SIZE];
	char fileBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char cmd[COMMAND_SIZE];
	int sock;
	// 150 send
	sprintf(sendBuffer, "150 File status okay; about to open data connection.\n");
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
	// file send
	sock = accept(d_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

	int fd = open(arg, O_WRONLY | O_CREAT, 0755);
	int read_byte;
	char buffer;
	while ( (read_byte = read(sock, &buffer, 1)) > 0)
			write(fd, &buffer, 1);
	close(fd);

	// 226
	close(sock);
	close(d_sock);
	memset(sendBuffer, 0, sizeof(sendBuffer));
	sprintf(sendBuffer, "226 Closing data connection.\n");
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);

}
// RETR
void ftp_retr(int clnt_sock, char *arg, int d_sock)
{
	struct sockaddr_in clnt_addr;
	int clnt_addr_size = sizeof(clnt_addr);
	char readBuffer[BUFFER_SIZE];
	char fileBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char cmd[COMMAND_SIZE];
	int sock;
	// 150 send
	sprintf(sendBuffer, "150 File status okay; about to open data connection.\n");
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
	// file send
	sock = accept(d_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

	int fd = open(arg, O_RDONLY);
	int read_byte;
	while ( (read_byte = read(fd, fileBuffer, sizeof(fileBuffer)) ) > 0) 
		write(sock, fileBuffer, read_byte);
	close(fd);

	// 226
	close(sock);
	close(d_sock);
	memset(sendBuffer, 0, sizeof(sendBuffer));
	sprintf(sendBuffer, "226 Closing data connection.\n");
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);

}
// REVRETR
void ftp_revretr(int clnt_sock, char *arg, int d_sock)
{
	struct sockaddr_in clnt_addr;
	int clnt_addr_size = sizeof(clnt_addr);
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char buffer;
	char cmd[COMMAND_SIZE];
	int sock;
	int i;
	// 150 send
	sprintf(sendBuffer, "150 File status okay; about to open data connection.\n");
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
	// file send
	sock = accept(d_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

	int fd = open(arg, O_RDONLY);
	int file_size = lseek(fd, (off_t) 0, SEEK_END);
	for(i=file_size-1; i>=0; i--)
	{
		lseek(fd, (off_t) i, SEEK_SET);
		read(fd, &buffer, 1);
		write(sock, &buffer, 1);
	}
	close(fd);

	// 226
	close(sock);
	close(d_sock);
	memset(sendBuffer, 0, sizeof(sendBuffer));
	sprintf(sendBuffer, "226 Closing data connection.\n");
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);

}
// LIST
void ftp_list(int clnt_sock, int d_sock)
{
	struct sockaddr_in clnt_addr;
	int clnt_addr_size = sizeof(clnt_addr);
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	char listBuffer[BUFFER_SIZE*64];
	char cmd[COMMAND_SIZE];
	int sock;
	// 150 send
	sprintf(sendBuffer, "150 File status okay; about to open data connection.\n");
	send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
	// file list send
	sock = accept(d_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
	
	char cwdBuffer[BUFFER_SIZE];
	getcwd(cwdBuffer, sizeof(cwdBuffer));
	DIR* dp = opendir(cwdBuffer);
	
	struct dirent *entry;
	struct stat file_stat;
	int list_length=0; // for sprintf append 
	while(entry = readdir(dp))
	{
		stat(entry->d_name, &file_stat); // load stat file
		// directory?
		char is_dir = (file_stat.st_mode & S_IFDIR) ? 'd' : '-';
		// permission 
		char owner_p[4], group_p[4], other_p[4];
		int perm = file_stat.st_mode & S_IRWXU;
		sprintf(owner_p, "%c%c%c", perm&0x4?'r':'-', perm&0x2?'w':'-', perm&0x1?'x':'-');
		perm = file_stat.st_mode & S_IRWXG;
		sprintf(group_p, "%c%c%c", perm&0x4?'r':'-', perm&0x2?'w':'-', perm&0x1?'x':'-');
		perm = file_stat.st_mode & S_IRWXO;
		sprintf(other_p, "%c%c%c", perm&0x4?'r':'-', perm&0x2?'w':'-', perm&0x1?'x':'-');
		// link number
		int link_num = file_stat.st_nlink;
		// uid, gid
		int uid = file_stat.st_uid;
		int gid = file_stat.st_gid;
		// filesize
		int filesize = file_stat.st_size;
		// modified time
		time_t rawtime = file_stat.st_mtime;
		struct tm *st_time = localtime(&rawtime);
		char mtime[60];
		strftime(mtime,80,"%b %d %H:%M",st_time);

		list_length += sprintf(listBuffer+list_length, "%c%s%s%s\t%d\t%d\t%d\t%d\t%s\t%s\n", 
				is_dir, owner_p, group_p, other_p, 
				link_num, uid, gid, filesize,
				mtime, entry->d_name);
	}
	if ( send(sock, listBuffer, strlen(listBuffer), 0) != -1) {
		// 226 send
		close(sock);
		close(d_sock);
		memset(sendBuffer, 0, sizeof(sendBuffer));
		sprintf(sendBuffer, "226 Closing data connection.\n");
		send(clnt_sock, sendBuffer, strlen(sendBuffer), 0);
	}

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
// CDUP
void ftp_cdup(int clnt_sock)
{
	char readBuffer[BUFFER_SIZE];
	char sendBuffer[BUFFER_SIZE];
	if (!chdir("..")) {
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
	int temp=1;
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

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(temp));
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


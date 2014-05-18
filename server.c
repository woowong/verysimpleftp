#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

int connectSocket(int port);
void* server_thread(void *args);

int main (int argc, char *argv[])
{
	int serv_sock;
	int clnt_sock;

	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;
	pthread_t thread;

	int tt=5;

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
			clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
			printf("after accept\n");
			if(clnt_sock==-1) {
				printf("accept() error\n");
				exit(-1);
			}
			
			pthread_create(&thread, NULL, server_thread, (void*)&tt);
			
		}

	}
}

void* server_thread (void *args)
{
	
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#include "server.h"

#define SWAP_UINT32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))

int debug_mode;

int serv_port;
int serv_sock;
int clnt_sock;
int clnt_addr_size;

static pthread_t server_thread;

struct sockaddr_in serv_addr;
struct sockaddr_in clnt_addr;

int is_running_server;

static void error_handling(char *message);

static int listen_server(int port)
{
	int on = 1;
	
	printf("listen_server(%d)\n", port);
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
		error_handling("socket() error");
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	clnt_addr_size=sizeof(clnt_addr);

	if( bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1 )
		error_handling("bind() error"); 

	if( listen(serv_sock, 5)==-1 )
		error_handling("listen() error");
	return serv_sock;
}

static int accept_client(int serv_sock)
{
	printf("accept_server(%d)\n", serv_sock);
	return accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
}

static void close_server()
{
	close(clnt_sock);
	close(serv_sock);
	is_running_server = 0;
}


static int service_loop(int clnt_sock)

{
	int requestNum;
	int rval;
	struct timeval current_time, prev_time;
	unsigned int t;
	unsigned int t2;

	printf("service_loop(%d)\n", clnt_sock);

	if(clnt_sock==-1)
		error_handling("accept() error");
	while (1) {
		if (debug_mode)	printf("read...");
		rval = read(clnt_sock, &requestNum, sizeof(requestNum));
		requestNum = SWAP_UINT32(requestNum);
		printf("read : %d\n", requestNum);

		requestNum = 101010;
		requestNum = SWAP_UINT32(requestNum);
		write(clnt_sock, &requestNum, sizeof(requestNum));
		if (rval <= 0) {
			goto close;
		}

		switch (requestNum) {
			case 0:
			break;

			default:
			break;
		}
	}

close:
	close_server();
	return -1;
}



static void *thr_server()
{
	printf("thr_server()\n");
	serv_sock = listen_server(serv_port);
	clnt_sock = accept_client(serv_sock);
	service_loop(clnt_sock);
	printf("server_thread exit\n");
	return NULL;
}

int start_server(int port)
{
	serv_port = port;
	if (pthread_create(&server_thread, NULL, thr_server, NULL) == 0) {
		is_running_server = 1;
		return 0;
	}
	return 	-1;
}

int is_exited_server()
{
	return is_running_server == 0;
}

int join_server_thread()
{
	return pthread_join(server_thread, (void **)NULL);
}

int main2(int argc, char **argv)
{
	if(argc!=2){
		printf("Usage : %s <port>\n", argv[0]);
		/* exit(1); */
	}
	start_server(9999);
	join_server_thread();

	return 0;
}

static void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

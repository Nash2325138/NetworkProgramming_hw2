#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/select.h>

#include <sys/stat.h>
#include <dirent.h>

#include <string>

#define MAXLINE 2048
#define LISTENQ 1024
void tcp_echo(int sockfd);
void dg_echo(int udpfd);
void handler(int kkkk)
{
	int pid, stats;
	for(;;){
		pid = waitpid(-1, &stats, WCONTINUED);
		if(pid == -1) break;
		printf("pid: %d terminated.\n", pid);
	}
}

class User
{
public:
	std::string acount;
	std::string password;
	std::string birthday;
	struct tm* registerTime;
	struct tm* lastLoginTime;
	bool isOnline;
	
	void processMessage(std::string message)
	{
		
	}
};

int main(int argc, char **argv)
{

	if(argc!=2){
		fprintf(stderr, "Usage: ./<executable file> <port>");
		exit(9999);
	}

	//server part
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl (INADDR_ANY); // for any interface
	servaddr.sin_port = htons(atoi(argv[1])); //user define


	//TCP client part
	int listenfd = socket (AF_INET, SOCK_STREAM, 0);	
	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);


	int udpfd;
	if( (udpfd = socket(AF_INET, SOCK_DGRAM, 0) ) < 0) perror("socket error");
	if( bind(udpfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) perror("bind error");

	fd_set rset, allset;
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	FD_SET(udpfd, &allset);

	// handle zombie process
	signal(SIGCHLD, handler);

	// start wait for any UDP data gram or TCP connection, using select()
	for ( ; ; ) {
		//printf("for loop begin\n");
		rset = allset;
		int maxfd = (listenfd > udpfd) ? listenfd : udpfd;
		select(maxfd+1, &rset, NULL, NULL, NULL);
		if(FD_ISSET(listenfd, &rset))
		{
			struct sockaddr_in cliaddr;
			socklen_t clilen = sizeof(cliaddr);
			int connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
			pid_t childpid;
			if ( (childpid = fork()) == 0) { /* child process */
				close(listenfd);

				// print the information of TCP's client connection
				int cliPort = ntohs(cliaddr.sin_port);
				char cliAddrStr[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &cliaddr.sin_addr, cliAddrStr, INET_ADDRSTRLEN);	
				printf("TCP Connection from %s, port: %d\n", cliAddrStr, cliPort);

				tcp_echo(connfd); // process the request 

				exit(0); //close child process 
			}
			close(connfd); // closes connected socket of parent process
		}
		
		if(FD_ISSET(udpfd, &rset))
		{
			dg_echo(udpfd);
		}
		
	}
}

void tcp_echo(int sockfd)
{
	ssize_t n;
	char buf[MAXLINE]; //MAXLINE is defined by user
	again:
		while ( (n = read(sockfd, buf, MAXLINE)) > 0)
			write(sockfd, buf, n);
		if (n < 0 && errno == EINTR) /* interrupted by a signal before any data was read*/
			goto again; //ignore EINTR
		else if (n < 0)
			printf("tcp_echo: read error");
}

void dg_echo(int udpfd)
{
	char message[MAXLINE];
	struct sockaddr_in cliaddr_in;
	socklen_t clilen = sizeof cliaddr_in;

	int n = recvfrom(udpfd, message, sizeof message, 0, (struct sockaddr *)&cliaddr_in, &clilen);

	// print information of data gram
	char cliAddrStr[INET_ADDRSTRLEN];
	if( inet_ntop(AF_INET, &(cliaddr_in.sin_addr), cliAddrStr, INET_ADDRSTRLEN) <= 0) perror("inet_ntop error");
	int cliPort = ntohs(cliaddr_in.sin_port);	
	fprintf(stdout, "UDP data gram from %s connect to port: %d\n", cliAddrStr, cliPort);
	
	// echo back
	sendto(udpfd, message, n, 0, (struct sockaddr *)&cliaddr_in, clilen);
}
/*
int udpReceiveOne(int udpfd, char* recvBuffer, struct sockaddr_in *cliaddr_in)
{
	socklen_t clilen = sizeof (struct sockaddr_in);
	int n = recvfrom(udpfd, recvBuffer, sizeof recvBuffer, (struct sockaddr *)cliaddr_in, &clilen);

	char ack[10];
	strcpy(ack, "ack");

	sendto(udpfd, ack, sizeof ack, 0, (struct sockaddr *)cliaddr_in, clilen);
	return n;
}
int udpSendOne(int udpfd, char* sendBuffer, struct sockaddr_in *cliaddr_in)
{
	socklen_t clilen = sizeof (struct sockaddr_in);

	fd_set rset, allset;
	FD_ZERO(&allset);
	FD_SET(udpfd, &allset);
	do{
		sendto(udpfd, sendBuffer, (struct sockaddr *)cliaddr_in, &clilen);
	};
}
*/
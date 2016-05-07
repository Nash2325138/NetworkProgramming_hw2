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
//#include <sys/ioctl.h>

#define MAXLINE 2048


void dg_cli(FILE *fp, int udpfd, const struct sockaddr *servaddr_ptr, socklen_t servlen);
void tcp_cli(FILE *fp, int serverfd);
int main (int argc, char **argv)
{
	int servfd;
	char recvBuffer[MAXLINE];
	char sendBuffer[MAXLINE];
	char acount[MAXLINE];
	struct sockaddr_in servaddr;
	if(argc!=3){
		fprintf(stderr, "Usage: ./<executable file> <server IP> <server port>\n");
		exit(EXIT_FAILURE);
	}

	// server setting
	//memset(servaddr.sin_zero, 0, sizeof(servaddr.sin_zero));
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	if( (inet_pton(AF_INET, argv[1], &servaddr.sin_addr)) <= 0 ) perror("inet_pton error");
	
	strcpy(sendBuffer, "NULL");
	sendCommand(udpfd, (struct sockaddr)&servaddr, command);


	fd_set rset, allset;
	FD_ZERO(&allset);
	FD_SET(udpfd, &allset);
	for( ; ; )
	{
		rset = allset;
		int maxfd = rset;
		select(maxfd+1, &rset, NULL, NULL, NULL);
		if(FD_ISSET(udpfd, &rset))
		{
			int n = recvfrom(udpfd, recvBuffer, MAXLINE, 0, NULL, NULL);
			recvBuffer[n] = '\0';
			sendAck(udpfd, )
		}
	}

	return 0;
}

void sendCommand(int udpfd, const struct sockaddr *servaddr_ptr, char *command)
{
	socklen_t servlen = sizeof (struct sockaddr);
	fd_set rset, allset;
	FD_ZERO(&allset);
	FD_SET(udpfd, &allset);
	struct timeval tv;
	tv.tv_usec = 2000;
	while(true)
	{
		sendto(udpfd, command, MAXLINE, 0, servaddr_ptr, servlen);
		rset = allset;
		int maxfd = udpfd;
		select(maxfd+1, &rset, NULL, NULL, &tv);
		if(FD_ISSET(udpfd, &rset))
		{
			char ack[MAXLINE];
			recvfrom(udpfd, ack, MAXLINE, 0, NULL, NULL);
			if(strcmp(ack, "ack") == 0) return;
			else fprintf(stderr, "receive something not ack\n");
		}
		else continue; // needs retransmition
	}
}

void receive_print(int udpfd, const struct sockaddr *servaddr_ptr, char *recvBuffer)
{
	int n = recvfrom(udpfd, recvBuffer, MAXLINE, 0, NULL, NULL);
	if(n <= 0) perror("receive_print error");
	else {
		recvBuffer[n] = '\0';
		if(fputs(recvBuffer, stdout)==EOF) perror("fputs error");
	}
}

void sendAck(int udpfd, const struct sockaddr *servaddr_ptr)
{
	socklen_t servlen = sizeof (struct sockaddr);
	char ack[10];
	strcpy(ack, "ack");
	sendto(udpfd, ack, sizeof ack, 0, servaddr_ptr, servlen);
}
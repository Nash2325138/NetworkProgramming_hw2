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

typedef enum
{
	INIT,
	LOGIN_ACCOUNT,
	LOGIN_PASSWORD,
	REGISTER_ACCOUNT,
	REGISTER_PASSWORD,
	REGISTER_NICKNAME,
	REGISTER_BIRTHDAY,
	ONLINE
}State;

State state;
char account[MAXLINE];
//void dg_cli(FILE *fp, int udpfd, const struct sockaddr *servaddr_ptr, socklen_t servlen);
//void tcp_cli(FILE *fp, int serverfd);

void sendCommand(int udpfd, const struct sockaddr *servaddr_ptr, char *command);
void receive_print(int udpfd, const struct sockaddr *servaddr_ptr, char *recvBuffer);
void sendAck(int udpfd, const struct sockaddr *servaddr_ptr);
int main (int argc, char **argv)
{
	setbuf(stdout, NULL);
	int servfd;
	char recvBuffer[MAXLINE];
	char sendBuffer[MAXLINE];
	char temp[MAXLINE];
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
	
	if( (servfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) perror("socket error");

	state = INIT;
	fd_set rset, allset;
	FD_ZERO(&allset);
	FD_SET(servfd, &allset);

	strcpy(sendBuffer, "NULL");
	sendCommand(servfd, (struct sockaddr *)&servaddr, sendBuffer);
	for( ; ; )
	{
		rset = allset;
		int maxfd = servfd;
		select(maxfd+1, &rset, NULL, NULL, NULL);
		if(FD_ISSET(servfd, &rset))
		{
			int n = recvfrom(servfd, recvBuffer, MAXLINE, 0, NULL, NULL);
			recvBuffer[n] = '\0';
			sendAck(servfd, (struct sockaddr *)&servaddr);
			if(state == ONLINE)	// already online!
			{
				if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error"); 
				if(fscanf(stdin, "%s", temp) == EOF) perror("fscanf error:");
				sprintf(sendBuffer, "ONLINE %s %s", account, temp);
				sendCommand(servfd, (struct sockaddr *)&servaddr, sendBuffer);
			}
			else // haven't login
			{
				if(state == INIT)
				{
					if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error"); 
					if(fscanf(stdin, "%s", temp) == EOF) perror("fscanf error:");
					state = LOGIN_ACCOUNT;

					strcpy(account, temp);	// suppose log in successfully
					sprintf(sendBuffer, "LOGIN_ACCOUNT %s", temp);
					sendCommand(servfd, (struct sockaddr *)&servaddr, sendBuffer);
				}
				else if(state == LOGIN_ACCOUNT)
				{
					if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error");
					if(fscanf(stdin, "%s", temp) == EOF) perror("fscanf error:");

					if(strncmp(recvBuffer, "Create", 6) == 0) {
						state = REGISTER_ACCOUNT;
						strcpy(account, temp);	// if client want to create new account, override the account
						sprintf(sendBuffer, "REGISTER_ACCOUNT %s", temp);
					}
					else {
						state = LOGIN_PASSWORD;
						sprintf(sendBuffer, "LOGIN_PASSWORD %s %s", account, temp);
					}
					sendCommand(servfd, (struct sockaddr *)&servaddr, sendBuffer);
				}
				else if(state == LOGIN_PASSWORD)
				{
					if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error");
					if(fscanf(stdin, "%s", temp) == EOF) perror("fscanf error:");
					if(strncmp(recvBuffer, "Fail", 4) == 0) {
						sprintf(sendBuffer, "LOGIN_ACCOUNT %s", temp);
						strcpy(account, temp);
						state = LOGIN_ACCOUNT;
					} else {
						sprintf(recvBuffer, "ONLINE %s %s", account, temp);
						state = ONLINE;
					}
					sendCommand(servfd, (struct sockaddr *)&servaddr, sendBuffer);
				}
				else if(state == REGISTER_ACCOUNT)
				{
					if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error"); 
					if(fscanf(stdin, "%s", temp) == EOF) perror("fscanf error:");

					if(strncmp(recvBuffer, "------------Account avalible!-----------\n", strlen("------------Account avalible!-----------\n")) == 0) {
						sprintf(sendBuffer, "REGISTER_PASSWORD %s %s", account, temp);
						state = REGISTER_PASSWORD;
					} else if(strncmp(recvBuffer, "--------------Account used!-------------\n", strlen("--------------Account used!-------------\n")) == 0) {
						sprintf(sendBuffer, "REGISTER_ACCOUNT %s", temp);
						strcpy(account, temp);	// if account is used, override the account
					}
					sendCommand(servfd, (struct sockaddr *)&servaddr, sendBuffer);
				}
				else if(state == REGISTER_PASSWORD)
				{
					if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error"); 
					if(fscanf(stdin, "%s", temp) == EOF) perror("fscanf error:");
					sprintf(sendBuffer, "REGISTER_NICKNAME %s %s", account, temp);
					state = REGISTER_NICKNAME;
					sendCommand(servfd, (struct sockaddr *)&servaddr, sendBuffer);
				}
				else if(state == REGISTER_NICKNAME)
				{
					if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error"); 
					if(fscanf(stdin, "%s", temp) == EOF) perror("fscanf error:");
					sprintf(sendBuffer, "REGISTER_BIRTHDAY %s %s", account, temp);
					state = REGISTER_BIRTHDAY;
					sendCommand(servfd, (struct sockaddr *)&servaddr, sendBuffer);
				}
				else if(state == REGISTER_BIRTHDAY)
				{
					if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error"); 
					if(fscanf(stdin, "%s", temp) == EOF) perror("fscanf error:");
					sprintf(sendBuffer, "ONLINE %s %s", account, temp);
					state = ONLINE;
					sendCommand(servfd, (struct sockaddr *)&servaddr, sendBuffer);
				}
			}	
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

	//printf("sending: %s\n", command);
	while(true)
	{
		sendto(udpfd, command, strlen(command), 0, servaddr_ptr, servlen);
		
		rset = allset;
		int maxfd = udpfd;
		tv.tv_sec = 1;
		select(maxfd+1, &rset, NULL, NULL, &tv);
		if(FD_ISSET(udpfd, &rset))
		{
			char ack[MAXLINE];
			recvfrom(udpfd, ack, MAXLINE, 0, NULL, NULL);
			if(strcmp(ack, "ack") == 0) return;
			else fprintf(stderr, "receive something not ack:%s\n", ack);
		}
		else {
			printf("retransmition: %s", command);
		} // needs retransmition
	}
}

void receive_print(int udpfd, const struct sockaddr *servaddr_ptr, char *recvBuffer)
{
	int n = recvfrom(udpfd, recvBuffer, MAXLINE, 0, NULL, NULL);
	sendAck(udpfd, servaddr_ptr);
	if(n <= 0) perror("receive_print error");
	else {
		recvBuffer[n] = '\0';
		if(fprintf(stdout, "%s", recvBuffer)==EOF) perror("fprintf error");
	}
}

void sendAck(int udpfd, const struct sockaddr *servaddr_ptr)
{
	socklen_t servlen = sizeof (struct sockaddr);
	char ack[MAXLINE];
	strcpy(ack, "ack");
	sendto(udpfd, ack, sizeof ack, 0, servaddr_ptr, servlen);
}
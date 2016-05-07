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
#include <vector>

#define MAXLINE 2048
#define LISTENQ 1024
typedef enum
{
	OFFLINE
}UserState;

class User
{
public:
	std::string acount;
	std::string password;
	std::string birthday;
	struct tm* registerTime;
	struct tm* lastLoginTime;
	UserState state;
	
	void processMessage(char *message)
	{

	}
};

char loginAccountString[MAXLINE];
char loginPasswordString[MAXLINE];
char menuString[MAXLINE];
char wellcomeString[MAXLINE];
void initialString()
{
	strcpy(menuString, //"[SP]Show Profile [SA]Show Article [A]dd Article\n
						//[E]nter Article [C]hat [S]earch [L]ogout\n\0");
		"");
	strcpy(loginAccountString, "Enter your account to login( or enter \"new\" to register ): ");
	strcpy(loginPasswordString, "Enter your password: ");

	strcpy(wellcomeString, "");
	FILE * ascii = fopen("wellcome_ASCII.txt", "r");
	char buffer[1024];
	while( fgets(buffer, 1024, ascii) != NULL ){
		strcat(wellcomeString, buffer);
		strcat(wellcomeString, "\n");
	}
	fclose(ascii);
}

void sendAck(int udpfd, const struct sockaddr *cliaddr_ptr);
void sendOne(int udpfd, char* sendBuffer, struct sockaddr *cliaddr_ptr);

int main(int argc, char **argv)
{
	char recvBuffer[MAXLINE];
	char sendBuffer[MAXLINE];
	if(argc!=2){
		fprintf(stderr, "Usage: ./<executable file> <port>\n");
		exit(9999);
	}

	//server part
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl (INADDR_ANY); // for any interface
	servaddr.sin_port = htons(atoi(argv[1])); //user define

	int udpfd;
	if( (udpfd = socket(AF_INET, SOCK_DGRAM, 0) ) < 0) perror("socket error");
	if( bind(udpfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) perror("bind error");

	initialString();

	fd_set rset, allset;
	FD_ZERO(&allset);
	FD_SET(udpfd, &allset);

	// structure to grab client's information
	struct sockaddr_in cliaddr_in;
	socklen_t clilen = sizeof cliaddr_in;

	std::vector<User> userList();

	for ( ; ; ) {
		//printf("for loop begin\n");
		rset = allset;
		int maxfd = udpfd;
		select(maxfd+1, &rset, NULL, NULL, NULL);
		
		if(FD_ISSET(udpfd, &rset))
		{
			int n = recvfrom(udpfd, recvBuffer, MAXLINE, 0, (struct sockaddr *)&cliaddr_in, &clilen);
			recvBuffer[n] = '\0';
			sendAck(udpfd, (struct sockaddr *)&cliaddr_in);

			char account[MAXLINE];
			sscanf(recvBuffer, "%s", account);
			if(strcmp(account, "NULL") == 0)
			{
				strcpy(sendBuffer, loginAccountString);
				sendOne(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
			}
			else
			{
				if(strcmp(account, "new") == 0)
				{

				}
				else
				{
					strcpy(sendBuffer, loginPasswordString);
					sendOne(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
				}
			}
		}
		
	}
}
void sendAck(int udpfd, const struct sockaddr *cliaddr_ptr)
{
	socklen_t servlen = sizeof (struct sockaddr);
	char ack[10];
	strcpy(ack, "ack");
	sendto(udpfd, ack, strlen(ack), 0, cliaddr_ptr, servlen);
}

void sendOne(int udpfd, char* sendBuffer, struct sockaddr *cliaddr_ptr)
{
	socklen_t servlen = sizeof (struct sockaddr);
	fd_set rset, allset;
	FD_ZERO(&allset);
	FD_SET(udpfd, &allset);
	struct timeval tv;

	printf("To send: %s", sendBuffer);
	fflush(stdout);
	while(true)
	{
		sendto(udpfd, sendBuffer, strlen(sendBuffer), 0, cliaddr_ptr, servlen);
		rset = allset;
		int maxfd = udpfd;
		tv.tv_usec = 0;
		tv.tv_sec = 1;
		select(maxfd+1, &rset, NULL, NULL, &tv);
		if(FD_ISSET(udpfd, &rset))
		{
			char ack[MAXLINE];
			recvfrom(udpfd, ack, MAXLINE, 0, NULL, NULL);
			if(strcmp(ack, "ack") == 0) return;
			else fprintf(stderr, "receive something not ack : %s\n", ack);
		}
		else
		{
			printf("needs retransmition: %s\n", sendBuffer);
		} 
	}
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
}*/

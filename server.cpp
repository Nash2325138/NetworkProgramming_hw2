#include <cstdlib>
#include <cstdio>
#include <cstring>

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

#include <iostream>
#include <string>
#include <vector>
#include <map>

#define MAXLINE 2048
typedef enum
{
	CREATING,
	OFFLINE,
	MENU,
	WRITING_ARTICLE
}UserState;

char loginAccountString[MAXLINE];
char loginPasswordString[MAXLINE];
char menuString[MAXLINE];
char wellcomeString[MAXLINE];
void sendAck(int udpfd, const struct sockaddr *cliaddr_ptr);
void sendOne(int udpfd, char* sendBuffer, struct sockaddr *cliaddr_ptr);

class User
{
public:
	char account[100];
	char password[100];
	char nickname[100];
	char birthday[100];
	struct tm* registerTime;
	struct tm* lastLoginTime;
	UserState state;
	User(char *account)
	{
		strcpy(this->account, account);
		password[0] = '\0';
		birthday[0] = '\0';
		registerTime = lastLoginTime = NULL;
		state = CREATING;
	}
	void catWellcomeToBuffer(char *sendBuffer)
	{
		strcat(sendBuffer, wellcomeString);
		strcat(sendBuffer, "Hello ");
		strcat(sendBuffer, account);
		strcat(sendBuffer, "!\n");
		if(lastLoginTime != NULL) {
			strcat(sendBuffer, "Last time when you login: ");
			strcat(sendBuffer, asctime(lastLoginTime));
		}
		else {
			strcat(sendBuffer, "This is the first log in!");
		}
		strcat(sendBuffer, "\n");
		strcat(sendBuffer, menuString);
		time_t rawtime;
		time(&rawtime);
		lastLoginTime = localtime(&rawtime);
		this->state = MENU;
	}
};

void initialString()
{
	strcpy(menuString, "[SP]Show Profile\n[SA]Show Article\n[A]dd Article\n[E]nter Article\n[C]hat\n[S]earch\n[L]ogout\n");
	strcpy(loginAccountString, "Enter your account( or enter \"new\" to register ): ");
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


int main(int argc, char **argv)
{
	setbuf(stdout, NULL);
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
	std::map<std::string, User *> accountMap;

	for ( ; ; ) {
		//printf("for loop begin\n");
		rset = allset;
		int maxfd = udpfd;
		select(maxfd+1, &rset, NULL, NULL, NULL);
		
		if(FD_ISSET(udpfd, &rset))
		{
			int n = recvfrom(udpfd, recvBuffer, MAXLINE, 0, (struct sockaddr *)&cliaddr_in, &clilen);
			recvBuffer[n] = '\0';
			printf("receive: %s\n", recvBuffer);
			sendAck(udpfd, (struct sockaddr *)&cliaddr_in);
			char status[MAXLINE];
			sscanf(recvBuffer, "%s", status);

			if(strcmp(status, "ONLINE") == 0)
			{
				printf("Let User class to process it\n");
				sprintf(sendBuffer, "Let User class to process it\n");
				sendOne(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
			}
			else // the user send this package has not logged in yet
			{
				if(strcmp(status, "NULL") == 0)
				{
					strcpy(sendBuffer, loginAccountString);
					sendOne(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
				}
				else if(strcmp(status, "LOGIN_ACCOUNT") == 0)
				{
					char accountBuffer[100];
					sscanf(recvBuffer, "LOGIN_ACCOUNT %s", accountBuffer);
					if(strcmp(accountBuffer, "new") == 0) {
						strcpy(sendBuffer, "Create a new account!! Please enter the following content of your new account\n");
						strcat(sendBuffer, "\n    Your new account: ");
					}
					else {
						strcpy(sendBuffer, loginPasswordString);
					}
					sendOne(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
				}
				else if(strcmp(status, "LOGIN_PASSWORD") == 0)
				{
					char accountBuffer[100];
					char passwordBuffer[100];
					sscanf(recvBuffer, "LOGIN_PASSWORD %s %s", accountBuffer, passwordBuffer);
					std::string accountCppString(accountBuffer);
					if(accountMap.find(accountCppString) == accountMap.end()) {
						strcpy(sendBuffer, "Fail to log in, account not exist\n\n");
						strcat(sendBuffer, loginAccountString);
					}	
					else if( strcmp(accountMap.at(accountCppString)->password, passwordBuffer) != 0) {
						strcpy(sendBuffer, "Fail to log in, password not fit\n\n");
						printf("Fail to log in, the real password for %s is %s\n",
												accountMap.at(accountCppString)->account,
												accountMap.at(accountCppString)->password);
						strcat(sendBuffer, loginAccountString);
					}
					else {
						strcpy(sendBuffer, "\n");
						accountMap.at(accountCppString)->catWellcomeToBuffer(sendBuffer);
					}
					sendOne(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
				}
				else if(strcmp(status, "REGISTER_ACCOUNT") == 0)
				{
					char newAccount[100];
					sscanf(recvBuffer, "REGISTER_ACCOUNT %s", newAccount);
					std::string cppTempString(newAccount);
					if(accountMap.find(cppTempString) == accountMap.end()) {
						strcpy(sendBuffer, "------------Account avalible!-----------\n");
						strcat(sendBuffer, "    Enter your password: ");
						accountMap[cppTempString] = new User(newAccount);
					} else {
						strcpy(sendBuffer, "--------------Account used!-------------\n");
						strcat(sendBuffer, "    Please enter another new account: ");
					}
					sendOne(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
				}
				else if(strcmp(status, "REGISTER_PASSWORD") == 0)
				{
					char newAccount[100];
					char newPassword[100];
					sscanf(recvBuffer, "REGISTER_PASSWORD %s %s", newAccount, newPassword);
					std::string cppTempString(newAccount);
					if(accountMap.find(cppTempString) == accountMap.end()) {
						fprintf(stderr, "No such account: %s\n", newAccount);
						exit(EXIT_FAILURE);
					}
					strcpy(accountMap.at(cppTempString)->password, newPassword);
					sprintf(sendBuffer, "    Enter a nickname: ");
					sendOne(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
				}
				else if(strcmp(status, "REGISTER_NICKNAME") == 0)
				{
					char newAccount[100];
					char newNickName[100];
					sscanf(recvBuffer, "REGISTER_NICKNAME %s %s", newAccount, newNickName);
					std::string cppTempString(newAccount);
					if(accountMap.find(cppTempString) == accountMap.end()) {
						fprintf(stderr, "No such account: %s\n", newAccount);
						exit(EXIT_FAILURE);
					}
					strcpy(accountMap.at(cppTempString)->nickname, newNickName);
					sprintf(sendBuffer, "    Enter your birthday( format: yyyy/mm/dd, EX: 1995/02/28 ): ");
					sendOne(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
				}
				else if(strcmp(status, "REGISTER_BIRTHDAY") == 0)
				{
					char newAccount[100];
					char newBirthday[100];
					sscanf(recvBuffer, "REGISTER_BIRTHDAY %s %s", newAccount, newBirthday);
					std::string cppTempString(newAccount);
					if(accountMap.find(cppTempString) == accountMap.end()) {
						fprintf(stderr, "No such account: %s\n", newAccount);
						exit(EXIT_FAILURE);
					}
					strcpy(accountMap.at(cppTempString)->birthday, newBirthday);
					sprintf(sendBuffer, "Regitster Success!\n");
					accountMap.at(cppTempString)->catWellcomeToBuffer(sendBuffer);	
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

	//printf("To send: %s", sendBuffer);
	//fflush(stdout);
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
			printf("RETRASMITION: %s\n", sendBuffer);
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

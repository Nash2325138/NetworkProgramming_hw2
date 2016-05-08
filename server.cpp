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

#include "User.h"
#include "Article.h"

#define MAXLINE 2048

char loginAccountString[MAXLINE];
char loginPasswordString[MAXLINE];
char wellcomeString[MAXLINE];

char mainMenuString[MAXLINE];
char articleMenuString[MAXLINE];
char chatMenuString[MAXLINE];
char searchMenuString[MAXLINE];

void sendAck(int udpfd, const struct sockaddr *cliaddr_ptr);
void sendOne(int udpfd, char* sendBuffer, const struct sockaddr *cliaddr_ptr);



void initialString()
{
	strcpy(mainMenuString, "[SP]Show Profile\n[MP]Modify Profile\n[SA]Show Article\n[E]nter article <article number>\n[A]dd article <title>\n[C]hat\n[S]earch\n[L]ogout\n[H]elp commands\n");
	strcpy(articleMenuString, "[L]ike\n[C]omment <\"put your command here\">\n[B]ack\n");
	strcpy(chatMenuString, "[LF]List Friends\n[LC]List Chat room\n[C]reate chat room\n[E]nter chat room <chat room number>\n[B]ack\n");
	strcpy(searchMenuString, "[N]ickname search <nickname>\n[A]ccount search <account>\n[B]ack\n");
	
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
	std::vector<Article *> articleList;
	int article_ID_counter = 0;

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

			if(strcmp(status, "ONLINE_WRITING") == 0)
			{
				char account[200];
				char partContent[MAXLINE-50];
				sscanf(recvBuffer, "ONLINE_WRITING %s %s", account, partContent);
				std::string cppAccount(account);
				accountMap.at(account)->catBufferdArticle(partContent);
				// needs not sending anything to client, just let client keep writing
			}
			else if(strcmp(status, "END_WRITING") == 0)
			{
				char account[200];
				sscanf(recvBuffer, "END_WRITING %s", account);
				std::string cppAccount(account);
				articleList.push_back(new Article( accountMap.at(cppAccount), &cliaddr_in, article_ID_counter++,
									accountMap.at(cppAccount)->ba_title, accountMap.at(cppAccount)->bufferdArticle) );
				
				sprintf(sendBuffer, "Your writing has been successfully pushed to the articles' list.\n");
				sendOne(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
			}
			else if(strcmp(status, "ONLINE_MAIN_MENU") == 0)
			{
				char command[200];
				char account[200];
				sscanf(recvBuffer, "ONLINE_MAIN_MENU %s %s", account, command);
				std::string cppAccount(account);
				if(strcmp(command, "SP") == 0)
				{
					sendBuffer[0] = '\0';
					accountMap.at(cppAccount)->catProfileToBuffer(sendBuffer);
				}
				else if(strcmp(command, "MP") == 0)
				{
					
				}
				else if(strcmp(command, "SA") == 0)
				{
					
				}
				else if(strcmp(command, "E") == 0)
				{
					
				}
				else if(strcmp(command, "A") == 0)
				{
					char title[200];
					sscanf(recvBuffer, "ONLINE_MAIN_MENU %*s %*s %s", title); // account, command, title
					accountMap.at(cppAccount)->newBufferedArticle(title);
					sprintf(sendBuffer, "You can start writing now:\n");
				}
				else if(strcmp(command, "C") == 0)
				{
					
				}
				else if(strcmp(command, "S") == 0)
				{
					
				}
				else if(strcmp(command, "L") == 0)
				{
					
				}
				else if(strcmp(command, "H") == 0)
				{
					sprintf(sendBuffer, "%s", mainMenuString);
				}
				else
				{
					sendBuffer[0] = '\0';
				}
				sendOne(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
			}
			else if(strcmp(status, "ONLINE_ARTICLE_MENU") == 0)
			{
				
			}
			else if(strcmp(status, "ONLINE_CHAT_MENU") == 0)
			{
				
			}
			else if(strcmp(status, "ONLINE_SEARCH_MENU") == 0)
			{
				
			}
			else // the user who sent this package has not logged in yet
			{
				if(strcmp(status, "NULL") == 0)
				{
					strcpy(sendBuffer, loginAccountString);
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
						sprintf(sendBuffer, "Successfully log in! (Send any message to continue.)\n");
					}	
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
					sprintf(sendBuffer, "Regitster Success! (Send any message to continue.)\n");
					//accountMap.at(cppTempString)->catWellcomeToBuffer(sendBuffer);
				}
				else if(strcmp(status, "LOGIN_COMPLETE") == 0)
				{
					char nowAccount[100];
					sscanf(recvBuffer, "LOGIN_COMPLETE %s", nowAccount);

					std::string cppTempString(nowAccount);
					if(accountMap.find(cppTempString) == accountMap.end()) {
						fprintf(stderr, "No such account: %s\n", nowAccount);
						exit(EXIT_FAILURE);
					}
					strcpy(sendBuffer, "\n");
					accountMap.at(cppTempString)->catWellcomeToBuffer(sendBuffer);
				}
				sendOne(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
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

void sendOne(int udpfd, char* sendBuffer, const struct sockaddr *cliaddr_ptr)
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
void transFileTo(int udpfd, const struct sockaddr *cliaddr_ptr, FILE *fp, int fileSize)
{
	char sendline[MAXLINE];
	int numBytes;
	while(fileSize > 0)
	{
		numBytes = fread(sendline, sizeof(char), MAXLINE, fp);
		sendOne(udpfd, sendline, cliaddr_ptr);
		fileSize -= numBytes;

		//fprintf(stdout, "!!!\n%s\n!!!", sendline);
		//fprintf(stdout, "%d\n", numBytes);
	}
	//fprintf(stdout, "transfer finish\n");
}
void receiveFileFrom(int udpfd, const struct sockaddr *cliaddr_ptr, FILE *fp, int fileSize)
{
	int numBytes;
	char recvBuffer[MAXLINE+1];
	while(fileSize > 0)
	{
		numBytes = recvfrom(udpfd, recvBuffer, MAXLINE, 0, NULL, NULL);
		sendAck(udpfd, cliaddr_ptr);
		numBytes = fwrite(recvBuffer, sizeof(char), numBytes, fp);
		fileSize -= numBytes;
		//fprintf(stdout, "!!!\n%s\n!!!", recvline);
		//fprintf(stdout, "%d\n", numBytes);

	}
	//fprintf(stdout, "receive finish\n");
}
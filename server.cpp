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

#include <assert.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "User.h"
#include "Article.h"

#define MAXLINE 2048

char loginAccountString[MAXLINE];
char loginPasswordString[MAXLINE];
char wellcomeString[MAXLINE*20];

char mainMenuString[MAXLINE];
char articleMenuString[MAXLINE];
char chatMenuString[MAXLINE];
char searchMenuString[MAXLINE];

void sendAck(int udpfd, const struct sockaddr *cliaddr_ptr);
void sendOne(int udpfd, char* sendBuffer, const struct sockaddr *cliaddr_ptr);
void sendHuge(int udpfd, char* sendBuffer, const struct sockaddr *cliaddr_ptr);


void initialString()
{
	strcpy(mainMenuString, "  [SP]Show Profile\n  [MP]Modify Profile <[P]assword / [N]ickname / [B]irthday> <new content>\n  [D]elete account\n  [SA]Show Article\n  [E]nter article <article number>\n  [A]dd article <title>\n  [C]hat\n  [S]earch\n  [L]ogout\n  [H]elp commands\n");
	strcpy(articleMenuString, "  [L]ike\n  [W]ho likes the article\n  [C]omment <\"put your comment here\">\n  [EA]Edit Article\n  [DA_sure]Delete Article\n  [EC]Edit Comment <number> <content>\n  [DC]Delete Command <number>\n  [H]elp\n  [B]ack\n");
	strcpy(chatMenuString, "  [LF]List Friends\n  [LC]List Chat room\n  [C]reate chat room\n  [E]nter chat room <chat room number>\n  [B]ack\n");
	strcpy(searchMenuString, "  [N]ickname search <nickname>\n  [A]ccount search <account>\n  [LR]List Request\n  [AR]Accept Request <account>\n  [DR]Delete Request <account>\n  [RF]Remove Friend <account>\n  [B]ack\n");
	
	strcpy(loginAccountString, "Enter your account( or enter \"new\" to register ): ");
	strcpy(loginPasswordString, "Enter your password: ");

	strcpy(wellcomeString, "");
	FILE * ascii = fopen("wellcome_ASCII.txt", "r");
	char buffer[1024];
	while( fgets(buffer, 1024, ascii) != NULL ){
		strcat(wellcomeString, buffer);
	}
	fclose(ascii);
}


int main(int argc, char **argv)
{
	setbuf(stdout, NULL);
	char recvBuffer[MAXLINE];
	char sendBuffer[MAXLINE*2000];

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
			char command[200];
			char account[200];
			command[0] = '\0';
			if(strcmp(status, "ONLINE_WRITING") == 0)
			{
				sscanf(recvBuffer, "ONLINE_WRITING %s", account);
				std::string cppAccount(account);
				accountMap.at(cppAccount)->catBufferdArticle(recvBuffer + strlen("ONLINE_WRITING") + strlen(account) + 2);
				// needs not sending anything to client, just let client keep writing
			}
			else if(strcmp(status, "END_WRITING") == 0)
			{
				sscanf(recvBuffer, "END_WRITING %s", account);
				std::string cppAccount(account);
				articleList.push_back(new Article( accountMap.at(cppAccount), &cliaddr_in, article_ID_counter++,
									accountMap.at(cppAccount)->ba_title, accountMap.at(cppAccount)->bufferdArticle) );
				
				sprintf(sendBuffer, "Your writing has been successfully pushed to the articles' list.\n");
				sendHuge(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
			}
			else if(strcmp(status, "ONLINE_RE_WRITING") == 0)
			{
				char articleID[20];
				sscanf(recvBuffer, "ONLINE_RE_WRITING %s %s", account, articleID);
				std::string cppAccount(account);
				accountMap.at(cppAccount)->catBufferdArticle(recvBuffer + strlen("ONLINE_RE_WRITING") + strlen(account) + strlen(articleID) + 3);
				// needs not sending anything to client, just let client keep writing
			}
			else if(strcmp(status, "END_RE_WRITING") == 0)
			{
				int articleID;
				sscanf(recvBuffer, "END_WRITING %s %d", account, &articleID);
				std::string cppAccount(account);
				
				unsigned int i;
				for(i=0 ; i<articleList.size() ; i++) {
					if(articleList[i]->uniquedID == articleID) break;
				}
				if(i==articleList.size()) {
					sprintf(sendBuffer, "no such articleID\n");
				} else {
					articleList[i]->edit(&cliaddr_in, accountMap.at(cppAccount)->bufferdArticle);
					sprintf(sendBuffer, "Your re_writing has been successfully saved.\n");
				}

				sendHuge(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
			}
			else if(strcmp(status, "ONLINE_MAIN_MENU") == 0)
			{
				// [SP]Show Profile [MP]Modify Profile <[P]assword / [N]ickname / [B]irthday> <new content> [D]elete account [SA]Show Article [E]nter article <article number>
				// [A]dd article <title> [C]hat [S]earch [L]ogout [H]elp commands
				
				sscanf(recvBuffer, "ONLINE_MAIN_MENU %s %s", account, command);
				std::string cppAccount(account);
				if(strcmp(command, "SP") == 0)
				{
					sendBuffer[0] = '\0';
					accountMap.at(cppAccount)->catProfileToBuffer(sendBuffer);
				}
				else if(strcmp(command, "MP") == 0)
				{
					char target;
					char content[200];
					content[0] = '\0';
					sscanf(recvBuffer, "ONLINE_MAIN_MENU %*s %*s %c %s", &target, content);
					if(strlen(content)==0) sprintf(sendBuffer, "Please enter something after '%c'\n", target);
					else {
						bool valid = true;
						switch(target) {
							case'P': strcpy( accountMap.at(cppAccount)->password, content ); break;
							case'N': strcpy( accountMap.at(cppAccount)->nickname, content ); break;
							case'B': strcpy( accountMap.at(cppAccount)->birthday, content ); break;
							default: valid = false;
						}
						if(valid) sprintf(sendBuffer, "Your personal profile has been successfully updated!\n");
						else sprintf(sendBuffer, "There's no option '%c' to modify your profile\n", target);
					}

				}
				else if(strcmp(command, "D") == 0)
				{
					char sure[200];
					sure[0] = '\0';
					sscanf(recvBuffer, "ONLINE_MAIN_MENU %*s %*s %s", sure);
					if(strcmp(sure, "sure")==0) {
						sprintf(sendBuffer, "Deleting...");
						delete accountMap.at(cppAccount);
						accountMap.erase(cppAccount);
						strcat(sendBuffer, "finished.\n");
						strcat(sendBuffer, loginAccountString);
					}
					else sprintf(sendBuffer, "type \"D sure\" to make sure you are going to delete this account.\n");
				}
				else if(strcmp(command, "SA") == 0)
				{
					sprintf(sendBuffer, "        ID|                Title|          Author|                       Time|\n");
					for(unsigned int i=0 ; i<articleList.size() ; i++)
					{
						char temp[500];
						sprintf(temp, "%10d %21s %16s %28s", articleList[i]->uniquedID, articleList[i]->title,
								 articleList[i]->author->account, asctime(&articleList[i]->published_time));
						strcat(sendBuffer, temp);
					}
				}
				else if(strcmp(command, "E") == 0)
				{
					int desired_ID;
					sscanf(recvBuffer, "ONLINE_MAIN_MENU %*s %*s %d", &desired_ID);
					unsigned int i;
					for(i=0 ; i<articleList.size() ; i++)
					{
						if(articleList[i]->uniquedID == desired_ID) break;
					}
					if(i == articleList.size()) {
						sprintf(sendBuffer, "    No such article whose ID == %d\n", desired_ID);
					}
					else {
						sendBuffer[0] = '\0';
						articleList[i]->catArticleTobuffer(sendBuffer);
						strcat(sendBuffer, articleMenuString);
					}

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
					strcpy(sendBuffer, chatMenuString);
				}
				else if(strcmp(command, "S") == 0)
				{
					strcpy(sendBuffer, searchMenuString);
				}
				else if(strcmp(command, "L") == 0)
				{
					sprintf(sendBuffer, "Good Bye~\nEnter your account( or enter \"new\" to register ): ");	
					accountMap.at(cppAccount)->state = OFFLINE;
				}
				else if(strcmp(command, "H") == 0 || strcmp(command, "Back") == 0)
				{
					sprintf(sendBuffer, "%s", mainMenuString);
				}
				else
				{
					sprintf(sendBuffer, "Invalid command: %s\n", command);
					strcat(sendBuffer, mainMenuString);
				}
				sendHuge(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
			}
			else if(strcmp(status, "ONLINE_ARTICLE_MENU") == 0)
			{
				// [L]ike [W]ho likes the article [C]omment <\"put your command here\"> [EA]Edit Article  [DA]Delete Article
				// [EC]Edit Comment <number> <content>  [DC]Delete Command <number>  [H]elp [B]ack
				char desired_articleID[20];
				unsigned int i;
				sscanf(recvBuffer, "ONLINE_ARTICLE_MENU %s %s %s", account, desired_articleID, command);
				for(i=0 ; i<articleList.size() ; i++) {
					if(articleList[i]->uniquedID == atoi(desired_articleID) ) break;
				}
				if(i == articleList.size()) {
					sprintf(sendBuffer, "    No such article whose ID == %s\n", desired_articleID);
				}
				else {
					std::string cppAccount(account);
					if(strcmp(command, "L") == 0) {
						articleList[i]->addLiker( accountMap.at(cppAccount) );
						strcpy(sendBuffer, "\n\n\n\n\n\n\n\n\n");
						articleList[i]->catArticleTobuffer(sendBuffer);
					}
					else if(strcmp(command, "W") == 0) {
						sendBuffer[0] = '\0';
						articleList[i]->catLikers( sendBuffer );
					}
					else if(strcmp(command, "C") == 0) {
						articleList[i]->addComment( accountMap.at(cppAccount), 
							recvBuffer + strlen("ONLINE_ARTICLE_MENU") + 4 + strlen(account) + strlen(desired_articleID) + strlen(command));

						strcpy(sendBuffer, "\n\n\n");
						articleList[i]->catArticleTobuffer(sendBuffer);
					}
					else if(strcmp(command, "EA") == 0) {
						if(strcmp(articleList[i]->author->account, account) == 0 ) {
							accountMap.at(cppAccount)->cleanBufferedArticle();
							sprintf(sendBuffer, "You can start writing now:\n");
						} else {
							sprintf(sendBuffer, "Permission dinied\n");
						}
					}
					else if(strcmp(command, "DA_sure") == 0) {
						if(strcmp(articleList[i]->author->account, account) == 0 ) {
							delete articleList[i];
							articleList.erase( articleList.begin() + i );
							sprintf(sendBuffer, "Successfully delete article\n");
						} else {
							sprintf(sendBuffer, "Permission dinied\n");
						}
					}
					else if(strcmp(command, "EC") == 0) {
						// [EC]Edit Comment <number> <content>
						char numberString[20];
						int number;
						sscanf(recvBuffer, "%*s %*s %*s %*s %s", numberString);
						number = atoi(numberString);
						char *getStart = recvBuffer + strlen("ONLINE_ARTICLE_MENU") + 5 
								  + strlen(account) + strlen(desired_articleID) + strlen(command) + strlen(numberString);

						if(number > (int)articleList[i]->comments.size() || number < 0) {
							sprintf(sendBuffer, "No such comment number: %d\n", number);
						} else if( strcmp( articleList[i]->comments[number-1]->author->account, account) != 0) {
							sprintf(sendBuffer, "Permission denied.\n");
						} else {
							strcpy( articleList[i]->comments[number-1]->content, getStart);
							articleList[i]->comments[number-1]->content[strlen(getStart)-1] = '\0';
							sprintf(sendBuffer, "Comment successfully modified.\n");
						}
					}
					else if(strcmp(command, "DC") == 0) {
						// [DC]Delete Command <number>
						int number;
						sscanf(recvBuffer, "%*s %*s %*s %*s %d", &number);
						
						if(number > (int)articleList[i]->comments.size() || number < 0) {
							sprintf(sendBuffer, "No such comment number: %d\n", number);
						} else if( strcmp( articleList[i]->comments[number-1]->author->account, account) != 0) {
							sprintf(sendBuffer, "Permission denied.\n");
						} else {
							articleList[i]->comments.erase(articleList[i]->comments.begin() + number - 1);
							sprintf(sendBuffer, "Comment successfully deleted.\n");
						}
					}
					else if(strcmp(command, "H") == 0) {
						strcpy(sendBuffer, articleMenuString);
					}
					else if(strcmp(command, "B") == 0) { // never use
						strcpy(sendBuffer, mainMenuString);
					}
					else {
						sprintf(sendBuffer, "Invalid command: %s\n", command);
						strcat(sendBuffer, articleMenuString);
					}
				}
				sendHuge(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
			}
			else if(strcmp(status, "ONLINE_CHAT_MENU") == 0)
			{
				// [LF]List Friends [LC]List Chat room [C]reate chat room [E]nter chat room <chat room number> [B]ack
				
				sscanf(recvBuffer, "ONLINE_CHAT_MENU %s %s", account, command);
				std::string cppAccount(account);
				if(strcmp(command, "LF") == 0) {

				}
				else if(strcmp(command, "LC") == 0) {

				}
				else if(strcmp(command, "C") == 0) {
					
				}
				else if(strcmp(command, "E") == 0) {
					
				}
				else if(strcmp(command, "B") == 0) {
					
				}
				else {
					sprintf(sendBuffer, "Invalid command: %s\n", command);
					strcat(sendBuffer, chatMenuString);
				}
				sendHuge(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
			}
			else if(strcmp(status, "ONLINE_SEARCH_MENU") == 0)
			{
				// [N]ickname search <nickname> [A]ccount search <account> [AF]Add Friend <account> [B]ack
				// [LR]List Request [AR]Accept Request <account> [DR]Delete Request <account> [RF]Remove Friend <account>
				sscanf(recvBuffer, "ONLINE_SEARCH_MENU %s %s", account, command);
				std::string cppAccount(account);
				
				if(strcmp(command, "LR") == 0) {

				}
				else {
					char target[200];
					sscanf(recvBuffer, "%*s %*s %*s %s", target);
					std::map<std::string, User *>::iterator iter;
					sendBuffer[0] = '\0';
					if(strcmp(command, "N") == 0) {
						strcpy(sendBuffer, "Search result: ");
						for( iter = accountMap.begin() ; iter != accountMap.end() ; iter++) {
							if(strncmp( iter->second->nickname, target, strlen(target) ) == 0) {
								strcat(sendBuffer, iter->second->nickname);
								strcat(sendBuffer, " ");
							}
						}
					}
					else if(strcmp(command, "A") == 0) {
						strcpy(sendBuffer, "Search result: ");
						for( iter = accountMap.begin() ; iter != accountMap.end() ; iter++) {
							if(strncmp( iter->second->account, target, strlen(target) ) == 0) {
								strcat(sendBuffer, iter->second->account);
								strcat(sendBuffer, " ");
							}
						}
					}
					else if(strcmp(command, "AF") == 0) {

					}
					else if(strcmp(command, "B") == 0) {

					}
					else if(strcmp(command, "AR") == 0) {

					}
					else if(strcmp(command, "DR") == 0) {

					}
					else if(strcmp(command, "RF") == 0) {

					}
					else {
						sprintf(sendBuffer, "Invalid command: %s\n", command);
						strcat(sendBuffer, searchMenuString);
					}
				}
				sendHuge(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
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
						sprintf(sendBuffer, "Successfully log in!\n");
						accountMap.at(accountCppString)->catWellcomeToBuffer(sendBuffer);
						accountMap.at(accountCppString)->update_connectionInformation(&cliaddr_in);
						strcat(sendBuffer, "(Send any message to continue.)");
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
					sprintf(sendBuffer, "Regitster Success!\n");
					accountMap.at(cppTempString)->catWellcomeToBuffer(sendBuffer);
					accountMap.at(cppTempString)->update_connectionInformation(&cliaddr_in);
					strcat(sendBuffer, "(Send any message to continue.)");
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
					strcat(sendBuffer, mainMenuString);
				}
				sendHuge(udpfd, sendBuffer, (struct sockaddr *)&cliaddr_in);
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
	//fflush(stdout);
	//printf("before: %s", mainMenuString);
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
void sendHuge(int udpfd, char* sendBuffer, const struct sockaddr *cliaddr_ptr)
{
	int length = strlen(sendBuffer);
	int times = length/MAXLINE;
	if( length % MAXLINE != 0 )times++;

	//printf("times: %d\n", times);
	if(times <= 1) {
		assert(length < MAXLINE);
		sendOne(udpfd, sendBuffer, cliaddr_ptr);
	}
	else {
		char temp[MAXLINE+1];
		sprintf(temp, "Going to sendHuge: %d", times);
		sendOne(udpfd, temp, cliaddr_ptr);
		for(int i=0 ; i<times ; i++) {
			memmove( temp, sendBuffer + i*MAXLINE, MAXLINE);
			temp[MAXLINE] = '\0';
			sendOne(udpfd, temp, cliaddr_ptr);
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
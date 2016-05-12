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
	LOGIN_COMPLETE,

	ONLINE_MAIN_MENU,
	ONLINE_ARTICLE_MENU,
	ONLINE_CHAT_MENU,
	ONLINE_SEARCH_MENU,
	ONLINE_WRITING
}State;

bool isStateOnline(State state)
{
	if(state==ONLINE_MAIN_MENU || state==ONLINE_ARTICLE_MENU || state==ONLINE_CHAT_MENU
		|| state==ONLINE_SEARCH_MENU || state==ONLINE_WRITING) return true;
	return false;
}
int getFileSize(FILE *file)
{
	int ans;
	fseek(file, 0L, SEEK_END);
	ans = ftell(file);
	rewind(file);
	return ans;
}

State state;
char account[MAXLINE];
//void dg_cli(FILE *fp, int udpfd, const struct sockaddr *servaddr_ptr, socklen_t servlen);
//void tcp_cli(FILE *fp, int serverfd);

void sendOne(int udpfd, const struct sockaddr *servaddr_ptr, char *command);
void receive_print(int udpfd, const struct sockaddr *servaddr_ptr, char *recvBuffer);
void sendAck(int udpfd, const struct sockaddr *servaddr_ptr);
void transFileTo(int udpfd, const struct sockaddr *servaddr_ptr, FILE *fp, int fileSize);
void receiveFileFrom(int udpfd, const struct sockaddr *servaddr_ptr, FILE *fp, int fileSize);
void login_changeStage(State *state, char *recvBuffer);
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
	sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
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
			if(isStateOnline(state))	// already online!
			{
				if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error");
				if(state == ONLINE_WRITING)
				{
					char typingBuffer[MAXLINE-50];
					while(true)
					{
						if(feof(stdin)) break;
						fgets(typingBuffer, sizeof(typingBuffer), stdin);
						if(feof(stdin)) break;
						sprintf(sendBuffer, "ONLINE_WRITING %s %s", account, typingBuffer);
						sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
					}
					printf("break out\n");
					sprintf(sendBuffer, "END_WRITING %s", account);
					state = ONLINE_MAIN_MENU;

					sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
					continue;
				}


				if(state == ONLINE_MAIN_MENU)
				{
					// [SP]Show Profile [MP]Modify Profile [D]elete account [SA]Show Article [E]nter article <article number>
					// [A]dd article <title> [C]hat [S]earch [L]ogout [H]elp commands
					
					// get input line from stdin
					if(fgets(temp, sizeof(temp), stdin) == NULL) perror("fgets error:");
					temp[strlen(temp)] = '\0';	// replace '\n'

					// send message
					sprintf(sendBuffer, "ONLINE_MAIN_MENU %s %s", account, temp);
					
					// state change
					char command[100];
					sscanf(temp, " %s", command);
					if(strcmp(command, "SP") == 0) {
						
					}
					else if(strcmp(command, "MP") == 0)
					{
						
					}
					else if(strcmp(command, "D") == 0)
					{
						char sure[200];
						sure[0] = '\0';
						sscanf(temp, " %*s %s", sure);
						if(strcmp(sure, "sure") == 0) state = INIT;
					}
					else if(strcmp(command, "SA") == 0) {

					}
					else if(strcmp(command, "E") == 0) {
						state = ONLINE_ARTICLE_MENU;
					}
					else if(strcmp(command, "A") == 0) {
						state = ONLINE_WRITING;
					}
					else if(strcmp(command, "C") == 0) {
						state = ONLINE_CHAT_MENU;
					}
					else if(strcmp(command, "S") == 0) {
						state = ONLINE_SEARCH_MENU;
					}
					else if(strcmp(command, "L") == 0) {
						state = INIT;
					}
					else
					{

					}
					sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);

				}
				else if(state == ONLINE_ARTICLE_MENU)
				{
					// exception
					if(strncmp(recvBuffer, "    No such article", strlen("    No such article")) == 0){
						state = ONLINE_MAIN_MENU;
						sprintf(sendBuffer, "ONLINE_MAIN_MENU %s Back", account);
						sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
						continue;
					}

					// [L]ike [C]omment <\"put your command here\">  [B]ack

					// get input line from stdin
					fgets(temp, sizeof(temp), stdin);
					temp[strlen(temp)] = '\0';	// replace '\n'
					sprintf(sendBuffer, "ONLINE_ARTICLE_MENU %s %s", account, temp);
					
					// state change
					char command[100];
					sscanf(temp, " %s", command);
					if(strcmp(command, "L") == 0) {

					}
					else if(strcmp(command, "C") == 0) {

					}
					else if(strcmp(command, "B") == 0) {
						state = ONLINE_MAIN_MENU;
						sprintf(sendBuffer, "ONLINE_MAIN_MENU %s %s", account, "Back");
					}
					else {

					}

					sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
				}
				else if(state == ONLINE_CHAT_MENU)
				{
					// [LF]List Friends [LC]List Chat room [C]reate chat room [E]nter chat room <chat room number> [B]ack

					// get input line from stdin
					if(fgets(temp, sizeof(temp), stdin) == NULL) perror("fgets error:");
					temp[strlen(temp)] = '\0';	// replace '\n'
					sprintf(sendBuffer, "ONLINE_CHAT_MENU %s %s", account, temp);

					// state change
					char command[100];
					sscanf(temp, " %s", command);
					if(strcmp(command, "LF") == 0) {
					}
					else if(strcmp(command, "LC") == 0) {

					}
					else if(strcmp(command, "C") == 0) {
						
					}
					else if(strcmp(command, "E") == 0) {
						
					}
					else if(strcmp(command, "B") == 0) {
						state = ONLINE_MAIN_MENU;
						sprintf(sendBuffer, "ONLINE_MAIN_MENU %s %s", account, "Back");
					}
					else {
					}

					sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
				}
				else if(state == ONLINE_SEARCH_MENU)
				{
					// [N]ickname search <nickname> [A]ccount search <account> [F]riend <account> [B]ack

					// get input line from stdin
					if(fgets(temp, sizeof(temp), stdin) == NULL) perror("fgets error:");
					temp[strlen(temp)] = '\0';	// replace '\n'
					sprintf(sendBuffer, "ONLINE_SEARCH_MENU %s %s", account, temp);

					// state change
					char command[100];
					sscanf(temp, " %s", command);
					if(strcmp(command, "N") == 0) {

					}
					else if(strcmp(command, "A") == 0) {

					}
					else if(strcmp(command, "F") == 0) {

					}
					else if(strcmp(command, "B") == 0) {
						state = ONLINE_MAIN_MENU;
						sprintf(sendBuffer, "ONLINE_MAIN_MENU %s %s", account, "Back");
					}
					else {

					}
					sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
				}
			}
			else // haven't login
			{
				// put message from server
				if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error"); 
				
				// scanf input from user and store in temp[]
				if(fgets(temp, sizeof temp, stdin) == NULL) perror("fgets error:");
				temp[strlen(temp)-1] = '\0';
				
				// change state depending on message received from server 
				login_changeStage(&state, recvBuffer);

				if(state == LOGIN_ACCOUNT)
				{
					strcpy(account, temp);	// suppose log in successfully
					sprintf(sendBuffer, "LOGIN_ACCOUNT %s", temp);
				}
				else if(state == LOGIN_PASSWORD)
				{
					sprintf(sendBuffer, "LOGIN_PASSWORD %s %s", account, temp);
				}
				else if(state == REGISTER_ACCOUNT)
				{
					strcpy(account, temp);	// if client want to create new account, override the account
					sprintf(sendBuffer, "REGISTER_ACCOUNT %s", temp);
				}
				else if(state == REGISTER_PASSWORD)
				{
					sprintf(sendBuffer, "REGISTER_PASSWORD %s %s", account, temp);
				}
				else if(state == REGISTER_NICKNAME)
				{
					sprintf(sendBuffer, "REGISTER_NICKNAME %s %s", account, temp);
				}
				else if(state == REGISTER_BIRTHDAY)
				{
					sprintf(sendBuffer, "REGISTER_BIRTHDAY %s %s", account, temp);
				}
				else if(state == LOGIN_COMPLETE)
				{
					sprintf(sendBuffer, "LOGIN_COMPLETE %s", account);
					state = ONLINE_MAIN_MENU;
				}
				sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
			}	
		}
	}

	return 0;
}

void sendOne(int udpfd, const struct sockaddr *servaddr_ptr, char *command)
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

void transFileTo(int udpfd, const struct sockaddr *servaddr_ptr, FILE *fp, int fileSize)
{
	char sendline[MAXLINE];
	int numBytes;
	while(fileSize > 0)
	{
		numBytes = fread(sendline, sizeof(char), MAXLINE, fp);
		sendOne(udpfd, servaddr_ptr, sendline);
		fileSize -= numBytes;

		//fprintf(stdout, "!!!\n%s\n!!!", sendline);
		//fprintf(stdout, "%d\n", numBytes);
	}
	//fprintf(stdout, "transfer finish\n");
}
void receiveFileFrom(int udpfd, const struct sockaddr *servaddr_ptr, FILE *fp, int fileSize)
{
	int numBytes;
	char recvBuffer[MAXLINE+1];
	while(fileSize > 0)
	{
		recvfrom(udpfd, recvBuffer, MAXLINE, 0, NULL, NULL);
		recvBuffer[strlen(recvBuffer)+1] = '\0';
		sendAck(udpfd, servaddr_ptr);
		numBytes = fwrite(recvBuffer, sizeof(char), numBytes, fp);
		fileSize -= numBytes;
		//fprintf(stdout, "!!!\n%s\n!!!", recvline);
		//fprintf(stdout, "%d\n", numBytes);

	}
	//fprintf(stdout, "receive finish\n");
}
void login_changeStage(State *state, char *recvBuffer)
{
	if(*state == INIT)
	{
		*state = LOGIN_ACCOUNT;
	}
	else if(*state == LOGIN_ACCOUNT)
	{
		if(strncmp(recvBuffer, "Create", 6) == 0) {
			*state = REGISTER_ACCOUNT;
		}
		else {
			*state = LOGIN_PASSWORD;
		}
	}
	else if(*state == LOGIN_PASSWORD)
	{
		if(strncmp(recvBuffer, "Fail", 4) == 0) {
			*state = LOGIN_ACCOUNT;
		} else {
			*state = LOGIN_COMPLETE;
		}
	}
	else if(*state == REGISTER_ACCOUNT)
	{
		if(strncmp(recvBuffer, "------------Account avalible!-----------\n", strlen("------------Account avalible!-----------\n")) == 0) {
			*state = REGISTER_PASSWORD;
		} else if(strncmp(recvBuffer, "--------------Account used!-------------\n", strlen("--------------Account used!-------------\n")) == 0) {
		
		}
	}
	else if(*state == REGISTER_PASSWORD)
	{
		*state = REGISTER_NICKNAME;
	}
	else if(*state == REGISTER_NICKNAME)
	{
		*state = REGISTER_BIRTHDAY;
	}
	else if(*state == REGISTER_BIRTHDAY)
	{
		*state = LOGIN_COMPLETE;
	}
	else if(*state == LOGIN_COMPLETE)
	{
		// change state immediately when it finish sending message after state = LOGIN_COMPLETE
	}
}
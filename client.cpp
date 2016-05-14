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
	INIT = 0,
	LOGIN_ACCOUNT = 1,
	LOGIN_PASSWORD = 2,
	REGISTER_ACCOUNT = 3,
	REGISTER_PASSWORD = 4,
	REGISTER_NICKNAME = 5,
	REGISTER_BIRTHDAY = 6,
	LOGIN_COMPLETE = 7,

	ONLINE_MAIN_MENU = 8,
	ONLINE_ARTICLE_MENU = 9,
	ONLINE_CHAT_MENU = 10,
	ONLINE_CHAT_ROOM_MENU = 11,
	ONLINE_SEARCH_MENU = 12,
	ONLINE_WRITING = 13
}State;

bool isStateOnline(State state)
{
	if(state==ONLINE_MAIN_MENU || state==ONLINE_ARTICLE_MENU || state==ONLINE_CHAT_MENU
		|| state==ONLINE_SEARCH_MENU || state==ONLINE_WRITING || state==ONLINE_CHAT_ROOM_MENU) return true;
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
int articleID, chatRoomID;
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
	char recvBuffer[MAXLINE+1];
	char sendBuffer[MAXLINE];
	char temp[MAXLINE+1];
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
			
			//printf("state: %d\n", state);
			
			// first scan whether it's a huge send from server 
			if(strncmp(recvBuffer, "Going to sendHuge: ", strlen("Going to sendHuge: ")) == 0) {
				// if it is, record the times they will send
				int times, n;
				sscanf(recvBuffer, "Going to sendHuge: %d", &times);
				
				// and replace recvBuffer with second package
				n = recvfrom(servfd, recvBuffer, MAXLINE, 0, NULL, NULL);
				recvBuffer[n] = '\0';
				sendAck(servfd, (struct sockaddr *)&servaddr);
				if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error");
				
				for(int i=1 ; i<times ; i++) {
					n = recvfrom(servfd, temp, MAXLINE, 0, NULL, NULL);
					temp[n] = '\0';
					sendAck(servfd, (struct sockaddr *)&servaddr);

					if(fprintf(stdout, "%s", temp) == EOF) perror("fprintf error");
				}
				
			} else {
				// if it's not, just put message from server
				if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error");
			}

			if(isStateOnline(state))	// already online!
			{
				if(state == ONLINE_WRITING)
				{
					char typingBuffer[MAXLINE-50];
					while(true)
					{
						fgets(typingBuffer, sizeof(typingBuffer), stdin);
						if(feof(stdin)) break;
						sprintf(sendBuffer, "ONLINE_WRITING %s %s", account, typingBuffer);
						sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
					}
					clearerr(stdin);
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
						sscanf(temp, "E %d", &articleID);
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
					if(strncmp(recvBuffer, "    No such article", strlen("    No such article")) == 0) {
						state = ONLINE_MAIN_MENU;
						sprintf(sendBuffer, "ONLINE_MAIN_MENU %s Back", account);
						sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
						continue;
					}

					// [L]ike [W]ho likes the article [C]omment <\"put your command here\"> [EA]Edit Article  [DA]Delete Article
					// [EC]Edit Comment <number> <content>  [DC]Delete Command <number>  [H]elp  [B]ack

					// get input line from stdin
					fgets(temp, sizeof(temp), stdin);
					temp[strlen(temp)] = '\0';	// replace '\n'
					sprintf(sendBuffer, "ONLINE_ARTICLE_MENU %s %d %s", account, articleID, temp);
					
					// state change
					char command[100];
					sscanf(temp, " %s", command);
					if(strcmp(command, "L") == 0) {

					}
					else if(strcmp(command, "W") == 0) {

					}
					else if(strcmp(command, "C") == 0) {

					}
					// we need to know the reply of server to determine how to change state
					else if(strcmp(command, "EA") == 0) {
						sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
						int n = recvfrom(servfd, recvBuffer, MAXLINE, 0, NULL, NULL);
						recvBuffer[n] = '\0';
						sendAck(servfd, (struct sockaddr *)&servaddr);

						// display message from server
						if(fprintf(stdout, "%s", recvBuffer) == EOF) perror("fprintf error");

						// state change
						if(strncmp(recvBuffer, "Permission dinied", strlen("Permission dinied")) == 0) {
							sprintf(sendBuffer, "ONLINE_ARTICLE_MENU %s %d H", account, articleID);
						} else {
							char typingBuffer[MAXLINE-1];
							while(true)
							{
								fgets(typingBuffer, sizeof(typingBuffer), stdin);
								if(feof(stdin)) break;
								sprintf(sendBuffer, "ONLINE_RE_WRITING %s %d %s", account, articleID, typingBuffer);
								sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
							}
							clearerr(stdin);
							printf("break out\n");
							sprintf(sendBuffer, "END_RE_WRITING %s %d", account, articleID);
							state = ONLINE_ARTICLE_MENU;

							sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
							continue;
						}
						
					}
					else if(strcmp(command, "DA_sure") == 0) {
						state = ONLINE_MAIN_MENU;
					}
					else if(strcmp(command, "EC") == 0) {

					}
					else if(strcmp(command, "DC") == 0) {

					}
					else if(strcmp(command, "H") == 0) {

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
					temp[strlen(temp)-1] = '\0';	// replace '\n'
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
						state = ONLINE_CHAT_ROOM_MENU;
						sscanf(temp, " %*s %d", &chatRoomID);
					}
					else if(strcmp(command, "B") == 0) {
						state = ONLINE_MAIN_MENU;
						sprintf(sendBuffer, "ONLINE_MAIN_MENU %s %s", account, "Back");
					}
					else if(strcmp(command, "H") == 0) {
						
					}
					else {
					}

					sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
				}
				else if(state == ONLINE_CHAT_ROOM_MENU)
				{
					// exception
					if(    strncmp(recvBuffer, "    No such room", strlen("    No such room")) == 0
						|| strncmp(recvBuffer, "    You are not a member of this chat room", strlen("    You are not a member of this chat room")) == 0) {
						state = ONLINE_CHAT_MENU;
						sprintf(sendBuffer, "ONLINE_CHAT_MENU %s H", account);
						sendOne(servfd, (struct sockaddr *)&servaddr, sendBuffer);
						continue;
					}
					// [S]end message <message> [U]pdate chat room [A]dd account <account> [L]eave from this chat room [H]elp [B]ack

					// get input line from stdin
					if(fgets(temp, sizeof(temp), stdin) == NULL) perror("fgets error:");
					temp[strlen(temp)] = '\0';	// replace '\n'
					sprintf(sendBuffer, "ONLINE_CHAT_ROOM_MENU %s %d %s", account, chatRoomID, temp);

					// state change
					char command[100];
					sscanf(temp, " %s", command);
					if(strcmp(command, "S") == 0) {
					}
					else if(strcmp(command, "U") == 0) {

					}
					else if(strcmp(command, "A") == 0) {
						
					}
					else if(strcmp(command, "L") == 0) {
						
					}
					else if(strcmp(command, "B") == 0) {
						state = ONLINE_CHAT_MENU;
						sprintf(sendBuffer, "ONLINE_CHAT_MENU %s H", account);
					}
					else if(strcmp(command, "H") == 0) {
						
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
				
				// scanf input from user and store in temp[]
				if(fgets(temp, sizeof temp, stdin) == NULL) perror("fgets error:");
				temp[strlen(temp)-1] = '\0';
				
				// change state depending on message received from server 
				login_changeStage(&state, recvBuffer);

				if(state == LOGIN_ACCOUNT)
				{
					sscanf(temp, "%s", account);	// suppose log in successfully
					sprintf(sendBuffer, "LOGIN_ACCOUNT %s", temp);
				}
				else if(state == LOGIN_PASSWORD)
				{
					sprintf(sendBuffer, "LOGIN_PASSWORD %s %s", account, temp);
				}
				else if(state == REGISTER_ACCOUNT)
				{
					sscanf(temp, "%s", account);	// if client want to create new account, override the account
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
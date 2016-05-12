#ifndef USER_H
#define USER_H

#define MAXLINE 2048
typedef enum
{
	OFFLINE,
	ONLINE
}UserState;

extern char loginAccountString[MAXLINE];
extern char loginPasswordString[MAXLINE];
extern char wellcomeString[MAXLINE];

extern char mainMenuString[MAXLINE];
extern char articleMenuString[MAXLINE];
extern char chatMenuString[MAXLINE];
extern char searchMenuString[MAXLINE];

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
	struct sockaddr_in cliaddr_in;

	char bufferdArticle[200000];
	char ba_title[100];

	User(char *account)
	{
		strcpy(this->account, account);
		password[0] = '\0';
		birthday[0] = '\0';
		registerTime = lastLoginTime = NULL;
		state = OFFLINE;
	}
	void catWellcomeToBuffer(char *sendBuffer)
	{
		strcat(sendBuffer, wellcomeString);
		strcat(sendBuffer, "Hello ");
		strcat(sendBuffer, nickname);

		strcat(sendBuffer, "!\n");
		if(lastLoginTime != NULL) {
			strcat(sendBuffer, "Last time when you login: ");
			strcat(sendBuffer, asctime(lastLoginTime));
		}
		else {
			strcat(sendBuffer, "This is your first log in!");
		}
		strcat(sendBuffer, "\n");
		strcat(sendBuffer, mainMenuString);
		time_t rawtime;
		time(&rawtime);
		lastLoginTime = localtime(&rawtime);
		this->state = ONLINE;
	}
	void update_connectionInformation(struct sockaddr_in * _cliaddr_in)
	{
		this->cliaddr_in = (* _cliaddr_in);
	}
	void catProfileToBuffer(char *sendBuffer)
	{
		char temp[20000];
		snprintf(temp, sizeof(temp), "    Account: %s\n    Password: %s\n    Nickname: %s\n    Birthday: %s\n", account, password, nickname, birthday);
		strcat(sendBuffer, temp);
	}
	void newBufferedArticle(char *title)
	{
		strcpy(this->ba_title, title);
		bufferdArticle[0] = '\0';
	}
	void catBufferdArticle(char *typing)
	{
		strcat(bufferdArticle, typing);
	}
};

#endif
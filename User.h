#ifndef USER_H
#define USER_H

#define MAXLINE 2048
typedef enum {
	OFFLINE,
	ONLINE
} UserState;

#include <set>
#include <algorithm>

extern char loginAccountString[MAXLINE];
extern char loginPasswordString[MAXLINE];
extern char wellcomeString[MAXLINE*20];

extern char mainMenuString[MAXLINE];
extern char articleMenuString[MAXLINE];
extern char chatMenuString[MAXLINE];
extern char searchMenuString[MAXLINE];
extern char charRoomMenuString[MAXLINE];

class User
{
public:
	char account[100];
	char password[100];
	char nickname[100];
	char birthday[100];
	struct tm* registerTime;
	struct tm* lastLoginTime;
	struct tm* lastLogoutTime;
	UserState state;
	struct sockaddr_in cliaddr_in;

	char bufferdArticle[200000];
	char ba_title[100];

	std::set<User *> friends;
	std::set<User *> requests;

	User(char *account)
	{
		strcpy(this->account, account);
		password[0] = '\0';
		birthday[0] = '\0';
		registerTime = lastLoginTime = lastLogoutTime = NULL;
		state = OFFLINE;
	}
	~User()
	{
		delete registerTime;
		delete lastLoginTime;
		delete lastLogoutTime;
	}
	void catWellcomeToBuffer(char *sendBuffer)
	{
		time_t rawtime;
		time(&rawtime);

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
			lastLoginTime = (struct tm *)malloc(sizeof(struct tm));
			registerTime = (struct tm *)malloc(sizeof(struct tm));
			(*registerTime) = *(localtime(&rawtime));
		}
		strcat(sendBuffer, "\n");
		//strcat(sendBuffer, mainMenuString);

		(*lastLoginTime) = *(localtime(&rawtime));
		this->state = ONLINE;
	}
	void logOut()
	{
		time_t rawtime;
		time(&rawtime);	// rawtime == now time

		this->state = OFFLINE;
		if(lastLogoutTime == NULL) lastLogoutTime = (struct tm *)malloc(sizeof(struct tm));
		(*lastLogoutTime) = *(localtime(&rawtime));
		return;
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
		
		snprintf(temp, sizeof(temp), "    Register time: %s\n", asctime(registerTime));
		strcat(sendBuffer, temp);
	}
	void newBufferedArticle(char *title)
	{
		strcpy(this->ba_title, title);
		bufferdArticle[0] = '\0';
	}
	void cleanBufferedArticle()
	{
		bufferdArticle[0] = '\0';
	}
	void catBufferdArticle(char *typing)
	{
		strcat(bufferdArticle, typing);
	}
	void addRequest(User *requester)
	{
		//std::set<User *>::iterator iter = find(requests.begin(), requests.end(), requester);
		std::set<User *>::iterator iter = requests.find(requester);
		if(iter != requests.end()) return; // if already in 
		
		requests.insert(requester);
	}
	bool acceptRequest(User *requester) // return false if this account is not in requests
	{
		std::set<User *>::iterator iter = requests.find(requester);
		if(iter == requests.end()) return false;

		this->addFriend(requester);
		requests.erase(iter);
		return true;
	}
	void addFriend(User *target)
	{
		friends.insert(target);
	}
	bool removeRequest(User *requester)
	{
		std::set<User *>::iterator iter = requests.find(requester);
		if(iter == requests.end()) return false;

		requests.erase(iter);
		return true;
	}
	void catRequests(char *sendBuffer)
	{
		strcat(sendBuffer, "  Requesters:");
		std::set<User *>::iterator iter;
		char temp[400];
		for(iter = requests.begin() ; iter != requests.end() ; iter++) {
			sprintf(temp, " %s(%s)", (*iter)->nickname, (*iter)->account );
			strcat(sendBuffer, temp);
		}

		strcat(sendBuffer, "\n");
	}
	bool hasFriend(User *target)
	{
		if(friends.find(target) == friends.end()) return false;
		return true;
	}
	bool removeFriend(User *target)
	{
		std::set<User *>::iterator iter = friends.find(target);
		if(iter == friends.end()) return false;

		friends.erase(target);
		return true;
	}
	void catFriends(char *sendBuffer)
	{
		std::set<User *>::iterator iter;
		time_t rawtime;
		time(&rawtime);	// get now time
		char temp[500];
		char temptemp[500];
		strcat(sendBuffer, "            Nickname|             Account|                    State|\n");
		for(iter = friends.begin() ; iter != friends.end() ; iter++) {
			if( (*iter)->state == ONLINE ) {
				strcpy(temptemp, "ONLINE");
				sprintf(temp, "%20s|%20s|%25s|\n", (*iter)->nickname, (*iter)->account, temptemp);
			} else {

				double seconds = difftime(rawtime, mktime((*iter)->lastLogoutTime));
				int minutes = (int)(seconds/60);
				sprintf(temptemp, "OFFLINE for %d minutes", minutes);
				sprintf(temp, "%20s|%20s|%25s|\n", (*iter)->nickname, (*iter)->account, temptemp);
			}
			strcat(sendBuffer, temp);	
		}
	}
};

#endif
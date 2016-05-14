#ifndef CHATROOM_H
#define CHATROOM_H

#define MAXLINE 2048

#include <vector>
#include <set>

typedef struct Message {
	User *author;
	char content[500];
	Message(User *writer, char * _content) {
		author = writer;
		strcpy(content, _content);
		content[strlen(content)-1] = '\0';
	}
}Message;

class ChatRoom
{
public:
	char name[500];
	int roomID;
	std::set<User *> members;
	std::vector<Message *> messages;
	ChatRoom(int _roomID, char *name, User *creator)
	{
		this->roomID = _roomID;
		strcpy(this->name, name);
		members.insert(creator);
	}
	bool hasMember(User *target)
	{
		if(members.find(target) == members.end()) return false;
		return true;
	}
	void catMessages(char *sendBuffer)
	{
		char temp[600];
		sprintf(temp, "------------ Chat room: %s ------------\n", this->name);
		strcat(sendBuffer, temp);
		for(std::vector<Message *>::iterator iter = messages.begin() ; iter != messages.end() ; iter++) {
			sprintf(temp, "  %s: %s\n", (*iter)->author->account, (*iter)->content);
		}
	}
	void addMessage(User *writer, char *content)
	{
		messages.push_back(new Message(writer, content));
	}
	void addMember(User *target)
	{
		members.insert(target);
	}
	void removeMember(User *target)
	{
		members.erase(target);
	}
};


#endif
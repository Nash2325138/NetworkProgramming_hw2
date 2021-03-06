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
	~ChatRoom()
	{
		for(std::vector<Message *>::iterator iter = messages.begin() ; iter != messages.end() ; iter ++) {
			delete (*iter);
		}
	}
	bool hasMember(User *target)
	{
		if(members.find(target) == members.end()) return false;
		return true;
	}
	void catMembers(char *sendBuffer)
	{
		char temp[600];
		strcat(sendBuffer, "This Chat room has members: ");
		for(std::set<User *>::iterator iter = members.begin() ; iter != members.end() ; iter++) {
			sprintf(temp, "  %s(%s)", (*iter)->nickname, (*iter)->account );
			strcat(sendBuffer, temp);
		}
		strcat(sendBuffer, "\n");
	}
	void catMessages(char *sendBuffer)
	{
		char temp[600];
		sprintf(temp, "------------ Chat room: %s ------------\n", this->name);
		strcat(sendBuffer, temp);
		for(std::vector<Message *>::iterator iter = messages.begin() ; iter != messages.end() ; iter++) {
			sprintf(temp, "  %s: %s\n", (*iter)->author->account, (*iter)->content);
			strcat(sendBuffer, temp);
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
	void removeAccountAllMessage(User *target)
	{
		for(unsigned int i=0 ; i<messages.size() ; i++) {
			if(messages[i]->author == target) {
				messages.erase(messages.begin() + i);
				i--;
			}
		}
	}
};


#endif
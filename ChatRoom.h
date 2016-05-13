#ifndef CHATROOM_H
#define CHATROOM_H

#define MAXLINE 2048

#include <vector>
#include <set>

typedef struct Message {
	User *author;
	char content[500];
	Message(User *chatter, char * _content) {
		author = chatter;
		strcpy(content, _content);
		content[strlen(content)-1] = '\0';
	}
}Message;

class ChatRoom
{
public:
	int roomID;
	std::set<User *> members;
	std::vector<Message> messages;
};


#endif
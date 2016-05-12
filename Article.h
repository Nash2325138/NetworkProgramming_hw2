#ifndef ARTICLE_H
#define ARTICLE_H

#define MAXLINE 2048

#include <set>
#include <vector>
#include <cstring>

extern char loginAccountString[MAXLINE];
extern char loginPasswordString[MAXLINE];
extern char wellcomeString[MAXLINE*20];

extern char mainMenuString[MAXLINE];
extern char articleMenuString[MAXLINE];
extern char chatMenuString[MAXLINE];
extern char searchMenuString[MAXLINE];
typedef struct Comment {
	User *author;
	char content[500];
	Comment(User *commenter, char * _content) {
		author = commenter;
		strcpy(content, _content);
		content[strlen(content)-1] = '\0';
	}
}Comment;

class Article
{
public:
	int uniquedID;
	
	char title[200];
	char content[200000];

	User *author;
	char published_IP[INET_ADDRSTRLEN];
	int published_port;
	struct tm published_time;

	std::set<User *> likers;
	std::vector<Comment *> comments;

	Article(User *_author, struct sockaddr_in * cliaddr_in, int ID_counter, char * _title, char * _content)
	{
		uniquedID = ID_counter;

		if( inet_ntop(AF_INET, &(cliaddr_in->sin_addr), published_IP, INET_ADDRSTRLEN) <= 0) perror("inet_ntop error");
		published_port = ntohs(cliaddr_in->sin_port);
		
		this->author = _author;
		strcpy(this->content, _content);
		strcpy(this->title, _title);

		time_t rawtime;
		time(&rawtime);
		published_time = *( localtime(&rawtime) );
	}
	void edit(struct sockaddr_in * cliaddr_in, char * _title, char * _content)
	{
		if( inet_ntop(AF_INET, &(cliaddr_in->sin_addr), published_IP, INET_ADDRSTRLEN) <= 0) perror("inet_ntop error");
		published_port = ntohs(cliaddr_in->sin_port);
		
		strcpy(this->content, _content);
		strcpy(this->title, _title);

		time_t rawtime;
		time(&rawtime);
		published_time = *( localtime(&rawtime) );
	}
	void catArticleTobuffer(char *sendBuffer)
	{
		// 
		char temp[30000];
		sprintf(temp, "~~~ Article ID: %d ~~~\n~~~ Title: %s ~~~\n~~~ IP:%s ~~~\n~~~ port:%d ~~~\n~~~ Content:\n%s\n", uniquedID, title, published_IP, published_port, content);
		strcat(sendBuffer, temp);

		sprintf(temp, "~~~ %lu people like this ~~~\n", likers.size());
		strcat(sendBuffer, temp);
		
		strcat(sendBuffer, "~~~ comments ~~~\n");
		for(unsigned int i=0 ; i<comments.size() ; i++) {
			sprintf(temp, "%u. %s: %s\n", i+1, comments[i]->author->account, comments[i]->content);
			strcat(sendBuffer, temp);
		}
		strcat(sendBuffer, "~~~~~~~~~~~~~~~~\n");
	}
	void addLiker(User *liker)
	{
		if(likers.find(liker) == likers.end()) {
			likers.insert(liker);
		} else {
			likers.erase(liker);	
		}	
	}
	void catLikers(char *sendBuffer)
	{
		char temp[500];
		strcat(sendBuffer, "likers: ");
		for(std::set<User *>::iterator it = likers.begin(); it != likers.end() ; it++) {
			sprintf(temp, "%s ", (*it)->account) ;
			strcat(sendBuffer, temp);
		}
	}
	void addComment(User *commenter, char *content)
	{
		comments.push_back(new Comment(commenter, content));
	}
};
#endif
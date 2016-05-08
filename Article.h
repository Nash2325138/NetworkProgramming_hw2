#ifndef ARTICLE_H
#define ARTICLE_H

#define MAXLINE 2048
extern char loginAccountString[MAXLINE];
extern char loginPasswordString[MAXLINE];
extern char wellcomeString[MAXLINE];

extern char mainMenuString[MAXLINE];
extern char articleMenuString[MAXLINE];
extern char chatMenuString[MAXLINE];
extern char searchMenuString[MAXLINE];
class Article
{
public:
	int uniquedID;
	
	char title[200];
	char content[1900];

	User *author;
	char published_IP[INET_ADDRSTRLEN];
	int published_port;
	struct tm *published_time;

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
		published_time = localtime(&rawtime);
	}
	void catArticleTobuffer(char *sendBuffer)
	{
		char temp[3000];
		sprintf(temp, "Article ID: %d\nTitle: %s\nIP:%s\nport:%d\nContent:\n%s\n", uniquedID, title, published_IP, published_port, content);
		strcat(sendBuffer, temp);
	}
};

#endif
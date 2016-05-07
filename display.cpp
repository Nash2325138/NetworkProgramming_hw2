#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
	FILE * ascii = fopen("wellcome_ASCII.txt", "r");
	char buffer[1024];
	while( fgets(buffer, 1024, ascii) != NULL )
		fputs(buffer, stdout);
	time_t rawtime;
  struct tm * timeinfo;

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  printf ("Current local time and date: %s", asctime(timeinfo));

  fclose(ascii);
	return 0;
}

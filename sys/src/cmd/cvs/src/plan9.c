#include "cvs.h"
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>


static int
readln(char *prompt, char *line, int len)
{
	char *p;
	int fd, ctl, n, nr;

	fd = open("/dev/cons", O_RDWR);
	if(fd < 0)
		return -1;
	ctl = open("/dev/consctl", O_WRONLY);
	if(ctl < 0) {
		close(fd);
		return -1;
	}
	write(ctl, "rawon", 5);
	printf("%s", prompt);
	fflush(stdout);
	nr = 0;
	p = line;
	for(;;){
		n = read(fd, p, 1);
		if(n < 0){
			close(fd);
			close(ctl);
			return -1;
		}
		if(n == 0 || *p == '\n' || *p == '\r'){
			*p = '\0';
			write(fd, "\n", 1);
			close(fd);
			close(ctl);
			return nr;
		}
		if(*p == '\b'){
			if(nr > 0){
				nr--;
				p--;
			}
		}else if(*p == 21){		/* cntrl-u */
			printf("\n%s", prompt);
			fflush(stdout);
			nr = 0;
			p = line;
		}else{
			nr++;
			p++;
		}
		if(nr == len){
			printf("line too long; try again\n%s", prompt);
			fflush(stdout);
			nr = 0;
			p = line;
		}
	}
	return -1;
}

char*
getpass(char *prompt)
{
	static char buf[80];

	if(readln(prompt, buf, sizeof(buf)) < 0)
		return NULL;
	return buf;
}

#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

void
getpass(char *key, int check)
{
	char pass[32], rpass[32];

	readln("Password: ", pass, sizeof pass);
	readln("Confirm password: ", rpass, sizeof rpass);
	if(strcmp(pass, rpass) != 0)
		error("password mismatch");
	if(!passtokey(key, pass) || check && !okpasswd(pass))
		error("bad password");
}

void
readln(char *prompt, char *line, int len)
{
	char *p;
	int fd, ctl, n, nr;

	fd = open("/dev/cons", ORDWR);
	if(fd < 0)
		error("couldn't open cons");
	ctl = open("/dev/consctl", OWRITE);
	if(ctl < 0)
		error("couldn't set raw mode");
	write(ctl, "rawon", 5);
	fprint(fd, "%s", prompt);
	nr = 0;
	p = line;
	for(;;){
		n = read(fd, p, 1);
		if(n < 0){
			close(fd);
			close(ctl);
			error("can't read cons\n");
		}
		if(n == 0 || *p == '\n' || *p == '\r'){
			*p = '\0';
			write(fd, "\n", 1);
			close(fd);
			close(ctl);
			return;
		}
		if(*p == '\b'){
			if(nr > 0){
				nr--;
				p--;
			}
		}else{
			nr++;
			p++;
		}
		if(nr == len){
			fprint(fd, "line too long; try again\n");
			nr = 0;
			p = line;
		}
	}
}

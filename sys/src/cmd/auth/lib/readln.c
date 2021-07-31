#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

void
getpass(char *key, int check)
{
	char pass[32], rpass[32];
	char *err;

	readln("Password: ", pass, sizeof pass, 1);
	readln("Confirm password: ", rpass, sizeof rpass, 1);
	if(strcmp(pass, rpass) != 0)
		error("password mismatch");
	if(!passtokey(key, pass))
		error("bad password");
	if(check)
		if(err = okpasswd(pass))
			error(err);
}

void
readln(char *prompt, char *line, int len, int raw)
{
	char *p;
	int fd, ctl, n, nr;

	fd = open("/dev/cons", ORDWR);
	if(fd < 0)
		error("couldn't open cons");
	if(raw){
		ctl = open("/dev/consctl", OWRITE);
		if(ctl < 0)
			error("couldn't set raw mode");
		write(ctl, "rawon", 5);
	} else
		ctl = -1;
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
		if(*p == 0x7f)
			exits(0);
		if(n == 0 || *p == '\n' || *p == '\r'){
			*p = '\0';
			if(raw)
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

#include <u.h>
#include <libc.h>
#include <authsrv.h>

int	readln(char*, char*, int);
void	error(char*, ...);
void	passwd(char*);

void
main(int argc, char *argv[])
{
	char np[64];

	argc--;
	argv0 = *argv;
	if(argc){
		fprint(2, "usage: typepasswd\n");
		exits("usage");
	}
	if(access("#i/draw", AEXIST) < 0){
		fprint(2, "typepasswd: must be run on a terminal\n");
		exits("run typepasswd on a terminal");
	}
	readln("New password:", np, sizeof np);
	passwd(np);
	exits(0);
}

void
passwd(char *p)
{
	char key[DESKEYLEN];
	int fd;

	if(passtokey(key, p) == 0)
		error("bad password");
	fd = open("/dev/key", OWRITE);
	if(fd < 0)
		error("can't open /dev/key");
	if(write(fd, key, DESKEYLEN) != DESKEYLEN)
		error("can't write key");
	close(fd);
	print("password changed\n");
}

int
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
			close(ctl);
			close(fd);
			return -1;
		}
		if(n == 0 || *p == '\n' || *p == '\r'){
			*p = '\0';
			write(fd, "\n", 1);
			close(ctl);
			close(fd);
			return nr;
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
			fprint(2, "line too long; try again\n");
			nr = 0;
			p = line;
		}
	}
	return -1;
}

void
error(char *fmt, ...)
{
	va_list arg;
	char buf[8192], *s;

	s = buf;
	s += sprint(s, "%s: ", argv0);
	va_start(arg, fmt);
	s = vseprint(s, buf + sizeof(buf) / sizeof(*buf), fmt, arg);
	va_end(arg);
	*s++ = '\n';
	write(2, buf, s - buf);
	exits(buf);
}

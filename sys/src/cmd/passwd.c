#include <u.h>
#include <libc.h>
#include <auth.h>

enum{
	CTLU		= 21,
	PASSLEN		= 10,
};

int	readln(char*, char*, int);
void	error(char*, ...);
void	passwd(char*, char*, char*);

void
main(int argc, char *argv[])
{
	char *u, op[64], np[64], rp[64];
	char statbuf[DIRLEN];

	argc--;
	argv0 = *argv++;
	u = 0;
	switch(argc){
	case 1:
		u = *argv;
		break;
	case 0:
		u = getuser();
		break;
	default:
		fprint(2, "usage: passwd [user]\n");
		exits("usage");
	}
	if(stat("#b/bitblt", statbuf) < 0){
		fprint(2, "passwd: must be run on a terminal\n");
		exits("run passwd on a terminal");
	}
	if(memchr(u, '\0', NAMELEN) == 0)
		error("bad user name");
	readln("Old password:", op, sizeof op);
	readln("New password:", np, sizeof np);
	readln("Confirm password:", rp, sizeof rp);
	if(strcmp(np, rp) != 0)
		error("password mismatch");
	passwd(u, op, np);
	exits(0);
}

void
passwd(char *user, char *op, char *np)
{
	char okey[DESKEYLEN], nkey[DESKEYLEN], chal[NAMELEN+AUTHLEN+2*PASSLEN], res[ERRLEN];
	int fd;

	fd = authdial("changekey");
	passtokey(okey, op);
	strncpy(chal, user, NAMELEN);
	chal[NAMELEN] = 1;
	if(read(fd, chal+NAMELEN+1, 7) != 7)
		error("can't read challenge from server");
	strncpy(&chal[NAMELEN+AUTHLEN], op, PASSLEN);
	strncpy(&chal[NAMELEN+AUTHLEN+PASSLEN], np, PASSLEN);
	encrypt(okey, chal+NAMELEN, AUTHLEN+2*PASSLEN);
	if(write(fd, chal, NAMELEN+AUTHLEN+2*PASSLEN) != NAMELEN+AUTHLEN+2*PASSLEN)
		error("can't send to server");
	if(read(fd, res, sizeof res) != sizeof res)
		error("bad reply from server");
	res[ERRLEN-1] = '\0';
	print("%s\n", res);
	if(strcmp(res, "password changed") == 0){
		passtokey(nkey, np);
		fd = open("/dev/key", OWRITE);
		if(fd < 0)
			error("can't open /dev/key");
		if(write(fd, nkey, DESKEYLEN) != DESKEYLEN)
			error("can't write key");
		close(fd);
		exits(0);
	}
	exits(res);
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
		}else if(*p == CTLU){
			fprint(fd, "\n%s", prompt);
			nr = 0;
			p = line;
		}else{
			nr++;
			p++;
		}
		if(nr == len){
			fprint(fd, "line too long; try again\n%s", prompt);
			nr = 0;
			p = line;
		}
	}
	return -1;		/*to shut compiler up */
}

void
error(char *fmt, ...)
{
	char buf[8192], *s;

	s = buf;
	s += sprint(s, "%s: ", argv0);
	s = doprint(s, buf + sizeof(buf) / sizeof(*buf), fmt, &fmt + 1);
	*s++ = '\n';
	write(2, buf, s - buf);
	exits(buf);
}

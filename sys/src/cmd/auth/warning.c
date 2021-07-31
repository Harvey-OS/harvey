#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include "authcmdlib.h"

/* working directory */
Dir	*dirbuf;
long	ndirbuf = 0;

int debug;

long	readdirect(int);
void	douser(Fs*, char*);
void	dodir(Fs*);
int	mail(Fs*, char*, char*, long);
int	mailin(Fs*, char*, long, char*, char*);
void	complain(char*, ...);
long	readnumfile(char*);
void	writenumfile(char*, long);

void
usage(void)
{
	fprint(2, "usage: %s [-n] [-p]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int which;

	which = 0;
	ARGBEGIN{
	case 'p':
		which |= Plan9;
		break;
	case 'n':
		which |= Securenet;
		break;
	case 'd':
		debug++;
		break;
	default:
		usage();
	}ARGEND
	argv0 = "warning";

	if(!which)
		which |= Plan9 | Securenet;
	if(which & Plan9)
		dodir(&fs[Plan9]);
	if(which & Securenet)
		dodir(&fs[Securenet]);
}

void
dodir(Fs *f)
{
	int nfiles;
	int i, fd;

	if(chdir(f->keys) < 0){
		complain("can't chdir to %s: %r", f->keys);
		return;
	}
 	fd = open(".", OREAD);
	if(fd < 0){
		complain("can't open %s: %r\n", f->keys);
		return;
	}
	nfiles = dirreadall(fd, &dirbuf);
	close(fd);
	for(i = 0; i < nfiles; i++)
		douser(f, dirbuf[i].name);
}

/*
 *  check for expiration
 */
void
douser(Fs *f, char *user)
{
	int n, nwarn;
	char buf[128];
	long rcvrs, et, now;
	char *l;

	sprint(buf, "%s/expire", user);
	et = readnumfile(buf);
	now = time(0);

	/* start warning 2 weeks ahead of time */
	if(et <= now || et > now+14*24*60*60)
		return;

	sprint(buf, "%s/warnings", user);
	nwarn = readnumfile(buf);
	if(et <= now+14*24*60*60 && et > now+7*24*60*60){
		/* one warning 2 weeks before expiration */
		if(nwarn > 0)
			return;
		nwarn = 1;
	} else {
		/* one warning 1 week before expiration */
		if(nwarn > 1)
			return;
		nwarn = 2;
	}

	/*
	 *  if we can't open the who file, just mail to the user and hope
 	 *  for it makes it.
	 */
	if(f->b){
		if(Bseek(f->b, 0, 0) < 0){
			Bterm(f->b);
			f->b = 0;
		}
	}
	if(f->b == 0){
		f->b = Bopen(f->who, OREAD);
		if(f->b == 0){
			if(mail(f, user, user, et) > 0)
				writenumfile(buf, nwarn);
			return;
		}
	}

	/*
	 *  look for matches in the who file and mail to every address on
	 *  matching lines
	 */
	rcvrs = 0;
	while(l = Brdline(f->b, '\n')){
		n = strlen(user);
		if(strncmp(l, user, n) == 0 && (l[n] == ' ' || l[n] == '\t'))
			rcvrs += mailin(f, user, et, l, l+Blinelen(f->b));
	}

	/*
	 *  if no matches, try the user directly
	 */
	if(rcvrs == 0)
		rcvrs = mail(f, user, user, et);
	rcvrs += mail(f, "netkeys", user, et);
	if(rcvrs)
		writenumfile(buf, nwarn);
}

/*
 *  anything in <>'s is an address
 */
int
mailin(Fs *f, char *user, long et, char *l, char *e)
{
	int n;
	int rcvrs;
	char *p;
	char addr[256];

	p = 0;
	rcvrs = 0;
	while(l < e){
		switch(*l){
		case '<':
			p = l + 1;
			break;
		case '>':
			if(p == 0)
				break;
			n = l - p;
			if(n > 0 && n <= sizeof(addr) - 2){
				memmove(addr, p, n);
				addr[n] = 0;
				rcvrs += mail(f, addr, user, et);
			}
			p = 0;
			break;
		}
		l++;
	}
	return rcvrs;
}

/*
 *  send mail
 */
int
mail(Fs *f, char *rcvr, char *user, long et)
{
	int pid, i, fd;
	int pfd[2];
	char *ct, *p;
	Waitmsg *w;
	char buf[128];

	if(pipe(pfd) < 0){
		complain("out of pipes: %r");
		return 0;
	}

	switch(pid = fork()){
	case -1:
		complain("can't fork: %r");
		return 0;
	case 0:
		break;
	default:
		if(debug)
			fprint(2, "started %d\n", pid);
		close(pfd[0]);
		ct = ctime(et);
		p = strchr(ct, '\n');
		*p = '.';
		fprint(pfd[1], "User '%s's %s expires on %s\n", user, f->msg, ct);
		if(f != fs)
			fprint(pfd[1], "If you wish to renew contact your local administrator.\n");
		p = strrchr(f->keys, '/');
		if(p)
			p++;
		else
			p = f->keys;
		sprint(buf, "/adm/warn.%s", p);
		fd = open(buf, OREAD);
		if(fd >= 0){
			while((i = read(fd, buf, sizeof(buf))) > 0)
				write(pfd[1], buf, i);
			close(fd);
		}
		close(pfd[1]);

		/* wait for warning to be mailed */
		for(;;){
			w = wait();
			if(w == nil)
				break;
			if(w->pid == pid){
				if(debug)
					fprint(2, "%d terminated: %s\n", pid, w->msg);
				if(w->msg[0] == 0){
					free(w);
					break;
				}else{
					free(w);
					return 0;
				}
			}else
				free(w);
		}
		return 1;
	}

	/* get out of the current namespace */
	newns("none", 0);

	dup(pfd[0], 0);
	close(pfd[0]);
	close(pfd[1]);
	putenv("upasname", "netkeys");
	if(debug){
		print("\nto %s\n", rcvr);
		execl("/bin/cat", "cat", nil);
	}
	execl("/bin/upas/send", "send", "-r", rcvr, nil);

	/* just in case */
	sysfatal("can't exec send: %r");

	return 0;		/* for compiler */
}

void
complain(char *fmt, ...)
{
	char buf[8192], *s;
	va_list arg;

	s = buf;
	s += sprint(s, "%s: ", argv0);
	va_start(arg, fmt);
	s = vseprint(s, buf + sizeof(buf) / sizeof(*buf), fmt, arg);
	va_end(arg);
	*s++ = '\n';
	write(2, buf, s - buf);
}

long
readnumfile(char *file)
{
	int fd, n;
	char buf[64];

	fd = open(file, OREAD);
	if(fd < 0){
		complain("can't open %s: %r", file);
		return 0;
	}
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n < 0){
		complain("can't read %s: %r", file);
		return 0;
	}
	buf[n] = 0;
	return atol(buf);
}

void
writenumfile(char *file, long num)
{
	int fd;

	fd = open(file, OWRITE);
	if(fd < 0){
		complain("can't open %s: %r", file);
		return;
	}
	fprint(fd, "%ld", num);
	close(fd);
}

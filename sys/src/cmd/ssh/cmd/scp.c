#include <u.h>
#include <libc.h>
#include <ctype.h>

#include "../ssh.h"

#define	OK		0x00
#define	ERROR	0x01
#define	FATAL	0x02

char *progname;

int	dflag;
int	fflag;
int	iflag;
int	pflag;
int	rflag;
int	tflag;
int	vflag;

int remote;

char *exitflag = nil;

void	scperror(int, char*, ...);
void	mustbedir(char*);
void receive(char*);
char	*fileaftercolon(char*);
void	destislocal(char *cmd, int argc, char *argv[], char *dest);
void	destisremote(char *cmd, int argc, char *argv[], char *host, char *dest);
int	remotessh(char *host, char *cmd);
void	send(char*);
void	senddir(char*, int, Dir*);
int 	getresponse(void);

char theuser[NAMELEN];

char ssh[] = "/bin/ssh";

int	remotefd0;
int	remotefd1;

int
runcommand(char *cmd)
{
	Waitmsg w;
	int pid, wpid;
	char *argv[4];

	if (cmd == nil)
		return -1;
	switch(pid = fork()){
	case -1:
		return -1;
	case 0:
		argv[0] = "rc";
		argv[1] = "-c";
		argv[2] = cmd;
		argv[3] = nil;
		exec("/bin/rc", argv);
		exits("exec failed");
	}
	for(;;){
		wpid = wait(&w);
		if(wpid < 0)
			return -1;
		if(wpid == pid)
			break;
	}
	if(w.msg[0])
		return -1;
	return 1;
}

void
vprint(char *fmt, ...)
{
	char buf[1024];
	va_list arg;
	static char *name;

	if(vflag == 0)
		return;

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);

	if(name == nil){
		name = sysname();
		if(name == nil)
			name = "<unknown>";
	}
	fprint(2, "%s: %s\n", name, buf);
}

void
usage(void)
{
	fprint(2, "Usage: scp [-Iidfprtv] source ... destination\n");
	exits("usage");
}


#pragma	varargck	type	"F"	int
#pragma	varargck	type	"V"	char*
static int flag;

/* flag: if integer flag, take following char *value */
int
flagconv(va_list *va, Fconv*)
{
	flag = va_arg(*va, int);
	return sizeof(int);
}

/* flag: if previous integer flag, take char *value */
int
valconv(va_list *va, Fconv *f)
{
	char *value;

	value = va_arg(*va, char*);
	if(flag){
		strconv(" ", f);
		strconv(value, f);
	}
	return sizeof(char*);
}

void
sendokresponse(void)
{
	char ok = OK;

	write(remotefd1, &ok, 1);
}

void
main(int argc, char *argv[])
{
	int i, fd;
	char cmd[32];
	char *p;

	progname = argv[0];
	fmtinstall('B', mpconv);
	fmtinstall('F', flagconv);
	fmtinstall('V', valconv);
	iflag = -1;

	ARGBEGIN {
	case 'I':
		iflag = 0;
		break;
	case 'i':
		iflag = 1;
		break;
	case 'd':
		dflag++;
		break;
	case 'f':
		fflag++;
		remote++;
		break;
	case 'p':
		pflag++;
		break;
	case 'r':
		rflag++;
		break;
	case 't':
		tflag++;
		remote++;
		break;
	case 'v':
		vflag++;
		break;
	default:
		scperror(1, "unknown option %c", ARGC());
	} ARGEND

	if(iflag == -1)
		iflag = isatty(0);

	remotefd0 = 0;
	remotefd1 = 1;

	if(fflag){
		getresponse();
		for(i=0; i<argc; i++)
			send(argv[i]);
		exits(0);
	}
	if(tflag){
		if(argc != 1)
			usage();
		receive(argv[0]);
		exits(0);
	}

	if (argc < 2)
		usage();
	if (argc > 2)
		dflag = 1;

	i = 0;
	fd = open("/dev/user", OREAD);
	if(fd >= 0){
		i = read(fd, theuser, sizeof theuser - 1);
		close(fd);
	}
	if(i <= 0)
		scperror(1, "can't read /dev/user: %r");

	remotefd0 = -1;
	remotefd1 = -1;

	snprint(cmd, sizeof cmd, "scp%F%V%F%V%F%V%F%V",
		dflag, "-d",
		pflag, "-p",
		rflag, "-r",
		vflag, "-v");

	p = fileaftercolon(argv[argc-1]);
	if(p != nil)	/* send to remote machine. */
		destisremote(cmd, argc-1, argv, argv[argc-1], p);
	else{
		if(dflag)
			mustbedir(argv[argc-1]);
		destislocal(cmd, argc-1, argv, argv[argc-1]);
	}

	exits(exitflag);
}

void
destislocal(char *cmd, int argc, char *argv[], char *dst)
{
	int i;
	char *src;
	char buf[4096];

	for(i = 0; i<argc; i++){
		src = fileaftercolon(argv[i]);
		if(src == nil){
			/* local file; no network */
			snprint(buf, sizeof buf, "exec cp%F%V%F%V %s %s",
				rflag, "-r",
				pflag, "-p",
				argv[i], dst);
	  		vprint("remotetolocal: %s", buf);
			if(runcommand(buf) < 0)
				exitflag = "local cp exec";
		}else{
			/* remote file; use network */
			snprint(buf, sizeof buf, "%s -f %s", cmd, src);
		  	if(remotessh(argv[i], buf) < 0)
				exitflag = "remote ssh exec";
			else{
				receive(dst);
				close(remotefd0);
				remotefd0 = -1;
				remotefd1 = -1;
			}
		}
	}
}

void
destisremote(char *cmd, int argc, char *argv[], char *host, char *dest)
{
	int i;
	char *src;
	char buf[4096];

	for(i = 0; i < argc; i++){
		vprint("remote destination: send %s to %s:%s", argv[i], host, dest);
		/* destination is remote, but source may be local */
		src = fileaftercolon(argv[i]);
		if(src != nil){
			/* remote to remote */
			snprint(buf, sizeof buf, "exec %s%F%V%F%V %s %s %s '%s:%s'",
				ssh,
				iflag, " -i",
				vflag, "-v",
				argv[i], cmd, src,
				host, dest);
			vprint("localtoremote: %s", buf);
			runcommand(buf);
		}else{
			/* local to remote */
			if(remotefd0 == -1){
				snprint(buf, sizeof buf, "%s -t %s", cmd, dest);
				if(remotessh(host, buf) < 0)
					exits("remotessh");
				if(getresponse() < 0)
					exits("bad response");
			}
			send(argv[i]);
		}
	}
}

void
readhdr(char *p, int n)
{
	int i;

	for(i=0; i<n; i++){
		if(read(remotefd0, &p[i], 1) != 1)
			break;
		if(p[i] == '\n'){
			p[i] = '\0';
			return;
		}
	}
	/* if at beginning, this is regular EOF */
	if(i == 0)
		exits(nil);
	scperror(1, "read error on receive header: %r");
}

void
receivedir(char *dir, int exists, Dir *d, int settimes, ulong atime, ulong mtime, ulong mode)
{
	int setmodes;

	setmodes = pflag;
	if(exists){
		if((d->mode & CHDIR) == 0) {
			scperror(0, "%s: protocol botch: directory requrest for non-directory", dir);
			return;
		}
	}else{
		/* create it writeable; will fix later */
		setmodes = 1;
		if (create(dir, OREAD, CHDIR|mode|0700) < 0){
			scperror(0, "%s: can't create: %r", dir);
			return;
		}
	}
	receive(dir);
	if(settimes || setmodes){
		if (dirstat(dir, d)) {
			scperror(0, "can't stat %s: %r", dir);
			return;
		}
		if(settimes){
			d->atime = atime;
			d->mtime = mtime;
		}
		if(setmodes)
			d->mode = (d->mode & ~0777) | (mode & 0777);
		if(dirwstat(dir, d))
			scperror(0, "can't wstat %s: %r", dir);
	}
}

void
receive(char *dest)
{
	int isdir, settimes, mode;
	int exists, n, i, fd, m;
	int errors;
	ulong atime, mtime, size;
	char buf[8192], *p;
	char name[1024];
	Dir d;

	mtime = 0L;
	atime = 0L;
	settimes = 0;
	isdir = 0;
	if (dirstat(dest, &d) == 0 && (d.mode & CHDIR))
		isdir = 1;
	if(dflag && !isdir)
		scperror(1, "%s: not a directory: %r", dest);

	sendokresponse();

	for (;;) {
		readhdr(buf, sizeof buf);

		switch(buf[0]){
		case ERROR:
		case FATAL:
			if(!remote)
				fprint(2, "%s\n", buf+1);
			exitflag = "bad receive";
			if(buf[0] == FATAL)
				exits(exitflag);
			continue;

		case 'E':
			sendokresponse();
			return;

		case 'T':
			settimes = 1;
			p = buf + 1;
			mtime = strtol(p, &p, 10);
			if(*p++ != ' '){
		Badtime:
				scperror(1, "bad time format: %s", buf+1);
			}
			strtol(p, &p, 10);
			if(*p++ != ' ')
				goto Badtime;
			atime = strtol(p, &p, 10);
			if(*p++ != ' ')
				goto Badtime;
			strtol(p, &p, 10);
			if(*p++ != 0)
				goto Badtime;

			sendokresponse();
			continue;

		case 'D':
		case 'C':
			p = buf + 1;
			mode = strtol(p, &p, 8);
			if (*p++ != ' '){
		Badmode:
				scperror(1, "bad mode/size format: %s", buf+1);
			}
			size = strtoll(p, &p, 10);
			if(*p++ != ' ')
				goto Badmode;

			if(isdir){
				if(dest[0] == '\0')
					snprint(name, sizeof name, "%s", p);
				else
					snprint(name, sizeof name, "%s/%s", dest, p);
			}else
				snprint(name, sizeof name, "%s", dest);
			if(strlen(name) > sizeof name-UTFmax)
				scperror(1, "file name too long: %s", dest);

			exists = 1;
			if(dirstat(name, &d) < 0)
				exists = 0;

			if(buf[0] == 'D'){
				vprint("receive directory %s", name);
				receivedir(name, exists, &d, settimes, atime, mtime, mode);
				settimes = 0;
				continue;
			}

			vprint("receive file %s by %s", name, getuser());
			fd = create(name, OWRITE, mode);
			if(fd < 0){
				scperror(0, "can't create %s: %r", name);
				continue;
			}
			sendokresponse();

			/*
			 * Committed to receive size bytes
			 */
			errors = 0;
			for(i = 0; i < size; i += m){
				n = sizeof buf;
				if(n > size - i)
					n = size - i;
				m = readn(remotefd0, buf, n);
				if(m <= 0)
					scperror(1, "read error on connection: %r");
				if(errors == 0){
					n = write(fd, buf, m);
					if(n != m)
						errors = 1;
				}
			}

			/* if file exists, modes could be wrong */
			if(errors)
				scperror(0, "%s: write error: %r", name);
			else if(settimes || (exists && (d.mode&0777) != (mode&0777))){
				if(!exists && dirfstat(fd, &d) < 0){
					scperror(0, "can't stat %s: %r", name);
					continue;
				}
				if(settimes){
					settimes = 0;
					d.atime = atime;
					d.mtime = mtime;
				}
				d.mode = (d.mode & ~0777) | (mode & 0777);
				if(dirwstat(name, &d) < 0)
					scperror(0, "can't wstat %s: %r", name);
			}

			close(fd);
			getresponse();
			if(errors)
				exits("write error");
			sendokresponse();
			break;

		default:
			scperror(0, "unrecognized header type char %c", buf[0]);	
			scperror(1, "input line: %s", buf);	
		}
	}
}

/*
 * Lastelem is called when we have a Dir with the final element, but if the file
 * has been bound, we want the original name that was used rather than
 * the contents of the stat buffer, so do this lexically.
 */
char*
lastelem(char *file)
{
	char *elem;

	elem = strrchr(file, '/');
	if(elem == nil)
		return file;
	return elem+1;
}

void
send(char *file)
{
	Dir d;
	ulong i;
	int m, n, fd;
	char buf[8192];

	if((fd = open(file, OREAD)) < 0){
		scperror(0, "can't open %s: %r", file);
		return;
	}
	if(dirfstat(fd, &d) < 0){
		scperror(0, "can't fstat %s: %r", file);
		goto Return;
	}

	if(d.mode & CHDIR){
		if(rflag)
			senddir(file, fd, &d);
		else
			scperror(0, "%s: is a directory", file);
		goto Return;
	}

	if(pflag){
		fprint(remotefd1, "T%lud 0 %lud 0\n", d.mtime, d.atime);
		if(getresponse() < 0)
			goto Return;
	}

	fprint(remotefd1, "C%.4luo %lld %s\n", d.mode&0777, d.length, lastelem(file));
	if(getresponse() < 0)
		goto Return;

	/*
	 * We are now committed to send d.length bytes, regardless
	 */
	for(i=0; i<d.length; i+=m){
		n = sizeof buf;
		if(n > d.length - i)
			n = d.length - i;
		m = readn(fd, buf, n);
		if(m <= 0)
			break;
		write(remotefd1, buf, m);
	}

	if(i == d.length)
		sendokresponse();
	else{
		/* continue to send gibberish up to d.length */
		for(; i<d.length; i+=n){
			n = sizeof buf;
			if(n > d.length - i)
				n = d.length - i;
			write(remotefd1, buf, n);
		}
		scperror(0, "%s: %r", file);
	}
		
	getresponse();

    Return:
	close(fd);
}

int
getresponse(void)
{
	uchar first, byte, buf[256];
	int i;

	if (read(remotefd0, &first, 1) != 1)
		scperror(1, "lost connection");

	if(first == 0)
		return 0;

	i = 0;
	if(first > FATAL){
		fprint(2, "scp: unexpected response character 0x%.2ux\n", first);
		buf[i++] = first;
	}

	/* read error message up to newline */
	for(;;){
		if(read(remotefd0, &byte, 1) != 1)
			error("response: dropped connection");
		if(byte == '\n')
			break;
		if(i < sizeof buf)
			buf[i++] = byte;
	}

	exitflag = "bad response";
	if(!remote)
		fprint(2, "%.*s\n", utfnlen((char*)buf, i), (char*)buf);

	if (first == ERROR)
		return -1;
	exits(exitflag);
	return 0;	/* not reached */
}

void
senddir(char *name, int fd, Dir *dirp)
{
	Dir dir;
	char file[256];

	if(pflag){
		fprint(remotefd1, "T%lud 0 %lud 0\n", dirp->mtime, dirp->atime);
		if(getresponse() < 0)
			return;
	}

	vprint("directory %s mode: D%.4lo %d %.1024s", name, dirp->mode&0777, 0, lastelem(name));

	fprint(remotefd1, "D%.4lo %d %.1024s\n", dirp->mode&0777, 0, dirp->name);
	if(getresponse() < 0)
		return;

	while(dirread(fd, &dir, sizeof(dir)) > 0){
		/* shouldn't happen with plan 9, but worth checking anyway */
		if(strcmp(dir.name, ".")==0 || strcmp(dir.name, "..")==0)
			continue;
		if(snprint(file, sizeof file, "%s/%s", name, dir.name) > sizeof file-UTFmax){
			scperror(0, "%.20s.../%s: name too long; skipping file", file, dir.name);
			continue;
		}
		send(file);
	}
	fprint(remotefd1, "E\n");
	getresponse();
}

int
remotessh(char *host, char *cmd)
{
	int i, p[2];
	char *arg[32];

	vprint("remotessh: %s: %s", host, cmd);

	if(pipe(p) < 0)
		error("pipe: %r");

	switch(fork()){
	case -1:
		error("fork: %r");

	case 0:
		/* child */
		close(p[0]);
		dup(p[1], 0);
		dup(p[1], 1);
		for (i = 3; i < 100; i++)
			close(i);
	
		i = 0;
		arg[i++] = ssh;
		arg[i++] = "-x";
		arg[i++] = "-a";
		arg[i++] = "-m";
		if(iflag)
			arg[i++] = "-i";
		if(vflag)
			arg[i++] = "-v";
		arg[i++] = host;
		arg[i++] = cmd;
		arg[i] = nil;
	
		exec(ssh, arg);
		exits("exec failed");

	default:
		/* parent */
		close(p[1]);
		remotefd0 = p[0];
		remotefd1 = p[0];
	}
	return 0;
}

void
scperror(int exit, char *fmt, ...)
{
	char buf[2048];
	va_list arg;


	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);

	fprint(remotefd1, "%cscp: %s\n", ERROR, buf);

	if (!remote)
		fprint(2, "scp: %s\n", buf);
	exitflag = buf;
	if(exit)
		exits(exitflag);
}

char *
fileaftercolon(char *file)
{
	char *c, *s;

	c = utfrune(file, ':');
	if(c == nil)
		return nil;

	/* colon must be in middle of name to be a separator */
	if(c == file)
		return nil;

	/* does slash occur before colon? */
	s = utfrune(file, '/');
	if(s != nil && s < c)
		return nil;

	*c++ = '\0';
	if(*c == '\0')
		return ".";
	return c;
}

void
mustbedir(char *file)
{
	Dir d;

	if(dirstat(file, &d) < 0){
		scperror(1, "%s: %r", file);
		return;
	}
	if(d.mode & CHDIR)
		return;
	scperror(1, "%s: Not a directory", file);
}

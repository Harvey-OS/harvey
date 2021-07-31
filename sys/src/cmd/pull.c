#include <u.h>
#include <libc.h>

char SERVER[]="Rpush";
int server;

#define MAXCHARS 8192
char args[MAXCHARS];

char *basename(char*);
char *str(char*, int);
void createfile(char*, int);
void error(char*, char*);
int mkdir(char*, int);
int o3(char*);


ipcexec(char *dest, char *params, char *cmd)
{
	char c;
	int fd, n;
	char devdir[40];

	USED(params);
	fd = dial(netmkaddr(dest, "dk", "exec"), 0, devdir, 0);
	if(fd < 0){
		error("can't dial", 0);
		return -1;
	}
	fprint(2, "connected to %s on %s\n", dest, devdir);
	if(read(fd, &c, 1)==1 && c=='O')
	if(read(fd, &c, 1)==1 && c=='K'){
		n = strlen(cmd)+1;
		if(write(fd, cmd, n) != n){
			error("can't write", 0);
			return -1;
		}
	} else {
		error("permission denied", 0);
		close(fd);
		return -1;
	}
	return fd;
}

char *bldargs(char *argv[])
{
	register char *s, *t;

	s=strcpy(args, SERVER);
	s+=strlen(s);
	*s++=' ';
	while(t= *argv++){	/* assignment = */
		while(*s = *t++)
			if(s++ >= &args[MAXCHARS-1]){
				error("arg list too long", "");
				exits("args");
			}
		*s++=' ';
	}
	s[-1]='\0';
	return(args);
}

char *
basename(char *s)
{
	register char *t;

	t=utfrrune(s, '/');
	return(t? t+1 : s);
}

char *
str(char *s, int n)
{
	static char strbuf[129];

	strncpy(strbuf, s, n);
	strbuf[n]=0;
	return(strbuf);
}

int errors=0;
int vflag;

void
main(int argc, char *argv[])
{
	register n;
	char *targetdir;
	char buf[512];

	if(argc>1 && strcmp(argv[1], "-v")==0){
		vflag=1;
		argv++;
		--argc;
	}
	if(basename(argv[0])[0]=='R'){
		if(argc!=2){
			error("arg count", "");	/* It's ok; other guy will decrypt it */
			exits("arg count");
		}
		targetdir=argv[1];
	}else{
		if(argc<4){
			error("Usage: pull machine <remotefiles|dirs> localdir", "");
			exits("usage");
		}
		targetdir=argv[argc-1];
		server=0;	/* Standard input */
		server=ipcexec(argv[1], "heavy delim hup", bldargs(&argv[1]));
		if(server<0){
			error("can't execute remote server", 0);
			exits("remote");
		}
	}
	if(mkdir(targetdir, 0777)==0){
		error("can't make directory", argv[1]);
		exits("mkdir");
	}
	while((n=read(server, buf, sizeof buf))>0){
		switch(buf[0]){
		case 'D':
			if(mkdir(str(buf+4, n-4), o3(buf+1))==0)
				error("can't make directory", str(buf+1, n-1));
			break;
		case 'E':
			error("(remote)", str(buf+1, n-1));
			break;
		case 'F':
			createfile(str(buf+4, n-4), o3(buf+1));
			continue;	/* createfile() will read null message */
		default:
			buf[1]='\0';
			error("unknown message", buf);
			break;
		}
		if(read(server, buf, sizeof buf)!=0){
			error("synchronization error", "");
			do; while(read(server, buf, sizeof buf)>0);
		}
	}
	if(errors)
		exits("errors");
	exits(0);
}

void
createfile(char *s, int m)
{
	char buf[512];
	register f;
	register cc;
	if(server>1 && vflag)
		print("pull: receive %s\n", s);
	if((f=create(s, OWRITE, m))<0)
		error("can't create", s);
	else{
		if(read(server, buf, sizeof buf)>0){
			error("synchronization", "error");
			close(f);
			return;
		}
		while((cc=read(server, buf, sizeof buf))>0)
			if(write(f, buf, cc)!=cc){
				error("write error", s);
/*				if(errno==ENOSPC){
					error("file system is full; aborting", "");
					exits("fs full");
				}/**/
				close(f);
				return;
			}
		close(f);
	}
}

void
error(char *s, char *t)
{
	char ebuf[128];
	char buf[ERRLEN];

	errstr(buf);
	errors=1;
	if(t==0)
		t = "";
	sprint(ebuf, "%s%s %s (%s)\n", server>0? "pull: " : "" , s, t, buf);
	write(2, ebuf, strlen(ebuf));
}

mkdir(char *s, int m)
{
	Dir dir;
	int fd;

	USED(m);
	if(dirstat(s, &dir)<0){
		fd = create(s, OREAD, 0x80000000L + 0775L);
		if(fd < 0)
			return 0;
		close(fd);
		return 1;
	}
	return (dir.qid.path & CHDIR)!=0;
}

o3(char *s)
{
	return(((s[0]-'0')*0100)+((s[1]-'0')*010)+(s[2]-'0'));
}

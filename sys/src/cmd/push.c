#include <u.h>
#include <libc.h>

char targetdir[128];

char SERVER[]="Rpull";
int serverin;
int serverout;

char *basename(char*);
void pushdir(char*, int);
void popdir(void);
void sendfile(char*);
void senddata(char*, char*);
void senddir(char*, char*, int);
void error(char*, char*);
void report(char*, int);
void message(int, char*);

connect(char *dest, char *cmd)
{
	char c, devdir[40];
	int n;

	serverout = dial(netmkaddr(dest, "dk", "exec"), 0, devdir, 0);
	if(serverout < 0)
		return -1;
	serverin = dup(serverout, -1);
	fprint(2, "connected to %s on %s\n", dest, devdir);
	if(read(serverin, &c, 1)==1 && c=='O')
	if(read(serverin, &c, 1)==1 && c=='K'){
		n = strlen(cmd)+1;
		if(write(serverout, cmd, n) != n){
			error("can't write", 0);
			return -1;
		}
	} else {
		error("permission denied", 0);
		return -1;
	}
	return serverin;
}

char *
basename(char *s)
{
	register char *t;

	t=utfrrune(s, '/');
	return(t? t+1 : s);
}

int errors=0;
int vflag;

void
main(int argc, char *argv[])
{
	register i;
	Waitmsg status;
	register pid;
	char buf[129];

	if(argc>1 && strcmp(argv[1], "-v")==0){
		vflag=1;
		argv++;
		--argc;
	}
	if(argc<4){
		error("Usage: push machine <files|dirs> remotedir>", "");
		exits("usage");
	}
	strcat(strcpy(targetdir, argv[argc-1]), "/");
	if(basename(argv[0])[0]=='R'){
		serverin = 0;
		serverout = 1;
	} else {
		/* Set up the remote process */
		sprint(buf, "%s %s", SERVER, argv[argc-1]);
		if(connect(argv[1], buf)<0){
			error("can't connect", "");
			exits("connect");
		}
		/* Must print remote errors locally */
		if((pid=fork())==0){
			close(serverout);
			while((i=read(serverin, buf, sizeof buf))>0)
				report(buf, i);
			exits(0);
		}else if(pid==-1){
			error("can't fork", "try again");
			exits("fork");
		}
		close(serverin);
	}
	for(i=2; i<argc-1; i++)
		sendfile(argv[i]);
	write(serverout, buf, 0);		/* End of file */
	if(serverout>1)
		wait(&status);		/* To get all the error messages */
	if(errors)
		exits("errors");
	exits(0);
}

#define	NDIRSTACK 20
char directory[128];		/* Name buffer for directories */
char *dirstack[NDIRSTACK];
char *basep=directory;
int dirstkp=0;

void
pushdir(char *s, int m)
{
	char buf[128];

	if(dirstkp>=NDIRSTACK){
		error("directory stack overflow", "");
		exits("dir stack");
	}
	dirstack[dirstkp++]=directory+strlen(directory);
	strcat(directory, s);
	if(dirstkp==1)	/* First entry on stack */
		basep=basename(directory);
	sprint(buf, "%.3o%s%s", m, targetdir, basep);
	message('D', buf);
	strcat(directory, "/");
}

void
popdir(void)
{
	if(dirstkp<=0){
		error("directory stack underflow", "");
		exits("dir stack");
	}
	*dirstack[--dirstkp]='\0';
	if(dirstkp<=0)
		basep=directory;
}

void
sendfile(char *s)
{
	Dir dir;
	char filebuf[128];
	char remotebuf[128];

	strcat(strcpy(filebuf, directory), s);
	if(dirstat(filebuf, &dir)<0){
		error("can't stat", s);
		return;
	}
	if(serverout>1 && vflag)
		print("push: send %s\n", filebuf);
	if(dir.qid.path&CHDIR){
		senddir(filebuf, s, dir.mode&0777);
	} else {
		sprint(remotebuf, "%.3o%s%s%s", dir.mode&0777,
			targetdir, basep, basename(s));
		senddata(filebuf, remotebuf);
	}
}

void
senddata(char *s, char *remote)
{
	char buf[512];
	register f, cc;

	if((f=open(s, 0))<0){
		error("can't open", s);
		return;
	}
	message('F', remote);
	while((cc=read(f, buf, sizeof buf))>0)
		write(serverout, buf, cc);
	write(serverout, buf, 0);
	close(f);
}

void
senddir(char *fullname, char *localname, int m)
{
	/* Same as <sys/dir.h> but with DIRSIZ+1 chars */
	Dir dir;
	int df;

	if((df=open(fullname, OREAD))<0){
		error("can't open directory", fullname);
		return;
	}
	pushdir(localname, m);
	while(dirread(df, &dir, sizeof dir) > 0){
		if(strcmp(dir.name, ".")!=0 && strcmp(dir.name, "..")!=0)
			sendfile(dir.name);
	}
	popdir();
	close(df);
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
	sprint(ebuf, "%s%s %s (%s)\n", serverout>0? "push: " : "" , s, t, buf);
	write(2, ebuf, strlen(ebuf));
}

void
report(char *s, int n)
{
	static char msgbuf[512];
	static char *msgp=msgbuf;
	register i;

	for(i=0; i<n; i++){
		*msgp= *s++;
		if(*msgp++=='\n'){
			write(2, "push: (remote) ", 14);
			write(2, msgbuf, msgp-msgbuf);
			msgp=msgbuf;
		}
	}
}

void
message(int c, char *s)
{
	char buf[128];

	
	buf[0]=c;
	strcpy(buf+1, s);
	write(serverout, buf, strlen(buf));
	write(serverout, buf, 0);		/* Terminate & synchronize */
}

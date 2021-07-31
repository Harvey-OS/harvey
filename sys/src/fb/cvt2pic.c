/*
 * Unknown junk is presented either in a file or on standard input.
 * We look at the first few bytes, guess what kind of image file it is,
 * and send it off to the appropriate decoding program.
 *
 * We have programs to read ega and targa images, but I know no
 * good way to recognize them.
 */
#include <u.h>
#include <libc.h>
#define	NBUF	2048
char buf[NBUF];
char *ebuf;
int pid=-1;
int prefix(char *str){
	int len;
	len=strlen(str);
	return ebuf>=buf+len && strncmp(buf, str, len)==0;
}
int faceserver(void){
	char *p;
	p=buf;
	while(p+8<ebuf){
		if(strncmp(p, "Picdata:", 8)==0) return 1;
		do
			++p;
		while(p!=ebuf && *p!='\n');
	}
	return 0;
}
/*
 * This looks for the string RECORD_TYPE followed by some blanks and an '='.
 * I'm not really sure that this is a good idea.
 */
int nasa(void){
	char *p, *q;
	for(p=buf;p+12!=ebuf;p++){
		if(strncmp(p, "RECORD_TYPE", 11)==0){
			for(q=p+11;q+1!=ebuf && *q==' ';q++);
			if(*q=='=') return 1;
		}
	}
	return 0;
}
int filter(char *cmd, char *arg){
	int pfd[2];
	pipe(pfd);
	switch(pid=fork()){
	case -1:
		fprint(2, "cvt2pic: can't fork!\n");
		exits("no fork");
	case 0:
		dup(pfd[0], 0);
		close(pfd[0]);
		close(pfd[1]);
		execl(cmd, cmd, arg, 0);
		fprint(2, "cvt2pic: can't exec %s\n", cmd);
		exits("no exec");
	}
	close(pfd[0]);
	return pfd[1];
}
/*
 * Leffler's tiff library needs to seek, so tiff2pic can't read from a pipe.
 */
void cvttiff(int rfd){
	char name[50], cmd[200];
	int wfd, n;
	for(n=0;n!=100;n++){
		sprint(name, "/tmp/cvt.%d.%d", getpid(), n);
		wfd=create(name, OWRITE, 0600);
		if(wfd>=0) break;
	}
	if(wfd==-1){
		fprint(2, "cvt2pic: can't create temporary!\n");
		exits("no tmp");
	}
	write(wfd, buf, ebuf-buf);
	while((n=read(rfd, buf, NBUF))>0)
		write(wfd, buf, n);
	sprint(cmd, "/bin/fb/tiff2pic %s;rm -f %s\n", name, name);
	execl("/bin/rc", "rc", "-c", cmd, 0);
	fprint(2, "cvt2pic: can't exec /bin/rc!\n");
	exits("no rc");
}
void main(int argc, char *argv[]){
	int fd, pfd, n, pid, wpid;
	Waitmsg msg;
	switch(argc){
	case 1:
		fd=0;
		break;
	case 2:
		fd=open(argv[1], OREAD);
		if(fd==-1){
			fprint(2, "%s: can't open %s\n", argv[0], argv[1]);
			exits("no open");
		}
		break;
	}
	for(ebuf=buf;ebuf!=&buf[NBUF];ebuf+=n){
		n=&buf[NBUF]-ebuf;
		n=read(fd, ebuf, n);
		if(n<=0) break;
	}
	if(prefix("MM") || prefix("II"))
		cvttiff(fd);
	if(prefix("TYPE="))	/* already a picfile, no conversion required */
		pfd=1;
	else if(prefix("#define"))
		pfd=filter("/bin/fb/xbm2pic", 0);
	else if(prefix("GIF"))
		pfd=filter("/bin/fb/gif2pic", "-m");
	else if(faceserver())
		pfd=filter("/bin/fb/face2pic", 0);
	else if(nasa())
		pfd=filter("/bin/fb/nasa2pic", 0);
	else if(ebuf>=buf+2 && buf[0]==(char)0x52 && buf[1]==(char)0xcc)
		pfd=filter("/bin/fb/utah2pic", 0);
	else if(ebuf>=buf+2 && buf[0]==(char)0x01 && buf[1]==(char)0xda)
		pfd=filter("/bin/fb/sgi2pic", 0);
	else if(ebuf>=buf+2 && buf[0]==(char)0xff && buf[1]==(char)0xd8)
		pfd=filter("/bin/fb/jpg2pic", 0);
	else if(ebuf>buf+3 && buf[0]==(char)10
	     && (buf[1]==(char)0 || buf[1]==(char)2 || buf[1]==(char)3 || buf[1]==(char)5)
	     && buf[2]==(char)1)
		pfd=filter("/bin/fb/pcx2pic", 0);
	else{
		fprint(2, "%s: can't decode %s\n", argv[0], argc==2?argv[1]:"stdin");
		exits("inscrutible");
	}
	write(pfd, buf, ebuf-buf);
	while((n=read(fd, buf, NBUF))>0)
		write(pfd, buf, n);
	close(pfd);
	for(;;){
		wpid=wait(&msg);
		if(wpid==-1) exits(0);
		if(wpid==pid) exits(msg.msg);
	}
}

#include <u.h>
#include <libc.h>

int echo = 1;
int raw;
int ctrlp;

void
usage(void)
{
	fprint(2, "usage: aux/tty [-p] [-f comfile] cmd args...\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int frchld[2];
	int tochld[2];
	int pid;
	int i, j;
	char *file;

	file = nil;

	ARGBEGIN{
	case 'p':
		ctrlp++;
		break;
	case 'f':
		file = EARGF(usage());
		break;
	}ARGEND

	if(file != nil){
		int fd;
		if((fd = open(file, ORDWR)) == -1)
			sysfatal("open %s", file);
		dup(fd, 0);
		dup(fd, 1);
		dup(fd, 2);
		close(fd);
	}

	pipe(frchld);
	pipe(tochld);

	switch(pid = rfork(RFPROC|RFFDG|RFNOTEG)){
	case -1:
		sysfatal("fork");
	case 0:
		close(tochld[0]);
		close(frchld[1]);

		dup(tochld[1], 0);
		dup(frchld[0], 1);
		dup(frchld[0], 2);
		close(tochld[1]);
		close(frchld[0]);

		exec(argv[0], argv);
		sysfatal("exec");
	default:
		close(tochld[1]);
		close(frchld[0]);
		break;
	}

	static char buf[512];
	static char obuf[512];
	int nfr, nto;
	int wpid;

	switch(wpid = rfork(RFPROC|RFFDG)){
	case -1:
		sysfatal("rfork");
	case 0:
		close(0);
		while((nfr = read(frchld[1], buf, sizeof buf)) > 0){
			int i, j;
			j = 0;
			for(i = 0; i < nfr; i++){
				if(buf[i] == '\n'){
					if(j > 0){
						write(1, buf, j);
						j = 0;
					}
					write(1, "\r\n", 2);
				} else {
					buf[j++] = buf[i];
				}
			}

			if(write(1, buf, j) != j)
				exits("write");
		}
		fprint(1, "aux/tty: got eof\n");
		postnote(PNPROC, getppid(), "interrupt");
		exits(nil);
	default:
		close(frchld[1]);
		j = 0;
		while((nto = read(0, buf, sizeof buf)) > 0){
			int oldj;
			oldj = j;
			for(i = 0; i < nto; i++){
				if(buf[i] == '\r' || buf[i] == '\n'){
					obuf[j++] = '\n';
					write(tochld[0], obuf, j);
					if(echo){
						obuf[j-1] = '\r';
						obuf[j++] = '\n';
						write(1, obuf+oldj, j-oldj);
					}
					j = 0;
				} else if(ctrlp && buf[i] == '\x10'){ // ctrl-p
					if(j > 0){
						if(echo)write(1, obuf+oldj, j-oldj);
						write(tochld[0], obuf, j);
						j = 0;
					}
					int fd;
					fd = open("/dev/reboot", OWRITE);
					if(fd != -1){
						fprint(1, "aux/tty: rebooting the system\n");
						sleep(2000);
						write(fd, "reboot", 6);
						close(fd);
					} else {
						fprint(1, "aux/tty: open /dev/reboot: %r\n");
					}
				} else if(buf[i] == '\003'){ // ctrl-c
					if(j > 0){
						if(echo)write(1, obuf+oldj, j-oldj);
						write(tochld[0], obuf, j);
						j = 0;
					}
					fprint(1, "aux/tty: sent interrupt to %d\n", pid);
					postnote(PNGROUP, pid, "interrupt");
					continue;
				} else if(buf[i] == '\x1d' ){ // ctrl-]
					if(j > 0){
						if(echo)write(1, obuf+oldj, j-oldj);
						write(tochld[0], obuf, j);
						j = 0;
					}
					fprint(1, "aux/tty: sent interrupt to %d\n", pid);
					postnote(PNGROUP, pid, "term");
					continue;
				} else if(buf[i] == '\004'){ // ctrl-d
					if(j > 0){
						if(echo)write(1, obuf+oldj, j-oldj);
						write(tochld[0], obuf, j);
						j = 0;
					}
					fprint(1, "aux/tty: sent eof to %d\n", pid);
					write(tochld[0], obuf, 0); //eof
					continue;
				} else if(buf[i] == 0x15){ // ctrl-u
					if(!raw){
						while(j > 0){
							j--;
							if(echo)write(1, "\b \b", 3); // bs
							else write(tochld[0], "\x15", 1); // bs
						}
					} else {
						obuf[j++] = buf[i];
					}
					continue;
				} else if(buf[i] == 0x7f || buf[i] == '\b'){ // backspace
					if(!raw){
						if(j > 0){
							j--;
							if(echo)write(1, "\b \b", 3); // bs
							else write(tochld[0], "\b", 1); // bs
						}
					} else {
						obuf[j++] = '\b';
					}
					continue;
				} else {
					obuf[j++] = buf[i];
				}
			}
			if(j > 0){
				if(raw){
					if(echo)write(1, obuf, j);
					write(tochld[0], obuf, j);
					j = 0;
				} else if(echo && j > oldj){
					write(1, obuf+oldj, j-oldj);
				}

			}
		}
		close(0);
		close(1);
		close(2);
		close(tochld[0]);
		postnote(PNGROUP, pid, "hangup");
		waitpid();
		waitpid();
	}
	exits(nil);
}


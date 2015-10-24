#include <u.h>
#include <libc.h>

int ctrlp;

void
usage(void)
{
	fprint(2, "usage: aux/tty [-p] cmd args...\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int frchld[2];
	int tochld[2];
	int pid;
	int i, j;

	ARGBEGIN{
	case 'p':
		ctrlp++;
		break;
	}ARGEND

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
					j = 0;
				} else if(ctrlp && buf[i] == '\x10'){ // ctrl-p
					if(j > 0){
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
						write(tochld[0], obuf, j);
						j = 0;
					}
					fprint(1, "aux/tty: sent interrupt to %d\n", pid);
					postnote(PNGROUP, pid, "interrupt");
					continue;
				} else if(buf[i] == '\x1d' ){ // ctrl-]
					if(j > 0){
						write(tochld[0], obuf, j);
						j = 0;
					}
					fprint(1, "aux/tty: sent interrupt to %d\n", pid);
					postnote(PNGROUP, pid, "term");
					continue;
				} else if(buf[i] == '\004'){ // ctrl-d
					if(j > 0){
						write(tochld[0], obuf, j);
						j = 0;
					}
					fprint(1, "aux/tty: sent eof to %d\n", pid);
					write(tochld[0], obuf, 0); //eof
					continue;
				} else {
					obuf[j++] = buf[i];
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

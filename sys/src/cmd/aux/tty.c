#include <u.h>
#include <libc.h>

int echo = 1;

void
main(int argc, char *argv[])
{
	int frchld[2];
	int tochld[2];
	int pid;

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

		exec(argv[1], argv+1);
		sysfatal("exec");
	default:
		close(tochld[1]);
		close(frchld[0]);
		break;
	}

	static char buf[512];
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
		while((nto = read(0, buf, sizeof buf)) > 0){
			int i, j;
			j = 0;
			for(i = 0; i < nto; i++){
				if(buf[i] == '\r'){
					if(j > 0){
						if(echo)write(1, buf, j);
						write(tochld[0], buf, j);
						j = 0;
					}
					write(1, "\r", 1);
					buf[j++] = '\n';
				} else if(buf[i] == '\003'){ // ctrl-c
					if(j > 0){
						if(echo)write(1, buf, j);
						write(tochld[0], buf, j);
						j = 0;
					}
					fprint(1, "aux/tty: sent interrupt\n", pid);
					postnote(PNGROUP, pid, "interrupt");
					continue;
				} else if(buf[i] == '\004'){ // ctrl-d
					if(j > 0){
						if(echo)write(1, buf, j);
						write(tochld[0], buf, j);
						j = 0;
					}
					fprint(1, "aux/tty: sent eof child\n", pid);
					write(tochld[0], buf, 0); //eof
					continue;
				} else if(buf[i] == 0x7f){ // backspace
					if(j > 0){
						if(echo)write(1, buf, j);
						write(tochld[0], buf, j);
						j = 0;
					}
					if(echo)write(1, "\008", 1); // bs
					write(tochld[0], "\008", 1); // bs
					continue;
				} else {
					buf[j++] = buf[i];
				}
			}
			if(j > 0){
				if(echo)write(1, buf, j);
				write(tochld[0], buf, j);
				j = 0;
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


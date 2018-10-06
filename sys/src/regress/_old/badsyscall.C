#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include <ureg.h>

int handled = 0;
int back = 0;
int inhandler = 0;
int badsys = 0;
int verbose = 0;
char *msg = "Writing from NxM program to stdout via linux write(2)\n";

void
handler(void *v, char *s)
{
        char *f[7];
	u64int parm[7];
	struct Ureg* u = v;
        int i, n, nf;

	inhandler = 1;
	if (verbose)
		fprint(2, "handler: %p %s\n", v, s);
	handled++;
	if (strncmp(s, "sys: bad sys call", 17)==0 && badsys){
		badsys = 0;
		noted(NCONT);
	}
	if (strncmp(s, "linux:", 6))
		noted(NDFLT);
	s += 6;
        nf = tokenize(s, f, nelem(f));

	for(i = 0; i < nelem(parm); i++)
		parm[i] = strtoull(f[i], 0, 0);
	switch(parm[0]){
		case 22:
			u->ax = pipe((void*)(parm[1]));
			break;
	}
	inhandler = 0;
	noted(NCONT);
}

void
main(int argc, char *argv[])
{
	uvlong callbadsys(uvlong, ...);
	int fd[2];
	int ret;
	char data[4];
  
	if (notify(handler)){
		fprint(2, "%r\n");
		exits("notify fails");
	}
	fprint(2, "Notify passed\n");

	//callbadsys(0xc0FF);
	//fprint(2, "Handled %d\n", handled);

	/* try a good linux system call */
	ret = callbadsys((uvlong)1, (uvlong)1, msg, strlen(msg));
	fprint(2, "Write to stdout is %d, should be %d\n", ret, strlen(msg));
	/* try a emulated linux system call */
	fprint(2, "Call pipe\n");
	ret = callbadsys((uvlong)22, (uvlong)fd);
	fprint(2, "PIPE ret %d = fd [%d,%d]\n", ret, fd[0], fd[1]);
	ret = write(fd[0], "hi.", 3);
	fprint(2, "write to pipe is %d (should be 3)\n", ret);
	data[3] = 0;
	read(fd[1], data, 3);
	fprint(2, "read from pipe is %d (should be 3); data is :%s:\n", ret, data);
	/* now try a bad linux system call */
	badsys = 1;
	callbadsys(0x3FFF);
	fprint(2, "Handled %d\n", handled);
	back++;

	if (!handled)
		exits("badsyscall test fails");
	exits(nil);
}

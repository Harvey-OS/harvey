/*
 * Attempt at emulation of Unix tar by calling Plan 9 tar.
 * 
 * The differences from Plan 9 tar are:
 *	In the absence of an "f" flag, the file /dev/tape is used.
 *	An "f" flag with argument "-" causes use of stdin/stdout
 *		by passing no "f" flag (nor argument) to Plan 9 tar.
 *	By default, the "T" flag is passed to Plan 9 tar.
 *		The "m" flag to this tar inhibits this behavior.
 */

#include <u.h>
#include <libc.h>

void
usage(void)
{
	fprint(2, "usage: ape/tar [crtx][vfm] [args...] [file...]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, j, verb, vflag, fflag, Tflag, nargc;
	char *p, *file, **nargv, *cpu, flagbuf[10], execbuf[128];
	Waitmsg *w;

	argv++, argc--;
	if(argc < 1)
		usage();

	p = argv[0];
	argv++, argc--;

	if(*p == '-')
		p++;

	if(strchr("crtx", *p) == nil)
		usage();
	verb = *p++;

	/* unix defaults */
	fflag = 1;
	file = "/dev/tape";
	Tflag = 1;
	vflag = 0;

	for(; *p; p++) {
		switch(*p) {
		default:
			usage();
		case 'v':
			vflag = 1;
			break;
		case 'f':
			if(argc <= 0)
				usage();

			fflag = 1;
			file = argv[0];
			argv++, argc--;
			if(strcmp(file, "-") == 0) {
				/*
				 * plan9 doesn't know about "-" meaning stdin/stdout,
				 * but it's the default,
				 * so rewrite to not use f flag at all.
				 */
				file = nil;
				fflag = 0;
			}
			break;
		case 'm':
			Tflag = 0;
			break;
		case 'p':		/* pretend nothing's wrong */
			break;
		}
	}

	nargc = 1 + 1 + fflag + argc + 1;
	nargv = malloc(sizeof(char*) * nargc);
	if(nargv == nil) {
		fprint(2, "ape/tar: out of memory\n");
		exits("memory");
	}

	cpu = getenv("cputype");
	if(cpu == nil) {
		fprint(2, "ape/tar: need cputype environment variable set\n");
		exits("cputype");
	}
	snprint(execbuf, sizeof execbuf, "/%s/bin/tar", cpu);

	nargv[0] = "tar";
	sprint(flagbuf, "%c%s%s%s", verb, vflag ? "v" : "", Tflag ? "T" : "", fflag ? "f" : "");
	nargv[1] = flagbuf;

	i = 2;
	if(fflag)
		nargv[i++] = file;

	for(j=0; j<argc; j++, i++)
		nargv[i] = argv[j];

	nargv[i++] = nil;
	assert(i == nargc);

	switch(fork()){
	case -1:
		fprint(2, "ape/tar: fork failed: %r\n");
		exits("fork");
	case 0:
		exec(execbuf, nargv);
		fprint(2, "exec %s fails: %r\n", execbuf);
		_exits("exec");
	default:
		w = wait();
		if(w == nil)
			exits("wait failed");
		if(w->msg[0] == '\0')
			exits(nil);
		else
			exits(w->msg);
	}
	assert(0);
}

#include "all.h"
#include "io.h"

Mconf mconf;

static void
mconfinit(void)
{
	int nf, pgsize = 0;
	ulong size, userpgs = 0, userused = 0;
	char *ln, *sl;
	char *fields[2];
	Biobuf *bp;
	Mbank *mbp;

	size = 64*MB;
	bp = Bopen("#c/swap", OREAD);
	if (bp != nil) {
		while ((ln = Brdline(bp, '\n')) != nil) {
			ln[Blinelen(bp)-1] = '\0';
			nf = tokenize(ln, fields, nelem(fields));
			if (nf != 2)
				continue;
			if (strcmp(fields[1], "pagesize") == 0)
				pgsize = atoi(fields[0]);
			else if (strcmp(fields[1], "user") == 0) {
				sl = strchr(fields[0], '/');
				if (sl == nil)
					continue;
				userpgs = atol(sl+1);
				userused = atol(fields[0]);
			}
		}
		Bterm(bp);
		if (pgsize > 0 && userpgs > 0)
			size = (((userpgs - userused)*3LL)/4)*pgsize;
	}
	mconf.memsize = size;
	mbp = mconf.bank;
	mbp->base = 0x10000000;			/* fake addresses */
	mbp->limit = mbp->base + size;
	mbp++;

	mconf.nbank = mbp - mconf.bank;
}

ulong
meminit(void)
{
	conf.nmach = 1;
	mconfinit();
	return mconf.memsize;
}

/*
 * based on libthread's threadsetname, but drags in less library code.
 * actually just sets the arguments displayed.
 */
void
procsetname(char *fmt, ...)
{
	int fd;
	char *cmdname;
	char buf[128];
	va_list arg;

	va_start(arg, fmt);
	cmdname = vsmprint(fmt, arg);
	va_end(arg);
	if (cmdname == nil)
		return;
	snprint(buf, sizeof buf, "#p/%d/args", getpid());
	if((fd = open(buf, OWRITE)) >= 0){
		write(fd, cmdname, strlen(cmdname)+1);
		close(fd);
	}
	free(cmdname);
}

void
newproc(void (*f)(void *), void *arg, char *text)
{
	int kid = rfork(RFPROC|RFMEM|RFNOWAIT);

	if (kid < 0)
		sysfatal("can't fork: %r");
	if (kid == 0) {
		procsetname("%s", text);
		(*f)(arg);
		exits("child returned");
	}
}

#include <u.h>
#include <libc.h>
#include <bio.h>

#include "modem.h"

static Modem modems[1];

static char *spool = "/mail/faxqueue";
static char *type = "default";
static char *receiverc = "/sys/lib/fax/receiverc";

static void
receivedone(Modem *m, int ok)
{
	char *argv[10], *p, time[16], pages[16];
	int argc;

	faxrlog(m, ok);
	if(ok != Eok)
		return;

	argc = 0;
	if(p = strrchr(receiverc, '/'))
		argv[argc++] = p+1;
	else
		argv[argc++] = receiverc;
	sprint(time, "%lud.%d", m->time, m->pid);
	argv[argc++] = time;
	argv[argc++] = "Y";
	sprint(pages, "%d", m->pageno-1);
	argv[argc++] = pages;
	if(m->valid & Vftsi)
		argv[argc++] = m->ftsi;
	argv[argc] = 0;
	exec(receiverc, argv);
	exits("can't exec");
}

static void
usage(void)
{
	fprint(2, "%s: usage: %s [-v] [-s dir]\n", argv0, argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	Modem *m;

	m = &modems[0];

	ARGBEGIN{
	case 'v':
		vflag = 1;
		break;

	case 's':
		spool = ARGF();
		break;

	default:
		usage();
		break;

	}ARGEND

	initmodem(m, 0, -1, type, 0);
	receivedone(m, faxreceive(m, spool));

	exits(0);
}

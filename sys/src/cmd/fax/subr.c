/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>

#include "modem.h"

int vflag;

void
verbose(char *fmt, ...)
{
	va_list arg;
	char buf[512];

	if(vflag){
		va_start(arg, fmt);
		vseprint(buf, buf+sizeof(buf), fmt, arg);
		va_end(arg);
		syslog(0, "fax", buf);
	}
}

void
error(char *fmt, ...)
{
	va_list arg;
	char buf[512];
	int n;

	n = sprint(buf, "%s: ", argv0);
	va_start(arg, fmt);
	vseprint(buf+n, buf+sizeof(buf)-n, fmt, arg);
	va_end(arg);
	fprint(2, buf);
	if(vflag)
		print(buf+n);
	exits("error");
}

static char *errors[] = {
	[Eok] =		"no error",
	[Eattn] =	"can't get modem's attention",
	[Enoanswer] =	"Retry, no answer or busy",
	[Enoresponse] =	"Retry, no response from modem",
	[Eincompatible] = "Retry, incompatible",
	[Esys] =	"Retry, system call error",
	[Eproto] =	"Retry, fax protocol botch",
};

int
seterror(Modem *m, int error)
{
	if(error == Esys)
		sprint(m->error, "%s: %r", errors[Esys]);
	else
		strcpy(m->error, errors[error]);
	verbose("seterror: %s", m->error);
	return error;
}

void
faxrlog(Modem *m, int ok)
{
	char buf[1024];
	int n;

	n = sprint(buf, "receive %lu %c %d", m->time, ok == Eok ? 'Y': 'N', m->pageno-1);
	if(ok == Eok && (m->valid & Vftsi))
		sprint(buf+n, " %s", m->ftsi);
	syslog(0, "fax", buf);
}

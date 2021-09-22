#include <u.h>
#include <libc.h>
#include <thread.h>
#include <error.h>

int	tmach;
int	Errorprint	= 1;

Mach*
getmach(void)
{
	Mach *m;

	if(tmach == 0)
		tmach = tprivalloc();
	if((m = *tprivaddr(tmach)) == nil){
		m = mallocz(sizeof(Mach), 1);
		assert(m);
		*tprivaddr(tmach) = m;
	}
	return m;
}

void
fmterror(char *fmt, ...)
{
	Mach *m;
	va_list args;
	char e[128];

	va_start(args, fmt);
	vsnprint(e, sizeof e - 1, fmt, args);
	va_end(args);
	if(Errorprint)
		fprint(2, "ERROR(%#p): %s\n", getcallerpc(&fmt), e);
	m = *tprivaddr(tmach);
	errstr(e, strlen(e)+1);	/* Include null byte at the end */
	assert(m->nerrlab > 0 && m->nerrlab < NERR);
	longjmp(m->errlab[--m->nerrlab], -1);
}

void
nexterror(void)
{
	Mach *m;

	m = *tprivaddr(tmach);
	assert(m->nerrlab > 0 && m->nerrlab < NERR);
	longjmp(m->errlab[--m->nerrlab], -1);
}

jmp_buf *
_waserror(void)
{
	Mach *m;

	if(tmach == 0)
		tmach = tprivalloc();
	if ((m = *tprivaddr(tmach)) == nil){
		m = mallocz(sizeof(Mach), 1);
		*tprivaddr(tmach) = m;
	}
	m->nerrlab++;
	assert(m->nerrlab > 0 && m->nerrlab < NERR);
	return &m->errlab[m->nerrlab-1];
}

void
poperror(void)
{
	Mach *m;

	m = *tprivaddr(tmach);
	assert(m->nerrlab > 0 && m->nerrlab < NERR);
	m->nerrlab--;
}

void
error(char *err)
{
	char e[128];
	Mach *m;

	strncpy(e, err, sizeof(e));
	e[sizeof(e)-1] = 0;
	if(Errorprint)
		fprint(2, "ERROR(%#p): %s\n", getcallerpc(&err), err);
	m = *tprivaddr(tmach);
	assert(m->nerrlab > 0 && m->nerrlab < NERR);
	errstr(e, sizeof(e));
	longjmp(m->errlab[--m->nerrlab], -1);
}

void
silenterror(char *fmt, ...)
{
	Mach *m;
	va_list args;
	char e[128];

	va_start(args, fmt);
	vsnprint(e, sizeof e - 1, fmt, args);
	va_end(args);
	m = *tprivaddr(tmach);
	errstr(e, sizeof(e));	/* Include null byte at the end */
	assert(m->nerrlab > 0 && m->nerrlab < NERR);
	longjmp(m->errlab[--m->nerrlab], -1);
}

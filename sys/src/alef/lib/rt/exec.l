#include	<alef.h>
#include	"run.h"

int	_exec(byte*, byte**);
void	ALEF_lockrele(void);

intern
void
ALEF_etrans(byte *name, byte **argv)
{
	void *s;
	byte e[ERRLEN];

	s = PROC.tdb->ctask->stack;
	free(s);
	ALEF_lockrele();
	_exec(name, argv);
	errstr(e);
	_exits(e);
}

int
exec(byte *name, byte **argv)
{
	int l, n, i;
	byte **sp, **p, *aspace;

	if(PROC.tdb->ntask != 1) {
		werrstr("proc has more than one task");
		return -1;
	}
	if(access(name, 1) < 0)
		return -1;

	n = 0;
	p = argv;
	while(*p++)
		n++;
	n += 4;

	sp = (byte**)Execstk;
	aspace = (byte*)sp + sizeof(byte*)*n;

	l = strlen(name)+1;
	memmove(aspace, name, l);
	*sp++ = aspace;
	aspace += l;

	*sp = (byte*)sp+4;
	sp++;
	while(*argv) {
		l = strlen(*argv)+1;
		memmove(aspace, *argv, l);
		*sp++ = aspace;
		aspace += l;
		argv++;
	}
	*sp = nil;
	ALEF_linksp(&i, (byte*)Execstk, ALEF_etrans);

	return 0;	/* Not reached */
}

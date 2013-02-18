#include	"dat.h"
#include	"fns.h"

/*
 * General OS interface to errors
 */
void
kwerrstr(char *fmt, ...)
{
	va_list arg;
	char buf[ERRMAX];

	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	kstrcpy(up->env->errstr, buf, ERRMAX);
}

void
kerrstr(char *err, uint size)
{
	char tmp[ERRMAX];

	kstrcpy(tmp, up->env->errstr, sizeof(tmp));
	kstrcpy(up->env->errstr, err, ERRMAX);
	kstrcpy(err, tmp, size);
}

void
kgerrstr(char *err, uint size)
{
	char *s;

	s = "<no-up>";
	if(up != nil)
		s = up->env->errstr;
	kstrcpy(err, s, size);	/* TO DO */
}

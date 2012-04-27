#include <u.h>
#include <libc.h>
#include <error.h>

Error**	__ep;

void
errinit(int stack)
{
	Error *e;

	e = mallocz(sizeof(Error) + sizeof(e->label)*(stack-1), 1);
	if(e == nil)
		sysfatal("errinit: no memory");
	if(__ep == nil)
		__ep = privalloc();
	*__ep = e;
	e->nlabel = stack;
}

int
nerrors(void)
{
	return (*__ep)->nerr;
}

void
noerror(void)
{
	Error *e;

	e = *__ep;
	if(e->nerr > e->nlabel - 3){
		fprint(2, "error stack about to overflow. resizing");
		e->nlabel *= 2;
		(*__ep) = realloc(e, sizeof(Error) +
				sizeof(e->label)*(e->nlabel-1));
		if(*__ep == nil)
			sysfatal("can't resize error stack: no memory");
	}
	if((*__ep)->nerr-- == 0)
		sysfatal("noerror w/o catcherror");
}

void
error(char* msg, ...)
{
	char	buf[500];
	va_list	arg;

	if(msg != nil){
		va_start(arg, msg);
		vseprint(buf, buf+sizeof(buf), msg, arg);
		va_end(arg);
		werrstr("%s", buf);
	}
	if((*__ep)->nerr == 0)
		sysfatal("%s", buf);
	(*__ep)->nerr--;
	longjmp((*__ep)->label[(*__ep)->nerr], 1);
}

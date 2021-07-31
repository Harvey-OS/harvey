#include "all.h"

#define	DOTDOT	(&fmt+1)

static Cmd *head;
static Cmd *tail;
static Cmd *freecmd;

int	cmdpending;

void
putcmd(char *fmt, ...)
{
	Cmd *cp;

	if(cp = freecmd)	/* assign = */
		freecmd = cp->next;
	else
		cp = malloc(sizeof(Cmd));
	cp->next = 0;

	cp->wptr = doprint(cp->buf, cp->buf+CMDSIZE, fmt, DOTDOT);
	if(head)
		tail->next = cp;
	else
		head = cp;
	tail = cp;
	if(debug)
		print("queued \"%s\"\n", cp->buf);
	sendcmd();
}

void
sendcmd(void)
{
	Cmd *cp;

	if(cmdpending)
		return;
	if(!(cp = head))	/* assign = */
		return;
	head = cp->next;
	if(head == 0)
		tail = 0;
	write(fonefd, cp->buf, cp->wptr-cp->buf);
	if(debug)
		print("sent \"%s\"\n", cp->buf);
	cmdpending = 1;
	cp->next = freecmd;
	freecmd = cp->next;
}

void
flushcmd(void)
{
	if(!cmdpending || !head)
		return;
	cmdpending = 0;
	tail->next = freecmd;
	freecmd = head;
	head = 0;
	tail = 0;
}

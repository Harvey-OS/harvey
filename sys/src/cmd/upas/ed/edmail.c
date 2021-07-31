/*
 *	print mail
 */
#include "common.h"
#include "print.h"

/* global to this file */
static int reverse=0; 		/* ordering of mail messages */
static jmp_buf	errjbuf;
int fflg;
int mflg;
int pflg;
int eflg;

/* predeclared */
static char	*doargs(int, char*[], char*);
static int	check_mbox (char*);
static void	dumpmail(void);
static int	notatnl(char*);
static int	atnl(char*);
static int	zero(message*);
static int	edmail(char*, int);
static message	*range(message*, String*);
static message	*grange(String*);
static message	*addr(message*, String*);
static message	*incraddr(message*, int, int);
static message	*research(message*, char*, int);
static message	*bresearch(message*, char*, int);
static int	getnumb(String*);
static char	*getre(String*);

Biobuf in;
Biobuf out;

void
main(int ac, char *av[])
{
	int writeable;
	String *user = s_new();
	char *mailfile;
	char *logname;

	Binit(&in, 0, OREAD);
	Binit(&out, 1, OWRITE);

	logname = getlog();
	if (logname == 0) {
		fprint(2, "cannot determine login name\n");
		exits(0);
	}
	s_append(user, logname);
	mailfile = doargs(ac, av, s_to_c(user));
	if (!fflg)
		writeable = P(mailfile)==0;
	else
		writeable = 1;
	if (eflg) {
		int r = check_mbox(mailfile);
		V();
		exit(r);
	}
	if (read_mbox(mailfile, reverse)<0 || mlist == 0){
		fprint(2, "No mail\n");
		V();
		exits(0);
	}
	fprint(2, "%d messages\n", mlast->pos);
	if (pflg)
		dumpmail();
	else{
		while (edmail(mailfile, reverse) && writeable){
			if(!write_mbox(mailfile, reverse))
				break;
		}
	}
	V();
	exits(0);
}

/*  create a mailbox  */
static void
creatembox(char *file, char *user)
{
	char *cp;
	Biobuf *fp;

	/*
	 *  don't destroy an existing mail box
	 */
	if(sysexist(file)){
		fprint(2, "mailbox already exists\n");
		return;
	}
	fprint(2, "creating new mbox\n");

	/*
	 *  make sure one level up exists
	 */
	cp = strrchr(file, '/');
	if(cp){
		*cp = 0;
		if(!sysexist(file)){
			if(sysmkdir(file, 0711)<0)
				fprint(2, "couldn't create %s\n", file);
			syschgrp(file, user);
		}
		*cp = '/';
	}

	/*
	 *  create the file
	 */
	fp = sysopen(file, "larc", MBOXMODE);
	if(fp == 0)
		fprint(2, "couldn't create %s\n", file);
	else
		sysclose(fp);
}

/*  parse arguments  */
static char *
doargs(int argc, char **argv, char *user)
{
	String *mailfile;
	char *f;

	/* process args */
	mailfile = s_new();
	mboxpath("mbox", user, mailfile, 0);
	ARGBEGIN{
	case 'c':
		creatembox(s_to_c(mailfile), user);
		break;
	case 'r':
		reverse = 1;
		break;
	case 'm':
		mflg = 1;
		break;
	case 'p':
		pflg = 1;
		break;
	case 'e':
		eflg = 1;
		break;
	case 'f':
		fflg = 1;
		f = ARGF();
		if(f)
			mboxpath(f, getlog(), s_restart(mailfile), 0);
		else
			mboxpath("stored", getlog(), s_restart(mailfile), 0);
		break;
	case 'F':
		fflg = 1;
		f = ARGF();
		if (f)
			mboxpath(f, getlog(), s_restart(mailfile), 1);
		else
			mboxpath("stored", getlog(), s_restart(mailfile), 1);
		break;
	default:
		fprint(2, "usage: mail [-cmpre] [-f mbox]\n");
		exit(1);
	} ARGEND
	return s_to_c(mailfile);
}

/* is the mailbox empty?  1 if yes, 0 if no */
static int
check_mbox(char *mf)
{
	Biobuf *fp;
	int len;

	/* if file doesn't exist, no mail */
	if ((fp = sysopen(mf, "r", 0)) == 0)
		return 1;
	len = sysfilelen(fp);
	sysclose(fp);
	return len > 0;
}

int
complain(char *msg, void *x)
{
	fprint(2, "!%s\n", msg, x);
	return -1;
}

/* mesage dump */
static void
dumpmail(void)
{
	message *mp;

	for (mp=mlist; mp!=0; mp=mp->next)
		m_print(mp, &out, 1, 1);
	Bflush(&out);
}

void
catchint(void *a, char *msg)
{
	char buf[256];

	if(strcmp(msg, "interrupt") && strncmp(msg, "sys: write on closed pipe", 25))
		noted(NDFLT);
	sprint(buf, "!!%s!!\n", msg);
	write(2, buf, strlen(buf));
	notejmp(a, errjbuf, 1);
}

static int
notatnl(char *cp)
{
	if (*cp=='\n')
		return complain("argument expected", 0);
	return 0;
}

static int
atblank(char *cp)
{
	if (*cp!='\n' && *(cp-1)!=' ' && *(cp-1)!='\t')
		return complain("newline or space expected", 0);
	return 0;
}

static int
atnl(char *cp)
{
	if (*cp!='\n')
		return complain("newline expected", 0);
	return 0;
}

static int
zero(message *mp)
{
	if (mp==mzero)
		return complain("message 0", 0);
	return 0;
}

#define s_skipwhite(s) for (; *s->ptr==' ' || *s->ptr=='\t'; s->ptr++);

/* ed style interface */
static int
edmail(char *mailfile, int reverse)
{
	message *dot;
	message *extent;
	String *cmd=s_new();
	char *cp;
	int cmdc;
	int i, abort, nopr, change;
	int del;

	dot = mzero;
	nopr = mflg;
	for(;;) {
		extent = dot;	/* in case of interrupt */
		if (!nopr) {
			/* advance only if we want to print next message */
			if(dot->next!=0)
				dot = dot->next;
			else
				nopr = 1;
		}
		if (setjmp(errjbuf)) {
			/* come here after interrupt */
			if (extent!=0)
				dot = extent;
			nopr = 1;
			Binit(&out, 1, OWRITE);		/* dump current output */
			Bprint(&out, "\n");
		}
		notify(catchint);
		if (!nopr&&dot!=mzero) {
			/* print next message */
			printm(dot);
		}
		abort = 0;
		nopr = 1;
		change = 1;
		Bprint(&out, "?");
		Bflush(&out);
		if(s_read_line(&in, s_restart(cmd)) == 0)
			return 1;
		s_restart(cmd);
		s_skipwhite(cmd);
		if (!mflg && *cmd->ptr=='\n' && dot==mlast)
			return 1;
		extent = range(dot, cmd);
		s_skipwhite(cmd);
		del = 0;
compoundcmd:
		/* hack to catch a common mistake */
		if (strncmp(cmd->ptr, "mail", 4)==0)
			cmdc = -1;
		else
			cmdc = *cmd->ptr++;
		s_skipwhite(cmd);
		cp = cmd->ptr;
		for(; extent!=0 && !abort; extent=extent->extent) {
			switch(cmdc){
			case 'b':
				abort = atnl(cp)||(del&&delete(extent));
				if (!abort)
					if (extent!=mzero)
						prheader(extent);
				for(i=0; extent->next!=0&&i<9; i++){
					extent = extent->next;
					prheader(extent);
				}
				break;
			case 'h':
				abort = atnl(cp)||zero(extent)
					||(del&&delete(extent))
					||prheader(extent);
				break;
			case 'd':
				if(*cp=='\n' || *cp==0){
					abort = zero(extent)||delete(extent);
					nopr = abort||mflg;
					break;
				}
				del = 1;
				goto compoundcmd;
			case 's':
				abort = atblank(cp)||zero(extent)||store(extent, cp, 1, 0)
					||(del&&delete(extent));
				nopr = abort||mflg||!del;
				break;
			case 'S':
				abort = atblank(cp)||zero(extent)||store(extent, cp, 1, 1)
					||(del&&delete(extent));
				nopr = abort||mflg||!del;
				break;
			case 'w':
				abort = atblank(cp)||zero(extent)||notatnl(cp)
					||store(extent, cp, 0, 0)
					||(del&&delete(extent));
				nopr = abort||mflg||!del;
				break;
			case 'W':
				abort = atblank(cp)||zero(extent)||notatnl(cp)
					||store(extent, cp, 0, 1)
					||(del&&delete(extent));
				nopr = abort||mflg||!del;
				break;
			case 'm':
				abort = atblank(cp)||zero(extent)||notatnl(cp)
					||remail(extent, cp, 0)
					||(del&&delete(extent));
				nopr = abort||mflg||!del;
				break;
			case 'M':
				abort = atblank(cp)||zero(extent)||notatnl(cp)
					||remail(extent, cp, 1)
					||(del&&delete(extent));
				nopr = abort||mflg||!del;
				break;
			case '\n':
				abort = zero(extent)||printm(extent);
				break;
			case 'p':
				if(del){
					nopr = abort = atnl(cp)||zero(extent)
						||(del&&delete(extent));
					break;
				}
				abort = atnl(cp)||zero(extent)||printm(extent);
				break;
			case 'r':
				abort = zero(extent)||atnl(cp)||reply(extent, 0)
					||(del&&delete(extent));
				nopr = abort||mflg||!del;
				break;
			case 'R':
				abort = zero(extent)||atnl(cp)||reply(extent, 1)
					||(del&&delete(extent));
				nopr = abort||mflg||!del;
				break;
			case 'q':
				abort=atnl(cp)
				    ||(del&&(zero(extent)||delete(extent)));
				if(abort)
					break;
				return 1;
			case '|':
				abort = zero(extent)||notatnl(cp)
					||pipemail(extent, cp, 0)
					||(del&&delete(extent));
				nopr = abort||mflg||!del;
				break;
			case '=':
				abort = atnl(cp)||(del&&delete(extent))
					||whereis(extent);
				change = 0;
				break;
			case '!':
				abort=del&&(zero(extent)||delete(extent));
				if(abort)
					break;
				escape(cp);
				break;
			case 'u':
				abort = zero(extent)||atnl(cp)||undelete(extent);
				break;
			case 'x':
				return 0;
			case '?':
				abort = atnl(cp)
					||del&&(zero(extent)||delete(extent));
				if(abort)
					break;
				help();
				nopr = abort = 1;
				break;
			case 'i':
				abort = atnl(cp)
					||(del&&(zero(extent)||delete(extent)));
				if(abort)
					break;
				reread_mbox(mailfile, reverse);
				nopr = abort = 1;
				break;
			default:
				Bprint(&out, "!unknown command (type ? for help)\n");
				nopr = abort = 1;
				break;
			}
			if (!abort&&change&&extent!=0)
				dot = extent;
		}
	}
	return 0;
}

/* parse an address range */
static message *
range(message *dot, String *cmd)
{
	message *first, *last;

	s_skipwhite(cmd);

	/* get first address in range */
	switch(*cmd->ptr) {
	case 'g':
		cmd->ptr++;
		return grange(cmd);
	case ',':
		first = mlist;
		break;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		first = addr((message *)0, cmd);
		break;
	case '?':
		if (*(cmd->ptr+1)=='\n') {
			if(dot!=0)
				dot->extent = 0;
			return dot;
		}
	case '%': case '/': case '+': case '-': case '$': case '.':
		first = addr(dot, cmd);
		break;
	case '\n':
		first = dot->next;
		if (first==0)
			complain("address", 0);
		break;
	default:
		if(dot!=0)
			dot->extent = 0;
		return dot;
	}
	if (first == 0)
		return 0;
	while(*cmd->ptr == ' ' || *cmd->ptr == '\t')
		cmd->ptr++;
	if (*cmd->ptr != ',') {
		first->extent = 0;
		return first;
	}

	/* get second address in range */
	cmd->ptr++;
	while(*cmd->ptr == ' ' || *cmd->ptr == '\t')
		cmd->ptr++;
	switch(*cmd->ptr) {
	case '%': case '/': case '?': case '+': case '-': case '$':
		last = addr(first, cmd);
		break;
	case '.':
		last = addr(dot, cmd);
		break;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		last = addr((message *)0, cmd);
		break;
	default:
		last = mlast;
		break;
	}
	if (last==0)
		return 0;

	/* fill in the range */
	for(dot=first; dot!=last && dot!=0; dot=dot->next)
		dot->extent = dot->next;
	if (dot==0) {
		complain("addresses out of order", 0);
		return 0;
	}
	last->extent = 0;
	return first;
}

/* parse a global range */
static message *
grange(String *cmd)
{
	char *re;
	message *first, *last, *next;
	Reprog *pp;

	if (*cmd->ptr == '/') {
		re = getre(cmd);
		if (re == 0)
			return 0;
		if ((pp = regcomp(re))==0)
			return 0;
		first = last = 0;
		for(next=mlist; next!=0; next=next->next)
			if (regexec(pp, header(next), 0, 0)) {
				if (first==0)
					first = last = next;
				else {
					last->extent = next;
					last = next;
				}
				last->extent = 0;
			}
		if (first==0)
			complain("match", 0);
		free((char *)pp);
		return first;
	} else if (*cmd->ptr == '%') {
		re = getre(cmd);
		if (re == 0)
			return 0;
		if ((pp = regcomp(re))==0)
			return 0;
		first = last = 0;
		for(next=mlist; next!=0; next=next->next){
			s_to_c(next->body)[next->size-1]='\0';
			if (regexec(pp, s_to_c(next->body), 0, 0)) {
				if (first==0)
					first = last = next;
				else {
					last->extent = next;
					last = next;
				}
				last->extent = 0;
			}
		}
		if (first==0)
			complain("match", 0);
		free((char *)pp);
		return first;
	} else {
		/* fill in the range */
		for(next=mlist; next!=0; next=next->next)
			next->extent = next->next;
		mlast->extent = 0;
		return mlist;
	}
}

/* parse an address */
static message *
addr(message *base, String *cmd)
{
	int forward = -1;

	/* get direction */
	switch(*cmd->ptr) {
	case '+':
		forward = 1;
		cmd->ptr++;
		break;
	case '-':
		forward = 0;
		cmd->ptr++;
		break;
	}
	while(*cmd->ptr == ' ' || *cmd->ptr == '\t')
		cmd->ptr++;
	switch(*cmd->ptr) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		base = incraddr(base, getnumb(cmd), forward);
		if (base==0)
			break;
		return addr(base, cmd);
	case '?':
		forward = !forward;
	case '/':
		base = research(base, getre(cmd), forward);
		if (base==0){
			complain("search", 0);
			return 0;
		}
		return addr(base, cmd);
	case '%':
		base = bresearch(base, getre(cmd), forward);
		if (base==0) {
			complain("search", 0);
			return 0;
		}
		return addr(base, cmd);
	case '.':
		cmd->ptr++;
		return addr(base, cmd);;
	case '$':
		cmd->ptr++;
		return addr(mlast, cmd);
	default:
		/* default increment is 1 */
		if (forward != -1) {
			base = incraddr(base, 1, forward);
			return addr(base, cmd);
		}
		break;
	}
	if (base==0)
		complain("address", 0);
	return base;
}

/* increment the base address by offset */
static message *
incraddr(message *base, int offset, int forward)
{
	if (base==0) {
		base = mzero;
	}
	for(; offset > 0 && base != 0; offset--)
		base = forward?(base->next):(base->prev);
	return base;
}

/* look for the message header after base that matches the reg exp */
static message *
research(message *base, char *rp, int forward)
{
	Reprog *pp;
	message *mp;

	if (rp == 0) 
		return 0;
	if ((pp = regcomp(rp))==0)
		return 0;
	mp = base;
	base = (base==mzero)?(forward?(mp->prev):(mp->next)):base;
	do {
		mp = forward?(mp->next):(mp->prev);
		if (mp==0)
			mp = forward?mlist:mlast;
		if (regexec(pp, header(mp), 0, 0)) {
			free((char *)pp);
			return mp;
		}
	} while (mp!=base);
	free((char *)pp);
	return 0;
}

/* look for the message after base that matches the reg exp */
static message *
bresearch(message *base, char *rp, int forward)
{
	Reprog *pp;
	message *mp;

	if (rp == 0) 
		return 0;
	if ((pp = regcomp(rp))==0)
		return 0;
	mp = base = (base==mzero)?mzero->next:base;
	do {
		mp = forward?(mp->next):(mp->prev);
		if (mp==0)
			mp = forward?mlist:mlast;
		s_to_c(mp->body)[mp->size-1]='\0';
		if (regexec(pp, s_to_c(mp->body), 0, 0)) {
			free((char *)pp);
			return mp;
		}
	} while (mp!=base);
	free((char *)pp);
	return 0;
}

#define isdigit(x) (x >= '0' && x <='9')

/* get a number out of the command */
static int
getnumb(String *cmd)
{
	int offset=0;
	while (isdigit(*cmd->ptr))
		offset = offset*10 + *cmd->ptr++ - '0';
	return offset;
}

/* get a regular exression out of the command */
static char *
getre(String *cmd)
{
	static char re[80];
	char *cp=re;
	char term = *cmd->ptr++;

	while (*cmd->ptr!='\0' && *cmd->ptr!='\n' && *cmd->ptr!=term)
		*cp++ = *cmd->ptr++;
	if (*cmd->ptr == term)
		cmd->ptr++;
	if (cp == re) {
		if (*re == '\0') {
			complain("no previous regular expression", 0);
			return 0;
		}
	} else
		*cp = '\0';
	return re;
}

extern void
regerror(char *msg)
{
	complain("illegal address: %s", msg);
}

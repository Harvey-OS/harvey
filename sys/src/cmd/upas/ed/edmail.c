/*
 *	print mail
 */
#include "common.h"
#include "print.h"

/* global to this file */
static int reverse=0; 		/* ordering of mail messages */
int interrupted;
int fflg;
int iflg;
int mflg;
int pflg;
int eflg;
int Dflg;
int Pflg = 1;
int writeable;

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
int alarmed;

void
main(int ac, char *av[])
{
	String *user = s_new();
	char *mailfile;
	char *logname;

	Binit(&in, 0, OREAD);
	Binit(&out, 1, OWRITE);

	logname = getlog();
	if(logname == nil){
		fprint(2, "cannot determine login name\n");
		exits(0);
	}
	s_append(user, logname);
	mailfile = doargs(ac, av, s_to_c(user));
	if(!fflg)
		writeable = P(mailfile)==0;
	else
		writeable = 1;
	if(eflg){
		int r = check_mbox(mailfile);
		V();
		exit(!r);
	}
	if(read_mbox(mailfile, reverse)<0 || mlist == 0){
		fprint(2, "No mail\n");
		V();
		exits(0);
	}
	fprint(2, "%d messages\n", mlast->pos);
	if(pflg)
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
static String*
creatembox(char *user, char *folder)
{
	char *p;
	Biobuf *fp;
	String *mailfile;
	char buf[512];

	p = getenv("upasname");
	if(p)
		user = p;

	mailfile = s_new();
	if(folder == 0)
		mboxname(user, mailfile);
	else {
		snprint(buf, sizeof(buf), "%s/mbox", folder);
		mboxpath(buf, user, mailfile, 0);
	}
	/*
	 *  don't destroy an existing mail box
	 */
	if(sysexist(s_to_c(mailfile))){
		fprint(2, "mailbox already exists\n");
		return mailfile;
	}
	fprint(2, "creating new mbox\n");

	/*
	 *  make sure preceding levels exist
	 */
	for(p = s_to_c(mailfile); p; p++) {
		if(*p == '/')	/* skip leading or consecutive slashes */
			continue;
		p = strchr(p, '/');
		if(p == 0)
			break;
		*p = 0;
		if(!sysexist(s_to_c(mailfile))){
			if(sysmkdir(s_to_c(mailfile), 0711)<0)
				fprint(2, "couldn't create %s\n", s_to_c(mailfile));
			syschgrp(s_to_c(mailfile), user);
		}
		*p = '/';
	}

	/*
	 *  create the file
	 */
	fp = sysopen(s_to_c(mailfile), "larc", MBOXMODE);
	if(fp == 0)
		fprint(2, "couldn't create %s\n", s_to_c(mailfile));
	else
		sysclose(fp);
	return mailfile;
}

/*  parse arguments  */
static char *
doargs(int argc, char **argv, char *user)
{
	String *mailfile;
	char *f;

	/* process args */
	mailfile = s_new();
	mboxname(user, mailfile);
	ARGBEGIN{
	case 'c':
		s_free(mailfile);
		mailfile = creatembox(user, ARGF());
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
	case 'i':
		iflg = 1;
		break;
	case 'D':
		Dflg = 1;
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
		if(f)
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
	fp = sysopen(mf, "r", 0);
	if(fp == 0)
		return 0;
	len = sysfilelen(fp);
	sysclose(fp);
	return len > 0;
}

int
complain(char *msg, void *x)
{
	char buf[512];

	snprint(buf, sizeof(buf), msg, x);
	fprint(2, "!%s\n", buf);
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

static int
notatnl(char *cp)
{
	while(*cp == ' ' && *cp == '\t')
		cp++;
	if(*cp=='\n')
		return complain("argument expected", 0);
	return 0;
}

static int
atblank(char *cp)
{
	if(*cp!='\n' && *cp!=' ' && *cp!='\t')
		return complain("newline or space expected", 0);
	return 0;
}

static int
atnl(char *cp)
{
	if(*cp!='\n')
		return complain("newline expected", 0);
	return 0;
}

static int
zero(message *mp)
{
	if(mp==mzero)
		return complain("message 0", 0);
	return 0;
}

static void
renew(void)
{
	extern Mlock *readingl;

	if(readingl == 0)
		return;

	if(seek(readingl->fd, 0, 0) < 0
	|| write(readingl->fd, readingl, 0) < 0)
		/*print("\nlost L.reading\n")/**/;
	alarm(1000*30);
}

/*
 *  read a command making sure we renew our P() lock every minute
 */
static char*
read_cmd(Biobuf *b, String *cmd)
{
	char *p;

	do {
		alarmed = 0;
		renew();
		p = s_read_line(b, s_restart(cmd));
		alarm(0);
	} while(!interrupted && p == 0 && alarmed);
	return p;
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
	for(;;){
		extent = dot;	/* in case of interrupt */

		/*
		 *  advance only if we want to print next message
		 */
		if(!nopr){
			if(dot->next!=0)
				dot = dot->next;
			else
				nopr = 1;
		}

		/*
		 *  on interrupt during processing command
		 *  come here, set dot, and print newline for neatness.
		 */
		if(interrupted){
			interrupted = 0;
			if(extent!=0)
				dot = extent;
			nopr = 1;
			Binit(&out, 1, OWRITE);		/* dump current output */
			Bprint(&out, "\n");
		}
		atnotify(notecatcher, 1);		/* install normal note handler */

		/*
		 *  print next message unless told not to
		 */
		if(!nopr && dot != mzero){
			if(Pflg)
				seanprintm(dot);
			else
				printm(dot);
		}

		/*
		 *  get next command
		 */
		abort = 0;
		nopr = 1;
		change = 1;
		Bprint(&out, "?");
		Bflush(&out);
		if(read_cmd(&in, s_restart(cmd)) == 0){
			if(interrupted)
				continue;
			return 1;
		}
		s_restart(cmd);
		s_skipwhite(cmd);
		if(!mflg && *cmd->ptr=='\n' && dot==mlast)
			return 1;

		/* get message range to act on */
		extent = range(dot, cmd);
		s_skipwhite(cmd);
		del = 0;

		/* get actual command character */
		cmdc = *cmd->ptr++;
		cp = cmd->ptr;

		/* 'd' can compound with any other command */
		if(cmdc == 'd' && *cp && *cp != '\n'){
			del = 1;
			cmdc = *cmd->ptr++;
		}

		for(; extent!=0 && !abort; extent=extent->extent){
			cp = strdup(cmd->ptr);
			switch(cmdc){
			case 'a':
				abort = zero(extent)||atnl(cp)||replyall(extent, 0)
					||(del&&delete(extent));
				nopr = abort||mflg||!del;
				break;
			case 'A':
				abort = zero(extent)||atnl(cp)||replyall(extent, 1)
					||(del&&delete(extent));
				nopr = abort||mflg||!del;
				break;
			case 'b':
				abort = atnl(cp)||(del&&delete(extent));
				if(!abort)
					if(extent!=mzero)
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
				abort = zero(extent)||delete(extent);
				nopr = abort||mflg;
				break;
			case 's':
				abort = atblank(cp)||zero(extent)
					||store(extent, cp, 1, 0)
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
				abort = notatnl(cp)||zero(extent)||notatnl(cp)
					||remail(extent, cp, 1)
					||(del&&delete(extent));
				nopr = abort||mflg||!del;
				break;
			case '\n':
				abort = zero(extent)
					||(Pflg?seanprintm(extent):printm(extent));
				break;
			case 'p':
				if(del){
					nopr = abort = atnl(cp)||zero(extent)
						||delete(extent);
					break;
				}
				abort = atnl(cp)||zero(extent)
					||(Pflg?seanprintm(extent):printm(extent));
				break;
			case 'P':
				if(del){
					nopr = abort = atnl(cp)||zero(extent)
						||delete(extent);
					break;
				}
				abort = atnl(cp)||zero(extent)
					||(Pflg?printm(extent):seanprintm(extent));
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
				free(cp);
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
				abort = atnl(cp);
				if(abort == 0){
					free(cp);
					return 0;
				}
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
			free(cp);
			if(!abort && change && extent!=0)
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
	switch(*cmd->ptr){
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
		if(*(cmd->ptr+1)=='\n'){
			if(dot!=0)
				dot->extent = 0;
			return dot;
		}
	case '%': case '/': case '+': case '-': case '$': case '.':
		first = addr(dot, cmd);
		break;
	case '\n':
		first = dot->next;
		if(first==0)
			complain("address", 0);
		break;
	default:
		if(dot!=0)
			dot->extent = 0;
		return dot;
	}
	if(first == 0)
		return 0;
	while(*cmd->ptr == ' ' || *cmd->ptr == '\t')
		cmd->ptr++;
	if(*cmd->ptr != ','){
		first->extent = 0;
		return first;
	}

	/* get second address in range */
	cmd->ptr++;
	while(*cmd->ptr == ' ' || *cmd->ptr == '\t')
		cmd->ptr++;
	switch(*cmd->ptr){
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
	if(last==0)
		return 0;

	/* fill in the range */
	for(dot=first; dot!=last && dot!=0; dot=dot->next)
		dot->extent = dot->next;
	if(dot==0){
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

	if(*cmd->ptr == '/'){
		re = getre(cmd);
		if(re == 0)
			return 0;
		if((pp = regcomp(re))==0)
			return 0;
		first = last = 0;
		for(next=mlist; next!=0; next=next->next)
			if(regexec(pp, header(next), 0, 0)){
				if(first==0)
					first = last = next;
				else {
					last->extent = next;
					last = next;
				}
				last->extent = 0;
			}
		if(first==0)
			complain("match", 0);
		free((char *)pp);
		return first;
	} else if(*cmd->ptr == '%'){
		re = getre(cmd);
		if(re == 0)
			return 0;
		if((pp = regcomp(re))==0)
			return 0;
		first = last = 0;
		for(next=mlist; next!=0; next=next->next){
			if(regexec(pp, s_to_c(next->body), 0, 0)){
				if(first==0)
					first = last = next;
				else {
					last->extent = next;
					last = next;
				}
				last->extent = 0;
			}
		}
		if(first==0)
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
	switch(*cmd->ptr){
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
	switch(*cmd->ptr){
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		base = incraddr(base, getnumb(cmd), forward);
		if(base==0)
			break;
		return addr(base, cmd);
	case '?':
		forward = !forward;
	case '/':
		base = research(base, getre(cmd), forward);
		if(base==0){
			complain("search", 0);
			return 0;
		}
		return addr(base, cmd);
	case '%':
		base = bresearch(base, getre(cmd), forward);
		if(base==0){
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
		if(forward != -1){
			base = incraddr(base, 1, forward);
			return addr(base, cmd);
		}
		break;
	}
	if(base==0)
		complain("address", 0);
	return base;
}

/* increment the base address by offset */
static message *
incraddr(message *base, int offset, int forward)
{
	if(base==0){
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

	if(rp == 0) 
		return 0;
	if((pp = regcomp(rp))==0)
		return 0;
	mp = base;
	base = (base==mzero)?(forward?(mp->prev):(mp->next)):base;
	do {
		mp = forward?(mp->next):(mp->prev);
		if(mp==0)
			mp = forward?mlist:mlast;
		if(regexec(pp, header(mp), 0, 0)){
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

	if(rp == 0) 
		return 0;
	if((pp = regcomp(rp))==0)
		return 0;
	mp = base = (base==mzero)?mzero->next:base;
	do {
		mp = forward?(mp->next):(mp->prev);
		if(mp==0)
			mp = forward?mlist:mlast;
		if(regexec(pp, s_to_c(mp->body), 0, 0)){
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
	if(*cmd->ptr == term)
		cmd->ptr++;
	if(cp == re){
		if(*re == '\0'){
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

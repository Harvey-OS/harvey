#include <ctype.h>
#include "common.h"
#include "print.h"

int holding;
extern int interrupted, alarmed;

typedef struct Ignorance Ignorance;
struct Ignorance
{
	Ignorance *next;
	char	*str;		/* string */
	int	partial;	/* true if not exact match */
};
Ignorance *ignore;

static char pipemsg[] = "sys: write on closed pipe";

int
notecatcher(void *a, char *msg)
{
	USED(a);
	if(strcmp(msg, "interrupt") == 0 
	|| strncmp(msg, pipemsg, sizeof(pipemsg)-1) == 0){
		fprint(2, "!!%s!!\n", msg);
		interrupted = 1;
		return 1;
	}
	if(strstr(msg, "alarm")){
		alarmed = 1;
		return 1;
	}
	V();
	return 0;
}

static int
pipecatcher(void *a, char *msg)
{
	USED(a);
	if(strcmp(msg, "interrupt") == 0)
		return 1;
	if(strncmp(msg, pipemsg, sizeof(pipemsg)-1) == 0) {
		fprint(2, "!!%s!!\n", msg);
		return 1;
	}
	V();
	return 0;
}

/*
 * auxiliary routines used by commands
 */

/* drop the newline off a String */
static char *
dropleadingwhite(char *cmd)
{
	while(*cmd==' ' || *cmd=='\t')
		cmd++;
	return cmd;
}

/* drop the newline off a String */
static void
dropnewline(char *cmd)
{
	cmd = cmd+strlen(cmd)-1;
	if(*cmd=='\n')
		*cmd = '\0';
}

/* make sure an argument exists */
static int
needargument(char *cmd)
{
	if(*cmd=='\0') {
		fprint(2, "?argument missing\n");
		return -1;
	}
	return 0;
}

/* make the header line for a message */
extern char *
header(message *mp)
{
	static char hd[128];

	if(mp->subject)
		snprint(hd, sizeof(hd), "%3d %c %4d %s %.16s \"%.20s\"", mp->pos,
			mp->status&DELETED?'d':' ',
			mp->size, s_to_c(mp->sender),
			s_to_c(mp->date), s_to_c(mp->subject));
	else
		snprint(hd, sizeof(hd), "%3d %c %4d %s %.16s", mp->pos, mp->status&DELETED?'d':' ',
			mp->size, s_to_c(mp->sender), s_to_c(mp->date));
	return hd;
}

/*
 *	command routines 
 */

/* current message */
extern int
whereis(message *mp)
{
	Bprint(&out, "%d\n", mp->pos);
	return 0;
}

/* print header */
extern int
prheader(message *mp)
{
	Bprint(&out, "%s\n", header(mp));
	return 0;
}

/* change message status */
extern int
delete(message *mp)
{
	mp->status |= DELETED;
	return 0;
}
extern int
undelete(message *mp)
{
	mp->status &= ~DELETED;
	return 0;
}

/* store a message into a file */
extern int
store(message *mp, char *cmd, int header, int dot)
{
	static String *mbox;
	Biobuf *fp;
	char *u;

	dropnewline(cmd);
	cmd = dropleadingwhite(cmd);
	if(*cmd=='\0')
		cmd = "stored";
	mbox = s_reset(mbox);
	u = getlog();
	if(u == nil){
		fprint(2, "?cannot determine login name\n");
		return -1;
	}
	mboxpath(cmd, u, mbox, dot);
	fp = sysopen(s_to_c(mbox), "cA", 0660);
	if(fp == 0) {
		fprint(2, "?can't open %s\n", s_to_c(mbox));
		return -1;
	}
	if(header){
		if(m_store(mp, fp) < 0){
			sysclose(fp);
			fprint(2, "?error writing %s\n", s_to_c(mbox));
			return -1;
		}
	} else {
		if(m_print(mp, fp, 0, 0) < 0){
			sysclose(fp);
			fprint(2, "?error writing %s\n", s_to_c(mbox));
			return -1;
		}
	}
	sysclose(fp);
	return 0;
}

/* pipe a message (and possible tty input) to a command */
static int
pipecmd(message *mp, char *cmd, int fromtty, int mailinput, int forward, String *prologue)
{
	int rv=0;
	int n;
	char *line;
	stream *ins;
	process *p;

	if(fromtty){
		Bprint(&out, "!%s\n", cmd);
		Bflush(&out);
	}

	p = proc_start(cmd, ins = instream(), 0, 0, 0, "me");
	if(p == 0) {
		fprint(2, "?can't exec %s\n", cmd);
		return -1;
	}
	atnotify(notecatcher, 0);		/* special interrupt handler */
	atnotify(pipecatcher, 1);		/* special interrupt handler */
	if(prologue){
		if(fromtty){
			Bprint(&out, "%s\n", s_to_c(prologue));
			Bflush(&out);
		}
		Bprint(ins->fp, "%s\n", s_to_c(prologue));
	}
	if(fromtty){
		holding = holdon();
		for(;;){
			line = Brdline(&in, '\n');
			if(line == 0)
				break;
			if(iflg && strncmp(line, ".\n", 2) == 0)
				break;
			n = Blinelen(&in);
			if(Bwrite(ins->fp, line, n) != n){
				rv = -1;
				break;
			}
			Bflush(ins->fp);
		}
		holdoff(holding);
	}
	if(forward)
		Bprint(ins->fp, "\n------ forwarded message follows ------\n\n");
	if(mailinput)
		m_print(mp, ins->fp, 0, 1);
	stream_free(ins);
	p->std[0] = 0;
	if(proc_wait(p))
		rv = -1;
	proc_free(p);
	if(fromtty)
		Bprint(&out, "!\n");
	atnotify(pipecatcher, 0);		/* back to normal interrupt handler */
	atnotify(notecatcher, 1);
	return rv;
}

/* pass a message to someone else */
extern int
remail(message *mp, char *cmd, int ttyinput)
{
	static String *cmdString=0;

	dropnewline(cmd);
	if(cmdString == 0)
		cmdString = s_new();
	s_append(s_restart(cmdString), "/bin/mail ");
	s_append(cmdString, cmd);
	if(pipecmd(mp, s_to_c(cmdString), ttyinput, 1, 1, 0)<0)
		return -1;
	return 0;
}

/* output a message */
extern int
printm(message *mp)
{
	m_print(mp, &out, 0, 1);
	return 0;
}

/* pipe mail into a command */
extern int
pipemail(message *mp, char *cmd, int ttyinput)
{
	dropnewline(cmd);
	if(pipecmd(mp, cmd, ttyinput, 1, 0, 0)<0)
		return -1;
	return 0;
}

/* escape to the shell */
extern int
escape(char *cmd)
{
	char *cp;
	process *p;

	atnotify(notecatcher, 0);
	atnotify(pipecatcher, 1);		/* special interrupt handler */
	cp = cmd+strlen(cmd)-1;
	if(*cp=='\n')
		*cp = '\0';
	p = proc_start(cmd, 0, 0, 0, 0, "me");
	proc_wait(p);
	proc_free(p);
	Bprint(&out, "!\n");
	atnotify(pipecatcher, 0);		/* back to normal interrupt handler */
	atnotify(notecatcher, 1);
	return 0;
}

/*
 *  case independent string compare
 */
int
cistrncmp(char *s1, char *s2, int n)
{
	int c1, c2;

	for(; n > 0; n--){
		c1 = *s1++;
		c2 = *s2++;
		if(isupper(c1))
			c1 = tolower(c1);
		if(isupper(c2))
			c2 = tolower(c2);
		c2 -= c1;
		if(c2)
			return c2;
		if(c1 == 0)
			break;
	}
	return 0;
}

/* reply to a message */
extern int
reply(message *mp, int mailinput)
{
	String *cmdString;
	String *raddr;
	String *prologue;

	raddr = mp->reply_to822;
	if(raddr == 0)
		raddr = mp->from822;
	if(raddr == 0)
		raddr = mp->sender822;
	if(raddr == 0)
		raddr = mp->sender;

	cmdString = s_new();
	s_append(s_restart(cmdString), UPASBIN);
	s_append(cmdString, "/sendmail '");
	s_append(cmdString, s_to_c(raddr));
	s_append(cmdString, "'");

	if(mp->subject){
		prologue = s_new();
		s_append(prologue, "Subject: ");
		if(cistrncmp(s_to_c(mp->subject), "re:", 3) != 0)
			s_append(prologue, "re: ");
		s_append(prologue, s_to_c(mp->subject));
		s_append(prologue, "\n");
	} else
		prologue = 0;

	if(pipecmd(mp, s_to_c(cmdString), 1, mailinput, mailinput, prologue)<0)
		return -1;

	if(prologue)
		s_free(prologue);
	s_free(cmdString);

	return 0;
}

static int
rcvrseen(message *mp, String *s)
{
	Addr *a;

	if(s == mp->sender)
		return 0;

	if(strcmp(s_to_c(mp->sender), s_to_c(s)) == 0)
		return 1;

	for(a = mp->addrs; a; a = a->next){
		if(a->addr == s)
			return 0;
		if(strcmp(s_to_c(a->addr), s_to_c(s)) == 0)
			return 1;
	}
	return 0;
}

static int
addrcvr(message *mp, String *s, String *cmdString)
{
	static String *input;
	extern Biobuf in;
	char *cp;

	if(rcvrseen(mp, s))
		return 0;

	if(input == 0)
		input = s_new();

	print("reply to %s? ", s_to_c(s));
	cp = s_read_line(&in, s_restart(input));
	if(cp == 0 || (*cp != 'y' && *cp != 'Y'))
		return 0;

	if(strstr(s_to_c(s), "all")){
		print("are you really sure you want to reply to %s? ",
			s_to_c(s));
		cp = s_read_line(&in, s_restart(input));
	}
	if(cp == 0 || (*cp != 'y' && *cp != 'Y'))
		return 0;

	s_append(cmdString, " '");
	s_append(cmdString, s_to_c(s));
	s_append(cmdString, "'");
	return 1;
}

/* reply to all destiniations */
extern int
replyall(message *mp, int mailinput)
{
	String *cmdString;
	String *prologue;
	int i;
	Addr *a;

	cmdString = s_new();
	s_append(s_restart(cmdString), UPASBIN);
	s_append(cmdString, "/sendmail");

	i = addrcvr(mp, mp->sender, cmdString);
	if(interrupted)
		return -1;
	for(a = mp->addrs; a; a = a->next){
		if(strcmp(s_to_c(a->addr), s_to_c(mp->sender)) == 0)
			continue;
		i += addrcvr(mp, a->addr, cmdString);
		if(interrupted)
			return -1;
	}
	if(i == 0){
		s_free(cmdString);
		fprint(2, "!noone to reply to!\n");
		return -1;
	}

	if(mp->subject){
		prologue = s_new();
		s_append(prologue, "Subject: ");
		if(cistrncmp(s_to_c(mp->subject), "re:", 3) != 0)
			s_append(prologue, "re: ");
		s_append(prologue, s_to_c(mp->subject));
		s_append(prologue, "\n");
	} else
		prologue = 0;

	if(pipecmd(mp, s_to_c(cmdString), 1, mailinput, mailinput, prologue)<0)
		return -1;

	if(prologue)
		s_free(prologue);
	s_free(cmdString);

	return 0;
}


/*
 *  read the file of headers to ignore 
 */
void
readignore(void)
{
	char *p;
	Ignorance *i;
	Biobuf *b;
	char buf[128];

	if(ignore)
		return;

	snprint(buf, sizeof(buf), "%s/ignore", UPASLIB); 
	b = Bopen(buf, OREAD);
	if(b == 0)
		return;
	while(p = Brdline(b, '\n')){
		p[Blinelen(b)-1] = 0;
		while(*p && (*p == ' ' || *p == '\t'))
			p++;
		if(*p == '#')
			continue;
		i = malloc(sizeof(Ignorance));
		if(i == 0)
			break;
		i->partial = strlen(p);
		i->str = strdup(p);
		if(i->str == 0){
			free(i);
			break;
		}
		i->next = ignore;
		ignore = i;
	}
	Bterm(b);
}

/*
 *  print a message ignoring some fields
 */
extern int
seanprintm(message *mp)
{
	char *cp, *ep;
	int n, match;
	Ignorance *i;
	char buf[256];
	static String *dir;
	char *u;

	/* use unmime if this is a mime message */
	if(mp->mime){
		u = getlog();
		if(u == nil){
			fprint(2, "?can't determine login name\n");
			return 0;
		}
		dir = s_reset(dir);
		mboxpath("", u, dir, 0);
		if(Dflg)
			sprint(buf, "%s/unmime -D", UPASBIN);
		else
			sprint(buf, "%s/unmime -a %s", UPASBIN, s_to_c(dir));
		pipemail(mp, buf, 0);
		return 0;
	}
	readignore();
	interrupted = 0;

	/*
	 *  print out the header
	 */
	match = 0;
	print_header(&out, s_to_c(mp->sender), s_to_c(mp->date));
	for(cp = s_to_c(mp->body); cp && cp < mp->body822; cp = ep){
		ep = strchr(cp, '\n');
		if(ep == 0)
			break;
		ep++;

		/* continuation lines */
		if(*cp == ' ' || *cp == '\t'){
			if(!match)
				Bwrite(&out, cp, ep-cp);
			continue;
		}

		/* start of header */
		for(i = ignore; i; i = i->next){
			match = cistrncmp(i->str, cp, i->partial) == 0;
			if(match)
				break;
		}
		if(!match)
			Bwrite(&out, cp, ep-cp);
	}

	/*
	 *  print out the rest
	 */
	for(ep = s_to_c(mp->body) + mp->size; !interrupted && cp < ep; cp += n){
		n = 256;
		if(n > ep-cp)
			n = ep-cp;
		if(Bwrite(&out, cp, n) != n)
			break;
	}

	return 0;
}

/* print out the command list */
extern int
help(void)
{
	String *s;

	s = s_new();
	mboxpath("file", getuser(), s, 0);
	Bprint(&out, "Commands are of the form [range] command [argument].\n");
	Bprint(&out, "The commmands are:\n");
	Bprint(&out, "a\treply to all addresses in the header\n");
	Bprint(&out, "A\treply to all addresses in the header with original message appended\n");
	Bprint(&out, "b\tprint the next ten headers\n");
	Bprint(&out, "d\tmark for deletion\n");
	Bprint(&out, "h\tprint message header (,h to print all headers)\n");
	Bprint(&out, "i\tincorporate new mail\n");
	Bprint(&out, "m addr\tremail message to addr\n");
	Bprint(&out, "M addr\tremail message to addr preceded by user input\n");
	Bprint(&out, "p\tprint the message\n");
	Bprint(&out, "q\texit from mail, and save messages not marked for deletion\n");
	Bprint(&out, "r\treply to sender\n");
	Bprint(&out, "R\treply to sender with original message appended\n");
	Bprint(&out, "s file\tappend message to %s\n", s_to_c(s));
	Bprint(&out, "S file\tappend message to file\n");
	Bprint(&out, "u\tunmark message for deletion\n");
	Bprint(&out, "w file\tappend message to %s without the From line\n", s_to_c(s));
	Bprint(&out, "W file\tappend message to file without the From line\n");
	Bprint(&out, "x\texit without changing mail box\n");
	Bprint(&out, "| cmd\tpipe mail to command\n");
	Bprint(&out, "! cmd\tescape to commmand\n");
	Bprint(&out, "?\tprint this message\n");
	s_free(s);
	return 0;
}

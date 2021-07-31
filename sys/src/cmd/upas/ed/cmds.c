#include <ctype.h>
#include "common.h"
#include "print.h"

int holding;

typedef struct Ignorance Ignorance;
struct Ignorance
{
	Ignorance *next;
	char	*str;		/* string */
	int	partial;	/* true if not exact match */
};
Ignorance *ignore;

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
		sprint(hd, "%3d %c %4d %s %.16s \"%.20s\"", mp->pos,
			mp->status&DELETED?'d':' ',
			mp->size, s_to_c(mp->sender),
			s_to_c(mp->date), s_to_c(mp->subject));
	else
		sprint(hd, "%3d %c %4d %s %.16s", mp->pos, mp->status&DELETED?'d':' ',
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

	dropnewline(cmd);
	cmd = dropleadingwhite(cmd);
	if(*cmd=='\0')
		cmd = "stored";
	mbox = s_reset(mbox);
	mboxpath(cmd, getlog(), mbox, dot);
	fp = sysopen(s_to_c(mbox), "cA", 0660);
	if(fp == 0) {
		fprint(2, "?can't open %s\n", cmd);
		return -1;
	}
	if(header){
		if(m_store(mp, fp) < 0){
			sysclose(fp);
			fprint(2, "?error writing %s\n", cmd);
			return -1;
		}
	} else {
		if(m_print(mp, fp, 0, 0) < 0){
			sysclose(fp);
			fprint(2, "?error writing %s\n", cmd);
			return -1;
		}
	}
	sysclose(fp);
	return 0;
}

static void
pipeint(void *a, char *msg)
{
	char buf[2*ERRLEN];

	USED(a);
	if(strcmp(msg, "interrupt") && strncmp(msg, "sys: write on closed pipe", 25))
		noted(NDFLT);
	sprint(buf, "!!%s!!\n", msg);
	write(2, buf, strlen(buf));
	noted(NCONT);
}

/* pipe a message (and possible tty input) to a command */
static int
pipecmd(message *mp, char *cmd, int fromtty, int mailinput, String *prologue)
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

	p = proc_start(cmd, ins = instream(), 0, 0, 0, 0);
	if(p == 0) {
		
		fprint(2, "?can't exec %s\n", cmd);
		return -1;
	}
	notify(pipeint);	/* special interrupt handler */
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
			n = Blinelen(&in);
			if(Bwrite(ins->fp, line, n) != n){
				rv = -1;
				break;
			}
			Bflush(ins->fp);
		}
		holdoff(holding);
	}
	if(mailinput){
		if(fromtty)
			Bprint(ins->fp, "\n------ original message follows ------\n\n");
		m_print(mp, ins->fp, 0, 1);
	}
	stream_free(ins);
	p->std[0] = 0;
	if(proc_wait(p))
		rv = -1;
	proc_free(p);
	if(fromtty)
		Bprint(&out, "!\n");
	notify(catchint);	/* back to normal interrupt handler */
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
	if(pipecmd(mp, s_to_c(cmdString), ttyinput, 1, 0)<0)
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
	if(pipecmd(mp, cmd, ttyinput, 1, 0)<0)
		return -1;
	return 0;
}

/* escape to the shell */
extern int
escape(char *cmd)
{
	char *cp;
	process *p;

	notify(pipeint);	/* special interrupt handler */
	cp = cmd+strlen(cmd)-1;
	if(*cp=='\n')
		*cp = '\0';
	p = proc_start(cmd, 0, 0, 0, 0, 0);
	proc_wait(p);
	proc_free(p);
	Bprint(&out, "!\n");
	notify(catchint);	/* back to normal interrupt handler */
	return 0;
}

char*
findaddr(message *mp, int x)
{
	Field *f;
	Node *np;

	for(f = mp->first; f; f = f->next){
		if(f->node->c == x){
			for(np = f->node; np; np = np->next){
				if(np->addr)
					return s_to_c(np->s);
			}
		}
	}
	return 0;
}

/*
 *  case independent string compare
 */
int
cistrncmp(char *s1, char *s2, int n)
{
	int c1, c2;

	for(; *s1; s1++, s2++, n--){
		if(n <= 0)
			return 0;
		c1 = isupper(*s1) ? tolower(*s1) : *s1;
		c2 = isupper(*s2) ? tolower(*s2) : *s2;
		if (c1 != c2)
			return -1;
	}
	return 0;
}

/* reply to a message */
extern int
reply(message *mp, int mailinput)
{
	String *cmdString;
	char *raddr;
	String *prologue;

	raddr = findaddr(mp, REPLY_TO);
	if(raddr == 0)
		raddr = findaddr(mp, FROM);
	if(raddr == 0)
		raddr = findaddr(mp, SENDER);

	cmdString = s_new();
	s_append(s_restart(cmdString), "/bin/mail '");
	if(raddr)
		s_append(cmdString, raddr);
	else
		s_append(cmdString, s_to_c(mp->sender));
	s_append(cmdString, "'");

	if(mp->subject){
		prologue = s_new();
		s_append(prologue, "Subject: ");
		if(cistrncmp(s_to_c(mp->subject), "re:", 3) != 0)
			s_append(prologue, "re: ");
		s_append(prologue, s_to_c(mp->subject));
	} else
		prologue = 0;

	if(pipecmd(mp, s_to_c(cmdString), 1, mailinput, prologue)<0)
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
	char *p, *colon;
	Ignorance *i;
	Biobuf *b;

	if(ignore)
		return;

	b = Bopen("/mail/lib/ignore", OREAD);
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
		colon = strchr(p, ':');
		if(colon)
			*colon = 0;
		else
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
	Field *f;
	Node *p;
	char *cp;
	int match;
	Ignorance *i;

	/*
	 *  print out the parsed header
	 */
	readignore();
	print_header(&out, s_to_c(mp->sender), s_to_c(mp->date));
	for(f = mp->first; f; f = f->next){
		if(f->node->s){
			match = 0;
			for(i = ignore; i; i = i->next){
				if(i->partial)
					match = cistrncmp(i->str, s_to_c(f->node->s), i->partial) == 0;
				else
					match = cistrcmp(i->str, s_to_c(f->node->s)) == 0;
				if(match)
					break;
			}
			if(match)
				continue;
		}

		for(p = f->node; p; p = p->next){
			if(p->s)
				Bprint(&out, "%s", s_to_c(p->s));
			else {
				Bprint(&out, "%c", p->c);
			}
			if(p->white){
				cp = s_to_c(p->white);
				Bprint(&out, "%s", cp);
			}
		}
		Bprint(&out, "\n");
	}
	Bwrite(&out, mp->body822, mp->size - (mp->body822 - s_to_c(mp->body)));

	return 0;
}

/* print out the command list */
extern int
help(void)
{
	char *user;

	user = getuser();
	Bprint(&out, "Commands are of the form [range] command [argument].\n");
	Bprint(&out, "The commmands are:\n");
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
	Bprint(&out, "s file\tappend message to /mail/box/%s/file\n", user);
	Bprint(&out, "S file\tappend message to file\n");
	Bprint(&out, "u\tunmark message for deletion\n");
	Bprint(&out, "w file\tappend message to /mail/box/%s/file without the From line\n", user);
	Bprint(&out, "W file\tappend message to file without the From line\n");
	Bprint(&out, "x\texit without changing mail box\n");
	Bprint(&out, "| cmd\tpipe mail to command\n");
	Bprint(&out, "! cmd\tescape to commmand\n");
	Bprint(&out, "?\tprint this message\n");
	return 0;
}

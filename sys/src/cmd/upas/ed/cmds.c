#include "common.h"
#include "print.h"

int holding;


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
	if (*cmd=='\n')
		*cmd = '\0';
}

/* make sure an argument exists */
static int
needargument(char *cmd)
{
	if (*cmd=='\0') {
		fprint(2, "?argument missing\n");
		return -1;
	}
	return 0;
}

/* make the header line for a message */
extern char *
header(message *mp)
{
	static char hd[256];

	sprint(hd, "%3d %c %4d %s %.30s", mp->pos, mp->status&DELETED?'d':' ',
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
	if (*cmd=='\0')
		cmd = "stored";
	mbox = s_reset(mbox);
	mboxpath(cmd, getlog(), mbox, dot);
	fp = sysopen(s_to_c(mbox), "cA", 0660);
	if (fp == 0) {
		fprint(2, "?can't open %s\n", cmd);
		return -1;
	}
	if(m_print(mp, fp, header, header) < 0){
		sysclose(fp);
		fprint(2, "?error writing %s\n", cmd);
		return -1;
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
pipecmd(message *mp, char *cmd, int fromtty, int mailinput)
{
	int rv=0;
	int n;
	char *line;
	stream *ins;
	process *p;

	if (fromtty){
		Bprint(&out, "!%s\n", cmd);
		Bflush(&out);
	}

	p = proc_start(cmd, ins = instream(), 0, 0, 0, 0);
	if (p == 0) {
		
		fprint(2, "?can't exec %s\n", cmd);
		return -1;
	}
	notify(pipeint);	/* special interrupt handler */
	if (fromtty) {
		holding = open("/dev/consctl", OWRITE);
		write(holding, "holdon", 6);
		while(line = Brdline(&in, '\n')){
			n = Blinelen(&in);
			if(Bwrite(ins->fp, line, n) != n){
				rv = -1;
				break;
			}
			Bflush(ins->fp);
		}
	}
	if (mailinput)
		m_print(mp, ins->fp, 0, 1);
	stream_free(ins);
	p->std[0] = 0;
	if (proc_wait(p))
		rv = -1;
	proc_free(p);
	if (fromtty){
		if(holding > 0){
			write(holding, "holdoff", 7);
			close(holding);
			holding = -1;
		}
		Bprint(&out, "!\n");
	}
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
	if (pipecmd(mp, s_to_c(cmdString), ttyinput, 1)<0)
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
	if (pipecmd(mp, cmd, ttyinput, 1)<0)
		return -1;
	return 0;
}

/* escape to the shell */
extern int
escape(char *cmd)
{
	char *cp;
	process *p;

	cp = cmd+strlen(cmd)-1;
	if (*cp=='\n')
		*cp = '\0';
	p = proc_start(cmd, 0, 0, 0, 0, 0);
	proc_wait(p);
	proc_free(p);
	Bprint(&out, "!\n");
	return 0;
}

/* reply to a message */
extern int
reply(message *mp, int mailinput)
{
	static String *cmdString=0;

	if (cmdString == 0)
		cmdString = s_new();
	s_append(s_restart(cmdString), "/bin/mail '");
	s_append(cmdString, s_to_c(mp->sender));
	s_append(cmdString, "'");
	if (pipecmd(mp, s_to_c(cmdString), 1, mailinput)<0)
		return -1;
	return 0;
}

/* print out the command list */
extern int
help(void)
{
	Bprint(&out, "Commands are of the form [range] command [argument].\n");
	Bprint(&out, "The commmands are:\n");
	Bprint(&out, "b\tprint the next ten headers\n");
	Bprint(&out, "d\tmark for deletion\n");
	Bprint(&out, "h\tprint message header (,h to print all headers)\n");
	Bprint(&out, "m addr\tremail message to addr\n");
	Bprint(&out, "M addr\tremail message to addr preceded by user input\n");
	Bprint(&out, "p\tprint the message\n");
	Bprint(&out, "q\texit from mail, and save messages not marked for deletion\n");
	Bprint(&out, "r\treply to sender\n");
	Bprint(&out, "R\treply to sender with original message appended\n");
	Bprint(&out, "s file\tappend message to file\n");
	Bprint(&out, "u\tunmark message for deletion\n");
	Bprint(&out, "x\texit without changing mail box\n");
	Bprint(&out, "| cmd\tpipe mail to command\n");
	Bprint(&out, "! cmd\tescape to commmand\n");
	Bprint(&out, "?\tprint this message\n");
	return 0;
}

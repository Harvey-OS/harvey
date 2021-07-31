#include "common.h"
#include "send.h"

/* globals to all files */
int rmail;
int onatty;
char *thissys;
int nflg;
int xflg;
int debug;
int rflg;

/* global to this file */
static String *errstring;
static message *mp;
static int interrupt;
static int savemail;
static Biobuf in;

/* predeclared */
static int	send(dest *, message *, int);
static void	lesstedious(void);
static void	save_mail(message *);
static int	complain_mail(dest *, message *);
static int	pipe_mail(dest *, message *);
static int	cat_mail(dest *, message *);
static void	appaddr(String *, dest *);
static int	refuse(dest *, message *, char *, int);
static void	mkerrstring(String *, message *, dest *, dest *, char *, int);
static int	replymsg(String *, message *, dest *);

void
main(int argc, char *argv[])
{
	dest *dp=0;
	int checkforward;
	char *base;
	int rv, fd;

	/* process args */
	ARGBEGIN{
	case '#':
		nflg = 1;
		break;
	case 'x':
		nflg = 1;
		xflg = 1;
		break;
	case 'd':
		debug = 1;
		break;
	case 'r':
		rflg = 1;
		break;
	default:
		fprint(2, "usage: mail [-x] list-of-addresses\n");
		exit(1);
	}ARGEND

	while(*argv)
		d_insert(&dp, d_new(s_copy(*argv++)));

	if (dp == 0) {
		fprint(2, "usage: mail [-#] address-list\n");
		exit(1);
	}

	/*
	 * get context:
	 *	- whether we're rmail or mail
	 *	- whether on a tty
	 */
	base = basename(argv0);
	checkforward = rmail = (strcmp(base, "rmail")==0) | rflg;
	onatty = !rmail;
	thissys = sysname_read();

	/*
	 *  read the mail.  If an interrupt occurs while reading, save in
	 *  dead.letter
	 */
	fd = -1;
	if(onatty){
		fd = open("/dev/consctl", OWRITE);
		write(fd, "holdon", 6);
	}
	if (!nflg) {
		Binit(&in, 0, OREAD);
		mp = m_read(&in, rmail);
		if (mp == 0)
			exit(0);
		if (interrupt != 0) {
			save_mail(mp);
			exit(1);
		}
	} else {
		mp = m_new();
		default_from(mp);
	}
	if(onatty){
		write(fd, "holdoff", 7);
		close(fd);
	}
	errstring = s_new();
	getrules();

	/*
	 *  If this is a gateway, translate the sender address into a local
	 *  address.  This only happens if mail to the local address is 
	 *  forwarded to the sender.
	 */
	gateway(mp);

	/*
	 *  Protect against shell characters in the sender name for
	 *  security reasons.
	 */
	USE(s_restart(mp->sender));
	if (shellchars(s_to_c(mp->sender)))
		mp->replyaddr = s_copy("postmaster");
	else
		mp->replyaddr = s_clone(mp->sender);
	USE(s_restart(mp->replyaddr));

	/*
	 *  reject messages that are too long.  We don't do it earlier
	 *  in m_read since we haven't set up enough things yet.
	 */
	if(mp->size < 0)
		exit(refuse(dp, mp, "message too long", 0));

	rv = send(dp, mp, checkforward);
	if(savemail)
		save_mail(mp);
	if(mp)
		m_free(mp);
	exit(rv);
}


/* send a message to a list of sites */
static int
send(dest *destp, message *mp, int checkforward)
{
	dest *dp;		/* destination being acted upon */
	dest *bound;		/* bound destinations */
	int errors=0;
	static int forked;

	/* bind the destinations to actions */
	bound = up_bind(destp, mp, checkforward);

	/* loop through and execute commands */
	for (dp = d_rm(&bound); dp != 0; dp = d_rm(&bound)) {
		switch (dp->status) {
		case d_cat:
			errors += cat_mail(dp, mp);
			break;
		case d_pipeto:
		case d_pipe:
			if (!rmail && !nflg && !forked) {
				forked = 1;
				lesstedious();
			}
			errors += pipe_mail(dp, mp);
			break;
		default:
			errors += complain_mail(dp, mp);
			break;
		}
	}

	return errors;
}

/* avoid user tedium (as Mike Lesk said in a previous version) */
static void
lesstedious(void)
{
	int i;

	if(debug)
		return;
	switch(fork()){
	case -1:
		onatty = 0;
		break;
	case 0:
		rfork(RFENVG|RFNAMEG|RFNOTEG);
		for(i=0; i<nsysfile; i++)
			close(i);
		onatty = 0;
		savemail = 0;
		break;
	default:
		exit(0);
	}
}


/* save the mail */
static void
save_mail(message *mp)
{
	Biobuf *fp;
	String *file;
	static saved = 0;

	file = s_new();
	mboxpath("dead.letter", getlog(), file, 0);
	if ((fp = sysopen(s_to_c(file), "cA", 0660)) == 0)
		return;
	m_bprint(mp, fp);
	sysclose(fp);
	fprint(2, "saved in %s\n", s_to_c(file));
	s_free(file);
}

/* remember the interrupt happened */
/* dispose of incorrect addresses */
static int
complain_mail(dest *dp, message *mp)
{
	char *msg;

	switch (dp->status) {
	case d_undefined:
		msg = "Invalid address"; /* a little different, for debugging */
		break;
	case d_syntax:
		msg = "invalid address";
		break;
	case d_unknown:
		msg = "unknown user";
		break;
	case d_eloop:
	case d_loop:
		msg = "forwarding loop";
		break;
	case d_noforward:
		if(dp->pstat && *s_to_c(dp->repl2))
			return refuse(dp, mp, s_to_c(dp->repl2), dp->pstat);
		else
			msg = "destination unknown or forwarding disallowed";
		break;
	case d_pipe:
		msg = "broken pipe";
		break;
	case d_cat:
		msg = "broken cat";
		break;
	case d_translate:
		if(dp->pstat && *s_to_c(dp->repl2))
			return refuse(dp, mp, s_to_c(dp->repl2), dp->pstat);
		else
			msg = "name translation failed";
		break;
	case d_alias:
		msg = "broken alias";
		break;
	case d_badmbox:
		msg = "corrupted mailbox";
		break;
	case d_resource:
		msg = "out of some resource.  Try again later.";
		break;
	default:
		msg = "unknown d_";
		break;
	}
	if (nflg) {
		print("%s: %s\n", msg, s_to_c(dp->addr));
		return 0;
	}
	return refuse(dp, mp, msg, 0);
}

/* dispose of remote addresses */
static int
pipe_mail(dest *dp, message *mp)
{
	String *file;
	dest *next, *list=0;
	String *cmd;
	process *pp;
	int status, none;
	String *errstring=s_new();

	none = dp->status == d_pipeto;
	/*
	 *  collect the arguments
	 */
	file = s_new();
	abspath(s_to_c(dp->addr), MAILROOT, file);
	next = d_rm_same(&dp);
	if(xflg)
		cmd = s_new();
	else
		cmd = s_clone(s_restart(next->repl1));
	for(; next != 0; next = d_rm_same(&dp)){
		if(xflg){
			s_append(cmd, s_to_c(next->addr));
			s_append(cmd, "\n");
		} else {
			if (next->repl2 != 0) {
				s_append(cmd, " ");
				s_append(cmd, s_to_c(next->repl2));
			}
		}
		d_insert(&list, next);
	}

	if (nflg) {
		if(xflg)
			print("%s", s_to_c(cmd));
		else
			print("%s\n", s_to_c(cmd));
		s_free(cmd);
		s_free(file);
		return 0;
	}

	/*
	 *  run the process
	 */
	pp = proc_start(s_to_c(cmd), instream(), 0, outstream(), 1, none);
	if(pp==0 || pp->std[0]==0 || pp->std[2]==0)
		return refuse(list, mp, "out of processes, pipes, or memory", 0);
	m_print(mp, pp->std[0]->fp, thissys, 0);
	stream_free(pp->std[0]);
	pp->std[0] = 0;
	while(s_read_line(pp->std[2]->fp, errstring))
		;
	status = proc_wait(pp);
	proc_free(pp);
	s_free(cmd);

	/*
	 *  return status
	 */
	if (status != 0)
		return refuse(list, mp, s_to_c(errstring), status);
	loglist(list, mp, "remote");
	return 0;
}

/* dispose of local addresses */
static int
cat_mail(dest *dp, message *mp)
{
	Biobuf *fp;
	char *rcvr, *cp;
	Lock *l;
	String *tmp;

	if (nflg) {
		if(!xflg)
			print("cat >> %s\n", s_to_c(dp->repl1));
		else
			print("%s\n", s_to_c(dp->addr));
		return 0;
	}
	l = lock(s_to_c(dp->repl1));
	if(l == 0)
		return refuse(dp, mp, "can't lock mail file", 0);
	fp = sysopen(s_to_c(dp->repl1), "cal", MBOXMODE);
	if (fp == 0){
		tmp = s_append(0, s_to_c(dp->repl1));
		s_append(tmp, ".tmp");
		fp = sysopen(s_to_c(tmp), "cal", MBOXMODE);
		if(fp == 0){
			unlock(l);
			return refuse(dp, mp, "mail file cannot be opened", 0);
		}
		syslog(0, "mail", "error: used %s", s_to_c(tmp));
		s_free(tmp);
	}
	if(m_print(mp, fp, (char *)0, 1) < 0
	|| Bprint(fp, "\nmorF\n") < 0
	|| Bflush(fp) < 0){
		sysclose(fp);
		unlock(l);
		return refuse(dp, mp, "error writing mail file", 0);
	}
	sysclose(fp);
	unlock(l);
	rcvr = s_to_c(dp->addr);
	if(cp = strrchr(rcvr, '!'))
		rcvr = cp+1;
	logdelivery(dp, rcvr, mp);
	return 0;
}

static void
appaddr(String *sp, dest *dp)
{
	dest *parent;

	if (dp->parent != 0) {
		for(parent=dp->parent; parent->parent!=0; parent=parent->parent)
			;
		s_append(sp, s_to_c(parent->addr));
		s_append(sp, "' alias `");
	}
	s_append(sp, s_to_c(dp->addr));
}

/* reject delivery */
static int
refuse(dest *list, message *mp, char *cp, int status)
{
	String *errstring=s_new();
	dest *dp;
	int rv;

	dp = d_rm(&list);
	mkerrstring(errstring, mp, dp, list, cp, status);
	/*
	 * if on a tty just report the error.  Otherwise send mail
	 * reporting the error.  N.B. To avoid mail loops, don't
	 * send mail reporting a failure of mail to reach the postmaster.
	 */
	if (onatty) {
		fprint(2, "%s\n", s_to_c(errstring));
		savemail = 1;
		rv = 1;
	} else {
		if (strcmp(s_to_c(mp->replyaddr), "postmaster")!=0)
			rv = replymsg(errstring, mp, dp);
		else
			rv = 1;
	}
	logrefusal(dp, mp, s_to_c(errstring));
	s_free(errstring);
	return rv;
}

/* make the error message */
static void
mkerrstring(String *errstring, message *mp, dest *dp, dest *list, char *cp, int status)
{
	dest *next;
	char smsg[64];

	/* list all aliases */
	s_append(errstring, "Mail to `");
	appaddr(errstring, dp);
	for(next = d_rm(&list); next != 0; next = d_rm(&list)) {
		s_append(errstring, "', '");
		appaddr(errstring, next);
		d_insert(&dp, next);
	}
	s_append(errstring, "' from '");
	s_append(errstring, s_to_c(mp->sender));
	s_append(errstring, "' failed.\n");

	/* >> and | deserve different flavored messages */
	switch(dp->status) {
	case d_pipe:
		s_append(errstring, "The mailer `");
		s_append(errstring, s_to_c(dp->repl1));
		sprint(smsg, "' returned error status %x.\n", status);
		s_append(errstring, smsg);
		s_append(errstring, "The error message was:\n");
		s_append(errstring, cp);
		break;
	default:
		s_append(errstring, "The error message was:\n");
		s_append(errstring, cp);
		break;
	}
}

/*
 *  reply with up to 1024 characters of the
 *  original message
 */
static int
replymsg(String *errstring, message *mp, dest *dp)
{
	message *refp = m_new();
	dest *ndp;
	char *rcvr;
	int rv;

	rcvr = dp->status==d_eloop ? "postmaster" : s_to_c(mp->replyaddr);
	ndp = d_new(s_copy(rcvr));
	s_append(refp->sender, "postmaster");
	s_append(refp->replyaddr, "postmaster");
	s_append(refp->date, thedate());
	s_append(refp->body, s_to_c(errstring));
	s_append(refp->body, "\nThe message began:\n");
	s_nappend(refp->body, s_to_c(mp->body), 8*1024);
	refp->size = strlen(s_to_c(refp->body));
	rv = send(ndp, refp, 0);
	m_free(refp);
	d_free(ndp);
	return rv;
}

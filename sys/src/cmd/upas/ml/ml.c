#include "common.h"
#include "dat.h"

Biobuf in;

Addr *al;
int na;
String *from;
String *sender;

void printmsg(int fd, String *msg, char *replyto);
void appendtoarchive(char* listname, String *firstline, String *msg);

void
usage(void)
{
	fprint(2, "usage: %s address-list-file listname\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	String *msg;
	String *firstline;
	char *listname, *alfile;
	Waitmsg *w;
	int fd;

	ARGBEGIN{
	}ARGEND;

	rfork(RFENVG|RFREND);

	if(argc < 2)
		usage();
	alfile = argv[0];
	listname = argv[1];

	readaddrs(alfile);

	if(Binit(&in, 0, OREAD) < 0)
		sysfatal("opening input: %r");

	msg = s_new();
	firstline = s_new();

	/* discard the 'From ' line */
	if(s_read_line(&in, firstline) == nil)
		sysfatal("reading input: %r");

	/* read up to the first 128k of the message.  more is redculous */
	if(s_read(&in, msg, 128*1024) <= 0)
		sysfatal("reading input: %r");

	/* parse the header */
	yyinit(s_to_c(msg), s_len(msg));
	yyparse();

	/* get the sender */
	getaddrs();
	if(from == nil)
		from = sender;
	if(from == nil)
		sysfatal("message must contain From: or Sender:");
	if(strcmp(listname, s_to_c(from)) == 0)
		sysfatal("can't remail messages from myself");
	addaddr(s_to_c(from));

	/* start the mailer up and return a pipe to it */
	fd = startmailer(listname);

	/* send message adding our own reply-to and precedence */
	printmsg(fd, msg, listname);
	close(fd);

	/* wait for mailer to end */
	while(w = wait()){
		if(w->msg != nil && w->msg[0])
			sysfatal("%s", w->msg);
		free(w);
	}

	/* if the mailbox exits, cat the mail to the end of it */
	appendtoarchive(listname, firstline, msg);
	exits(0);
}

/* send message filtering Reply-to out of messages */
void
printmsg(int fd, String *msg, char *replyto)
{
	Field *f;
	Node *p;
	char *cp, *ocp;

	cp = s_to_c(msg);
	fprint(fd, "Reply-To: %s\nPrecedence: bulk\n", replyto);
	for(f = firstfield; f; f = f->next){
		ocp = cp;
		for(p = f->node; p; p = p->next)
			cp = p->end+1;
		if(f->node->c == REPLY_TO)
			continue;
		if(f->node->c == PRECEDENCE)
			continue;
		write(fd, ocp, cp-ocp);
	}
	write(fd, cp, s_len(msg) - (cp - s_to_c(msg)));
}

/* if the mailbox exits, cat the mail to the end of it */
void
appendtoarchive(char* listname, String *firstline, String *msg)
{
	String *mbox;
	int fd;

	mbox = s_new();
	mboxpath("mbox", listname, mbox, 0);
	if(access(s_to_c(mbox), 0) < 0)
		return;
	fd = open(s_to_c(mbox), OWRITE);
	if(fd < 0)
		return;
	s_append(msg, "\n");
	write(fd, s_to_c(firstline), s_len(firstline));
	write(fd, s_to_c(msg), s_len(msg));
}

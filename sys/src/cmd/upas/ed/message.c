#include "common.h"
#include "print.h"

/* definition of UNIX message headers */

#define MSGALLOC 32

typedef struct {
	message m[MSGALLOC];
	int o;
} msgalloc;
static msgalloc *freep=0;

extern int interrupted;

/* get a copy of an 822 field */
String*
copyfield(message *mp, int x)
{
	Field *f;
	Node *p;
	char *start, *end;
	int n;

	for(f = mp->first; f; f = f->next){
		if(f->node->c == x){
			start = end = f->node->end+1;
			for(p = f->node->next; p; p = p->next)
				end = p->end+1;
			while(start < end && (*start == ' ' || *start == '\t'))
				start++;
			n = end-start;
			if(n <= 0)
				break;
			if(start[n-1] == '\n')
				n--;
			return s_nappend(s_new(), start, n);
		}
	}
	return 0;
}

String*
copyaddr(message *mp, int x)
{
	Field *f;
	Node *np;
	String *y;

	y = nil;

	for(f = mp->first; f; f = f->next){
		if(f->node->c == x){
			for(np = f->node; np; np = np->next){
				if(np->addr){
					/*
					 *  the quotes are a hack and depend on
					 *  how sendmail is called.
					 */
					if(y == nil)
						y = s_new();
					else
						s_append(y, "' '");
					s_append(y, s_to_c(np->s));
				}
			}
			if(y != nil)
				return y;
		}
	}
	return 0;
}

String*
copyother(message *mp, int x)
{
	Field *f;
	Node *np;

	for(f = mp->first; f; f = f->next){
		if(f->node->c == x){
			for(np = f->node; np; np = np->next)
				return s_append(s_new(), s_to_c(np->s));
		}
	}
	return 0;
}

/*
 *  free parse tree and remember all addresses and
 *  the start of the body
 */
void
freeparsetree(message *mp)
{
	Field *f, *fnext;
	Node *p, *pnext;
	Addr **l, *a;

	firstfield = lastfield = 0;

	mp->body822 = s_to_c(mp->body);
	l = &mp->addrs;
	for(f = mp->first; f; f = fnext){
		for(p = f->node; p; p = pnext){
			mp->body822 = p->end+1;
			if(p->s){
				if(p->addr){
					a = malloc(sizeof(Addr));
					a->next = 0;
					a->addr = p->s;
					*l = a;
					l = &a->next;
				} else
					s_free(p->s);
			}
			if(p->white)
				s_free(p->white);
			pnext = p->next;
			free(p);
		}
		fnext = f->next;
		free(f);
	}
}

/* read in a message, interpret the 'From' header */
extern message *
m_get(String *sp)
{
	message *mp;
	register char *cp;

	if (freep==0 || freep->o >= MSGALLOC) {
		freep = (msgalloc *)malloc(sizeof(msgalloc));
		if (freep==0) {
			perror("allocating message");
			exit(1);
		}
		freep->o = 0;
	}
	mp = &(freep->m[freep->o++]);
	mp->body = 0;
	mp->sender = s_new();
	mp->date = s_new();
	mp->extent = mp->prev = mp->next = 0;
	mp->status = 0;
	mp->pos = 0;

	if (sp->ptr >= sp->end) {
		mzero = mp;
		return 0;
	}

	/* parse From lines */
	for(cp=sp->ptr; *cp=='\n'||*cp==' '||*cp=='\t'; cp++);
	for(sp->ptr=cp; *cp != '\n' && *cp; cp++);
	*cp = '\0';
	if (parse_header(sp->ptr, mp->sender, mp->date)<0) {
		fprint(2, "!mailbox format incorrect, this session is read only\n");
		fprint(2, "!	bad line is: %s\n", sp->ptr);
		writeable = 0;
		mzero = mp;
		return 0;
	}
	*cp++ = '\n';
	sp->ptr = cp;

	/* get body */
	while (cp < sp->end && !IS_HEADER(cp) && !IS_TRAILER(cp)) {
		while (*cp++ != '\n')
			;
	}
	cp[-1] = '\0';	/* wipes out last newline */

	mp->size = cp - sp->ptr - 1;
	mp->body = s_array(sp->ptr, mp->size);
	if(IS_TRAILER(cp))
		cp += 5;
	sp->ptr = cp;

	/* parse 822 headers */
	yyinit(s_to_c(mp->body));
	yyparse();

	mp->first = firstfield;

	mp->subject = copyfield(mp, SUBJECT);
	mp->reply_to822 = copyaddr(mp, REPLY_TO);
	mp->from822 = copyaddr(mp, FROM);
	mp->sender822 = copyaddr(mp, SENDER);
	mp->date822 = copyother(mp, DATE);
	mp->mime = copyfield(mp, MIMEVERSION);

	freeparsetree(mp);

	return mp;
}

/* store a message back in a mail file */
extern int
m_store(message *mp, Biobuf *fp)
{
	int rv = 0;

	print_header(fp, s_to_c(mp->sender), s_to_c(mp->date));

	if (mp->size > 0)
		if(Bwrite(fp, s_to_c(mp->body), mp->size) != mp->size)
			rv = -1;

	if(rv == 0)
		Bwrite(fp, "\n", 1);

	return rv;

}

char*
unixto822date(char *s)
{
	char du[32];
	static char d822[35];

	if(strlen(s) != 28)
		return du;
	strcpy(du, s);
	du[3] = du[7] = du[10] = du[23] = 0;
	snprint(d822, sizeof(d822), "%s, %s %s %s %s", du, du+8, du+4, du+24, du+11);
	return d822;
}

/*
 *  print the header and up to n lines of the message followed by crnl's
 */
extern int
m_printcrnl(message *mp, Biobuf *fp, int n)
{
	char *p, *e;
	int c;
	int last;

	last = 0;
	p = s_to_c(mp->body);
	e = p + mp->size;
	if(mp->from822 == 0 && mp->sender822 == 0)
		Bprint(fp, "From: %s\r\n", s_to_c(mp->sender));
	if(mp->date822 == 0)
		Bprint(fp, "Date: %s\r\n", unixto822date(s_to_c(mp->date)));
	while(p < e){
		if(p > mp->body822 && n <= 0)
			break;
		c = *p;
		if(c == '\n' && last != '\r')
			Bputc(fp, '\r');
		else if(c == '.' && last == '\n')
			Bputc(fp, '.');
		last = c;	
		Bputc(fp, c);
		if(p > mp->body822 && c == '\n')
			n--;
		p++;
	}
	if(last != '\n')
		Bprint(fp, "\r\n");
	return p - s_to_c(mp->body);
}

/*
 *  output a message, return 0 if ok -1 otherwise, do it a chunk at a time in
 *  case the user wants to interrupt a long message.
 */
extern int
m_print(message *mp, Biobuf *fp, int nl, int header)
{
	int rv = 0;
	int i, n, size;
	char *p;

	if (header){
		print_header(fp, s_to_c(mp->sender), s_to_c(mp->date));
		p = s_to_c(mp->body);
		size = mp->size;
	} else {
		p = mp->body822;
		size = mp->size - (mp->body822 - s_to_c(mp->body));
	}

	interrupted = 0;
	for(i = 0; !interrupted && i < size; i += n){
		n = size - i;
		if(n > 256)
			n = 256;
		if(Bwrite(fp, p+i, n) != n){
			rv = -1;
			break;
		}
	}

	if (header && rv==0 && nl)
		if(Bwrite(fp, "\n", 1) != 1)
			rv = -1;

	return rv;
}

/* lists of mail messages */
message *mzero;		/* zeroth message */
message *mlist;		/* first mail message */
message *mlast;		/* last mail message */

long mbsize;		/* last size of mail box */


/* 
 *  read in the mail file.  mbsize is the spot to start reading from.
 *  if fpp is non-zero then don't close the locked fmailbox, just
 *  return the file pointer to it.
 */
static int
rd_mbox(char *file, int reverse, int newmail)
{
	Biobuf *fp;
	message *mp;
	int pos = 0;
	String *line;
	char *corefile;
	int len;
	String *tmp;
	int filelen;

	/*
	 *  open the mailbox.  If it doesn't exist, try the temporary one.
	 */
retry:
	fp = sysopen(file, "r", 0);
	if(fp == 0){
		if(e_nonexistent() && fflg==0){
			tmp = s_copy(file);
			s_append(tmp, ".tmp");
			if(sysrename(s_to_c(tmp), file) == 0)
				goto retry;
		}
		return -1;
	}

	/*
	 *  seek past what we've already read and read it in with 1 read
	 */
	filelen = sysfilelen(fp);
	if(filelen < 0){
		sysclose(fp);
		return -1;
	}
	if (filelen <= mbsize){
		sysclose(fp);
		return 0;
	}
	Bseek(fp, mbsize, 0);
	len = filelen-mbsize;
	corefile = malloc(len+1);
	if (corefile == 0 || Bread(fp, corefile, len) != len) {
		sysclose(fp);
		perror("reading mbox");
		return -1;
	}
	sysclose(fp);
	corefile[len-1] = '\n';		/* ensure last mbox char is \n */
	corefile[len] = '\0';		/* ensure null termination */

	line = s_array(corefile, len);

	/*
	 *  parse the messages
	 */
	s_restart(line);
	while((mp = m_get(line)) != 0) {
		if (mlist == 0)
			mlist = mlast = mp;
		else if (reverse) {
			mlast->next = mp;
			mp->prev = mlast;
			mlast = mp;
		} else {
			mp->next = mlist;
			mlist->prev = mp;
			mlist = mp;
		}
	}
	if (mlist==0)
		return 0;
	mzero->next = mlist;
	mzero->prev = mlast;
	for (mp=mzero; mp!=0; mp=mp->next)
		mp->pos=pos++;
	mbsize = filelen;
	if (newmail)
		print("mail: new message arrived\n");
	return 0;
}

/* read the mailbox */
extern int
read_mbox(char *file, int reverse)
{
	int rv;
	Mlock *l;

	l = 0;
	if(!fflg){
		l = syslock(file);
		if(l == 0)
			return -1;
	}
	rv = rd_mbox(file, reverse, 0);
	if(!fflg)
		sysunlock(l);
	return rv;
}

/* read the mailbox looking for new messages */
extern int
reread_mbox(char *file, int reverse)
{
	int rv;
	Mlock *l;

	l = 0;
	if(!fflg){
		l = syslock(file);
		if(l == 0)
			return -1;
	}
	rv = rd_mbox(file, reverse, 1);
	if(!fflg)
		sysunlock(l);
	return rv;
}

/* read any changes from the mailbox and write out the result */
static int
rdwr_mbox(char *file, int reverse)
{
	Biobuf *fp;
	message *mp;
	String *tmp;
	Mlock *l;
	int rv;
	int errcnt;
	int tries;
	int mode;
	Dir d;

	/*
	 *  reread mailbox to pick up any changes
	 */
	l = 0;
	if(!fflg){
		l = syslock(file);
		if(l == 0){
			fprint(2, "mail: can't lock mail file %s\n", file);
			return -1;
		}
	}
	rv = -1;
	if(rd_mbox(file, reverse, 1) < 0){
		fprint(2, "mail: can't reread mail file %s\n", file);
		goto err;
	}

	/*
	 *  if nothing has changed, just return
	 */
	for (mp=mlist; mp!=0; mp=mp->next)
		if(mp->status&DELETED)
			break;
	if (mp==0){
		if(!fflg)
			sysunlock(l);
		return 0;
	}

	/*
	 * preserve old files permissions, if possible
	 */
	if(dirstat(file, &d) >= 0)
		mode = d.mode&0777;
	else
		mode = MBOXMODE;
	/*
	 *  write new messages into a temporary file
	 */
	tmp = s_copy(file);
	s_append(tmp, ".tmp");
	sysremove(s_to_c(tmp));
	fp = sysopen(s_to_c(tmp), "alc", mode);
	if(fp == 0){
		fprint(2, "mail: error creating %s: %r\n", s_to_c(tmp));
		goto err;
	}
	mlist->prev = 0;	/* ignore mzero */
	errcnt = 0;
	for(mp=reverse?mlist:mlast; mp!=0; mp=reverse?mp->next:mp->prev)
		if ((mp->status&DELETED)==0)
			errcnt += m_store(mp, fp);
	if (errcnt != 0) {
		sysremove(s_to_c(tmp));
		sysclose(fp);
		fprint(2,"mail: error writing %s: %r\n", s_to_c(tmp));
		goto err;
	}
	sysclose(fp);

	/*
	 *  unlink the mail box
	 */
	if(sysremove(file) < 0){
		fprint(2, "can't remove %s\n", file);
		goto err;
	}

	/*
	 *  rename the temp file to be the mailbox
	 */
	for(tries = 0; tries < 10; tries++){
		if(sysrename(s_to_c(tmp), file) < 0){
			fprint(2, "can't rename %s to %s\n", s_to_c(tmp), file);
			goto err;
		}
		if(sysexist(file) && !sysexist(s_to_c(tmp)))
			break;
		syslog(0, "mail", "error: left %s", s_to_c(tmp));
		fprint(2, "rename %s to %s failed, retrying...\n", s_to_c(tmp), file);
	}
	rv = 0;
err:
	if(!fflg)
		sysunlock(l);
	return rv;
}

/* write out the mail file */
extern int
write_mbox(char *file, int reverse)
{
	int rv;

	rv = rdwr_mbox(file, reverse);
	return rv;
}

/* global to semaphores */
Mlock *readingl;

extern void
V(void)
{
	if(readingl)
		sysunlock(readingl);
	readingl = 0;
}

/* return name of tty if file is already being read, 0 otherwise */
extern int
P(char *mailfile)
{
	String *file;

	file = s_new();
	if(readlock(file) == 0) {
		s_free(file);
		return -1;
	}

	readingl = trylock(s_to_c(file));
	if (readingl == 0) {
		if(sysexist(mailfile)){
			fprint(2,"WARNING: You are already reading mail.\n");
			fprint(2, "\tThis instance of mail is read only.\n");
		} else {
			fprint(2, "Sorry, no (or damaged) mailbox.\n");
			exit(0);
		}
		s_free(file);
		return -1;
	}
	s_free(file);
	return 0;
}

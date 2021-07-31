#include "common.h"
#include "print.h"

/* definition of UNIX message headers */

#define MSGALLOC 32

typedef struct {
	message m[MSGALLOC];
	int o;
} msgalloc;
static msgalloc *freep=0;

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
		fprint(2, "!mailbox format incorrect\n");
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
	cp[-1] = '\0';

	mp->size = cp - sp->ptr;
	mp->body = s_array(sp->ptr, mp->size);
	if(IS_TRAILER(cp))
		cp += 5;
	sp->ptr = cp;
	return mp;
}

/* output a message, return 0 if ok -1 otherwise */
extern int
m_print(message *mp, Biobuf *fp, int nl, int header)
{
	int rv = 0;
	int i, n;

	if (header)
		print_header(fp, s_to_c(mp->sender), s_to_c(mp->date));

	if (mp->size > 0)
		for(n = 0; n < mp->size-1; n+=i){
			i = mp->size - 1 - n;
			if(i > 1024)
				i = 1024;
			if(Bwrite(fp, s_to_c(mp->body)+n, i) != i)
				rv = -1;
		}
	if (rv==0 && nl)
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
		if(e_nonexistant() && fflg==0){
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
	Lock *l;

	l = 0;
	if(!fflg){
		l = lock(file);
		if(l == 0)
			return -1;
	}
	rv = rd_mbox(file, reverse, 0);
	if(!fflg)
		unlock(l);
	return rv;
}

/* read the mailbox looking for new messages */
extern int
reread_mbox(char *file, int reverse)
{
	int rv;
	Lock *l;

	l = 0;
	if(!fflg){
		l = lock(file);
		if(l == 0)
			return -1;
	}
	rv = rd_mbox(file, reverse, 1);
	if(!fflg)
		unlock(l);
	return rv;
}

/* read any changes from the mailbox and write out the result */
static int
rdwr_mbox(char *file, int reverse)
{
	Biobuf *fp;
	message *mp;
	String *tmp;
	Lock *l;
	int rv;
	int errcnt;
	int tries;

	/*
	 *  if nothing has changed, just return
	 */
	for (mp=mlist; mp!=0; mp=mp->next)
		if(mp->status&DELETED)
			break;
	if (mp==0)
		return 0;

	/*
	 *  reread mailbox to pick up any changes
	 */
	l = 0;
	if(!fflg){
		l = lock(file);
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
	 *  write new messages into a temporary file
	 */
	tmp = s_copy(file);
	s_append(tmp, ".tmp");
	sysremove(s_to_c(tmp));
	fp = sysopen(s_to_c(tmp), "alc", MBOXMODE);
	if(fp == 0){
		fprint(2, "mail: error creating %s\n", tmp);
		sysclose(fp);
		goto err;
	}
	mlist->prev = 0;	/* ignore mzero */
	errcnt = 0;
	for(mp=reverse?mlist:mlast; mp!=0; mp=reverse?mp->next:mp->prev)
		if ((mp->status&DELETED)==0)
			errcnt += m_print(mp, fp, 1, 1);
	if (errcnt != 0) {
		sysremove(s_to_c(tmp));
		sysclose(fp);
		fprint(2,"mail: error writing %s\n", tmp);
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
		unlock(l);
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
static Lock *readingl;

extern void
V(void)
{
	unlock(readingl);
}

/* return name of tty if file is already being read, 0 otherwise */
extern int
P(char *mailfile)
{
	String *file;

	file = s_new();
	mboxpath("reading", getlog(), file, 0);
	readingl = trylock(s_to_c(file));

	if (readingl == 0) {
		if(sysexist(mailfile)){
			fprint(2,"WARNING: You are already reading mail.\n");
			fprint(2, "\tThis instance of mail is read only.\n");
		} else
			fprint(2, "Sorry, no (or damaged) mailbox.\n");
		return -1;
	}
	s_free(file);
	return 0;
}

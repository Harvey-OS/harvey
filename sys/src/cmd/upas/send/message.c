#include "common.h"
#include "send.h"

/* global to this file */
static Reprog *rfprog;
static Reprog *fprog;

#define VMLIMIT (64*1024)
#define MSGLIMIT (128*1024*1024)

extern void
default_from(message *mp)
{
	char *cp;

	cp = getenv("upasname");
	if(cp)
		s_append(mp->sender, cp);
	else
		s_append(mp->sender, getlog());
	s_append(mp->date, thedate());
}

extern message *
m_new(void)
{
	message *mp;

	mp = (message *)malloc(sizeof(message));
	if (mp == 0) {
		perror("message:");
		exit(1);
	}
	mp->sender = s_new();
	mp->replyaddr = s_new();
	mp->date = s_new();
	mp->body = s_new();
	mp->size = 0;
	mp->fd = -1;
	return mp;
}

extern void
m_free(message *mp)
{
	if(mp->fd >= 0){
		close(mp->fd);
		remove(s_to_c(mp->tmp));
		s_free(mp->tmp);
	}
	s_free(mp->sender);
	s_free(mp->date);
	s_free(mp->body);
	free((char *)mp);
}

/* read a message into a temp file , return an open fd to it */
static int
m_read_to_file(Biobuf *fp, message *mp)
{
	int fd;
	int n;
	String *file;
	char buf[4*1024];

	file = s_new();
	/*
	 *  create and unlink temp file
	 */
	abspath("tmp/mtXXXXXX", MAILROOT, file);
	mktemp(s_to_c(file));
	if((fd = syscreate(s_to_c(file), 0600))<0){
		s_free(mp->tmp);
		return -1;
	}
	mp->tmp = file;

	/*
	 *  read the rest into the temp file
	 */
	while((n = Bread(fp, buf, sizeof(buf))) > 0){
		if(write(fd, buf, n) != n){
			close(fd);
			return -1;
		}
		mp->size += n;
		if(mp->size > MSGLIMIT){
			mp->size = -1;
			break;
		}
	}

	mp->fd = fd;
	return 0;
}

/* read in a message, interpret the 'From' header */
extern message *
m_read(Biobuf *fp, int rmail)
{
	message *mp;
	Resub subexp[10];
	int first;
	int n;


	mp = m_new();

	/* parse From lines if remote */
	if (rmail) {
		/* get remote address */
		String *sender=s_new();

		if (rfprog == 0)
			rfprog = regcomp(REMFROMRE);
		first = 1;
		while(s_read_line(fp, s_restart(mp->body)) != 0) {
			memset(subexp, 0, sizeof(subexp));
			if (regexec(rfprog, s_to_c(mp->body), subexp, 10) == 0){
				if(first == 0)
					break;
				if (fprog == 0)
					fprog = regcomp(FROMRE);
				memset(subexp, 0, sizeof(subexp));
				if(regexec(fprog, s_to_c(mp->body), subexp,10) == 0)
					break;
				USE(s_restart(mp->body));
				append_match(subexp, s_restart(sender), SENDERMATCH);
				append_match(subexp, s_restart(mp->date), DATEMATCH);
				break;
			}
			append_match(subexp, s_restart(sender), REMSENDERMATCH);
			append_match(subexp, s_restart(mp->date), REMDATEMATCH);
			if(subexp[REMSYSMATCH].sp!=subexp[REMSYSMATCH].ep){
				append_match(subexp, mp->sender, REMSYSMATCH);
				s_append(mp->sender, "!");
			}
			first = 0;
		}
		s_append(mp->sender, s_to_c(sender));
		s_free(sender);
	}
	if (*s_to_c(mp->sender)=='\0')
		default_from(mp);

	/*
	 *  read up to VMLIMIT bytes (more or less) into main memory.
	 *  if message is longer put the rest in a tmp file.
	 */
	mp->size = mp->body->ptr - mp->body->base;
	n = s_read(fp, mp->body, VMLIMIT);
	if(n < 0){
		perror("m_read");
		exit(1);
	}
	mp->size += n;
	if(n == VMLIMIT){
		if(m_read_to_file(fp, mp) < 0){
			perror("m_read");
			exit(1);
		}
	}

	/*
	 *  ignore 0 length messages from a terminal
	 */
	if (!rmail && mp->size == 0)
		return 0;

	return mp;
}

/* return a piece of message starting at `offset' */
extern int
m_get(message *mp, long offset, char **pp)
{
	static char buf[4*1024];

	/*
	 *  are we past eof?
	 */
	if(offset >= mp->size)
		return 0;

	/*
	 *  are we in the virtual memory portion?
	 */
	if(offset < mp->body->ptr - mp->body->base){
		*pp = mp->body->base + offset;
		return mp->body->ptr - mp->body->base - offset;
	}

	/*
	 *  read it from the temp file
	 */
	offset -= mp->body->ptr - mp->body->base;
	if(mp->fd < 0)
		return -1;
	if(seek(mp->fd, offset, 0)<0)
		return -1;
	*pp = buf;
	return read(mp->fd, buf, sizeof buf);
}

/* output the message body without ^From escapes */
static int
m_noescape(message *mp, Biobuf *fp)
{
	long offset;
	int n;
	char *p;

	for(offset = 0; offset < mp->size; offset += n){
		n = m_get(mp, offset, &p);
		if(n <= 0){
			Bflush(fp);
			return -1;
		}
		if(Bwrite(fp, p, n) < 0)
			return -1;
	}
	return Bflush(fp);
}

/*
 *  Output the message body with '^From ' escapes.
 *  Ensures that any line starting with a 'From ' gets a ' ' stuck
 *  in front of it.
 */
static int
m_escape(message *mp, Biobuf *fp)
{
	char *p, *np;
	char *end;
	long offset;
	int m, n;
	char *start;

	for(offset = 0; offset < mp->size; offset += n){
		n = m_get(mp, offset, &start);
		if(n < 0){
			Bflush(fp);
			return -1;
		}

		p = start;
		for(end = p+n; p < end; p += m){
			np = memchr(p, '\n', end-p);
			if(np == 0){
				Bwrite(fp, p, end-p);
				break;
			}
			m = np - p + 1;
			if(m > 5 && strncmp(p, "From ", 5) == 0)
				Bputc(fp, ' ');
			Bwrite(fp, p, m);
		}
	}
	Bflush(fp);
	return 0;
}

/* output a message */
extern int
m_print(message *mp, Biobuf *fp, char *remote, int mbox)
{
	if (remote != 0){
		if(print_remote_header(fp,s_to_c(mp->sender),s_to_c(mp->date),remote) < 0)
			return -1;
	} else {
		if(print_header(fp, s_to_c(mp->sender), s_to_c(mp->date)) < 0)
			return -1;
	}

	if (!mbox)
		return m_noescape(mp, fp);
	return m_escape(mp, fp);
}

/* print just the message body */
extern int
m_bprint(message *mp, Biobuf *fp)
{
	return m_noescape(mp, fp);
}

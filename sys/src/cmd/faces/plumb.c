#include <u.h>
#include <libc.h>
#include <draw.h>
#include <plumb.h>
#include <regexp.h>
#include <bio.h>
#include "faces.h"

static int		showfd = -1;
static int		seefd = -1;
static int		logfd = -1;
static char	*user;
static char	*logtag;

void
initplumb(void)
{
	showfd = plumbopen("send", OWRITE);
	seefd = plumbopen("seemail", OREAD);

	if(seefd < 0){
		logfd = open("/sys/log/mail", OREAD);
		seek(logfd, 0LL, 2);
		user = getenv("user");
		if(user == nil){
			fprint(2, "faces: can't find user name: %r\n");
			exits("$user");
		}
		logtag = emalloc(32+strlen(user)+1);
		sprint(logtag, " delivered %s From ", user);
	}
}

char*
attr(Face *f)
{
	static char buf[128];

	if(f->str[Sdigest]){
		snprint(buf, sizeof buf, "digest=%s", f->str[Sdigest]);
		return buf;
	}
	return nil;
}

void
showmail(Face *f)
{
	char *s;
	int n;

	if(showfd<0 || f->str[Sshow]==nil || f->str[Sshow][0]=='\0')
		return;
	s = emalloc(128+strlen(f->str[Sshow])+1);
	n = sprint(s, "faces\nshowmail\n/mail/fs/\ntext\n%s\n%ld\n%s", attr(f), strlen(f->str[Sshow]), f->str[Sshow]);
	write(showfd, s, n);
	free(s);
}

char*
value(Plumbattr *attr, char *key, char *def)
{
	char *v;

	v = plumblookup(attr, key);
	if(v)
		return v;
	return def;
}

void
setname(Face *f, char *sender)
{
	char *at, *bang;
	char *p;

	/* works with UTF-8, although it's written as ASCII */
	for(p=sender; *p!='\0'; p++)
		*p = tolower(*p);
	f->str[Suser] = sender;
	at = strchr(sender, '@');
	if(at){
		*at++ = '\0';
		f->str[Sdomain] = estrdup(at);
		return;
	}
	bang = strchr(sender, '!');
	if(bang){
		*bang++ = '\0';
		f->str[Suser] = estrdup(bang);
		f->str[Sdomain] = sender;
		return;
	}
}

int
getc(void)
{
	static uchar buf[512];
	static int nbuf = 0;
	static int i = 0;

	while(i == nbuf){
		i = 0;
		nbuf = read(logfd, buf, sizeof buf);
		if(nbuf == 0){
			sleep(15000);
			continue;
		}
		if(nbuf < 0)
			return -1;
	}
	return buf[i++];
}

char*
getline(char *buf, int n)
{
	int i, c;

	for(i=0; i<n-1; i++){
		c = getc();
		if(c <= 0)
			return nil;
		if(c == '\n')
			break;
		buf[i] = c;
	}
	buf[i] = '\0';
	return buf;
}

/* achille Jul 23 14:05:15 delivered jmk From ms.com!bub Fri Jul 23 14:05:14 EDT 1999 (plan9.bell-labs.com!jmk) 1352 */
int
parselog(char *s, char **sender, char **date)
{
	char *t, *sp, *dp;

	t = strstr(s, logtag);
	if(t == nil)
		return 0;
	dp = t-8;
	dp[5] = '\0';
	sp = t+strlen(logtag);
	for(t=sp; *t!='\0' && *t!=' '; t++)
		;
	*t = '\0';
	*sender = estrdup(sp);
	*date = estrdup(dp);
	return 1;
}

int
logrecv(char **sender, char **date)
{
	char buf[4096];

	for(;;){
		if(getline(buf, sizeof buf) == nil)
			return 0;
		if(parselog(buf, sender, date))
			return 1;
	}
	return -1;
}

char*
tweakdate(char *d)
{
	char e[8];

	/* d, date = "Mon Aug  2 23:46:55 EDT 1999" */

	if(strlen(d) < strlen("Mon Aug  2 23:46:55 EDT 1999"))
		return estrdup("");
	if(strncmp(date, d, 4+4+3) == 0)
		snprint(e, sizeof e, "%.5s", d+4+4+3);	/* 23:46 */
	else
		snprint(e, sizeof e, "%.6s", d+4);	/* Aug  2 */
	return estrdup(e);
}

Face*
nextface(void)
{
	Face *f;
	Plumbmsg *m;
	char *t, *senderp, *datep, *showmailp, *digestp;

	f = emalloc(sizeof(Face));
	for(;;){
		if(seefd >= 0){
			m = plumbrecv(seefd);
			if(m == nil)
				killall("error on seemail plumb port");
			t = value(m->attr, "mailtype", "");
			if(strcmp(t, "delete") == 0){
				delete(m->data, value(m->attr, "digest", nil));
				plumbfree(m);
				continue;
			}
			if(strcmp(t, "new") != 0){
				fprint(2, "faces: unknown plumb message type %s\n", t);
				plumbfree(m);
				continue;
			}
			if(strncmp(m->data, maildir, strlen(maildir)) != 0){	/* not about this mailbox */
				plumbfree(m);
				continue;
			}
			datep = tweakdate(value(m->attr, "date", date));
			digestp = value(m->attr, "digest", nil);
			if(alreadyseen(m->data, datep, digestp)){
				/* duplicate upas/fs can send duplicate messages */
				free(datep);
				plumbfree(m);
				continue;
			}
			senderp = estrdup(value(m->attr, "sender", "???"));
			showmailp = estrdup(m->data);
			if(digestp)
				digestp = estrdup(digestp);
			plumbfree(m);
		}else{
			if(logrecv(&senderp, &datep) <= 0)
				killall("error reading log file");
			showmailp = estrdup("");
			digestp = nil;
		}
		setname(f, senderp);
		f->str[Stime] = datep;
		f->str[Sshow] = showmailp;
		f->str[Sdigest] = digestp;
		return f;
	}
	return nil;
}

char*
iline(char *data, char **pp)
{
	char *p;

	for(p=data; *p!='\0' && *p!='\n'; p++)
		;
	if(*p == '\n')
		*p++ = '\0';
	*pp = p;
	return data;
}

Face*
dirface(char *dir, char *num)
{
	Face *f;
	char *from, *date;
	char buf[1024], pwd[1024], *info, *p, *digest;
	int n, fd;
	ulong len;

	/*
	 * loadmbox leaves us in maildir, so we needn't
	 * walk /mail/fs/mbox for each face; this makes startup
	 * a fair bit quicker.
	 */
	if(getwd(pwd, sizeof pwd) != nil && strcmp(pwd, dir) == 0)
		sprint(buf, "%s/info", num);
	else
		sprint(buf, "%s/%s/info", dir, num);
	len = dirlen(buf);
	if(len <= 0)
		return nil;
	fd = open(buf, OREAD);
	if(fd < 0)
		return nil;
	info = emalloc(len+1);
	n = readn(fd, info, len);
	close(fd);
	if(n < 0){
		free(info);
		return nil;
	}
	info[n] = '\0';
	f = emalloc(sizeof(Face));
	from = iline(info, &p);	/* from */
	iline(p, &p);	/* to */
	iline(p, &p);	/* cc */
	iline(p, &p);	/* replyto */
	date = iline(p, &p);	/* date */
	setname(f, estrdup(from));
	f->str[Stime] = tweakdate(date);
	sprint(buf, "%s/%s", dir, num);
	f->str[Sshow] = estrdup(buf);
	iline(p, &p);	/* subject */
	iline(p, &p);	/* mime content type */
	iline(p, &p);	/* mime disposition */
	iline(p, &p);	/* filename */
	digest = iline(p, &p);	/* digest */
	f->str[Sdigest] = estrdup(digest);
	free(info);
	return f;
}

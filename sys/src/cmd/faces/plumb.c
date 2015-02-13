/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
static int8_t	*user;
static int8_t	*logtag;

int8_t		**maildirs;
int		nmaildirs;

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

void
addmaildir(int8_t *dir)
{
	maildirs = erealloc(maildirs, (nmaildirs+1)*sizeof(int8_t*));
	maildirs[nmaildirs++] = dir;
}

int8_t*
attr(Face *f)
{
	static int8_t buf[128];

	if(f->str[Sdigest]){
		snprint(buf, sizeof buf, "digest=%s", f->str[Sdigest]);
		return buf;
	}
	return nil;
}

void
showmail(Face *f)
{
	int8_t *s;
	int n;

	if(showfd<0 || f->str[Sshow]==nil || f->str[Sshow][0]=='\0')
		return;
	s = emalloc(128+strlen(f->str[Sshow])+1);
	n = sprint(s, "faces\nshowmail\n/mail/fs/\ntext\n%s\n%ld\n%s", attr(f), strlen(f->str[Sshow]), f->str[Sshow]);
	write(showfd, s, n);
	free(s);
}

int8_t*
value(Plumbattr *attr, int8_t *key, int8_t *def)
{
	int8_t *v;

	v = plumblookup(attr, key);
	if(v)
		return v;
	return def;
}

void
setname(Face *f, int8_t *sender)
{
	int8_t *at, *bang;
	int8_t *p;

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
	static uint8_t buf[512];
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

int8_t*
getline(int8_t *buf, int n)
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

static int8_t* months[] = {
	"jan", "feb", "mar", "apr",
	"may", "jun", "jul", "aug", 
	"sep", "oct", "nov", "dec"
};

static int
getmon(int8_t *s)
{
	int i;

	for(i=0; i<nelem(months); i++)
		if(cistrcmp(months[i], s) == 0)
			return i;
	return -1;
}

/* Fri Jul 23 14:05:14 EDT 1999 */
uint32_t
parsedatev(int8_t **a)
{
	int8_t *p;
	Tm tm;

	memset(&tm, 0, sizeof tm);
	if((tm.mon=getmon(a[1])) == -1)
		goto Err;
	tm.mday = strtol(a[2], &p, 10);
	if(*p != '\0')
		goto Err;
	tm.hour = strtol(a[3], &p, 10);
	if(*p != ':')
		goto Err;
	tm.min = strtol(p+1, &p, 10);
	if(*p != ':')
		goto Err;
	tm.sec = strtol(p+1, &p, 10);
	if(*p != '\0')
		goto Err;
	if(strlen(a[4]) != 3)
		goto Err;
	strcpy(tm.zone, a[4]);
	if(strlen(a[5]) != 4)
		goto Err;
	tm.year = strtol(a[5], &p, 10);
	if(*p != '\0')
		goto Err;
	tm.year -= 1900;
	return tm2sec(&tm);
Err:
	return time(0);
}

uint32_t
parsedate(int8_t *s)
{
	int8_t *f[10];
	int nf;

	nf = getfields(s, f, nelem(f), 1, " ");
	if(nf < 6)
		return time(0);
	return parsedatev(f);
}

/* achille Jul 23 14:05:15 delivered jmk From ms.com!bub Fri Jul 23 14:05:14 EDT 1999 (plan9.bell-labs.com!jmk) 1352 */
/* achille Oct 26 13:45:42 remote local!rsc From rsc Sat Oct 26 13:45:41 EDT 2002 (rsc) 170 */
int
parselog(int8_t *s, int8_t **sender, uint32_t *xtime)
{
	int8_t *f[20];
	int nf;

	nf = getfields(s, f, nelem(f), 1, " ");
	if(nf < 14)
		return 0;
	if(strcmp(f[4], "delivered") == 0 && strcmp(f[5], user) == 0)
		goto Found;
	if(strcmp(f[4], "remote") == 0 && strncmp(f[5], "local!", 6) == 0 && strcmp(f[5]+6, user) == 0)
		goto Found;
	return 0;

Found:
	*sender = estrdup(f[7]);
	*xtime = parsedatev(&f[8]);
	return 1;
}

int
logrecv(int8_t **sender, uint32_t *xtime)
{
	int8_t buf[4096];

	for(;;){
		if(getline(buf, sizeof buf) == nil)
			return 0;
		if(parselog(buf, sender, xtime))
			return 1;
	}
}

int8_t*
tweakdate(int8_t *d)
{
	int8_t e[8];

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
	int i;
	Face *f;
	Plumbmsg *m;
	int8_t *t, *senderp, *showmailp, *digestp;
	uint32_t xtime;

	f = emalloc(sizeof(Face));
	for(;;){
		if(seefd >= 0){
			m = plumbrecv(seefd);
			if(m == nil)
				killall("error on seemail plumb port");
			t = value(m->attr, "mailtype", "");
			if(strcmp(t, "delete") == 0)
				delete(m->data, value(m->attr, "digest", nil));
			else if(strcmp(t, "new") != 0)
				fprint(2, "faces: unknown plumb message type %s\n", t);
			else for(i=0; i<nmaildirs; i++)
				if(strncmp(m->data, maildirs[i], strlen(maildirs[i])) == 0)
					goto Found;
			plumbfree(m);
			continue;

		Found:
			xtime = parsedate(value(m->attr, "date", date));
			digestp = value(m->attr, "digest", nil);
			if(alreadyseen(digestp)){
				/* duplicate upas/fs can send duplicate messages */
				plumbfree(m);
				continue;
			}
			senderp = estrdup(value(m->attr, "sender", "???"));
			showmailp = estrdup(m->data);
			if(digestp)
				digestp = estrdup(digestp);
			plumbfree(m);
		}else{
			if(logrecv(&senderp, &xtime) <= 0)
				killall("error reading log file");
			showmailp = estrdup("");
			digestp = nil;
		}
		setname(f, senderp);
		f->time = xtime;
		f->tm = *localtime(xtime);
		f->str[Sshow] = showmailp;
		f->str[Sdigest] = digestp;
		return f;
	}
}

int8_t*
iline(int8_t *data, int8_t **pp)
{
	int8_t *p;

	for(p=data; *p!='\0' && *p!='\n'; p++)
		;
	if(*p == '\n')
		*p++ = '\0';
	*pp = p;
	return data;
}

Face*
dirface(int8_t *dir, int8_t *num)
{
	Face *f;
	int8_t *from, *date;
	int8_t buf[1024], pwd[1024], *info, *p, *digest;
	int n, fd;
	uint32_t len;

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
	f->time = parsedate(date);
	f->tm = *localtime(f->time);
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

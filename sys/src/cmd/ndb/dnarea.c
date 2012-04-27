#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include <ip.h>
#include "dns.h"

Area *owned, *delegated;

/*
 *  true if a name is in our area
 */
Area*
inmyarea(char *name)
{
	int len;
	Area *s, *d;

	len = strlen(name);
	for(s = owned; s; s = s->next){
		if(s->len > len)
			continue;
		if(cistrcmp(s->soarr->owner->name, name + len - s->len) == 0)
			if(len == s->len || name[len - s->len - 1] == '.')
				break;
	}
	if(s == nil)
		return nil;

	/* name is in area `s' */
	for(d = delegated; d; d = d->next){
		if(d->len > len)
			continue;
		if(cistrcmp(d->soarr->owner->name, name + len - d->len) == 0)
			if(len == d->len || name[len - d->len - 1] == '.')
				return nil; /* name is in a delegated subarea */
	}

	return s;	/* name is in area `s' and not in a delegated subarea */
}

/*
 *  our area is the part of the domain tree that
 *  we serve
 */
void
addarea(DN *dp, RR *rp, Ndbtuple *t)
{
	Area *s;
	Area **l;

	lock(&dnlock);
	if(t->val[0])
		l = &delegated;
	else
		l = &owned;

	for (s = *l; s != nil; s = s->next)
		if (strcmp(dp->name, s->soarr->owner->name) == 0) {
			unlock(&dnlock);
			return;		/* we've already got one */
		}

	/*
	 *  The area contains a copy of the soa rr that created it.
	 *  The owner of the the soa rr should stick around as long
	 *  as the area does.
	 */
	s = emalloc(sizeof(*s));
	s->len = strlen(dp->name);
	rrcopy(rp, &s->soarr);
	s->soarr->owner = dp;
	s->soarr->db = 1;
	s->soarr->ttl = Hour;
	s->neednotify = 1;
	s->needrefresh = 0;

	if (debug)
		dnslog("new area %s %s", dp->name,
			l == &delegated? "delegated": "owned");

	s->next = *l;
	*l = s;
	unlock(&dnlock);
}

void
freearea(Area **l)
{
	Area *s;

	while(s = *l){
		*l = s->next;
		lock(&dnlock);
		rrfree(s->soarr);
		memset(s, 0, sizeof *s);	/* cause trouble */
		unlock(&dnlock);
		free(s);
	}
}

/*
 * refresh all areas that need it
 *  this entails running a command 'zonerefreshprogram'.  This could
 *  copy over databases from elsewhere or just do a zone transfer.
 */
void
refresh_areas(Area *s)
{
	int pid;
	Waitmsg *w;

	for(; s != nil; s = s->next){
		if(!s->needrefresh)
			continue;

		if(zonerefreshprogram == nil){
			s->needrefresh = 0;
			continue;
		}

		pid = fork();
		if (pid == -1) {
			sleep(1000);	/* don't fork again immediately */
			continue;
		}
		if (pid == 0){
			execl(zonerefreshprogram, "zonerefresh",
				s->soarr->owner->name, nil);
			exits("exec zonerefresh failed");
		}
		while ((w = wait()) != nil && w->pid != pid)
			free(w);
		if (w && w->pid == pid)
			if(w->msg == nil || *w->msg == '\0')
				s->needrefresh = 0;
		free(w);
	}
}

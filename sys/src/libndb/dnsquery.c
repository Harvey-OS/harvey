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
#include <bio.h>
#include <ndb.h>
#include <ndbhf.h>

static void nstrcpy(int8_t*, int8_t*, int);
static void mkptrname(int8_t*, int8_t*, int);
static Ndbtuple *doquery(int, int8_t *dn, int8_t *type);

/*
 *  search for a tuple that has the given 'attr=val' and also 'rattr=x'.
 *  copy 'x' into 'buf' and return the whole tuple.
 *
 *  return 0 if not found.
 */
Ndbtuple*
dnsquery(int8_t *net, int8_t *val, int8_t *type)
{
	int8_t rip[128];
	int8_t *p;
	Ndbtuple *t;
	int fd;

	/* if the address is V4 or V6 null address, give up early */
	if(strcmp(val, "::") == 0 || strcmp(val, "0.0.0.0") == 0)
		return nil;

	if(net == nil)
		net = "/net";
	snprint(rip, sizeof(rip), "%s/dns", net);
	fd = open(rip, ORDWR);
	if(fd < 0){
		if(strcmp(net, "/net") == 0)
			snprint(rip, sizeof(rip), "/srv/dns");
		else {
			snprint(rip, sizeof(rip), "/srv/dns%s", net);
			p = strrchr(rip, '/');
			*p = '_';
		}
		fd = open(rip, ORDWR);
		if(fd < 0)
			return nil;
		if(mount(fd, -1, net, MBEFORE, "") < 0){
			close(fd);
			return nil;
		}
		/* fd is now closed */
		snprint(rip, sizeof(rip), "%s/dns", net);
		fd = open(rip, ORDWR);
		if(fd < 0)
			return nil;
	}

	/* zero out the error string */
	werrstr("");

	/* if this is a reverse lookup, first lookup the domain name */
	if(strcmp(type, "ptr") == 0){
		mkptrname(val, rip, sizeof rip);
		t = doquery(fd, rip, "ptr");
	} else
		t = doquery(fd, val, type);

	/*
	 * TODO: make fd static and keep it open to reduce 9P traffic
	 * walking to /net*^/dns.  Must be prepared to re-open it on error.
	 */
	close(fd);
	ndbsetmalloctag(t, getcallerpc(&net));
	return t;
}

/*
 *  convert address into a reverse lookup address
 */
static void
mkptrname(int8_t *ip, int8_t *rip, int rlen)
{
	int8_t buf[128];
	int8_t *p, *np;
	int len;

	if(cistrstr(ip, "in-addr.arpa") || cistrstr(ip, "ip6.arpa")){
		nstrcpy(rip, ip, rlen);
		return;
	}

	nstrcpy(buf, ip, sizeof buf);
	for(p = buf; *p; p++)
		;
	*p = '.';
	np = rip;
	len = 0;
	while(p >= buf){
		len++;
		p--;
		if(*p == '.'){
			memmove(np, p+1, len);
			np += len;
			len = 0;
		}
	}
	memmove(np, p+1, len);
	np += len;
	strcpy(np, "in-addr.arpa");
}

static void
nstrcpy(int8_t *to, int8_t *from, int len)
{
	strncpy(to, from, len);
	to[len-1] = 0;
}

static Ndbtuple*
doquery(int fd, int8_t *dn, int8_t *type)
{
	int8_t buf[1024];
	int n;
	Ndbtuple *t, *first, *last;

	seek(fd, 0, 0);
	snprint(buf, sizeof(buf), "!%s %s", dn, type);
	if(write(fd, buf, strlen(buf)) < 0)
		return nil;
		
	seek(fd, 0, 0);

	first = last = nil;
	
	for(;;){
		n = read(fd, buf, sizeof(buf)-2);
		if(n <= 0)
			break;
		if(buf[n-1] != '\n')
			buf[n++] = '\n';	/* ndbparsline needs a trailing new line */
		buf[n] = 0;

		/* check for the error condition */
		if(buf[0] == '!'){
			werrstr("%s", buf+1);
			return nil;
		}

		t = _ndbparseline(buf);
		if(t != nil){
			if(first)
				last->entry = t;
			else
				first = t;
			last = t;

			while(last->entry)
				last = last->entry;
		}
	}

	ndbsetmalloctag(first, getcallerpc(&fd));
	return first;
}

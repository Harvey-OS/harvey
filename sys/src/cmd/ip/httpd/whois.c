#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include "whois.h"

int debug;

enum
{
	Rripe=		1,
	Rapnic=		2,
	Rlacnic=	3,
};

Ndbtuple*
addpair(Ndbtuple *old, char *attr, char *val, int newline)
{
	Ndbtuple *t, *nt, *last;
	char *p;

	while(*val == ' ' || *val == '\t')
		val++;
	for(p = val; *p; p++)
		;
	while(--p >= val){
		if(*p == ' ' || *p == '\t')
			*p = 0;
		else
			break;
	}

	t = ndbnew(attr, val);
	if(old == nil){
		t->line = t;
		return t;
	}

	last = old;
	for(nt = old; nt->entry != nil; nt = nt->entry)
		if(nt->entry != nt->line)
			last = nt->entry;
	if(newline){
		nt->line = last;
		t->line = t;
	} else {
		nt->line = t;
		t->line = last;
	}
	nt->entry = t;
	return old;
}

int
doquery(char *server, char *format, char *addr, char *buf, int blen, char **lines, int max)
{
	int n, fd, tries;

	if(debug)
		fprint(2, "query %s\n", server);
	fd = -1;
	for(tries = 0; tries < 6; tries++){
		fd = dial(server, 0, 0, 0);
		if(fd >= 0)
			break;
		sleep(1000*(tries+1));
	}
	if(fd < 0){
		return -1;
	}
	if(fprint(fd, format, addr) < 0){
		close(fd);
		return -1;
	}
	n = readn(fd, buf, blen-1);
	close(fd);
	if(n <= 0)
		return -1;
	buf[n] = 0;
	if(debug)
		fprint(2, "%s\n", buf);
	return getfields(buf, lines, max, 0, "\r\n");
}

Ndbtuple*
arin(char *ip, int *referral)
{
	int i, j, n, newline;
	char buf[4096];
	char *l[256];
	char *f[3];
	Ndbtuple *t;
	int exists = 0;

	*referral = 0;

	n = doquery("tcp!whois.arin.net!43", "+%s\n", ip, buf, sizeof(buf), l, nelem(l));
	if(n <= 0)
		return nil;

	/* parse the lines */
	t = nil;
	newline = 1;
	for(i = 0; i < n; i++){
		j = getfields(l[i], f, nelem(f), 0, ":");
		if(j < 2){
			newline = 1;
			continue;
		}
		if(cistrcmp(f[0], "cidr") == 0){
			t = addpair(t, "iprange", f[1], newline);
			newline = 0;
			exists = 1;
		}else if(cistrcmp(f[0], "orgname") == 0){
			t = addpair(t, "org", f[1], newline);
			newline = 0;
		}else if(cistrcmp(f[0], "address") == 0){
			t = addpair(t, "address", f[1], newline);
			newline = 0;
		}else if(cistrcmp(f[0], "city") == 0){
			t = addpair(t, "address", f[1], newline);
			newline = 0;
		}else if(cistrcmp(f[0], "stateprov") == 0){
			t = addpair(t, "address", f[1], newline);
			newline = 0;
		}else if(cistrcmp(f[0], "postalcode") == 0){
			t = addpair(t, "address", f[1], newline);
			newline = 0;
		}else if(cistrcmp(f[0], "country") == 0){
			t = addpair(t, "country", f[1], newline);
			newline = 0;
		}else if(cistrcmp(f[0], "nettype") == 0){
			if(strstr(f[1], "APNIC"))
				*referral = Rapnic;
			else if(strstr(f[1], "RIPE"))
				*referral = Rripe;
			else if(strstr(f[1], "LACNIC"))
				*referral = Rlacnic;
		}
	}
	if(exists)
		return addpair(t, "db", "arin", 1);
	ndbfree(t);
	return nil;
}

Ndbtuple*
apnic(char *ip)
{
	int i, j, n, newline;
	char buf[4096];
	char *l[256];
	char *f[3];
	Ndbtuple *t;
	int exists = 0;
	int bad = 0;

	n = doquery("tcp!whois.apnic.net!43", "%s\n", ip, buf, sizeof(buf), l, nelem(l));
	if(n <= 0)
		return nil;

	/* parse the lines */
	t = nil;
	newline = 1;
	for(i = 0; i < n; i++){
		j = getfields(l[i], f, nelem(f), 0, ":");
		if(j < 2){
			newline = 1;
			continue;
		}
		if(cistrcmp(f[0], "descr") == 0){
			if(strstr(f[1], "Not allocated by APNIC") != nil)
				bad = 1;
		}else if(cistrcmp(f[0], "inetnum") == 0){
			t = addpair(t, "iprange", f[1], newline);
			newline = 0;
			exists = 1;
		}else if(cistrcmp(f[0], "role") == 0){
			t = addpair(t, "org", f[1], newline);
			newline = 0;
		}else if(cistrcmp(f[0], "address") == 0){
			t = addpair(t, "address", f[1], newline);
			newline = 0;
		}else if(cistrcmp(f[0], "country") == 0){
			t = addpair(t, "country", f[1], newline);
			newline = 0;
		}
	}
	if(exists && !bad)
		return addpair(t, "db", "apnic", 1);
	ndbfree(t);
	return nil;
}

Ndbtuple*
ripe(char *ip)
{
	int i, j, n, newline;
	char buf[4096];
	char *l[256];
	char *f[3];
	Ndbtuple *t;
	int exists = 0;
	int bad = 0;

	n = doquery("tcp!whois.ripe.net!43", "%s\n", ip, buf, sizeof(buf), l, nelem(l));
	if(n <= 0)
		return nil;

	/* parse the lines */
	t = nil;
	newline = 1;
	for(i = 0; i < n; i++){
		j = getfields(l[i], f, nelem(f), 0, ":");
		if(j < 2){
			newline = 1;
			continue;
		}
		if(cistrcmp(f[0], "descr") == 0){
			if(strstr(f[1], "The whole IPv4 address space") != nil)
				bad = 1;
		}else if(cistrcmp(f[0], "inetnum") == 0){
			t = addpair(t, "iprange", f[1], newline);
			newline = 0;
			exists = 1;
		}else if(cistrcmp(f[0], "role") == 0){
			t = addpair(t, "org", f[1], newline);
			newline = 0;
		}else if(cistrcmp(f[0], "address") == 0){
			t = addpair(t, "address", f[1], newline);
			newline = 0;
		}else if(cistrcmp(f[0], "country") == 0){
			t = addpair(t, "country", f[1], newline);
			newline = 0;
		}
	}
	if(exists && !bad)
		return addpair(t, "db", "ripe", 1);
	ndbfree(t);
	return nil;
}


Ndbtuple*
lacnic(char *ip)
{
	int i, j, n, newline;
	char buf[4096];
	char *l[256];
	char *f[3];
	char *country;
	int foundcountry;
	Ndbtuple *t;
	int exists = 0;

	n = doquery("tcp!whois.lacnic.net!43", "%s\n", ip, buf, sizeof(buf), l, nelem(l));
	if(n <= 0)
		return nil;

	/* parse the lines */
	t = nil;
	newline = 1;
	country = nil;
	foundcountry = 0;
	for(i = 0; i < n; i++){
		/*
		 * 23 Sep 2005 - When Lacnic defers to
		 * Brazil, it shows a copyright notice but
		 * does not add a country: line.
		 */
		if(strstr(l[i], "Copyright registro.br") == 0)
			country = "BR";
		j = getfields(l[i], f, nelem(f), 0, ":");
		if(j < 2){
			newline = 1;
			continue;
		}
		if(cistrcmp(f[0], "inetnum") == 0){
			t = addpair(t, "iprange", f[1], newline);
			newline = 0;
			exists = 1;
		}
		if(cistrcmp(f[0], "owner") == 0){
			t = addpair(t, "org", f[1], newline);
			newline = 0;
		}
		if(cistrcmp(f[0], "address") == 0){
			t = addpair(t, "address", f[1], newline);
			newline = 0;
		}
		if(cistrcmp(f[0], "country") == 0){
			t = addpair(t, "country", f[1], newline);
			foundcountry = 1;
			newline = 0;
		}
	}
	if(!foundcountry && country)
		t = addpair(t, "country", country, newline);
	if(exists)
		return addpair(t, "db", "lacnic", 1);
	ndbfree(t);
	return nil;
}

int
isco(Ndbtuple *t)
{
	for(; t != nil; t = t->entry)
		if(strcmp(t->attr, "country") == 0)
			return 1;
	return 0;
}

Ndbtuple*
concat(Ndbtuple *t, Ndbtuple *nt)
{
	Ndbtuple **l;

	for(l = &t; *l != nil; l = &(*l)->entry)
		;
	*l = nt;

	return t;
}

Ndbtuple*
whois(char *netroot, char *ip)
{
	Ndbtuple *t, **l, *nt, *last;
	int referral;

	// look for a country in the various databases
	t = arin(ip, &referral);
	if(referral){
		// if we're referred, ignore what we already have
		free(t);
		t = nil;
		switch(referral){
		case Rripe:
			t = ripe(ip);
			break;
		case Rlacnic:
			t = lacnic(ip);
			break;
		case Rapnic:
			t = apnic(ip);
			break;
		}
	} else if(!isco(t)){
		// keep trying till we have a country
		concat(t, ripe(ip));
		if(!isco(t)){
			t = concat(t, apnic(ip));
			if(!isco(t)){
				t = concat(t, lacnic(ip));
			}
		}
	}
	for(l = &t; *l != nil; l = &(*l)->entry)
		;

	// do a reverse dns lookup on the address
	*l = dnsquery(netroot, ip, "ptr");

	// now do forward lookup for each domain name
	last = nt = *l;
	for(; *l != nil; l = &(*l)->entry)
		last = *l;
	for(; nt != nil; nt = nt->entry){
		if(strcmp(nt->attr, "dom") == 0){
			*l = dnsquery(netroot, nt->val, "ip");
			for(; *l != nil; l = &(*l)->entry)
				;
		}
		if(nt == last)
			break;
	}

	return t;
}

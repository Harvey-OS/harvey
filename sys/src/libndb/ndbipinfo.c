#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>

typedef struct Attr Attr;
struct Attr
{
	Attr		*next;
	int		addronly;
	char		*attr;
	Ndbtuple	*first;	
	Ndbtuple	*last;
	int		masklen;
};

#ifdef DEBUGGING
static void
prtuple(char *tag, Ndbtuple *t)
{
	print("%s tag\n\t", tag);
	for(; t != nil; t = t->entry){
		print("%s=%s ", t->attr, t->val);
		if(t->entry != t->line){
			if(t->entry != nil)
				print("\n\t");
			else
				print("\n");
		}
	}
}
#endif DEBUGGING

/*
 *  reorder the tuple to put x's line first in the entry
 */
static Ndbtuple*
reorder(Ndbtuple *t, Ndbtuple *x)
{
	Ndbtuple *nt;
	Ndbtuple *last, *prev;

	/* if x is first, we're done */
	if(x == t)
		return t;

	/* find end of x's line */
	for(last = x; last->line == last->entry; last = last->line)
		;

	/* rotate to make this line first */
	if(last->line != t){

		/* detach this line and everything after it from the entry */
		for(nt = t; nt->entry != last->line; nt = nt->entry)
			;
		nt->entry = nil;
	
		/* switch */
		for(nt = last; nt->entry != nil; nt = nt->entry)
			;
		nt->entry = t;
	}

	/* rotate line to make x first */
	if(x != last->line){

		/* find entry before x */
		for(prev = last; prev->line != x; prev = prev->line);
			;

		/* detach line */
		nt = last->entry;
		last->entry = last->line;

		/* reattach */
		prev->entry = nt;
	}

	return x;
}

/*
 *  lookup an ip addresses
 */
static Ndbtuple*
toipaddr(Ndb *db, Ndbtuple *t)
{
	Ndbtuple *xt, *nt, **l;
	char buf[Ndbvlen];
	Ndbs s;
	char *attr;

	attr = ipattr(t->val);
	if(strcmp(attr, "ip") == 0)
		return t;

	nt = ndbgetval(db, &s, attr, t->val, "ip", buf);
	if(nt == nil)
		return 0;
	nt = reorder(nt, s.t);

	/* throw out non ip twoples */
	l = &nt;
	while(*l != nil){
		xt = *l;
		if(strcmp(xt->attr, "ip") != 0){
			/* unlink */
			*l = xt->entry;
			xt->entry = nil;

			/* free */
			ndbfree(xt);
			continue;
		}
		strcpy(xt->attr, t->attr);
		l = &xt->entry;
	}

	ndbfree(t);
	return nt;
}

/*
 *  Mine the entry for the desired attributes.  Longer masks
 *  take priority.
 */
static Ndbtuple*
mine(Ndb *db, Ndbtuple *t, Attr *a, int masklen, int *needed)
{
	Ndbtuple **l, *nt;

	for(; a != nil; a = a->next){
		for(l = &t; *l != nil;){
			nt = *l;
			if(strcmp(a->attr, nt->attr) != 0){
				l = &nt->entry;
				continue;
			}

			if(a->first){
				/* longer masks replace shorter */
				if(masklen > a->masklen){
					ndbfree(a->first);
					a->first = nil;
					a->last = nil;
					(*needed)++;
				} else if(a->masklen > masklen) {
					l = &nt->entry;
					continue;
				}
			}

			/* unchain tuple from t */
			*l = nt->entry;
			nt->entry = nil;

			if(a->addronly)
				nt = toipaddr(db, nt);
			if(nt == nil)
				continue;

			/* chain tuple into the Attr */
			if(a->last)
				a->last->entry = nt;
			else
				a->first = nt;
			while(nt->entry)
				nt = nt->entry;
			a->last = nt;
			a->masklen = masklen;
			(*needed)--;
		}
	}

	return t;
}

static int
prefixlen(uchar *ip)
{
	int y, i;

	for(y = IPaddrlen-1; y >= 0; y--)
		for(i = 8; i > 0; i--)
			if(ip[y] & (1<<(8-i)))
				return y*8 + i;
	return 0;
}

/*
 *  look for all networks whose masks are shorter than lastlen
 *  and whose IP address matches net.
 */
static void
subnet(Ndb *db, uchar *net, Attr *a, int prefix, int *needed)
{
	Ndbs s;
	Ndbtuple *t;
	char netstr[Ndbvlen];
	char maskstr[Ndbvlen];
	uchar mask[IPaddrlen];
	int masklen;

	sprint(netstr, "%I", net);
	t = ndbsearch(db, &s, "ip", netstr);
	while(t != nil){
		if(ndblookval(t, t, "ipnet", maskstr) != nil){
			if(ndblookval(t, t, "ipmask", maskstr))
				parseipmask(mask, maskstr);
			else
				ipmove(mask, defmask(net));
			masklen = prefixlen(mask);
			if(masklen <= prefix)
				t = mine(db, t, a, masklen, needed);
		}
		ndbfree(t);
		t = ndbsnext(&s, "ip", netstr);
	}
}

/*
 *  fill in all the requested attributes for a system.
 *  if the system's entry doesn't have all required,
 *  walk through successively more inclusive networks
 *  for inherited attributes.
 */
Ndbtuple*
ndbipinfo(Ndb *db, char *attr, char *val, char **alist, int n)
{
	Ndbtuple *t, *nt, **lt;
	Ndbs s;
	char *p;
	Attr *a, *list, **l;
	int i, needed;
	char ipstr[Ndbvlen];
	uchar ip[IPaddrlen];
	uchar net[IPaddrlen];
	int prefix, smallestprefix;

	/* just in case */
	fmtinstall('I', eipfmt);
	fmtinstall('M', eipfmt);

	/* get needed attributes */
	needed = 0;
	l = &list;
	for(i = 0; i < n; i++){
		p = *alist++;
		if(p == nil)
			break;
		a = mallocz(sizeof(*a), 1);
		if(a == nil)
			break;
		if(*p == '@'){
			a->addronly = 1;
			p++;
		}
		a->attr = p;
		*l = a;
		l = &(*l)->next;
		needed++;
	}

	/*
	 *  first look for a matching entry with an ip address
	 */
	t = ndbgetval(db, &s, attr, val, "ip", ipstr);
	if(t == nil){
		/* none found, make one up */
		if(strcmp(attr, "ip") != 0)
			return nil;
		strncpy(ipstr, val, sizeof(ipstr)-1);
		ip[sizeof(ip)-1] = 0;
		t = malloc(sizeof(*t));
		strcpy(t->attr, "ip");
		strcpy(t->val, ipstr);
		t->line = t;
		t->entry = nil;
		t = mine(db, t, list, 128, &needed);
		ndbfree(t);
	} else {
		/* found one */
		while(t != nil){
			t = reorder(t, s.t);
			t = mine(db, t, list, 128, &needed);
			ndbfree(t);
			t = ndbsnext(&s, attr, val);
		}
	}
	parseip(ip, ipstr);
	ipmove(net, ip);

	/*
	 *  now go through subnets to fill in any missing attributes
	 */
	if(isv4(ip)){
		prefix = 127;
		smallestprefix = 100;
	} else {
		/* in v6, the last 8 bytes have no structure */
		prefix = 64;
		smallestprefix = 2;
		memset(net+8, 0, 8);
	}

	/*
	 *  to find a containing network, keep turning off
	 *  the lower bit and look for a network with
	 *  that address and a shorter mask.  tedius but
	 *  complete, we may need to find a trick to speed this up.
	 */
	for(; prefix >= smallestprefix; prefix--){
		if(needed == 0)
			break;
		if((net[prefix/8] & (1<<(7-(prefix%8)))) == 0)
			continue;
		net[prefix/8] &= ~(1<<(7-(prefix%8)));
		subnet(db, net, list, prefix, &needed);
	}

	/*
	 *  chain together the results and free the list
	 */
	lt = &t;
	while(list != nil){
		a = list;
		list = a->next;
		*lt = a->first;
		if(*lt == nil && strcmp(a->attr, "ipmask") == 0){
			/* add a default ipmask if we need one */
			*lt = mallocz(sizeof( Ndbtuple), 1);
			strcpy((*lt)->attr, "ipmask");
			snprint((*lt)->val, sizeof((*lt)->val), "%M", defmask(ip));
		}
		while(nt = *lt){
			nt->line = nt->entry;
			lt = &nt->entry;
		}
		free(a);
	}

	return t;
}

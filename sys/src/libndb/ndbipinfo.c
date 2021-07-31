#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>

typedef struct Attr Attr;
struct Attr
{
	Attr		*next;
	int		gen;
	int		addronly;
	char		*attr;
	Ndbtuple	*first;	
	Ndbtuple	*last;
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
 *  mine the entry for the desired attributes
 */
static Ndbtuple*
mine(Ndb *db, Ndbtuple *t, Attr *a, int gen)
{
	Ndbtuple **l, *nt;

	for(; a != nil; a = a->next){
		for(l = &t; *l != nil;){
			nt = *l;
			if(strcmp(a->attr, nt->attr) == 0)
			if(a->first == nil || a->gen >= gen){

				/* unchain tuple from t */
				*l = nt->entry;
				nt->entry = nil;

				if(a->addronly)
					nt = toipaddr(db, nt);
				if(nt == nil)
					continue;

				/* chain it into the Attr */
				if(a->last)
					a->last->entry = nt;
				else
					a->first = nt;
				while(nt->entry)
					nt = nt->entry;
				a->last = nt;
				a->gen = gen;
				continue;
			}
			l = &nt->entry;
		}
	}

	return t;
}


/*
 *  lookup a subnet and fill in anything we can
 */
static int
recursesubnet(Ndb *db, uchar *ipa, uchar *mask, Attr *list, int gen)
{
	Ndbs s;
	Ndbtuple *t, *nt;
	uchar ipnet[IPaddrlen];
	uchar nmask[IPaddrlen];
	char ip[Ndbvlen];
	char str[Ndbvlen];

	/* find entry ip=ipa&mask ipmask=mask */
	maskip(ipa, mask, ipnet);
	sprint(ip, "%I", ipnet);
//print("nrecurse ip=%I mask=%M net=%I\n", ipa, mask, ipnet);
	t = ndbsearch(db, &s, "ip", ip);
	while(t){
		if(ndblookval(t, s.t, "ipmask", str)){
//print("found net ipmask=%s\n", str);
			parseipmask(nmask, str);
			if(ipcmp(mask, nmask) == 0)
				break;
		} else {
			if(ipcmp(mask, defmask(ipa)) == 0)
				break;
		}
		ndbfree(t);
		t = ndbsnext(&s, "ip", ip);
	}
	if(t == nil)
		return gen;

	/* give priority to this line in the tuple */
	t = reorder(t, s.t);

	/* look through possible subnets */
	for(nt = t; nt != nil; nt = nt->entry){
		if(strcmp(nt->attr, "ipsubmask") == 0){
			parseipmask(nmask, nt->val);

			/* recurse only if it has changed */
			/* (if it didn't change that's a bug in the database) */
			if(ipcmp(nmask, mask) != 0)
				gen = recursesubnet(db, ipa, nmask, list, gen);
		}
	}

	/* fill in with whatever this entry has */
	t = mine(db, t, list, gen);
	ndbfree(t);

	return gen+1;
}

enum
{
	Max=	32,
};

Ndbtuple*
ndbipinfo(Ndb *db, char *attr, char *val, char **alist, int n)
{
	Ndbtuple *t, *nt, **lt;
	Ndbs s;
	char *p;
	Attr *a, *list, **l;
	int i, gen, needipmask;
	char ip[Ndbvlen];
	uchar ipa[IPaddrlen];

	/* just in case */
	fmtinstall('I', eipfmt);
	fmtinstall('M', eipfmt);

	/* get needed attributes */
	needipmask = 0;
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
		if(strcmp(p, "ipmask") == 0)
			needipmask = 1;
		*l = a;
		l = &(*l)->next;
	}

	/*
	 *  look for an entry with an ip addr specified
	 */
	gen = 0;
	t = ndbgetval(db, &s, attr, val, "ip", ip);
	if(t == nil){
		/* none found, make one up */
		if(strcmp(attr, "ip") != 0)
			return nil;
		strncpy(ip, val, sizeof(ip)-1);
		ip[sizeof(ip)-1] = 0;
		t = malloc(sizeof(*t));
		strcpy(t->attr, "ip");
		strcpy(t->val, ip);
		t->line = t;
		t->entry = nil;
		t = mine(db, t, list, gen++);
		ndbfree(t);
	} else {
		/* found one */
		while(t != nil){
			t = reorder(t, s.t);
			t = mine(db, t, list, gen++);
			ndbfree(t);
			t = ndbsnext(&s, attr, val);
		}
	}
	parseip(ipa, ip);

	/*
	 *  if anything is left unspecified, look for defaults
	 *  specified from networks and subnets
	 */
	for(a = list; a != nil; a = a->next)
		if(a->first == nil)
			break;
	if(a != nil)
		recursesubnet(db, ipa, defmask(ipa), list, gen);

	/*
	 *  chain together the results and free the list
	 */
	lt = &t;
	while(list != nil){
		a = list;
		list = a->next;
		*lt = a->first;
		while(nt = *lt){
			if(needipmask && strcmp(nt->attr, "ipmask") == 0)
				needipmask = 0;
			nt->line = nt->entry;
			lt = &nt->entry;
		}
		free(a);
	}
	if(needipmask){
		*lt = nt = mallocz(sizeof(Ndbtuple), 1);
		strcpy(nt->attr, "ipmask");
		sprint(nt->val, "%M", defmask(ipa));
	}

	return t;
}

/*
 *
 *	example of categories
 *
 *	*X-xcountry
 *	*N-night
 *	*I-instrument
 *	*H-hood
 *	*M-simulator
 *	*D-dual
 *	*P-pic
 *	*T-teach
 *	*Z-total
 *	*S-solo
 *	%2
 *	*L-landings
 *
 *	fmt 0 print in tenths
 *	fmt 1 print in minutes
 *	fmt 2 print in whole numbers
 */
#include	<u.h>
#include	<libc.h>
#include	<bio.h>

typedef	vlong	Mask;
typedef	struct	Buck	Buck;
struct	Buck
{
	char*	name;
	Mask	mask;
	long	time;		/* total time */
	long	subtotal;	/* since last ! */
	char	order;		/* sort order */
	char	fmt;		/* print format */
	Buck*	link;
};
typedef	struct	Basic	Basic;
struct	Basic
{
	char	abbrev;
	long	time0;
	Mask	mask;
	Basic*	link;
};

Buck*	btype;
Buck*	btail;
Buck*	btime;
Basic*	basic;
Basic*	tlist;
Basic*	flist;
Biobuf*	b;
long	line;
char	col[500];
int	order;
int	deffmt;
Mask	mask;
long	curdate;
long	savedate;
long	lastyear;
int	all;
int	vflag;

void	dolog(void);
void	prlog(int);
char*	getcol(char*);
Buck*	lookup(Buck**, char*);
void	gettime(void);
void*	mal(long);
Buck*	lsort(Buck*, int(*)(Buck*, Buck*));
int	ocmp(Buck*, Buck*);
int	ncmp(Buck*, Buck*);
Mask	tomask(int);
Biobuf*	openfile(char*, char*);
long	getdate(char*);
long	getcurdate(void);
long	pastdate(int, char*);
void	saveall(void);

void
main(int argc, char *argv[])
{
	char *h;

	curdate = getcurdate();
	ARGBEGIN {
	default:
		fprint(2, "usage: log [file]\n");
		exits(0);
	case 'd':
		savedate = pastdate(1, ARGF());
		break;
	case 'm':
		savedate = pastdate(31, ARGF());
		break;
	case 'y':
		savedate = pastdate(31*12, ARGF());
		break;
	case 'a':
		all = 1;
		break;
	case 'v':
		vflag = 1;
		break;
	} ARGEND

	deffmt = '0';	/* hours.tenths */
	b = 0;
	h = getenv("home");
	if(h == 0)
		h = ".";
	if(argc > 0)
		b = openfile(h, argv[0]);
	if(b == 0)
		b = openfile(h, 0);
	if(b == 0) {
		fprint(2, "cant open log: %r\n");
		exits("open");
	}
	dolog();
	prlog(all);
	exits(0);
}

void
dolog(void)
{
	char *p;
	Buck *b1, *b2, *b3;
	Basic *b0;
	Mask mask;
	long da, oda;

	oda = 0;
	for(line=1;; line++) {
		p = Brdline(b, '\n');
		if(p == 0)
			break;
		if(*p == ' ' || *p == '\t' || *p == '\n')
			continue;
		if(*p == '!') {
			if(savedate)
				continue;
			break;
		}
		if(*p == '>') {
			if(savedate == 0)
				saveall();
			continue;
		}
		if(*p == '%') {
			deffmt = p[1];
			continue;
		}

		if(*p == '*') {
			memcpy(col, p+1, 100);
			col[100] = '\n';
			*strchr(col, '\n') = 0;
			mask = 0;
			for(p=col; *p; p++) {
				if(*p == '-')
					break;
				mask |= tomask(*p);
			}
			if(*p != '-')
				continue;
			b1 = lookup(&btime, p+1);
			b1->mask = mask;
			b1->order = order;
			order++;
			continue;
		}
		p = getcol(p);	/* date */
		if(p) {
			da = getdate(col);
			if(da < oda)
				print("cronology %ld: %s\n", line, col);
			if(oda < savedate && da >= savedate) {
				print("savedate  %ld: %s\n", line, col);
				saveall();
			}
			oda = da;
		}
		p = getcol(p);	/* type */
		b1 = nil;
		if(p)
			b1 = lookup(&btype, col);
		if(vflag && b1 == nil)
			print("no type  %ld: %s\n", line, col);
		p = getcol(p);	/* tail number */
		b2 = nil;
		if(p)
			b2 = lookup(&btail, col);
		if(vflag && b2 == nil)
			print("no tail  %ld: %s\n", line, col);
		p = getcol(p);	/* from to */
		p = getcol(p);	/* total time */
		while(p) {
			gettime();
			p = getcol(p);
		}
		if(vflag && tlist == nil)
			print("no times  %ld: %s\n", line, col);
		while(b0 = tlist) {
			tlist = b0->link;

			if(b1 != nil)
				b1->time += b0->time0;
			if(b2 != nil)
				b2->time += b0->time0;

			for(b3=btime; b3; b3=b3->link)
				if((b0->mask&b3->mask) == b3->mask)
					b3->time += b0->time0;

			b0->link = flist;
			flist = b0;
		}
	}
	if(savedate && oda < savedate) {
		print("no savedate %ld\n", line);
		saveall();
	}
}

char*
getcol(char *p)
{
	char *q;

	if(p == 0)
		return 0;
	while(*p == '\t' || *p == ' ')
		p++;
	if(*p == '\n')
		return 0;
	q = col;
	while(*p != '\t' && *p != ' ' && *p != '\n')
		*q++ = *p++;
	*q = 0;
	return p;
}

Buck*
lookup(Buck **ba, char *s)
{
	Buck *b;

	for(b=*ba; b; b=b->link)
		if(strcmp(b->name, s) == 0)
			return b;
	b = mal(sizeof(*b));
	b->link = *ba;
	*ba = b;
	b->name = strdup(s);
	b->time = 0;
	b->fmt = deffmt;
	return b;
}

void*
mal(long n)
{
	void *v;

	v = malloc(n);
	if(v == 0) {
		fprint(2, "cant alloc\n");
		exits("mem");
	}
	memset(v, 0, n);
	return v;
}

Mask
tomask(int c)
{
	Basic* b;

	for(b=basic; b; b=b->link)
		if(c == b->abbrev)
			return b->mask;
	b = mal(sizeof(*b));
	b->link = basic;
	basic = b;
	b->abbrev = c;
	mask <<= 1;
	if(mask == 0)
		mask = 1;
	b->mask = mask;
	return mask;
}

void
gettime(void)
{
	char *p;
	int c;
	long n1, n2, n3;
	Basic *b;
	Mask mask;

	p = strchr(col, '-');
	if(p == 0)
		return;
	p++;
	n1 = 0;
	while(*p >= '0' && *p <= '9')
		n1 = n1*10 + *p++ - '0';
	c = *p++;
	if(c != '.' && c != ':')
		goto whole;
	n2 = 0;
	n3 = 1;
	while(*p >= '0' && *p <= '9') {
		n2 = n2*10 + *p++ - '0';
		n3 *= 10;
	}
	if(*p != 0)
		goto bad;
	if(c == '.')
		n1 = n1*60 + n2*60/n3;
	else
		n1 = n1*60 + n2*100/n3;
whole:
	mask = 0;
	for(p=col; *p; p++) {
		if(*p == '-')
			break;
		mask |= tomask(*p);
	}
	b = flist;
	if(b == 0)
		b = mal(sizeof(*b));
	else
		flist = b->link;
	b->time0 = n1;
	b->mask = mask;
	if(tlist == 0) {
		tlist = b;
		b->link = 0;
		return;
	}
	b->link = tlist->link;
	tlist->link = b;

	b->mask |= tlist->mask;
	tlist->time0 -= b->time0;
	return;

bad:
	print("bad time %ld: %s\n", line, col);
}

void
dopr1(Buck *b0, char *str, int all)
{
	Buck *b;
	int f;
	long t;

	f = 0;
	if(!all) {
		for(b=b0; b; b=b->link)
			if(b->subtotal != b->time)
				f++;
		if(f == 0)
			return;
	}

	print("%s\n", str);
	for(b=b0; b; b=b->link) {
		if(f) {
			t = b->time - b->subtotal;
			if(t == 0)
				continue;
			switch(b->fmt) {
			default:
				print("	%7ld*%d*", t, b->fmt);
				break;
			case '0':
				print("	%7.1f", t/60.0);
				break;
			case '1':
				print("	%4ld:%.2ld", t/60, t%60);
				break;
			case '2':
				print("	%7ld", t);
				break;
			}
		}
		t = b->time;
		switch(b->fmt) {
		default:
			print("	%7ld*%d* %s\n", t, b->fmt, b->name);
			break;
		case '0':
			print("	%7.1f %s\n", t/60.0, b->name);
			break;
		case '1':
			print("	%4ld:%.2ld %s\n", t/60, t%60, b->name);
			break;
		case '2':
			print("	%7ld %s\n", t, b->name);
			break;
		}
	}
}

void
prlog(int all)
{
	btime = lsort(btime, ocmp);
	btype = lsort(btype, ncmp);
	btail = lsort(btail, ncmp);

	dopr1(btime, "times", all);
	dopr1(btype, "types", all);
	dopr1(btail, "tails", all);
}

Buck*
lsort(Buck *l, int (*f)(Buck*, Buck*))
{
	Buck *l1, *l2, *le;

	if(l == 0 || l->link == 0)
		return l;

	l1 = l;
	l2 = l;
	for(;;) {
		l2 = l2->link;
		if(l2 == 0)
			break;
		l2 = l2->link;
		if(l2 == 0)
			break;
		l1 = l1->link;
	}

	l2 = l1->link;
	l1->link = 0;
	l1 = lsort(l, f);
	l2 = lsort(l2, f);

	/* set up lead element */
	if((*f)(l1, l2) < 0) {
		l = l1;
		l1 = l1->link;
	} else {
		l = l2;
		l2 = l2->link;
	}
	le = l;

	for(;;) {
		if(l1 == 0)
			goto only2;
		if(l2 == 0)
			goto only1;
		if((*f)(l1, l2) < 0) {
			le->link = l1;
			le = l1;
			l1 = l1->link;
		} else {
			le->link = l2;
			le = l2;
			l2 = l2->link;
		}
	}

only1:
	while(l1) {
		le->link = l1;
		le = l1;
		l1 = l1->link;
	}
	le->link = 0;
	return l;

only2:
	while(l2) {
		le->link = l2;
		le = l2;
		l2 = l2->link;
	}
	le->link = 0;
	return l;
}

int
ocmp(Buck *l1, Buck *l2)
{
	return l1->order - l2->order;
}

int
ncmp(Buck *l1, Buck *l2)
{
	return strcmp(l1->name, l2->name);
}

Biobuf*
openfile(char *h, char *file)
{
	char path[100];
	Biobuf *b;

	if(file == 0)
		file = "log";

	b = Bopen(file, OREAD);
	if(b) return b;

	sprint(path, "%s/lib/%s", h, file);
	b = Bopen(path, OREAD);
	if(b) return b;

	sprint(path, "%s/lib/cfi/%s", h, file);
	b = Bopen(path, OREAD);
	if(b) return b;

	sprint(path, "%s/lib/%s-log", h, file);
	b = Bopen(path, OREAD);
	if(b) return b;

	sprint(path, "%s/lib/cfi/%s-log", h, file);
	b = Bopen(path, OREAD);
	if(b) return b;

	sprint(path, "/usr/%s/lib/log", file);
	b = Bopen(path, OREAD);
	if(b) return b;

	sprint(path, "/usr/%s/lib/%s-log", file, file);
	b = Bopen(path, OREAD);
	if(b) return b;

	return nil;
}

long
getdate(char *s)
{
	int mo, yr, da, i;

	for(i=0; s[i]; i++)
		if(s[i] < '0'|| s[i] > '9')
			goto bad;

	switch(i) {
	default:
		goto bad;
	case 4:
		mo = (s[0]-'0')*10 + s[1]-'0';
		da = (s[2]-'0')*10 + s[3]-'0';
		yr = lastyear;
		break;
	case 6:
		mo = (s[0]-'0')*10 + s[1]-'0';
		da = (s[2]-'0')*10 + s[3]-'0';
		yr = (s[4]-'0')*10 + s[5]-'0';
		if(yr >= 50)
			yr += 1900;
		else
			yr += 2000;
		break;
	case 8:
		mo = (s[0]-'0')*10 + s[1]-'0';
		da = (s[2]-'0')*10 + s[3]-'0';
		yr = (s[4]-'0')*1000 + (s[5]-'0')*100 + (s[6]-'0')*10 + s[7]-'0';
		break;
	}
	lastyear = yr;
	return da + 31*(mo + 12*yr);
bad:
	print("bad date %ld: %s\n", line, s);
	return 0;
}

long
getcurdate(void)
{
	char s[10];
	Tm* t;

	t = localtime(time(0));
	sprint(s, "%.2d%.2d%.4d",
		t->mon+1, t->mday, t->year+1900);
	return getdate(s);
}

long
pastdate(int n, char *s)
{
	int c, t;

	t = 0;
	while(c = *s++) {
		if(c < '0' || c > '9') {
			print("bad argc %c\n", c);
			return 0;
		}
		t = t*10 + (c-'0');
	}
	return curdate - t*n;
}

void
saveall(void)
{
	Buck *b;

	for(b=btime; b; b=b->link)
		b->subtotal = b->time;
	for(b=btype; b; b=b->link)
		b->subtotal = b->time;
	for(b=btail; b; b=b->link)
		b->subtotal = b->time;
}

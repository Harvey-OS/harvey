#include "audio.h"

int parens;

Info*
info(char *n, char *v)
{
	Info *i;

	i = mal(sizeof(Info));
	i->name = n;
	i->value = v;
	if(i->name == strtitle)
		ntitle++;
	else
	if(i->name == strartist)
		nartist++;
	else
	if(i->name == strmusic)
		nmusic++;
	return i;
}

void*
mal(long n)
{
	void *v;

	totalmal += n;
	v = malloc(n);
	if(v == 0)
		sysfatal("out of mem");
	memset(v, 0, n);
	return v;
}

Sym*
symdict(char *name)
{
	Sym *s;
	ulong h;
	int c, c0;
	char *p;

loop:
	h = 0;
	for(p = name; c = *p; p++)
		h = h*3 + (uchar)c;
	if(p - name >= Strsize-10) {
		name[Strsize-11] = 0;
		goto loop;
	}

	if((long)h < 0)
		h = ~h;
	h %= nelem(hash);
	c0 = name[0];
	for(s = hash[h]; s; s = s->link)
		if(s->name[0] == c0)
			if(strcmp(s->name, name) == 0)
				return s;

	c = (p-name)+1;
	if(c > nstringsp) {
		nstringsp = 5000;
		stringsp = mal(nstringsp);
	}
	s = mal(sizeof(Sym));
	s->name = stringsp;
	stringsp += c;
	nstringsp -= c;
	strcpy(s->name, name);

	s->label = 0;
	s->link = hash[h];
	hash[h] = s;

	return s;
}

char*
strdict(char *name)
{
	return symdict(name)->name;
}

int
mapchar(char **key)
{
	Runeinf ri;
	int c;

loop:
	c = getrune(key);
	runeinf(c, &ri);
	switch(ri.class) {
	case Alpha:
		if(ri.lower != c)
			runeinf(ri.lower, &ri);
	case Numer:
		if(ri.latin[0])
			c = ri.latin[0];
		break;
	case Space:
		c = ' ';
		break;
	case Null:
		c = 0;
		break;
	default:
		goto loop;
	}
	return c;
}

int
tiebreak(Group *g1, Group *g2)
{
	Cut *c;
	int n1, n2;

	if(g1 == 0) {
		if(g2 == 0)
			return 0;
		return -1;
	}
	if(g2 == 0)
		return +1;
	n1 = 0;
	for(c=g1->cut; c; c=c->link)
		n1++;
	n2 = 0;
	for(c=g2->cut; c; c=c->link)
		n2++;
	return n2-n1;
}

int
cxcmp(void *vv1, void *vv2)
{
	Cxlist *v1, *v2;
	char *k1, *k2;
	int n1, n2;
	char buf[1000], *b2;

	v1 = vv1;
	v2 = vv2;
	k1 = v1->info->value;
	k2 = v2->info->value;

	for(;;) {
		n1 = mapchar(&k1);
		n2 = mapchar(&k2);
		if(n1 > n2)
			return +1;
		if(n1 < n2)
			return -1;
		if(n1 == 0) {
			strncpy(buf, titstring(v1->group), sizeof(buf));
			b2 = titstring(v2->group);
			n1 = strcmp(buf, b2);
			if(n1)
				return n1;
			break;
		}
	}
	return 0;
}

int
cxcmp1(void *vv1, void *vv2)
{
	Cxlist *v1, *v2;
	int n;

	v1 = vv1;
	v2 = vv2;
	n = cxcmp(v1, v2);
	if(n == 0)
		return tiebreak(v1->group, v2->group);
	return 0;
}

int
cxcmp2(void *vv1, void *vv2)
{
	Cxlist *v1, *v2;
	int n;

	v1 = vv1;
	v2 = vv2;
	n = cxcmp(v1, v2);
	if(n)
		return n;
	if(v1->group->type == Gdupl) {
		if(v2->group->type == Gdupl)
			return 0;
		return +1;
	}
	if(v2->group->type == Gdupl)
		return -1;
	return 0;
}

int
cutcmp(void *vv1, void *vv2)
{
	Cxlist *v1, *v2;
	Cut *u1, *u2;

	v1 = vv1;
	v2 = vv2;
	u1 = v1->cut;
	u2 = v2->cut;
	if(u1->title != u2->title)
		return strcmp(u2->title, u1->title);
	return 0;
}

int
cutcmp1(void *vv1, void *vv2)
{
	Cxlist *v1, *v2;
	Cut *u1, *u2;

	v1 = vv1;
	v2 = vv2;
	u1 = v1->cut;
	u2 = v2->cut;
	if(u1->title != u2->title)
		return strcmp(u2->title, u1->title);
	return u2->cut - u1->cut;
}

int
titcmp(void *vv1, void *vv2)
{
	Cxlist *v1, *v2;
	char *k1, *k2;
	int n;

	v1 = vv1;
	v2 = vv2;
	k1 = v1->group->title;
	k2 = v2->group->title;
	if(k1 == 0) {
		if(k2 == 0)
			return tiebreak(v1->group, v2->group);
		return -1;
	}
	if(k2 == 0)
		return +1;
	n = strcmp(k1, k2);
	if(n)
		return n;
	return tiebreak(v1->group, v2->group);
}

/*
 * this is same as access in database
 * must be constant and portable
 */
int
srchash(char *s)
{
	ulong h;
	int c;

	h = 0;
	for(; c = *s; s++)
		h = h*3 + (c & 0xff);
	return h & 0xff;
}

Group*
parse(void)
{
	char *p, *q;
	Group *g, *r;
	Sym *s1, *s;
	Info *f, *f0;
	int t, setup, xsetup;

	r = mal(sizeof(Group));
	r->type = Ggroup;

	setup = 0;
	xsetup = 0;
	f0 = 0;
	f = 0;
	while ((p = getline()) != nil) {
		q = strchr(p, ':');
		if(q) {
			*q++ = 0;
			s1 = trim(p);
			if(s1 == 0)
				continue;
			p = s1->name;
			s = trim1(q);
			if(s == 0) {
				/* BOTCH probably r isnt the labeled object! */
				if(s1->label)
					fprint(2, "%ld: dual label %s\n", lineno, s1->name);
				s1->label = r;
				continue;
			}
			if(p == strsetup) {
				p = s->name;
				if(p == strsort)
					setup |= Ssort;
				else
				if(p == strtitle)
					setup |= Stitle;
				else
				if(p == strartist)
					setup |= Sartist;
				else
				if(p == strmusic)
					setup |= Smusic;
				else
				if(p == strnosplit)
					setup |= Snosplit;
				else
				if(p == strnocut)
					setup |= Snocut;
				else
				if(p == strnoword)
					setup |= Snoword;
				else
				if(p == strmark)
					setup |= Smark;
				else
				if(p == strnumber)
					setup |= Snumber;
				else
				if(p == strrelative)
					setup |= Srelative;
				else
					print("unknown setup: %s\n",  p);
				if(f0 == 0) {
					xsetup |= setup;
					setup = 0;
				}
				continue;
			}
			if(p == strpath) {
				apath = s->name;
				if(apath == strdot)
					apath = strnull;
				continue;
			}
			if(p == strvolume) {
				avolume = s->name;
				continue;
			}
			if(p == strfile) {
				t = Gfile;
				goto mkg1;
			}
			if(p == strdupl) {
				t = Gdupl;
				goto mkg1;
			}
			if(p == strvirt) {
				t = Gvirt;
				if(allfile)
					t = Gfile;
				goto mkg1;
			}
			if(p == strrat) {
				g = mal(sizeof(Group));
				g->type = Grat;
				g->ref = s;
				if(f0)
					fprint(2, "%ld: rat shouldnt have info\n", lineno);
				goto mkg2;
			}
			if(p == strref) {
				g = mal(sizeof(Group));
				g->type = Gref;
				g->ref = s;
				if(f0)
					fprint(2, "%ld: ref shouldnt have info\n", lineno);
				goto mkg2;
			}
			if(p == strtext) {
				gettext(s);
				continue;
			}
			if(f0 == 0) {
				f = info(p, s->name);
				f0 = f;
			} else {
				f->link = info(p, s->name);
				f = f->link;
			}
			continue;

		mkg1:
			g = mal(sizeof(Group));
			g->type = t;
			g->path = apath;
			g->volume = avolume;
			avolume = 0;
			g->file = s->name;
			if(s->label == 0)
				s->label = g;

		mkg2:
			g->setup = setup;
			g->info = f0;
			if(r->group == 0)
				r->group = g;
			else
				r->egroup->link = g;
			r->egroup = g;
			f = 0;
			f0 = 0;
			setup = 0;
			continue;
		}
		q = strchr(p, '{');
		if(q) {
			parens++;
			g = parse();
			if(g == nil)		/* parse error? */
				continue;
			goto mkg2;
		}
		q = strchr(p, '}');
		if(q) {
			parens--;
			break;
		}
	}
	if(r->group == nil) {
		fprint(2, "%ld: syntax (empty set)\n", lineno);
//		error++;
		return nil;
	}
	r->setup = xsetup;
	return r;
}

Sym*
trim(char *p)
{
	char *q;
	int c;

	for(;;) {
		c = p[0];
		if(c != ' ' && c != '\t')
			break;
		p++;
	}
	q = strchr(p, 0);
	while(q > p) {
		c = q[-1];
		if(c != ' ' && c != '\t')
			break;
		q--;
	}
	if(q == p)
		return 0;
	*q = 0;
	return symdict(p);
}

Sym*
trim1(char *p)
{
	char *q;

	q = strchr(p, '~');
	if(q == 0)
		return trim(p);
	*q++ = 0;
	strncpy(buf, q, sizeof(buf));
	q = strchr(buf, 0);
	while(q > buf && (q[-1] == ' ' || q[-1] == '\t'))
		q--;
	while(*p == ' ' || *p == '\t')
		p++;
	q[0] = ',';
	q[1] = ' ';
	strcpy(q+2, p);
	return trim(buf);
}

char*
getstrinfo(Info *f, char *name)
{
	f = getinfo(f, name);
	if(f && f->value)
		return f->value;
	return "<none>";
}

Info*
getinfo(Info *f, char *name)
{

	for(; f; f=f->link)
		if(f->name == name)
			return f;
	return 0;
}

char*
xtitle(Cxlist *c, int flag)
{
	char buf[1000];
	char *p;
	Info *f, *o;
	Group* g;

	g = c->group;
	o = c->info;
	for(f=g->info; f; f=f->link)
		if(f == o) {
			if(f->name == strtitle)
				goto foundt;
			if(f->name == strartist)
				goto founda;
		}
	p = titstring(g);
	goto addterm;

foundt:
	buf[0] = buf[sizeof buf - 1] = 0;
	if(!flag)
		strncpy(buf, f->value, sizeof(buf));
	for(f=g->info; f; f=f->link)
		if(f->name == strartist) {
			if(buf[0])
				strncat(buf, "; ", sizeof(buf));
			strncat(buf, f->value, sizeof(buf));
		}
	for(f=g->info; f; f=f->link)
		if(f != o && f->name == strtitle) {
			if(buf[0])
				strncat(buf, "; ", sizeof(buf));
			strncat(buf, f->value, sizeof(buf));
		}
	p = buf;
	goto addterm;

founda:
	buf[0] = 0;
	if(!flag)
		strncpy(buf, f->value, sizeof(buf));
	for(f=g->info; f; f=f->link)
		if(f->name == strtitle) {
			if(buf[0])
				strncat(buf, "; ", sizeof(buf));
			strncat(buf, f->value, sizeof(buf));
		}
	for(f=g->info; f; f=f->link)
		if(f != o && f->name == strartist) {
			if(buf[0])
				strncat(buf, "; ", sizeof(buf));
			strncat(buf, f->value, sizeof(buf));
		}
	p = buf;
	goto addterm;

addterm:
	if(g->type == Ggroup)
		strncat(p, ">", sizeof(buf));
	else
	if(g->type == Gvirt)
		strncat(p, "*", sizeof(buf));
	assert(buf[sizeof buf - 1] == 0);
	return strdup(p);
}

char*
titstring(Group *g)
{
	Info *f;
	static char buf[1000];

	if(g == nil)
		return "gnil1";
	while(g->type == Gref || g->type == Grat) {
		g = g->group;
		if(g == nil)
			return "gnil2";
	}

	buf[0] = buf[sizeof buf - 1] = '\0';
	for(f=g->info; f; f=f->link)
		if(f->name == strtitle) {
			if(buf[0])
				strncat(buf, "; ", sizeof(buf));
			strncat(buf, f->value, sizeof(buf));
		}
	for(f=g->info; f; f=f->link)
		if(f->name == strartist) {
			if(buf[0])
				strncat(buf, "; ", sizeof(buf));
			strncat(buf, f->value, sizeof(buf));
		}
	assert(buf[sizeof buf - 1] == '\0');
	if(!buf[0]) {
		if(g->type == Ggroup)
			return titstring(g->group);
		snprint(buf, sizeof(buf), "noname/type=%d", g->type);
	}
	assert(buf[sizeof buf - 1] == '\0');
	return buf;
}

char*
artstring(Group *g)
{
	Info *f;
	static char buf[1000];

	buf[sizeof buf - 1] = '\0';
	while(g->type == Gref || g->type == Grat)
		g = g->group;

	buf[0] = 0;
	for(f=g->info; f; f=f->link)
		if(f->name == strartist) {
			if(buf[0])
				strncat(buf, "; ", sizeof(buf));
			strncat(buf, f->value, sizeof(buf));
		}
	for(f=g->info; f; f=f->link)
		if(f->name == strtitle) {
			if(buf[0])
				strncat(buf, "; ", sizeof(buf));
			strncat(buf, f->value, sizeof(buf));
		}
	if(!buf[0])
		snprint(buf, sizeof(buf), "noname/type=%d", g->type);
	assert(buf[sizeof buf - 1] == '\0');
	return buf;
}

void
gettext(Sym *s)
{
	char *p, *q;
	Info *i0, *in, *i;
	Group *g;
	Symlink *l;

	g = mal(sizeof(*g));
	g->link = textlist;
	textlist = g;
	g->type = Gtext;
	g->title = strtext;
	l = mal(sizeof(*l));
	l->sym = s;
	g->symlink = l;

	/*
	 * multiple text
	 */
	for(;;) {
		p = getline();
		if(p == 0)
			return;
		q = strstr(p, "text:");
		if(!q)
			break;
		q += 5;
		s = trim1(q);
		l = mal(sizeof(*l));
		l->sym = s;
		l->link = g->symlink;
		g->symlink = l;
	}

	/*
	 * title
	 */
	q = strstr(p, "title:");
	if(q) {
		q += 6;
		s = trim1(q);
		g->title = s->name;
		p = getline();
		if(p == 0) {
			fprint(2, "%ld: title after text\n", lineno);
			return;
		}
	}

	/*
	 * left quote {
	 */
	q = strchr(p, '{');
	if(!q) {
		fprint(2, "%ld: no { after text\n", lineno);
		error++;
	}

	/*
	 * all up to right quote }
	 */
	i0 = 0;
	in = 0;
	for(;;) {
		p = getline();
		if(p == 0) {
			fprint(2, "%ld: no } after {\n", lineno);
			error++;
			return;
		}
		q = strchr(p, '}');
		if(q)
			break;
		if(*p == '\t')
			p++;
		i = mal(sizeof(*i));
		i->value = strdict(p);
		if(in)
			in->link = i;
		else
			i0 = i;
		in = i;
	}
	g->info = i0;
}

char*	typename[] =
{
	[Ggroup]	= "Ggroup",
	[Gfile]		= "Gfile",
	[Gdupl]		= "Gdupl",
	[Gvirt]		= "Gvirt",
	[Grat]		= "Grat",
	[Gref]		= "Gref",
	[Glink]		= "Glink",
	[Gcut]		= "Gcut",
	[Gtext]		= "Gtext",
};

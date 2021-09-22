#include <audio.h>
#include <pool.h>

extern int parens;

/*
	bugs:
	sort shorter first
*/

void
main(int argc, char *argv[])
{
	int i;
	Group *g;
	char *ofile, *ifile;

//	mainmem->flags |= POOL_PARANOIA;
	strdot = strdict("./");
	strnull = strdict("");
	strartist = strdict("artist");
	strclass = strdict("class");
	strfile = strdict("file");
	strdupl = strdict("dupl");
	strpath = strdict("path");
	strrat = strdict("rat");
	strref = strdict("ref");
	strmusic = strdict("music");
	strtitle = strdict("title");
	strvirt = strdict("virt");
	strsetup = strdict("setup");
	strnumber = strdict("number");
	strsort = strdict("sort");
	strnosplit = strdict("nosplit");
	strnocut = strdict("nocut");
	strnoword = strdict("noword");
	strmark = strdict("mark");
	strrelative = strdict("relative");
	strvarious = strdict("various");
	strvolume = strdict("volume");
	strtext = strdict("text");

	apath = strnull;
	avolume = 0;

	searchinit();

	ofile = "rawmap";
	maxgroup = Maxgroup;
	ARGBEGIN {
	default:
		fprint(2, "usage: aprep [-dfprv] [-s splitsize] [-o ofile] ifile...\n");
		exits("usage");
	case 'o':	/* output file */
		ofile = ARGF();
		break;
	case 's':	/* split size */
		maxgroup = atoi(ARGF());
		break;
	case 'f':	/* turn virts to files */
		allfile++;
		break;
	case 'v':	/* dont add volume extension */
		volflag++;
		break;
	case 'd':
		dupflag++;
		break;
	case 'r':	/* kill no-ref's; normal use for cd's */
		refflag++;
		break;
	case 'p':	/* path */
		apath = strdict(ARGF());
		break;
	} ARGEND

	openw(ofile);
	gargc = argc;
	gargv = argv;

	if(argc == 0) {
		ifile = "map";
		gargc = 1;
		gargv = &ifile;
	}
	lineno = 0;
	root = parse();

	if(root == 0 || error != 0)
		exits("error");
	if (parens)
		fprint(2, "unbalanced braces; count %d\n", parens);
poolcheck(mainmem);
	print("patch1\n");
	patch1(root);
	patch1_5(root, 0);

	i = 0;
	for(g=root->group; g; g=g->link)
		if(g->type == Ggroup)
			i++;
	if(ntitle > i)
		i = ntitle;
	if(nartist > i)
		i = nartist;
	if(nmusic > i)
		i = nmusic;
	i += 500;
	cxlist = mal(i*sizeof(*cxlist));
	ecxlist = cxlist + i;

	print("patch2\n");
	patch2(root);

	print("patch3\n");
	patch3(root);

	print("patch4\n");
	patch4(root);

	print("patch5\n");
	patch5(root);

	print("text\n");
	gentext();

	print("cuts\n");
	gencut(root, 0, 0);
	unmark(root);

	print("title\n");
	gentitle(root);

	print("split\n");
	gensplit(root);

poolcheck(mainmem);
	for(pass=1; pass<=2; pass++) {
		print("pass %d\n", pass);
		offset = 0;
		for(i=0; i<4; i++)
			putint1("admp"[i]);
		for(i=0; i<10; i++)
			putint4(aux[i]);	/* future expansion */
		offsetnone = offset-2;
		putint2(1);			/* root count */
		putint4(root->offset1);
		putint4(root->offset2);
		for(i=0; i<6; i++)
			putint1("root>"[i]);
		expand(root);
		buildtitle();
		if(pass == 1) {
			buildword(strtitle);
			buildword(strartist);
			buildword(strmusic);
			buildclass(strclass);
			buildpath();
		}
		aux[0] = doindex();
	}
	print("size = %ld\n", offset);
	for(i=0; i<2048; i++)
		putint1(0);
	closew();
poolcheck(mainmem);
	exits(0);
}

void
gensplit(Group *r)
{
	Group *g;
	int n;

	switch(r->type) {
	default:
		print("unknown group type-1 %d\n", r->type);
		break;

	case Gref:
	case Grat:
	case Glink:
		if(r->group)
			gensplit(r->group);
		break;

	case Gtitle:
	case Gartist:
	case Gmusic:
	case Ggroup:
		if(r->setup & Swassplit)
			break;
		r->setup |= Swassplit;
		n = 0;
		for(g=r->group; g; g=g->link) {
			n++;
			gensplit(g);
		}
		splitgroup(r, n);
		break;

	case Gfile:
	case Gdupl:
	case Gvirt:
		break;

	}
}

void
gentitle(Group *r)
{
	Group *g;

	switch(r->type) {
	default:
		print("unknown group type-2 %d\n", r->type);
		break;

	case Gref:
	case Grat:
	case Glink:
//		if(r->group)
//			gentitle(r->group);
		break;

	case Gtitle:
	case Gartist:
	case Gmusic:
	case Ggroup:
		for(g=r->group; g; g=g->link) {
			gentitle(g);
			switch(g->type) {
			case Gtitle:
			case Gartist:
			case Gmusic:
				addtitle(g, r, 0);
				break;
			default:
				addtitle(g, r, g->ordinal);
			}
		}
		break;

	case Gfile:
	case Gdupl:
	case Gvirt:
		break;
	}
}

void
expand(Group *r)
{
	int n, c;
	char *p;
	Cut *u;
	Group *g, *rg;
	Info *f;

	if(pass == 1) {
		if(r->offset1)
			return;
		r->offset1 = offset;
	} else {
		if(r->setup & Soff2)
			return;
		r->setup |= Soff2;
		if(r->offset1 != offset) {
			print("expand phase error %ld %ld\n",
				r->offset1, offset);
			offset = r->offset1;
		}
	}

	rg = r->group;
	switch(r->type) {
	default:
		print("unknown group type-3 %d\n", r->type);
		putint2(0);
		break;

	case Gref:
	case Grat:
	case Glink:
		if(rg)
			expand(rg);
		break;

	case Ggroup:
	case Gtitle:
	case Gartist:
	case Gmusic:
		n = 0;
		for(g=rg; g; g=g->link)
			n++;
		r->count = n;
		putint2(r->count);
		for(g=rg; g; g=g->link)
			prgroup(g);
		for(g=rg; g; g=g->link)
			expand(g);
		gpath(r);
		break;

	case Gfile:
	case Gdupl:
	case Gvirt:
		putint2(r->count);
		n = 0;
		prpath(r);
		n++;
		for(f=r->info; f; f=f->link) {
			prinfo(f);
			n++;
		}
		if(pass == 1)
			sortcut(r);
		for(u=r->cut; u; u=u->link) {
			prcut(u);
			n++;
		}
		r->count = n;

		for(f=r->info; f; f=f->link)
			if(f->group)
				expand(f->group);
		gpath(r);
		switch(r->type) {
		case Gfile:
		case Gdupl:
			goff2(r);
		}
		break;

	case Gtext:
		n = 0;
		for(f=r->info; f; f=f->link)
			n++;
		r->count = n;
		putint2(r->count);
		for(f=r->info; f; f=f->link) {
			putint4(offsetnone);
			putint4(offsetnone);
			p = f->value;
			if(p != nil) {
				while(c = *p++) {
					if(c == '\t') {
						putint1(' ');
						putint1(' ');
						putint1(' ');
						putint1(' ');
					} else
						putint1(c);
				}
			}
			putint1(0);
		}
		break;


	}
	if(r->offset2 == 0)
		r->offset2 = offsetnone;
}

void
goff2(Group *g)
{
	char *p;
	int c;

	g->off2str = offset;
	putint4(g->offset1);
	putint4(g->offset2);

	p = titstring(g);
	while(c = *p++)
		putint1(c);
	putint1(0);

	p = g->path;
	if(p)
		while(c = *p++)
			putint1(c);
	p = g->file;
	if(p)
		while(c = *p++)
			putint1(c);
	p = g->volume;
	if(p) {
		putint1(' ');
		while(c = *p++)
			putint1(c);
	}
	putint1(0);
}

void
gpath(Group *r)
{
	int n;
	Cxlist *c;

	listp = cxlist;
	genpath(r);
	unmark(r);
	n = listp - cxlist;
	r->offset2 = offsetnone;
	if(n > 0) {
		r->offset2 = offset;
		putint2(n);
		for(c=cxlist; c<listp; c++)
			putint4(c->group->off2str);
	}
}

Group*
incr(Group *g)
{
	Group *t;

	if(g) {
		t = g->egroup;
		if(t->setup & Sgnext)
			return t;
	}
	return 0;
}

void
addspan(Group *r)
{
	Group *u, *t, *g5, *g25, *g125, *g625, *g3125;
	int i;

	g5 = r;
	for(i=0; g5 && i<5; i++)
		g5 = incr(g5);

	g25 = g5;
	for(i=0; g25 && i<25-5; i++)
		g25 = incr(g25);

	g125 = g25;
	for(i=0; g125 && i<125-25; i++)
		g125 = incr(g125);

	g625 = g125;
	for(i=0; g625 && i<625-125; i++)
		g625 = incr(g625);

	g3125 = g625;
	for(i=0; g3125 && i<3125-625; i++)
		g3125 = incr(g3125);

	if (i >= 3125-625)
		fprint(2, "addspan: %s: i >= 3125-625\n", r->title);

	while(g5) {
		t = incr(r);
		u = mal(sizeof(Group));
		u->title = ">> 5";
		u->type = Glink;
		u->group = g5;
		r->egroup->link = u;
		r->egroup = u;
		if(g25) {
			u = mal(sizeof(Group));
			u->title = ">> 25";
			u->type = Glink;
			u->group = g25;
			r->egroup->link = u;
			r->egroup = u;
			if(g125) {
				u = mal(sizeof(Group));
				u->title = ">> 125";
				u->type = Glink;
				u->group = g125;
				r->egroup->link = u;
				r->egroup = u;
				if(g625) {
					u = mal(sizeof(Group));
					u->title = ">> 625";
					u->type = Glink;
					u->group = g625;
					r->egroup->link = u;
					r->egroup = u;
					if(g3125) {
						u = mal(sizeof(Group));
						u->title = ">> 3125";
						u->type = Glink;
						u->group = g3125;
						r->egroup->link = u;
						r->egroup = u;
						g3125 = incr(g3125);
					}
					g625 = incr(g625);
				}
				g125 = incr(g125);
			}
			g25 = incr(g25);
		}
		g5 = incr(g5);
		r = t;
	}
}

void
splitgroup(Group *r, int n)
{
	int i;
	Group *g, *u, *t, *r0;

	if(maxgroup == 0 || n <= maxgroup)
		return;

	i = 1;
	r0 = r;
	for(g=r->group; g; g=g->link) {
		i++;
		if(i < maxgroup)
			continue;
		if(g->link == 0 || g->link->link == 0)
			break;

		u = mal(sizeof(Group));
		u->title = ">>>";
		u->type = r->type;
		u->setup |= Sgnext;

		t = mal(sizeof(Group));
		t->title = "<<<";
		t->type = Glink;

		t->link = g->link;
		g->link = u;
		u->group = t;
		u->egroup = r->egroup;
		r->egroup = u;
		t->group = r;
		r = u;
		g = t;
		i = 2;
	}
	addspan(r0);
}

void
prcut(Cut *u)
{
	Group *r, *g;
	char buf[100];
	int c;
	long n, m1, m5, m25;
	char *p;

	r = u->parent;
	if(pass == 2 && r->setup & Swassplit) {
		/* adjust cut backpointer to point at
		 * correct page of split group */
		m1 = maxgroup-2;
		m5 = m1 * 5;
		m25 = m1 * 25;
		n = u->cut - 1;
		while(n > m1) {
			if(r->type == Ggroup)
				for(g=r->group; g; g=g->link) {
					if(g->setup & Sgnext) {
						r = g;
						break;
					}
			}
			if(n > m25 &&
			   r->link != 0 &&
			   r->link->link != 0 &&
			   r->link->link->type == Glink) {
				r = r->link->link->group;
				n -= m25;
			} else
			if(n > m5 &&
			   r->link != 0 &&
			   r->link->type == Glink) {
				r = r->link->group;
				n -= m5;
			} else
				n -= m1;
		}
	}
	putint4(r->offset1);
	putint4(r->offset2);
	buf[sizeof(buf)-1] = 0;
	snprint(buf, sizeof(buf)-1, "%-16s %d", u->title, u->cut);
	assert(buf[sizeof(buf)-1] == 0);
	for(p=buf; c=*p; p++)
		putint1(c);
	putint1(0);
}

void
prpath(Group *g)
{
	int c;
	char *p;

	putint4(g->offset1);
	putint4(g->offset2);
	for(p="path             "; c=*p; p++)
		putint1(c);
	if(p=g->path) {
		for(; c=*p; p++)
			putint1(c);
	}
	if(p=g->file)
		for(; c=*p; p++)
			putint1(c);
	if(p=g->volume) {
		putint1(' ');
		for(; c=*p; p++)
			putint1(c);
	}
	putint1(0);
}

void
prinfo(Info *f)
{
	int c;
	char *p;
	Group *g;

	g = f->group;
	if(g) {
		putint4(g->offset1);
		putint4(g->offset2);
	} else {
		putint4(offsetnone);
		putint4(offsetnone);
	}
	snprint(buf, sizeof(buf), "%-16s %s", f->name, f->value);
	for(p=buf; c=*p; p++)
		putint1(c);
	putint1(0);
}

void
prgroup(Group *g)
{
	int c;
	char *p;

	p = g->title;

loop:
	if(g == 0) {
		putint4(offsetnone);
		putint4(offsetnone);
		putint1(0);
		return;
	}
	switch(g->type) {
	case Gref:
	case Grat:
	case Glink:
		g = g->group;
		goto loop;
	}

	putint4(g->offset1);
	putint4(g->offset2);
	while(c = *p++)
		putint1(c);
	putint1(0);
}

void
addtitle(Group *g, Group *r, int o)
{
	char buf[1000];
	Group *g0;

	if(g->title)
		return;
	buf[0] = buf[sizeof buf - 1] = 0;
	if(o != 0 && (r->setup & Snumber))
		snprint(buf, sizeof(buf), "%d. ", o);
	g0 = g;

loop:
	switch(g->type) {
	default:
		print("unknown %d %s\n", g->type, titstring(g));
		strncat(buf, "unknown", sizeof(buf));
		break;
	case Grat:
		strncat(buf, g->ref->name, sizeof(buf));
		break;
	case Gref:
		if(g = g->group)
			goto loop;
		break;
	case Gtitle:
		strncat(buf, "title>", sizeof(buf));
		break;
	case Gmusic:
		strncat(buf, "music>", sizeof(buf));
		break;
	case Gartist:
		strncat(buf, "artist>", sizeof(buf));
		break;
	case Ggroup:
		strncat(buf, titstring(g), sizeof(buf));
		strncat(buf, ">", sizeof(buf));
		break;
	case Gvirt:
		strncat(buf, titstring(g), sizeof(buf));
		strncat(buf, "*", sizeof(buf));
		break;
	case Gfile:
	case Gdupl:
		strncat(buf, titstring(g), sizeof(buf));
		break;
	}
	assert(buf[sizeof buf - 1] == 0);
	g0->title = strdup(buf);
}

void
sortcut(Group *g)
{
	Cut *u;
	Cxlist *c;

	listp = cxlist;
	for(u=g->cut; u; u=u->link) {
		u->title = getstrinfo(u->parent->info, strtitle);
		listp->cut = u;
		listp++;
	}
//	usort(cutcmp, cutcmp1);
	qsort(cxlist, listp-cxlist, sizeof(Cxlist), cutcmp1);
	g->cut = 0;
	for(c=cxlist; c<listp; c++) {
		u = c->cut;
		u->link = g->cut;
		g->cut = u;
	}
}

void
gencut(Group *g, Group *p, int cut)
{
	Cut *c;

	if(p) {
		c = mal(sizeof(Cut));
		c->parent = p;
		c->cut = cut;
		c->link = g->cut;
		g->cut = c;
	}

	if(g->setup & Srecur)
		return;
	g->setup |= Srecur;

	switch(g->type) {
	case Grat:
	case Gref:
		if(g->group)
			gencut(g->group, p, cut);
		break;
	case Ggroup:
		if(g->setup & Snocut)
			break;
		if(getinfo(g->info, strtitle) == 0) {
			for(g=g->group; g; g=g->link)
				gencut(g, p, cut);
			break;
		}

		p = g;
		for(g=g->group; g; g=g->link)
			gencut(g, p, g->ordinal);
		break;
	}
}

void
genpath(Group *r)
{
	Group *g, *rg;

	if(r->setup & Srecur)
		return;
	r->setup |= Srecur;
	rg = r->group;
	switch(r->type) {
	case Grat:
	case Gref:
		if(rg)
			genpath(rg);
		break;

	case Gfile:
	case Gdupl:
		if(listp >= ecxlist) {
			print("oops: genpath\n");
			break;
		}
		listp->group = r;
		listp++;
		break;

	case Ggroup:
		for(g=rg; g; g=g->link)
			if(g->type != Gtitle && g->type != Gartist)
				genpath(g);
		for(g=rg; g; g=g->link)
			if(g->type == Gtitle || g->type == Gartist)
				genpath(g);
		break;
	}
}

void
genlist(Group *g, char *str)
{
	Info *f;

	if(g->setup & Srecur)
		return;
	g->setup |= Srecur;

	if(g->type == Gref || g->type == Grat) {
		if(g->group)
			genlist(g->group, str);
		return;
	}
	for(f=g->info; f; f=f->link) {
		if(f->name == str) {
			if(listp >= ecxlist) {
				print("oops: genlist\n");
				continue;
			}
			listp->group = g;
			listp->info = f;
			listp++;
		}
	}
	if(g->type == Ggroup)
		for(g=g->group; g; g=g->link)
			genlist(g, str);
}

void
unmark(Group *g)
{
	if((g->setup & Srecur) == 0)
		return;
	g->setup &= ~Srecur;

	switch(g->type) {
	case Grat:
	case Gref:
		if(g->group)
			unmark(g->group);
		break;
	case Ggroup:
		for(g=g->group; g; g=g->link)
			unmark(g);
		break;
	}
}

Group*
igroup(Cxlist *x1, Cxlist *x2, int type, int flag)
{
	Group *g1, *g2;
	Cxlist *ox;
	int n;

	n = x2 - x1;
	if(n == 0) {
		print("n = 0\n");
		return 0;
	}
	g1 = mal(sizeof(Group));
	g1->type = type;
	n = 0;
	ox = 0;
	while(x1 < x2) {
		if(x1->group->type != Ggroup) {
			if(ox == 0 || cxcmp1(ox, x1) != 0) {
				g2 = mal(sizeof(Group));
				g2->type = Gref;
				g2->group = x1->group;
				g2->title = xtitle(x1, flag);
			
				if(g1->egroup == 0)
					g1->group = g2;
				else
					g1->egroup->link = g2;
				g1->egroup = g2;
				n++;
			}
			ox = x1;
		}
		x1++;
	}
	g1->setup |= Swassplit;
	splitgroup(g1, n);
	return g1;
}

/*
 * patch refs to defines put in ordinals
 */
void
patch1(Group *r)
{
	Group *g, *g0, *g1;
	int n, any;
	Sym *s;

	switch(r->type) {
	case Ggroup:
		/* rebuild group with non-null subgroups */
		g0 = nil;
		g1 = nil;
		n = 0;
		any = 0;
		for(g=r->group; g; g=g->link) {
			patch1(g);
			switch(g->type) {
			case Gtitle:
			case Gartist:
			case Gmusic:
			case Grat:
				break;

			case Ggroup:
			case Gref:
				n++;
				if(g->group == nil)
					continue;
				any = 1;
				g->ordinal = n;
				break;

			default:
				n++;
				any = 1;
				g->ordinal = n;
				break;
			}
			if(g0 == nil)
				g0 = g;
			else
				g1->link = g;
			g1 = g;
		}
		if(any == 0) {
			r->group = nil;
			break;
		}
		r->group = g0;
		g1->link = nil;
		break;

	case Gref:
		s = r->ref;
		if(s == 0) {
			if(!refflag)
				print("no ref\n");
			break;
		}
		if(s->label == 0) {
			if(!refflag)
				print("no ref %s\n", s->name);
			break;
		}
		r->group = s->label;
		break;
	}
}

/*
 * patch refs to defines put in ordinals
 */
void
patch1_5(Group *r, Info *d)
{
	Group *g;
	Info *f, *f1;

	switch(r->type) {
	case Ggroup:
		for(g=r->group; g; g=g->link)
			patch1_5(g, d);
		if(d != nil)
			break;
		for(f=r->info; f; f=f->link) {
			if(f->name == strmusic)
				for(g=r->group; g; g=g->link)
					patch1_5(g, f);
			if(f->name == strartist && f->value != strvarious)
				for(g=r->group; g; g=g->link)
					patch1_5(g, f);
		}
		break;

	case Gfile:
	case Gdupl:
	case Gvirt:
		if(d == nil)
			break;
		for(f=r->info; f; f=f->link)
			if(f->name == d->name && f->value == d->value)
				return;
		if(d->name == strartist)
			nartist++;
		if(d->name == strmusic)
			nmusic++;
		f1 = mal(sizeof(Info));
		f1->name = d->name;
		f1->value = d->value;
		if(r->info == 0) {
			r->info = f1;
			break;
		}
		for(f=r->info; f->link; f=f->link)
			if(f->name == d->name && f->link->name != d->name)
				break;
		f1->link = f->link;
		f->link = f1;
		break;
	}
}

void
patch2(Group *r)
{
	Group *g;

	if(r->type != Ggroup)
		return;
	for(g=r->group; g; g=g->link)
		patch2(g);
	if(r->setup & Ssort)
		sortgroup(r);
	if(r->setup & Smusic) {
		listp = cxlist;
		genlist(r, strmusic);
		unmark(r);
		if(listp-cxlist) {
			usort(cxcmp, cxcmp1);
//			qsort(cxlist, listp-cxlist, sizeof(Cxlist), cxcmp);
			g = igroup(cxlist, listp, Gmusic, 0);
			if(r->group)
				g->link = r->group;
			else
				r->egroup = g;
			r->group = g;
		}
	}
	if(r->setup & Sartist) {
		listp = cxlist;
		genlist(r, strartist);
		unmark(r);
		if(listp-cxlist) {
			usort(cxcmp, cxcmp1);
//			qsort(cxlist, listp-cxlist, sizeof(Cxlist), cxcmp);
			g = igroup(cxlist, listp, Gartist, 0);
			if(r->group)
				g->link = r->group;
			else
				r->egroup = g;
			r->group = g;
		}
	}
	if(r->setup & Stitle) {
		listp = cxlist;
		genlist(r, strtitle);
		unmark(r);
		if(listp-cxlist) {
			usort(cxcmp, cxcmp1);
//			qsort(cxlist, listp-cxlist, sizeof(Cxlist), cxcmp);
			g = igroup(cxlist, listp, Gtitle, 0);
			if(r->group)
				g->link = r->group;
			else
				r->egroup = g;
			r->group = g;
		}
	}
}

void
patch3(Group *r)
{
	Group *g, *g1;
	Info *f;
	Cxlist *x1, *x2;
	Sym *s;

	for(g = r->group; g; g = g->link) {
		switch(g->type) {
		case Ggroup:
			patch3(g);
			break;

		case Gfile:
		case Gdupl:
		case Gvirt:
			for(f = g->info; f; f = f->link) {
				if(f->group)
					continue;
				if(f->name != strartist &&
				   f->name != strtitle &&
				   f->name != strmusic)
					continue;
				listp = cxlist;
				genlist(root, f->name);
				unmark(root);
//				usort(cxcmp, cxcmp1);
				qsort(cxlist, listp-cxlist, sizeof(Cxlist), cxcmp2);
				x2 = cxlist;
				for(x1 = cxlist; x1 < listp; x1++) {
					if(x1->info->value == x2->info->value)
						continue;
					g1 = igroup(x2, x1, Ggroup, 1);
					if(f->name == strartist) {
						s = symdict(x2->info->value);
						if(s->label)
							print("rat redeclared: "
								"%s in %s\n",
								s->name,
								titstring(r));
						s->label = g1;
					}
					for(; x2 < x1; x2++)
						x2->info->group = g1;
				}
				g1 = igroup(x2, x1, Ggroup, 1);
				if(f->name == strartist) {
					s = symdict(x2->info->value);
					if(s->label)
						print("rat redeclared: %s in %s\n",
							s->name, titstring(r));
					s->label = g1;
				}
				for(; x2 < x1; x2++)
					x2->info->group = g1;
			}
			break;
		}
	}
}

void
patch4(Group *r)
{
	Group *g;
	Sym *s;

	switch(r->type) {
	case Ggroup:
		for(g=r->group; g; g=g->link)
			patch4(g);
		break;
	case Grat:
		s = r->ref;
		if(s == 0) {
			if(!refflag)
				print("no rat\n");
			break;
		}
		if(s->label == 0) {
			if(!refflag)
				print("no rat %s\n", s->name);
			break;
		}
		r->group = s->label;
		break;
	}
}

void
dorel(Group *g, Group *lg, Group *pg, Group *rg)
{
if (0) {
	Group *g0, *g1, *g2;

	if(g->type != Ggroup)
		return;

	g0 = mal(sizeof(Group));
	g0->type = Gref;
	g0->group = lg;

	g1 = mal(sizeof(Group));
	g1->type = Gref;
	g1->group = pg;

	g2 = mal(sizeof(Group));
	g2->type = Gref;
	g2->group = rg;

	g2->link = g->group;
	g1->link = g2;
	g0->link = g1;
	g->group = g0;
}
	USED(g);
}

void
patch5(Group *r)
{
	Group *g, *og, *ng;


	switch(r->type) {
	case Ggroup:
		og = nil;
		for(g=r->group; g; g=g->link) {
			patch5(g);
			og = g;
		}
		if(r->setup & Srelative) {
			for(g=r->group; g; g=g->link) {
				ng = g->link;
				if(ng == nil)
					ng = r->group;
				dorel(g, og, r, ng);
				og = g;
			}
		}
		break;
	}
}

void
gentext(void)
{
	Group *g, *g1;
	Symlink *l;
	Info *i, *i0;

	for(g=textlist; g; g=g->link) {
		for(l=g->symlink; l; l=l->link)
			if(g1 = l->sym->label) {
				i0 = mal(sizeof(*i0));
				i0->name = strtext;
				i0->value = g->title;
				i0->group = g;
				i = g1->info;
				if(i == 0) {
					g1->info = i;
					continue;
				}
				for(; i->link; i=i->link)
					;
				i->link = i0;
			}
	}
}

void
sortgroup(Group *r)
{
	Cxlist *x;
	Group *g;
	Info *f;

	if(r->type != Ggroup) {
		print("sort non-group: %s\n", titstring(r));
		return;
	}

	/*
	 * sort the titles just under the root
	 */
	x = cxlist;
	for(g = r->group; g; g = g->link) {
		if((f = getinfo(g->info, strtitle)) != nil ||
		    (f = getinfo(g->info, strartist)) != nil) {
			x->group = g;
			x->info = f;
			x++;
		} else
			print("sort and no tit/art: %s\n", titstring(r));
	}
	listp = x;
	usort(cxcmp, cxcmp1);
//	qsort(cxlist, listp-cxlist, sizeof(Cxlist), cxcmp);

	r->group = 0;
	r->egroup = 0;
	for(x=cxlist; x<listp; x++) {
		if(r->group == 0)
			r->group = x->group;
		else
			r->egroup->link = x->group;
		r->egroup = x->group;
	}
	if(r->group == 0) {
		print("nothing after sort: %s\n", titstring(r));
	} else
		r->egroup->link = 0;
}

int
gtype(Cxlist *x)
{
	Group *g;

	g = x->group;
	if(g == 0)
		return 0;
	if(g->type == Gref) {
		g = g->group;
		if(g == 0)
			return 0;
	}
	return g->type;
}

void
usort(int (*cmp)(void*, void*), int (*cmp1)(void*, void*))
{
	Cxlist *x, *y;

	qsort(cxlist, listp-cxlist, sizeof(Cxlist), cmp1);
	y = cxlist;
	if(y >= listp)
		return;

	for(x=y+1; x<listp; x++) {
		if(cmp(y, x) == 0) {
			if(gtype(x) == Gdupl)
				continue;
			if(gtype(y) == Gdupl) {
				*y = *x;
				continue;
			}
			if(dupflag && strchr(titstring(x->group), ';'))
				print("dup: %s\n", titstring(x->group));
		}
		y++;
		*y = *x;
	}
	listp = y+1;
}

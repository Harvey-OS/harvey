#include "audio.h"

struct
{
	char*	word;
	char*	word1;
	char*	word2;
	Sym*	sym1;
	Sym*	sym2;
} comp[] =
{
/*
	"im",		"i",		"am",		0,0,
	"dont",		"do",		"not",		0,0,
	"aint",		"am",		"not",		0,0,
	"arent",	"are",		"not",		0,0,
	"goin",		"going",	0,		0,0,
*/
	"XXXXXXX",	"xx",		0,		0,0,
};

char*	freqwd[32] =
{
	"V",	// 39637
	"s*",	// 28246
	"t*",	// 27320
	"b*",	// 26847
	"m*",	// 26525
	"c*",	// 23798
	"1*",	// 22681
	"a*",	// 21516
	"19*",	// 20031
	"l*",	// 19878
	"p*",	// 18189
	"d*",	// 17768
	"r*",	// 16891
	"i*",	// 16594
	"w*",	// 15988
	"g*",	// 14113
	"h*",	// 13949
	"f*",	// 13388
	"o*",	// 13298
	"j*",	// 12029
	"th*",	// 11430
	"y*",	// 9394
	"yo*",	// 8484
	"e*",	// 8466
	"you*",	// 8268
	"lo*",	// 8153
	"the*",	// 7967
	"n*",	// 7917
	"ma*",	// 6773
	"to*",	// 6652
	"the",	// 6636
	"196*",	// 6157
};

void
searchinit(void)
{
	int i;

	for(i=0; i<nelem(comp); i++) {
		comp[i].word = strdict(comp[i].word);
		if(comp[i].word1)
			comp[i].sym1 = symdict(comp[i].word1);
		if(comp[i].word2)
			comp[i].sym2 = symdict(comp[i].word2);
	}
}

void
addword1(Sym *s, Group *g, int star)
{
	Cxlist *x;
	char word[Maxstar+3];
	int i, c;

	if(s == 0)
		return;

	x = mal(sizeof(*x));
	x->group = g;
	x->link = s->index;
	s->index = x;

	if(star) {
		for(i=0; i<Maxstar; i++) {
			c = s->name[i];
			if(c == 0)
				break;
			word[i] = c;
			if(i < Minstar-1)
				continue;

			word[i+1] = '*';
			word[i+2] = 0;
			addword1(symdict(word), g, 0);
		}
	}
}

void
addword(char *w, Group *g)
{
	int i;
	Sym *s;

	if(w == 0)
		return;
	s = symdict(w);
	w = s->name;
	for(i=0; i<nelem(comp); i++)
		if(comp[i].word == w) {
			addword1(comp[i].sym1, g, 1);
			addword1(comp[i].sym2, g, 1);
			return;
		}
	addword1(s, g, 1);
}

int
getrune(char **sp)
{
	int c;
	Rune rune;

	c = **(uchar **)sp;
	if (c < Runeself) {
		++*sp;
		return c;
	} else {
		*sp += chartorune(&rune, *sp);
		return rune;
	}
}

int
transrune(int c, char **wp)
{
	Runeinf ri;
	int i;

	runeinf(c, &ri);
	switch(ri.class) {
	default:
		return 0;
	case Alpha:
		if(ri.lower != c)
			runeinf(ri.lower, &ri);
	case Numer:
		for(i=0; i<nelem(ri.latin); i++) {
			if(ri.latin[i] == 0)
				break;
			*(*wp)++ = ri.latin[i];
		}
	}
	return 1;
}

void
wprep(char *s, Group *g)
{
	char word[Strsize], *w;
	int c;

loop0:
	w = word;

loop1:	/* initial character */
	c = getrune(&s);
	switch(c) {
	case '\'':
		goto loop1;
	case 0:
		return;
	}
	if(!transrune(c, &w))
		goto loop1;

loop2:	/* subsequent */
	c = getrune(&s);
	switch(c) {
	case '\'':
		goto loop2;
	case '/':
	case ' ':
	case '\t':
	case 0:
		*w = 0;
		addword(word, g);
		if(c == 0)
			return;
		goto loop0;
	}
	transrune(c, &w);
	goto loop2;
}

void
gentit(Group *g)
{
	if(g->setup & Srecur)
		return;
	g->setup |= Srecur;

	if(g->type == Gref || g->type == Grat) {
		if(g->group)
			gentit(g->group);
		return;
	}
	if(g->title && g->info) {
		if(listp >= ecxlist) {
			print("oops: genlist\n");
			abort();
		}
		listp->group = g;
		listp++;
	}
	if(g->type == Ggroup)
		for(g=g->group; g; g=g->link)
			gentit(g);
}

void
buildtitle(void)
{
	Cxlist *x;
	char *p;
	Group *g;
	int c;

	listp = cxlist;
	gentit(root);
	unmark(root);
	qsort(cxlist, listp-cxlist, sizeof(Cxlist), titcmp);
	for(x=cxlist; x<listp; x++) {
		g = x->group;
		if(g->pair && g->pair != offset)
			print("buildtitle phase error %ld %ld\n", g->pair, offset);
		g->pair = offset;
		p = g->title;
		putint4(g->offset1);
		putint4(g->offset2);
		while(c = *p++)
			putint1(c);
		putint1(0);
	}
	if(pass == 1)
		print("%lld titles\n", listp-cxlist);
}

void
buildword(char *s)
{
	Cxlist *x;

	print("gen %s\n", s);
	listp = cxlist;
	genlist(root, s);
	unmark(root);

	for(x=cxlist; x<listp; x++)
		if(x->group->offset1)
		if((x->group->setup & Snoword) == 0)
			wprep(x->info->value, x->group);
}

void
buildpath(void)
{
	Cxlist *x;
	char buf[200], *p;

	print("gen path\n");
	listp = cxlist;
	genpath(root);
	unmark(root);
	for(x=cxlist; x<listp; x++)
		if(x->group->offset1 &&
		   x->group->path &&
		   x->group->file) {
			strncpy(buf, x->group->path, sizeof(buf));
			strncat(buf, x->group->file, sizeof(buf));
			p = buf;
			if(memcmp(buf, "/lib/audio/", 11) == 0)
				p += 11;
			addword(p, x->group);
		}
}

void
buildclass(char *s)
{
	Cxlist *x;
	char w[2];
	Sym *sy;
	char *p;
	int c;

	print("gen %s\n", s);
	listp = cxlist;
	genlist(root, s);
	unmark(root);

	w[1] = 0;
	for(x=cxlist; x<listp; x++)
		if(x->group->offset1)
			for(p = x->info->value; c = *p & 0xff ; p++) {
				sy = classes[c];
				if(sy == 0) {
					w[0] = c;
					sy = symdict(w);
					classes[c] = sy;
				}
				addword1(sy, x->group, 0);
			}
}


int
xlcmp(void *v1, void *v2)
{
	Word *w1, *w2;
	Group *g1, *g2;

	w1 = v1;
	w2 = v2;
	g1 = w1->index->group;
	g2 = w2->index->group;
	if(g1 > g2)
		return +1;
	if(g1 < g2)
		return -1;
	return 0;
}

int
wlcmp(void *v1, void *v2)
{
	Word *w1, *w2;
	int n;

	w1 = v1;
	w2 = v2;
	n = w1->offset - w2->offset;	// srchash
	if(n)
		return n;
	return strcmp(w1->sym->name, w2->sym->name);
}

long
doindex(void)
{
	long i, j, h, pair, o0, maxwords;
	int c, oc;
	char *p;
	Sym *s;
	Cxlist *x;

	maxwords = 0;
	o0 = offset;
	if(nwordlist == 0) {
		print("build words\n");
		i = 0;
		j = 0;
		for(h=0; h<nelem(hash); h++)
			for(s=hash[h]; s; s=s->link) {
				j = 0;
				for(x=s->index; x; x=x->link)
					j++;
				if(j > 0)
					i++;
				if(j > maxwords)
					maxwords = j;
			}
		if(i > j)
			j = i;
		wordlist = mal(j*sizeof(*wordlist));
		nwordlist = i;

		for(h=0; h<nelem(hash); h++)
			for(s=hash[h]; s; s=s->link) {
				j = 0;
				for(x=s->index; x; x=x->link) {
					wordlist[j].index = x;
					j++;
				}
				qsort(wordlist, j, sizeof(*wordlist), xlcmp);
			}

		i = 0;
		for(h=0; h<nelem(hash); h++)
			for(s=hash[h]; s; s=s->link)
				if(s->index) {
					s->index = ulsort(s->index);
					wordlist[i].sym = s;
					/* just for sort */
					wordlist[i].offset = srchash(s->name);
					i++;
				}
		qsort(wordlist, nwordlist, sizeof(*wordlist), wlcmp);
	}
	for(i=0; i<nwordlist; i++) {
		wordlist[i].offset = offset;
		p = wordlist[i].sym->name;
		while(c = *p++)
			putint1(c);
		putint1(0);
		j = 0;
		for(x=wordlist[i].sym->index; x; x=x->link)
			j++;
		putint2(j);
		pair = 0;
		for(x=wordlist[i].sym->index; x; x=x->link) {
			if(pair >= x->group->pair)
				print("search: not sorted %ld %ld\n",
					pair, x->group->pair);
			pair = x->group->pair;
			if(pair == 0)
				print("search: no pair\n");
			putint4(pair);
		}
	}

	h = offset;
	putint4(nwordlist);
	for(i=0; i<nwordlist; i++)
		putint4(wordlist[i].offset);

	oc = 0;
	j = 0;
	for(i=0; i<nwordlist; i++) {
		c = srchash(wordlist[i].sym->name);
		if(c > oc) {
			putint4(j);
			putint4(i-j);
			j = i;
			for(oc++; oc<c; oc++) {
				putint4(0);
				putint4(0);
			}
		}
	}
	putint4(j);
	putint4(nwordlist-j);
	for(oc++; oc<256; oc++) {
		putint4(0);
		putint4(0);
	}
	if(pass == 1)
		print("%ld words; %ld maxwords; %ld bytes\n", nwordlist, maxwords, offset-o0);
	return h;
}

Cxlist*
ulsort(Cxlist *l)
{
	Cxlist *l1, *l2, *le;

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
	l1 = ulsort(l);
	l2 = ulsort(l2);

	/* set up lead element */
	if(l2->group->pair > l1->group->pair) {
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
		if(l2->group->pair > l1->group->pair) {
			if(l1->group != le->group) {
				le->link = l1;
				le = l1;
			}
			l1 = l1->link;
		} else {
			if(l2->group != le->group) {
				le->link = l2;
				le = l2;
			}
			l2 = l2->link;
		}
	}

only1:
	l2 = l1;

only2:
	while(l2) {
		if(l2->group != le->group) {
			le->link = l2;
			le = l2;
		}
		l2 = l2->link;
	}
	le->link = 0;
	return l;
}

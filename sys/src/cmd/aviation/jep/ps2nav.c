#include	<u.h>
#include	<libc.h>
#include	<bio.h>

enum
{
	Fnear	= 1<<0,
	Fmorse	= 1<<1,
	Flatlng	= 1<<2,
	Ftvor	= 1<<3,
	Fdvor	= 1<<4,
	Fvfreq	= 1<<5,		// vor frequency 1##.#
	Fnfreq	= 1<<6,		// ndb frequency [123]##
	Fdfreq	= 1<<7,		// dme frequency (1##.#)
};

typedef	struct	Txt	Txt;
struct	Txt
{
	char*	txt;
	int	x;
	int	y;
	float	ps;
	ulong	flag;
	Txt*	near;
	Txt*	link;
};
typedef	struct	Page	Page;
struct	Page
{
	Txt*	page;
	Page*	link;
};

Page*	root;

void	loadtext(char*);
void	process(Page*);
int	classify(char*);
double	getlat(char*);
char*	ditdah(char*);

void
main(int argc, char *argv[])
{
	int i;
	Page *g;

	ARGBEGIN {
	default:
		fprint(2, "usage: a.out\n");
		exits(0);
	} ARGEND

	for(i=0; i<argc; i++)
		loadtext(argv[i]);
	if(argc <= 0)
		loadtext("/tmp/jepout1.ps");
	for(g=root; g!=nil; g=g->link)
		process(g);
	exits(0);
}

void
loadtext(char *file)
{
	Biobuf *bin;
	char *l, *p;
	Txt *t;
	Page *g;
	float size;
	char *fields[10];

	bin = Bopen(file, OREAD);
	if(bin == nil) {
		fprint(2, "cant open %s: %r\n", file);
		return;
	}

	g = malloc(sizeof(*g));
	memset(g, 0, sizeof(*g));
	g->link = root;
	root = g;

	size = 60;
	for(;;) {
		l = Brdline(bin, '\n');
		if(l == nil)
			break;
		l[Blinelen(bin)-1] = 0;
		if(l[0] != '(') {
			if(l[0] == '/' && l[1] == 'F' && l[5] == 'f')
				size = atof(l+14);
			continue;
		}
		p = strchr(l, ')');
		if(p == nil) {
			print("no right paren %s\n", l);
			continue;
		}
		if(tokenize(p+1, fields, nelem(fields)) < 4) {
			print("no right paren %s\n", l);
			continue;
		}

		t = malloc(sizeof(*t));
		memset(t, 0, sizeof(*t));
		t->link = root->page;
		root->page = t;
		
		*p = 0;
		t->txt = strdup(l+1);
		t->x = atoi(fields[1]);
		t->y = atoi(fields[2]);
		t->ps = size;
	}
}

int
near(Txt *n, Txt *t)
{
	int dx, dy;
	int xl, xh, yl, yh;

	xl = -3.33 * n->ps;
	xh = +3.33 * n->ps;
	yl = -1.67 * n->ps;
	yh = +.833 * n->ps;

	dx = n->x - t->x;
	dy = n->y - t->y;

	if(dx < xl || dx > xh)
		return 0;
	if(dy < yl || dy > yh)
		return 0;
//	if(dx < -200 || dx > 200)
//		return 0;
//	if(dy < -100 || dy > 50)
//		return 0;
	return 1;
}

void
addnear(Page *g, Txt *n)
{
	Txt *t;

	for(t=g->page; t!=nil; t=t->link) {
		if(t == n || (t->flag & Fnear))
			continue;
		if(near(n, t)) {
			t->flag |= Fnear;
			t->near = n->near;
			n->near = t;
		}
	}
}

void
process(Page *g)
{
	Txt *t, *u;
	ulong mask, mask1;
	char *dd;

	for(t=g->page; t!=nil; t=t->link)
		t->flag = classify(t->txt);
	for(t=g->page; t!=nil; t=t->link) {
		if(t->flag == Fmorse) {
			addnear(g, t);
			for(u=t->near; u!=nil; u=u->near)
				t->flag |= u->flag;
		}
	}

	mask = Fmorse|Flatlng;
	for(t=g->page; t!=nil; t=t->link) {
		if((t->flag & mask) != mask)
			continue;
		dd = ditdah(t->txt);
		mask1 = t->flag & (Fnfreq|Fvfreq|Fdfreq);
		if(mask1 != Fnfreq && mask1 != Fvfreq && mask1 != Fdfreq) {
			fprint(2, "miss %lux %s\n", t->flag, dd);
			continue;
		}

		print("\t\"%s\",", dd);

		if(t->flag & Flatlng)
		for(u=t->near; u!=nil; u=u->near) {
			if(u->flag & Flatlng) {
				print("	%8.4f, %9.4f,", getlat(u->txt), getlat(u->txt+9));
				break;
			}
		} else
			print("	%8.4f, %9.4f,", 0.0, 0.0);

		if(t->flag & Fvfreq) {
			if(strlen(dd) < 3)
				fprint(2, "short vor name %s\n", dd);
			if(t->flag & Fdvor)
				print(" VOR+DME,");
			else
				print(" VOR,");
		} else
		if(t->flag & Fnfreq) {
			if(strlen(dd) < 2)
				fprint(2, "short ndb name %s\n", dd);
			print(" NDB,");
		} else
		if(t->flag & Fdfreq) {
			if(strlen(dd) < 2)
				fprint(2, "short dme name %s\n", dd);
			print(" DME,");
		}

		for(u=t->near; u!=nil; u=u->near) {
			if(u->flag & Fvfreq) {
				print(" %.0f,", atof(u->txt)*100);
				break;
			}
			if(u->flag & Fnfreq) {
				print(" %.0f,", atof(u->txt));
				break;
			}
			if(u->flag & Fdfreq) {
				print(" %.0f,", atof(u->txt+4)*100);
				break;
			}
		}

		print(" 0,\n");

	}
}

int
classify(char *txt)
{
	char *t;
	int n;

	if(*txt == 0 || *txt == ' ')
		return 0;

	if(*txt == '*') {
		t = txt+1;
		while(*t == ' ')
			t++;
		n = classify(t);
		if(n)
			strcpy(txt, t);
		return n;
	}

	/* morse all dit-dot-blank */
	for(t=txt; *t;) {
		if(memcmp(t, "\\255", 4) == 0) {
			t += 4;
			continue;
		}
		if(memcmp(t, "\\267", 4) == 0) {
			t += 4;
			continue;
		}
		if(*t == ' ') {
			t ++;
			continue;
		}
		break;
	}
	if(*t == 0)
		return Fmorse;
	if(strcmp(txt, "\\050T\\051") == 0)
		return Ftvor;
	if(strcmp(txt, "D") == 0)
		return Fdvor;

	if(t[0] == '1')
	if(t[1] >= '0' && t[1] <= '9')
	if(t[2] >= '0' && t[2] <= '9')
	if(t[3] == '.')
	if(t[4] >= '0' && t[4] <= '9') {
		if(t[5] == ' ')
			return Fvfreq;
		if(t[5] == '5')
		if(t[6] == ' ')
			return Fvfreq;
	}

	if(t[0] >= '1' && t[0] <= '9')
	if(t[1] >= '0' && t[1] <= '9')
	if(t[2] >= '0' && t[2] <= '9')
	if(t[3] == ' ')
		return Fnfreq;

	if(memcmp(t, "\\050", 4) == 0)
	if(t[4] == '1')
	if(t[5] >= '0' && t[5] <= '9')
	if(t[6] >= '0' && t[6] <= '9')
	if(t[7] == '.')
	if(t[8] >= '0' && t[8] <= '9') {
		if(memcmp(t+9, "\\051", 4) == 0)
			return Fdfreq;
		if(t[9] == '5')
		if(memcmp(t+10, "\\051", 4) == 0)
			return Fdfreq;
	}

	if(t[0] == 'N' || t[0] == 'S')
	if(t[1] >= '0' && t[1] <= '9')
	if(t[2] >= '0' && t[2] <= '9')
	if(t[3] >= ' ')
	if(t[4] >= '0' && t[4] <= '9')
	if(t[5] >= '0' && t[5] <= '9')
	if(t[6] == '.')
	if(t[7] >= '0' && t[7] <= '9')
	if(t[8] >= ' ')
	if(t[9] == 'W' || t[9] == 'E')
	if(t[10] >= '0' && t[10] <= '9')
	if(t[11] >= '0' && t[11] <= '9')
	if(t[12] >= '0' && t[12] <= '9')
	if(t[13] >= ' ')
	if(t[14] >= '0' && t[14] <= '9')
	if(t[15] >= '0' && t[15] <= '9')
	if(t[16] == '.')
	if(t[17] >= '0' && t[17] <= '9')
		return Flatlng;

	return 0;
}

double
getlat(char *s)
{
	int f;
	double l;

	f = 0;
	if(*s == 'S' || *s == 'E')
		f = 1;
	l = atof(s+1);
	l += atof(strchr(s, ' ')+1) /60.;
	if(f)
		l = -l;
	return l;
}

char*	mcode[] =
{

	"A\\267\\255 ",
	"B\\255\\267\\267\\267 ",
	"C\\255\\267\\255\\267 ",
	"D\\255\\267\\267 ",
	"E\\267 ",
	"F\\267\\267\\255\\267 ",
	"G\\255\\255\\267 ",
	"H\\267\\267\\267\\267 ",
	"I\\267\\267 ",
	"J\\267\\255\\255\\255 ",
	"K\\255\\267\\255 ",
	"L\\267\\255\\267\\267 ",
	"M\\255\\255 ",
	"N\\255\\267 ",
	"O\\255\\255\\255 ",
	"P\\267\\255\\255\\267 ",
	"Q\\255\\255\\267\\255 ",
	"R\\267\\255\\267 ",
	"S\\267\\267\\267 ",
	"T\\255 ",
	"U\\267\\267\\255 ",
	"V\\267\\267\\267\\255 ",
	"W\\267\\255\\255 ",
	"X\\255\\267\\267\\255 ",
	"Y\\255\\267\\255\\255 ",
	"Z\\255\\255\\267\\267 ",
	"0\\255\\255\\255\\255\\255 ",
	"1\\267\\255\\255\\255\\255 ",
	"2\\267\\267\\255\\255\\255 ",
	"3\\267\\267\\267\\255\\255 ",
	"4\\267\\267\\267\\267\\255 ",
	"5\\267\\267\\267\\267\\267 ",
	"6\\255\\267\\267\\267\\267 ",
	"7\\255\\255\\267\\267\\267 ",
	"8\\255\\255\\255\\267\\267 ",
	"9\\255\\255\\255\\255\\267 ",
	0
};
char*
ditdah(char *t)
{
	static char out[1000];
	char *o, *s;
	int i, n;

	strcpy(out+10, t);
	strcat(out+10, " ");
	t = out+10;
	o = out;
	for(;;) {
		while(*t == ' ')
			t++;
		for(i=0; s = mcode[i]; i++) {
			n = strlen(s+1);
			if(memcmp(t, s+1, n) == 0) {
				*o++ = *s;
				t += n;
				break;
			}
		}
		if(s == nil || *t == 0)
			break;
	}
	*o = 0;
	return out;
}

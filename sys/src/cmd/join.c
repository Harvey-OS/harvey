/*	join F1 F2 on stuff */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>

enum {
	F1,
	F2,
	NIN,
	F0,
};

#define	NFLD	100	/* max field per line */
#define comp() runestrcmp(ppi[F1][j1], ppi[F2][j2])

Biobuf *f[NIN];
Rune buf[NIN][Bsize];	/* input lines */
Rune *ppi[NIN][NFLD+1];	/* pointers to fields in lines */
Rune	sep1	= ' ';	/* default field separator */
Rune	sep2	= '\t';
int	j1	= 1;	/* join of this field of file 1 */
int	j2	= 1;	/* join of this field of file 2 */
int	a1;
int 	a2;

int	olist[NIN*NFLD];  /* output these fields */
int	olistf[NIN*NFLD]; /* from these files */
int	no;		/* number of entries in olist */
char *sepstr	= " ";
int	discard;	/* count of truncated lines */
Rune	null[Bsize]	= L"";
Biobuf binbuf, boutbuf;
Biobuf *bin, *bout;

char	*getoptarg(int*, char***);
int	input(int);
void	join(int);
void	oparse(char*);
void	output(int, int);
Rune	*strtorune(Rune *, char *);

void
main(int argc, char **argv)
{
	int i;
	vlong off1, off2;

	bin = &binbuf;
	bout = &boutbuf;
	Binit(bin, 0, OREAD);
	Binit(bout, 1, OWRITE);

	argv0 = argv[0];
	while (argc > 1 && argv[1][0] == '-') {
		if (argv[1][1] == '\0')
			break;
		switch (argv[1][1]) {
		case '-':
			argc--;
			argv++;
			goto proceed;
		case 'a':
			switch(*getoptarg(&argc, &argv)) {
			case '1':
				a1++;
				break;
			case '2':
				a2++;
				break;
			default:
				sysfatal("incomplete option -a");
			}
			break;
		case 'e':
			strtorune(null, getoptarg(&argc, &argv));
			break;
		case 't':
			sepstr=getoptarg(&argc, &argv);
			chartorune(&sep1, sepstr);
			sep2 = sep1;
			break;
		case 'o':
			if(argv[1][2]!=0 ||
			   argc>2 && strchr(argv[2],',')!=0)
				oparse(getoptarg(&argc, &argv));
			else for (no = 0; no<2*NFLD && argc>2; no++){
				if (argv[2][0] == '1' && argv[2][1] == '.') {
					olistf[no] = F1;
					olist[no] = atoi(&argv[2][2]);
				} else if (argv[2][0] == '2' && argv[2][1] == '.') {
					olist[no] = atoi(&argv[2][2]);
					olistf[no] = F2;
				} else if (argv[2][0] == '0')
					olistf[no] = F0;
				else
					break;
				argc--;
				argv++;
			}
			break;
		case 'j':
			if(argc <= 2)
				break;
			if (argv[1][2] == '1')
				j1 = atoi(argv[2]);
			else if (argv[1][2] == '2')
				j2 = atoi(argv[2]);
			else
				j1 = j2 = atoi(argv[2]);
			argc--;
			argv++;
			break;
		case '1':
			j1 = atoi(getoptarg(&argc, &argv));
			break;
		case '2':
			j2 = atoi(getoptarg(&argc, &argv));
			break;
		}
		argc--;
		argv++;
	}
proceed:
	for (i = 0; i < no; i++)
		if (olist[i]-- > NFLD)	/* 0 origin */
			sysfatal("field number too big in -o");
	if (argc != 3) {
		fprint(2, "usage: join [-1 x -2 y] [-o list] file1 file2\n");
		exits("usage");
	}
	if (j1 < 1  || j2 < 1)
		sysfatal("invalid field indices");
	j1--;
	j2--;	/* everyone else believes in 0 origin */

	if (strcmp(argv[1], "-") == 0)
		f[F1] = bin;
	else if ((f[F1] = Bopen(argv[1], OREAD)) == 0)
		sysfatal("can't open %s: %r", argv[1]);
	if(strcmp(argv[2], "-") == 0)
		f[F2] = bin;
	else if ((f[F2] = Bopen(argv[2], OREAD)) == 0)
		sysfatal("can't open %s: %r", argv[2]);

	off1 = Boffset(f[F1]);
	off2 = Boffset(f[F2]);
	if(Bseek(f[F2], 0, 2) >= 0){
		Bseek(f[F2], off2, 0);
		join(F2);
	}else if(Bseek(f[F1], 0, 2) >= 0){
		Bseek(f[F1], off1, 0);
		Bseek(f[F2], off2, 0);
		join(F1);
	}else
		sysfatal("neither file is randomly accessible");
	if (discard)
		sysfatal("some input line was truncated");
	exits("");
}

char *
runetostr(char *buf, Rune *r)
{
	char *s;

	for(s = buf; *r; r++)
		s += runetochar(s, r);
	*s = '\0';
	return buf;
}

Rune *
strtorune(Rune *buf, char *s)
{
	Rune *r;

	for (r = buf; *s; r++)
		s += chartorune(r, s);
	*r = '\0';
	return buf;
}

void
readboth(int n[])
{
	n[F1] = input(F1);
	n[F2] = input(F2);
}

void
seekbotreadboth(int seekf, vlong bot, int n[])
{
	Bseek(f[seekf], bot, 0);
	readboth(n);
}

void
join(int seekf)
{
	int cmp, less;
	int n[NIN];
	vlong top, bot;

	less = seekf == F2;
	top = 0;
	bot = Boffset(f[seekf]);
	readboth(n);
	while(n[F1]>0 && n[F2]>0 || (a1||a2) && n[F1]+n[F2]>0) {
		cmp = comp();
		if(n[F1]>0 && n[F2]>0 && cmp>0 || n[F1]==0) {
			if(a2)
				output(0, n[F2]);
			if (seekf == F2)
				bot = Boffset(f[seekf]);
			n[F2] = input(F2);
		} else if(n[F1]>0 && n[F2]>0 && cmp<0 || n[F2]==0) {
			if(a1)
				output(n[F1], 0);
			if (seekf == F1)
				bot = Boffset(f[seekf]);
			n[F1] = input(F1);
		} else {
			/* n[F1]>0 && n[F2]>0 && cmp==0 */
			while(n[F2]>0 && cmp==0) {
				output(n[F1], n[F2]);
				top = Boffset(f[seekf]);
				n[seekf] = input(seekf);
				cmp = comp();
			}
			seekbotreadboth(seekf, bot, n);
			for(;;) {
				cmp = comp();
				if(n[F1]>0 && n[F2]>0 && cmp==0) {
					output(n[F1], n[F2]);
					n[seekf] = input(seekf);
				} else if(n[F1]>0 && n[F2]>0 &&
				    (less? cmp<0 :cmp>0) || n[seekf]==0)
					seekbotreadboth(seekf, bot, n);
				else {
					/*
					 * n[F1]>0 && n[F2]>0 &&
					 * (less? cmp>0 :cmp<0) ||
					 * n[seekf==F1? F2: F1]==0
					 */
					Bseek(f[seekf], top, 0);
					bot = top;
					n[seekf] = input(seekf);
					break;
				}
			}
		}
	}
}

int
input(int n)		/* get input line and split into fields */
{
	int c, i, len;
	char *line;
	Rune *bp;
	Rune **pp;

	bp = buf[n];
	pp = ppi[n];
	line = Brdline(f[n], '\n');
	if (line == nil)
		return(0);
	len = Blinelen(f[n]) - 1;
	c = line[len];
	line[len] = '\0';
	strtorune(bp, line);
	line[len] = c;			/* restore delimiter */
	if (c != '\n')
		discard++;

	i = 0;
	do {
		i++;
		if (sep1 == ' ')	/* strip multiples */
			while ((c = *bp) == sep1 || c == sep2)
				bp++;	/* skip blanks */
		*pp++ = bp;		/* record beginning */
		while ((c = *bp) != sep1 && c != sep2 && c != '\0')
			bp++;
		*bp++ = '\0';		/* mark end by overwriting blank */
	} while (c != '\0' && i < NFLD-1);

	*pp = 0;
	return(i);
}

void
prfields(int f, int on, int jn)
{
	int i;
	char buf[Bsize];

	for (i = 0; i < on; i++)
		if (i != jn)
			Bprint(bout, "%s%s", sepstr, runetostr(buf, ppi[f][i]));
}

void
output(int on1, int on2)	/* print items from olist */
{
	int i;
	Rune *temp;
	char buf[Bsize];

	if (no <= 0) {	/* default case */
		Bprint(bout, "%s", runetostr(buf, on1? ppi[F1][j1]: ppi[F2][j2]));
		prfields(F1, on1, j1);
		prfields(F2, on2, j2);
		Bputc(bout, '\n');
	} else {
		for (i = 0; i < no; i++) {
			if (olistf[i]==F0 && on1>j1)
				temp = ppi[F1][j1];
			else if (olistf[i]==F0 && on2>j2)
				temp = ppi[F2][j2];
			else {
				temp = ppi[olistf[i]][olist[i]];
				if(olistf[i]==F1 && on1<=olist[i] ||
				   olistf[i]==F2 && on2<=olist[i] ||
				   *temp==0)
					temp = null;
			}
			Bprint(bout, "%s", runetostr(buf, temp));
			if (i == no - 1)
				Bputc(bout, '\n');
			else
				Bprint(bout, "%s", sepstr);
		}
	}
}

char *
getoptarg(int *argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;
	if(argv[1][2] != 0)
		return &argv[1][2];
	if(argc<=2 || argv[2][0]=='-')
		sysfatal("incomplete option %s", argv[1]);
	*argcp = argc-1;
	*argvp = ++argv;
	return argv[1];
}

void
oparse(char *s)
{
	for (no = 0; no<2*NFLD && *s; no++, s++) {
		switch(*s) {
		case 0:
			return;
		case '0':
			olistf[no] = F0;
			break;
		case '1':
		case '2':
			if(s[1] == '.' && isdigit(s[2])) {
				olistf[no] = *s=='1'? F1: F2;
				olist[no] = atoi(s += 2);
				break;
			}
			/* fall thru */
		default:
			sysfatal("invalid -o list");
		}
		if(s[1] == ',')
			s++;
	}
}

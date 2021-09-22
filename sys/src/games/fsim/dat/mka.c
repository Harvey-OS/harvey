#include	<u.h>
#include	<libc.h>
#include	<bio.h>



void	doapt1(char*);
char*	kapt(char*);
int	ktla(char*);
double	getlat1(char *s, int t1, int t2);
int	getele1(char*);
int	getvar1(char*);
void	split(char*);

void	doapt2(char*);
char*	ucase(char*);
double	getlat2(char*);
int	getele2(char*);
int	getvar2(char*);

Biobuf*	bout;
Biobuf*	bin;
char	aptbuf[100];

#define	ERROR	1000

void
main(int argc, char *argv[])
{

	ARGBEGIN {
	default:
		fprint(2, "usage: a.out\n");
		exits(0);
	} ARGEND

	bout = malloc(sizeof(*bout));
	Binit(bout, 1, OWRITE);

	Bprint(bout, "#include \"type.h\"\n");
	Bprint(bout, "\n");
	Bprint(bout, "Dapt	dapt[] =\n");
	Bprint(bout, "{\n");

	doapt1("/usr/ken/dist/faa/raw/apt.col");
	doapt2("/usr/ken/jep/a1.stropria");
	doapt2("/usr/ken/jep/a2.stropria");

	Bprint(bout, "\n");
	Bprint(bout, "	nil\n");
	Bprint(bout, "};\n");

	Bterm(bout);

	exits(0);
}

void
doapt1(char *file)
{
	char *l;
	char *fields[100];
	double f1, f2;

	bin = Bopen(file, OREAD);
	if(bin == nil)
		return;
	for(;;) {
		l = Brdline(bin, '\n');
		if(l == nil)
			break;
		l[Blinelen(bin)-1] = 0;
		getfields(l, fields, nelem(fields), 0, ":");
		if(strcmp(fields[0], "APT") != 0)
			continue;

		if(strcmp(fields[14], "PU") != 0)
		if(!ktla(fields[3]))
			continue;
		if(strcmp(fields[2], "AIRPORT") != 0)
			continue;

		f1 = getlat1(fields[24], 'N', 'S');
		f2 = getlat1(fields[26], 'W', 'E');
		if(f1 == ERROR || f2 == ERROR)
			continue;

		Bprint(bout, "	\"%s\",", kapt(fields[3]));
		Bprint(bout, " %.6f,", f1);
		Bprint(bout, " %.6f,", f2);
		Bprint(bout, " %d,", getele1(fields[28]));
		Bprint(bout, " %d,", getvar1(fields[30]));
		Bprint(bout, "\n");
	}
	Bterm(bin);
}

void
doapt2(char *file)
{
	char *l;
	char *fields[100];

	bin = Bopen(file, OREAD);
	if(bin == nil)
		return;
	for(;;) {
		l = Brdline(bin, '\n');
		if(l == nil)
			break;
		l[Blinelen(bin)-1] = 0;
		getfields(l, fields, nelem(fields), 0, ":");
		if(strlen(fields[3]) != 9 || strlen(fields[4]) != 10) {
			fprint(2, "skipping %s: %s %s\n", fields[0], fields[3], fields[4]);
			continue;
		}
		if(strcmp(fields[10], "usa") == 0)
			continue;

		Bprint(bout, "	\"%s\",", ucase(fields[0]));
		Bprint(bout, " %.6f,", getlat2(fields[3]));
		Bprint(bout, " %.6f,", getlat2(fields[4]));
		Bprint(bout, " %d,", getele2(fields[6]));
		Bprint(bout, " %d,", getvar2(fields[5]));
		Bprint(bout, "\n");
	}
	Bterm(bin);
}

char*
ucase(char *s)
{
	int c;
	char *t;

	t = aptbuf;
	for(;;) {
		c = *s++;
		if(c >= 'a' && c <= 'z')
			c += 'A' - 'a';
		*t++ = c;
		if(c == 0)
			break;
	}
	return aptbuf;
}

double
getlat1(char *s, int t1, int t2)
{
	int c;
	double f;

	f = atof(s);
	c = s[strlen(s)-1];
	if(c == t1)
		return f/3600;
	if(c == t2)
		return -f/3600;
	fprint(2, "bad lat %s\n", s);
	return ERROR;
}

int
getele1(char *s)
{
	return atof(s);
}

int
getvar1(char *s)
{
	double f;
	int c;

	f = atof(s) * 10;
	c = s[strlen(s)-1];
	if(c == 'W')
		return -f;
	if(c == 'E')
		return +f;
//	fprint(2, "bad var %s\n", s);
	return 0;
}

int
ktla(char *s)
{
	int c;

	for(;;) {
		c = *s++;
		if(c >= 'a' && c <= 'z')
			continue;
		if(c >= 'A' && c <= 'Z')
			continue;
		if(c == 0)
			break;
		return 0;
	}
	return 1;
}

char*
kapt(char *s)
{
	int c, f;
	char *t;

	f = 0;
	t = aptbuf;
	*t++ = 'K';
	for(;;) {
		c = *s++;
		if(c >= 'a' && c <= 'z')
			c += 'A' - 'a';
		*t++ = c;
		if(c == 0)
			break;
		if(c < 'A' || c > 'Z')
			f = 1;
	}
	if(f)
		return aptbuf+1;
	return aptbuf;
}

double
getlat2(char *s)
{
	ulong n;
	int i;
	double ll;

	n = strtoul(s+1, nil, 10);
	i = n % 10000;
	ll = i / 360000.;
	i = (n/10000) % 100;
	ll += i/60.;
	i = n / 1000000;
	ll += i;
	if(*s == 's' || *s == 'e')
		ll = -ll;
	return ll;
}

int
getvar2(char *s)
{
	double mv;

	mv = atof(s+1);
	if(*s == 'w')
		mv = -mv;
	return mv*10;
}

int
getele2(char *s)
{
	int el;

	el = atof(s);
	if(el == 0)
		el = 1;
	return el;
}

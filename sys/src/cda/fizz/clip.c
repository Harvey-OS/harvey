#include	<u.h>
#include	<libc.h>
#include	<stdio.h>
#include	<cda/fizz.h>

#define		NPINS		1000

int ssig;
char *tcharg;
short pins[NPINS];
Pin pp[6][NPINS];
int np[6];
int ppp[6][NPINS];
char *cp[6][NPINS];
Board b;
char *optarg;
int hit(int sig, Pin p);
void clip(int sig, Pin p, Chip *c, int pin);
int optind;
int getmfields(char *s, char **ptrs, int n);
void dochip(Chip *), dott(Chip *);
void setpins(char *s);

int
pcmp(Pin *a, Pin *b)
{
	register k;

	if(k = a->p.x - b->p.x)
		return(k);
	return(a->p.y - b->p.y);
}

void
usage(void)
{
	fprint(2, "Usage: clip [-fclipfile] [input files]\n");
	exit(1);
}

void
main(int argc, char *argv[])
{
	int n, i;
	int line;
	char *s;
	char *wd[1000];
	Chip *c;
	char *clipfile;
	FILE *fp = 0;
	char buf[BUFSIZ];

	while((n = getopt(argc, argv, "f:")) != -1)
		switch(n)
		{
		case 'f':
			if((fp = fopen(clipfile = optarg, "r")) == 0){
				perror(clipfile);
				exit(1);
			}
			break;
		case '?':	usage();
		}
	fizzinit();
	f_init(&b);
	if(optind == argc)
		argv[--optind] = "/dev/stdin";
	for(; optind < argc; optind++)
		if(n = f_crack(argv[optind], &b)){
			fprint(2, "%s: %d errors\n", *--argv, n);
			exit(1);
		}
	fizzplane(&b);
	if(n = fizzplace()){
		fprint(2, "Warning: %d chips unplaced\n", n);
		/* exit(1); */
	}
	for(n = 0; n < 6; n++)
		if(b.v[n].npins)
			qsort((char *)b.v[n].pins, b.v[n].npins, sizeof(Pin), pcmp);
	line = 0;
	if(!fp)
		symtraverse(S_CHIP, dott);
	else
	while(s = fgets(buf, sizeof(buf), fp)){
		line++;
		if(*s == '%') continue;
		n = getmfields(s, wd, sizeof(wd)/sizeof(wd[0]));
		if(n < 4){
			fprint(2, "%s: line %d: insufficent number of fields\n",
				clipfile, line);
			exit(1);
		}
		if(strcmp(wd[0], "ssig") == 0)
			ssig = 1;
		else if(strcmp(wd[0], "pin") == 0)
			ssig = 0;
		else {
			fprint(2, "%s: line %d: '%s' not pin or ssig\n",
				clipfile, line, wd[0]);
			exit(1);
		}
		setpins(wd[1]);
		if(strcmp(wd[2], "chip") == 0){
			tcharg = 0;
			for(n = 3; wd[n]; n++)
				if(strcmp(wd[n], "ALL") == 0)
					symtraverse(S_CHIP, dochip);
				else if(c = (Chip *)symlook(wd[n], S_CHIP, (void *)0))
					dochip(c);
				else {
					fprint(2, "'%s' not a chip\n", wd[n]);
					exit(1);
				}
		} else if(strcmp(wd[2], "type") == 0){
			for(n = 3; wd[n]; n++)
				if(strcmp(wd[n], "ALL") == 0){
					tcharg = 0;
					symtraverse(S_CHIP, dochip);
				} else if(symlook(wd[n], S_TYPE, (void *)0)){
					tcharg = wd[n];
					symtraverse(S_CHIP, dochip);
				} else {
					fprint(2, "'%s' not a type\n", wd[n]);
					exit(1);
				}
		} else {
			fprint(2, "%s: line %d: '%s' not chip or type\n",
				clipfile, line, wd[2]);
			exit(1);
		}
	}
	for(n = 0; n < 6; n++){
		if(np[n] == 0) continue;
		fprint(1, "Vsig %d{\n\tpins %d{\n", n, np[n]);
		for(i = 0; i < np[n]; i++)
			fprint(1, "\t\t%d %p %c\t%% %s.%d\n", i+1,
				pp[n][i].p, pp[n][i].drill,
				cp[n][i], ppp[n][i]);
		fprint(1, "\t}\n}\n");
	}
	exit(0);
}

int
getmfields(char *s, char **ptrs, int n)
{
	char  *p;
	int i;
	for(p = s, i = 0; ; p++) {
		if((*p == 0) || (*p == ' ') || (*p == '\t')) {
			ptrs[i++] = s;
			if(*p == 0 || *p == '\n') break;
			*p++ = 0;
			while((*p == ' ') || (*p == '\t')) ++p;
			if((i >= n) || (*p == 0) || *p == '\n') break;
			s = p;
		}
	}
	ptrs[i] = (char *) 0;
	return(i);
}

void
dott(Chip *c)
{
	register i, ch;

	if (c->flags & UNPLACED)
		return;			/* jhs was here */
	for(i = 0; i < c->npins; i++){
		ch = c->type->tt[i];
		if(!isupper(ch))
			continue;
		ch = tolower(ch);
		if(strchr(ttnames, ch))
			clip((int)(strchr(ttnames, ch)-ttnames), c->pins[i], c, i+c->pmin);
	}
}

void
dochip(Chip *c)
{
	register i, j;
	char *s;
	char buf[7], sbuf[6];

	if (c->flags & UNPLACED)
		return;			/* jhs was here */
	if(tcharg && strcmp(tcharg, c->typename))
		return;
	if(ssig){
		if(c->type->tt[0] == 0)
			return;
		memset(sbuf, 0, 6);
		for(i = 0; pins[i] >= 0; i++)
			if(pins[i] < 6)
				sbuf[pins[i]] = 1;
		for(i = j = 0; i < 6; i++)
			if(sbuf[i])
				buf[j++] = ttnames[i];
		buf[j] = 0;
		for(i = 0; i < c->npins; i++)
			if(strchr(buf, c->type->tt[i]))
				clip((int)(strchr(ttnames, c->type->tt[i])-ttnames), c->pins[i], c, i+c->pmin);
	} else {
		for(i = 0; pins[i] >= 0; i++){
			j = pins[i]-c->pmin;
			if(j >= c->npins){
				fprint(2, "pin %d out of range for chip %s\n",
					pins[i], c->name);
				exit(1);
			}
			if(s = strchr(ttnames, c->type->tt[j]))
				clip((int)(s-ttnames), c->pins[j], c, pins[i]);
			else {
				fprint(2, "Warning: %s.%d is not a special signal\n",
					c->name, pins[i]);
				exit(1);
			}
		}
	}
}

void
clip(int sig, Pin p, Chip *c, int pin)
{
	if(!hit(sig, p)){
		cp[sig][np[sig]] = c->name;
		ppp[sig][np[sig]] = pin;
		pp[sig][np[sig]++] = p;
	}
}

void
setpins(char *s)
{
	int i, n;
	int last;
	char lastc;

	for(i = 0; *s;){
		switch(lastc = *s)
		{
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			for(n = 0; (*s >= '0') && (*s <= '9'); )
				n = n*10 + *s++ - '0';
			pins[i++] = n;
			if(lastc == '-'){
				while(++last < n)
					pins[i++] = last;
			}
			last = n;
			break;
		default:
			s++;
			break;
		}
	}
	pins[i] = -1;
}

int
hit(int sig, Pin p)
{
	register lo, mid, hi;
	register Pin *base = b.v[sig].pins;

	if((hi = b.v[sig].npins) == 0)
		return(0);
	if((pcmp(&p, base) < 0) || (pcmp(&p, &base[hi-1]) > 0))
		return(0);
	lo = 0;
	while(lo < hi-1){
		mid = (lo+hi)/2;
		if(pcmp(&p, &base[mid]) < 0)
			hi = mid;
		else
			lo = mid;
	}
	return(pcmp(&p, &base[lo]) == 0);
}

#include "xpal.h"
#include <stdio.h>
#include <ctype.h>

extern char LIBRARY[];

FILE	*fin;
int	base;
int	nimp;
long	numb;
char	name[STRLEN];
int	peeks;
int	peekc;
int	bflag;
int	dflag;
int	iflag;
int	Dflag;
int	vflag;
int	zflag;
char	*extern_id;
char	*package;
char	*manufacturer;
char	*filename;
char	*library = LIBRARY;
int	opin;
int	checksum = 0;
int	default_state = 0;
uchar	*fusebits;
int	checkfuse = 0;
int	next;

int	input_pin[NINPUTS];

#define	EON	1
#define	LINE	2
#define	OUT	3
#define	NUMB	4
#define	COLON	5
#define	PERC	6
#define	NAME	7
#define	ITER	8
#define EXTERN	9
#define	UNKN	10

extern	void	capxconv(void);
extern	void	expect(char*);
extern	int	gchar(void);
extern	int	symb(void);
extern	int	rdata(void);
extern	void	doexit(int);
extern	void	readminterms(void);
extern	void	yyparse(void);
extern	void	rdlibrary(void);
extern	void	convert(void);
extern	void	convert_pin(void);
extern	void	convert_output(void);
extern	void	convert_fuse(void);
extern	void	strcncpy(char *d, char *s, int l);
extern	void	chksumline(char *line);
extern	void	preamble(void);
extern	void	postamble(void);
extern	void	displayit(void);
extern	void	putfuse(int f, int v, int n);
extern	void	writefuses(void);
extern	void	bug(char *msg);

FILE	*scan_fp;
int	max_fuse = 0;
int	found = 0;

extern int yydebug;

unsigned char *lexprog;

struct PIN pin_array[MAXPIN];
extern struct ARRAY arrays[MAXARRAY];
extern int found;

void
main(int argc, char *argv[])
{
	int v, i;

	capxconv();
	dflag = 0;
	bflag = 0;
	iflag = 0;

	v = 0;
	fin = stdin; /* stdin */
	for(i=1; i<argc; i++) {
		if(*argv[i] == '-') {
			argv[i]++;
			if(*argv[i] == 'i') {
				iflag = 1;
				continue;
			}
			if(*argv[i] == 'd') {
				dflag = 1;
				continue;
			}
			if(*argv[i] == 'v') {
				vflag = 1;
				continue;
			}
			if(*argv[i] == 'D') {
				Dflag = 1;
				continue;
			}
			if(*argv[i] == 'X') {
				yydebug = 2;
				continue;
			}
			if(*argv[i] == 'b') {
				bflag = 1;
				default_state = 1;
				continue;
			}
			if(*argv[i] == 'z') {
				zflag = 1;
				continue;
			}
			if(*argv[i] == 'l') {
			    if (argv[++i])
				library = argv[i];
			    else
				fprint(2, "library filename missing for -l\n");
			    continue;
			}
			if(*argv[i] == 't') {
			    if (argv[++i])
				extern_id = argv[i];
			    else
				fprint(2, "device type missing for -t\n");
			    continue;
			}
			if(*argv[i] == 'm') {
			    if (argv[++i])
				manufacturer = argv[i];
			    else
				fprint(2, "manufacturer missing for -m\n");
			    continue;
			}
			fprint(2, "%s: unknown option -%c\n", argv[0], *argv[i]);
			continue;
		} /* end if - */
		if(v == 0) {
			filename = argv[i];
			v++;
			continue;
		}
		if(v > 1) {
			fprint(2, "too many arguments\n");
			exit(1);
		}
		v++;
	} /* end for */
	if (v) {
		fin = fopen(filename, "r");
		if(fin == 0) {
			fprint(2, "xpal: cannot open %s\n", filename);
			exit(1);
		}
	}
	if (extern_id && strlen(extern_id) > 0) (void) rdlibrary(); /* in case of -t */
	readminterms();
	if (Dflag) displayit();
	exit(0);
}

void
readminterms(void)
{
	for(;;) {
		if(rdata()) break;
		convert();
	}
}

rdata(void)
{
	char c;
	char *xcp;
	char *cmdlinepart = "";
	int s, pindex, ipin;
	long v;

loop:
	s = symb();
	if(s == EON) {
		(void) postamble();
		return 1;
	}
	if(s == EXTERN) {
		if (extern_id && strlen(extern_id) > 0) { /* seen -t */
			cmdlinepart = malloc(strlen(extern_id) + 1);
			strcpy(cmdlinepart, extern_id);
		}
		else	extern_id = malloc(STRLEN);
		while(isspace(c = gchar()));
		xcp = extern_id;
		*xcp++ = toupper(c);
		while((c = gchar()) != '\n') *xcp++ = toupper(c);
		*xcp = '\0';
		if (strlen(cmdlinepart) > 0)
			if (strcmp(cmdlinepart, extern_id) != 0)
				fprint(2, "warning: -t %s and .x %s don't agree\n", cmdlinepart, extern_id);
			else;
		else (void) rdlibrary();
		goto loop;
	}
	if(s == ITER) {
		fprint(2, ".r");
		for(;;) {
			c = gchar();
			if(c < 0)
				goto loop;
			fprint(2, "%c", (char) c);
			if(c == '\n')
				goto loop;
		}
	}
	if(s != OUT)
		expect(".o");
	if (!extern_id || (strlen(extern_id) == 0)) expect(".x");
	s = symb();
	if(s != NUMB)
		expect("output pin");
	opin = numb;
	if (opin > MAXPIN) {
		fprint(2, "output pin %d > MAXPIN! Time to recompile!\n", opin);
		exit(1);
	}
	if (!(pin_array[opin].properties & OUTPUT_TYPE))
		fprint(2, "output pin %d not found\n", opin);
	if (pin_array[opin].flags & USED_FLAG)
		fprint(2, "pin %d already used\n", opin);
	pin_array[opin].flags |= USED_FLAG;
	if (iflag)
		fprint(2, "\n.o %d", opin);
	for(pindex=0;;) {
		s = symb();
		if(s != NUMB) break;
/*??*/
		if(s == NAME) {
			fprint(2, "name %s unexpected", name);
			continue;
		}
		ipin = numb;
		if (!(pin_array[ipin].properties & INPUT_TYPE)) {
			fprint(2, "pin %d isn't an input\n", ipin);
			continue;
		}
		if (pindex > NINPUTS) {
			fprint(2, "too many inputs (NINPUTS too small)\n");
			exit(0);
		}
		input_pin[pindex++] = ipin;
		if (iflag)
			fprint(2, " %d", ipin);
	}
	input_pin[pindex] = 0;
	if (iflag)
		fprint(2, "\n");
	if(s != LINE)
		expect("new line");
	nimp = 0;
	for(;;) {
		s = symb();
		if(s == LINE)
			continue;
		if(s != NUMB)
			break;
		v = numb;
		c = symb();
		if(c != COLON && c != PERC)
			expect(":");
		s = symb();
		if(s != NUMB)
			expect("number");
		if(c == PERC) {
			expect("code for %"); /* tee hee */
			continue;
		}
		if(nimp >= NIMP) {
			fprint(2, "buffer[IMP] too small\n");
			exit(1);
		}
		if (iflag)
			fprint(2, " %d:%d", v, numb);
		imp[nimp].val = v;
		imp[nimp].mask = numb;
		nimp++;
	}
	if (iflag)
		fprint(2, "\n");
	peeks = s;
	return 0;
}

void
rdlibrary(void)
{
	int i, j, loweri, lowerj, upperi, upperj, pin;
	scan_fp = fopen(library, "r");
	if(scan_fp == 0) {
		fprint(2, "xpal: cannot open %s\n", library);
		exit(1);
	}
	for(i=0; i<MAXPIN; i++) {
		pin_array[i].input_line = pin_array[i].output_line = 0;
		pin_array[i].max_term = pin_array[i].flags = 0;
	}
	for(i=0; i<MAXARRAY; i++) {
		arrays[i].name = "";
		arrays[i].max_inputs = arrays[i].max_outputs = arrays[i].offset = 0;
	}
	yyparse();
	if (dflag | vflag) {
	    fprint(2, "%s:\n", extern_id);
	    for(i=0; i<found; i++) {
		fprint(2, "\t%s: %d max INPUT, %d max OUTPUT (offset %d)\n", arrays[i].name, arrays[i].max_inputs+1, arrays[i].max_outputs+1, arrays[i].offset);
	    }
	}
	for(i=0; i<found; i++) {
		loweri = arrays[i].offset;
		upperi = arrays[i].max_inputs+1 * arrays[i].max_outputs+1 + arrays[i].offset;
		for(j=i; j<found; j++) {
			lowerj = arrays[j].offset;
			upperj = arrays[j].max_inputs+1 * arrays[j].max_outputs+1 + arrays[j].offset;
			if (i != j)
			  if (((lowerj <= upperi) && (lowerj >= loweri)) ||
			      ((upperj >= loweri) && (upperj <= upperi)))
				fprint(2, "warning: array %s and array %s overlap fuses!\n", arrays[i].name, arrays[j].name);
		}
	}
	for (pin=0, next=0; pin<MAXPIN; pin++)
		if (pin_array[pin].properties & EXTERNAL_TYPE)
			next++;
	fclose(scan_fp);
	if (found) (void) preamble();
	else	{
		fprint(2, "%s not found\n", extern_id);
		exit(1);
	}
}

void
convert(void)
{
	int pinflags = pin_array[opin].flags;
	if (pinflags & FUSE_FLAG) convert_fuse();
	else
	convert_pin();
}

void
convert_fuse(void)
{
	int val;
	val = 0;
	if (nimp == 0)	/* leave fuse intact */
		/* nop */;
	else if (nimp == 1 && imp[0].val == 0 && imp[0].mask == 0)
		val = 1;
	else	{
		fprint(2, "nonconstant expression on fuse pin %d\n", opin);
		return;
	}
	if (dflag)
		fprint(2, "[%d=%d:%d]\n", opin, pin_array[opin].output_line, val);
	putfuse(pin_array[opin].output_line, val, 1);
}

void
convert_pin(void)
{
	int term, val, mask, bit, bitmask, input_line, output_line, pin, pinflags;
	int max_in, max_out, offset;
	if (nimp > pin_array[opin].max_term) {
		fprint(2, "%d terms > %d limit for output pin %d\n",
			nimp, pin_array[opin].max_term, opin);
		return;
	}
	max_in = arrays[pin_array[opin].index].max_inputs + 1;
	max_out = arrays[pin_array[opin].index].max_outputs + 1;
	offset = arrays[pin_array[opin].index].offset;
	for(term=0; term<nimp; term++) {
		output_line = (term + pin_array[opin].output_line) * max_in;
		val = imp[term].val;
		mask = imp[term].mask;
		if (default_state == 0)	/* fuses are intact, so zap 'em */
			putfuse(output_line + offset, 1, max_in);
		if ((mask == 0) & dflag)
			fprint(2, "[%d,-]", opin);
		for (bit=0; bit<NTERM; bit++) {
			bitmask = 1 << bit;
			if ((mask & bitmask) == 0) continue;
			pin = input_pin[bit];
			input_line = pin_array[pin].input_line;
			pinflags = pin_array[pin].flags;
			if (!(pin_array[pin].properties & INPUT_TYPE)) {
				fprint(2, "input line %d not an input!\n", pin);
				return;
			}
			if (val & bitmask) { /* don't complement */
				if (pinflags & COMPLEMENT_FIRST)
					input_line++;
			} else {
				if (!(pinflags & COMPLEMENT_FIRST))
					input_line++;
			}
			if (dflag)
				fprint(2, "[%d,%d]", input_line, output_line/max_in);
			putfuse(input_line + output_line + offset, 0, 1);
		}
		if (dflag) fprint(2, "\n");
	} /* end for */
}


int
symb(void)
{
	int c;
	char *cp;

	if(peeks) {
		c = peeks;
		peeks = 0;
		return c;
	}
	while ((c = gchar()) > 0) {
		if ((c == ' ') || (c == '\t')) continue;
		if (c == '!')
			while((c > 0) && (c != '\n')) c = gchar();
		else break;
	} /* end while */
	if(c <= 0) {
		peeks = EON;
		return EON;
	}
	if(c == '\n')
		return LINE;
	if(c == ':')
		return COLON;
	if(c == '%')
		return PERC;
	if(c == '.') {
		c = gchar();
		if(c == 'o')
			return OUT;
		if(c == 'r')
			return ITER;
		if(c == 'x')
			return EXTERN;
		return UNKN;
	}
	numb = 1;
	cp = name;
	while((c >= '0' && c <= '9') ||
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		c == '-' || c == '_' || c == '+' || c == '[' || c == ']') {
			*cp++ = c;
			if(c < '0' || c > '9')
				numb = 0;
			c = gchar();
	}
	if(cp != name) {
		peekc = c;
		*cp = 0;
		if(numb) {
			numb = atol(name);
			return NUMB;
		}
		return NAME;
	}
	return UNKN;
}

gchar(void)
{
	int c;

	if(peekc) {
		c = peekc;
		peekc = 0;
		return(c);
	}
	c = fgetc(fin);
	return(c);
}

void
expect(char *s)
{

	fprint(2, "%s expected\n", s);
	exit(1);
}

void
strcncpy(char *d, char *s, int l)
{
	int i = 0;
	while (*s && (i < l)) { *d++ = toupper(*s++); i++; }
	*s = '\0';
}

void
chksumline(char *line)
{
	while (*line) {
		checksum += *line & 0x7f;
		if (dflag)
			fprint(2, "%c", *line);
		fprint(1, "%c", *line++);
	}
}

void
preamble(void)
{
	int n, k, array_fuses;
	char line[STRLEN];
	array_fuses = 0;
	for(k = 0; k < found; k++)
		array_fuses += (arrays[k].max_inputs+1) * (arrays[k].max_outputs+1);
	if (array_fuses > ++max_fuse) max_fuse = array_fuses;
	if (vflag) fprint(2, "\tmax fuse=%d\n", max_fuse);
	if (max_fuse == 0) bug("zero fuse array\n");
	sprint(line, "%c %s\n", STX, filename); 
	chksumline(line);
	if (manufacturer && strlen(manufacturer) == 0)
		sprint(line, "%s %s*\n", manufacturer, extern_id);
	else	sprint(line, "%s*\n", extern_id);
	chksumline(line);
	sprint(line, "QP%d*\nQF%d*\nF%d*\n", next, max_fuse, default_state);
	chksumline(line);
	fusebits = (uchar *) malloc(n = (max_fuse+7)/8);
	memset(fusebits, default_state?0xff:0x00, n);
	if (k = max_fuse%8)
		fusebits[n-1] &= 0xff>>(8-k);
}

void
postamble(void)
{
	char line[STRLEN];
	int i, n = (max_fuse+7)/8;
	writefuses();
	for (i=0; i<n; i++)
		checkfuse += fusebits[i];
	checkfuse &= 0xffff;
	sprint(line, "C%.4x*\n%c", checkfuse, ETX);
	chksumline(line);
	checksum &= 0xffff;
	fprint(1, "%.4x", zflag ? 0 : checksum);
	if (dflag)
		fprint(2, "%.4x", checksum);
}

void
displayit(void)
{
	int p, flags;
	for (p=0; p<MAXPIN; p++) {
		flags = pin_array[p].flags;
		if (flags == 0) continue;
		fprint(2, "%3d [%4d]: ", p, pin_array[p].input_line);
		if (flags & FUSE_FLAG)
			fprint(2, "FUSE %4d", pin_array[p].output_line);
		else {
			if (flags & INTERNAL_FLAG) fprint(2, "INT ");
			if (flags & EXTERNAL_FLAG) fprint(2, "EXT ");
			if (flags & COMPLEMENT_FIRST) fprint(2, "++ ");
			if (pin_array[p].max_term > 0)
				fprint(2, "%4d:%d", pin_array[p].output_line, pin_array[p].max_term);
		}
		fprint(2, "\n");
	}
}

void
putfuse(int f, int v, int n)
{
	n += f;
	if (v) {
		for (; f<n; f++)
			fusebits[f>>3] |= 1<<(f&7);
	} else {
		for (; f<n; f++)
			fusebits[f>>3] &= ~(1<<(f&7));
	}
}

static int	lastfout;
static int	nfline;
static char	fline[STRLEN];

static void	write1fuse(int f);
static void	flushfuse(void);

void
writefuses(void)
{
	uchar *fw = fusebits;
	uchar *lw = &fusebits[(max_fuse+7)/8];
	int f, bit;
	lastfout = -9;
	nfline = 0;
	for (f=0; fw<lw; fw++)
		for (bit=0x01; f<max_fuse && bit<=0x80; f++, bit<<=1)
			if (default_state) {
				if (!(*fw & bit))
					write1fuse(f);
			} else {
				if (*fw & bit)
					write1fuse(f);
			}
	flushfuse();
}

static void
write1fuse(int f)
{
	if (f-lastfout > 8 || nfline+(f-lastfout) >= 62) {
		flushfuse();
		nfline = sprint(fline, "L%.4d %d", f, 1-default_state);
		lastfout = f;
		return;
	}
	while (++lastfout < f)
		fline[nfline++] = '0'+default_state;
	fline[nfline++] = '1'-default_state;
}

static void
flushfuse(void)
{
	if (nfline > 0) {
		sprint(&fline[nfline], "*\n");
		chksumline(fline);
	}
	nfline = 0;
}

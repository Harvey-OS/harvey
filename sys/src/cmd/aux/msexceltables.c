/* msexceltables.c    Steve Simon    5-Jan-2005 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>

enum {
	Tillegal = 0,
	Tnumber,		// cell types
	Tlabel,
	Tindex,
	Tbool,
	Terror,

	Ver8 = 0x600,		// only BIFF8 and BIFF8x files support unicode

	Nwidths = 4096,
};
	
	
typedef struct Biff Biff;
typedef struct Col Col;
typedef struct Row Row;

struct Row {
	Row *next;		// next row
	int r;			// row number
	Col *col;		// list of cols in row
};

struct Col {
	Col *next;		// next col in row
	int c;			// col number
	int f;			// index into formating table (Xf)
	int type;		// type of value for union below
	union {			// value
		int index;	// index into string table (Strtab)
		int error;
		int bool;
		char *label;
		double number;
	};
};

struct  Biff {
	Biobuf *bp;		// input file
	int op;			// current record type
	int len;		// length of current record
};

// options
static int Nopad = 0;		// disable padding cells to colum width
static int Trunc = 0;		// truncate cells to colum width
static int All = 0;		// dump all sheet types, Worksheets only by default
static char *Delim = " ";	// field delimiter
static char *Sheetrange = nil;	// range of sheets wanted
static char *Columnrange = nil;	// range of collums wanted
static int Debug = 0;

// file scope
static int Defwidth = 10;	// default colum width if non given
static int Biffver;		// file vesion
static int Datemode;		// date ref: 1899-Dec-31 or 1904-jan-1
static char **Strtab = nil;	// label contents heap
static int Nstrtab = 0;		// # of above
static int *Xf;			// array of extended format indices
static int Nxf = 0;		// # of above
static Biobuf *bo;		// stdout (sic)
static int Doquote = 1;		// quote text fields if they are rc(1) unfriendly

// table scope
static int Width[Nwidths];	// array of colum widths
static int Ncols = -1;		// max colums in table used
static int Content = 0;		// type code for contents of sheet
static Row *Root = nil;		// one worksheet's worth of cells
				
static char *Months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static char *Errmsgs[] = {
	[0x0]	"#NULL!",	// intersection of two cell ranges is empty
	[0x7]	"#DIV/0!",	// division by zero	
	[0xf]	"#VALUE!",	// wrong type of operand
	[0x17]	"#REF!",	// illegal or deleted cell reference
	[0x1d]	"#NAME?",	// wrong function or range name
	[0x24]	"#NUM!",	// value range overflow
	[0x2a]	"#N/A!",	// argument of function not available
};

int
wanted(char *range, int here)
{
	int n, s;
	char *p;

	if (! range)
		return 1;

	s = -1;
	p = range;
	while(1){
		n = strtol(p, &p, 10);
		switch(*p){
		case 0:
			if(n == here)
				return 1;
			if(s != -1 && here > s && here < n)
				return 1;
			return 0;
		case ',':
			if(n == here)
				return 1;
			if(s != -1 && here > s && here < n)
				return 1;
			s = -1;
			p++;
			break;
		case '-':
			if(n == here)
				return 1;
			s = n;
			p++;
			break;
		default:
			sysfatal("%s malformed range spec", range);
			break;
		}
	}
}

	
void
cell(int r, int c, int f, int type, void *val)
{
	Row *row, *nrow;
	Col *col, *ncol;

	if(c > Ncols)
		Ncols = c;

	if((ncol = malloc(sizeof(Col))) == nil)
		sysfatal("no memory");
	ncol->c = c;
	ncol->f = f;
	ncol->type = type;
	ncol->next = nil;

	switch(type){
	case Tnumber:	ncol->number = *(double *)val;	break;
	case Tlabel:	ncol->label = (char *)val;	break;
	case Tindex:	ncol->index = *(int *)val;	break;
	case Tbool:	ncol->bool = *(int *)val;	break;
	case Terror:	ncol->error = *(int *)val;	break;
	default:	sysfatal("can't happen error");
	}

	if(Root == nil || Root->r > r){
		if((nrow = malloc(sizeof(Row))) == nil)
			sysfatal("no memory");
		nrow->col = ncol;
		ncol->next = nil;
		nrow->r = r;
		nrow->next = Root;
		Root = nrow;
		return;
	}

	for(row = Root; row; row = row->next){
		if(row->r == r){
			if(row->col->c > c){
				ncol->next = row->col;
				row->col = ncol;
				return;
			}
			else{
				for(col = row->col; col; col = col->next)
					if(col->next == nil || col->next->c > c){
						ncol->next = col->next;
						col->next = ncol;
						return;
					}
			}
		}

		if(row->next == nil || row->next->r > r){
			if((nrow = malloc(sizeof(Row))) == nil)
				sysfatal("no memory");
			nrow->col = ncol;
			nrow->r = r;
			nrow->next = row->next;
			row->next = nrow;
			return;
		}
	}
	sysfatal("cannot happen error");
}

struct Tm *
bifftime(double num)
{
	long long t = num;

	/* Beware - These epochs are wrong, this
	 * is due to Excel still remaining compatible
	 * with Lotus-123, which incorrectly believed 1900
	 * was a leap year
	 */
	if(Datemode)
		t -= 24107;		// epoch = 1/1/1904
	else
		t -= 25569;		// epoch = 31/12/1899
	t *= 60*60*24;

	return localtime((long)t);
}

void
numfmt(int fmt, int min, int max, double num)
{
	char buf[1024];
	struct Tm *tm;

	if(fmt == 9)
		snprint(buf, sizeof(buf),"%.0f%%", num);
	else
	if(fmt == 10)
		snprint(buf, sizeof(buf),"%f%%", num);
	else
	if(fmt == 11 || fmt == 48)
		snprint(buf, sizeof(buf),"%e", num);
	else
	if(fmt >= 14 && fmt <= 17){
		tm = bifftime(num);
		snprint(buf, sizeof(buf),"%d-%s-%d",
			tm->mday, Months[tm->mon], tm->year+1900);
	}
	else
	if((fmt >= 18 && fmt <= 21) || (fmt >= 45 && fmt <= 47)){
		tm = bifftime(num);
		snprint(buf, sizeof(buf),"%02d:%02d:%02d", tm->hour, tm->min, tm->sec);

	}
	else
	if(fmt == 22){
		tm = bifftime(num);
		snprint(buf, sizeof(buf),"%02d:%02d:%02d %d-%s-%d",
			tm->hour, tm->min, tm->sec,
			tm->mday, Months[tm->mon], tm->year+1900);

	}else
		snprint(buf, sizeof(buf),"%g", num);

	Bprint(bo, "%-*.*q", min, max, buf);
}

void
dump(void)
{
	Row *r;
	Col *c, *c1;
	char *strfmt;
	int i, n, last, min, max;

	if(Doquote)
		strfmt = "%-*.*q";
	else
		strfmt = "%-*.*s";

	for(r = Root; r; r = r->next){
		n = 1;
		for(c = r->col; c; c = c->next){
			n++;
			if(! wanted(Columnrange, n))
				continue;

			if(c->c < 0 || c->c >= Nwidths || (min = Width[c->c]) == 0)
				min = Defwidth;
			if((c->next && c->c == c->next->c) || Nopad)
				min = 0;
			max = -1;
			if(Trunc && min > 2)
				max = min -2;	// FIXME: -2 because of bug %q format ?

			switch(c->type){
			case Tnumber:
				if(Xf == nil || Xf[c->f] == 0)
					Bprint(bo, "%-*.*g", min, max, c->number);
				else
					numfmt(Xf[c->f], min, max, c->number);
				break;
			case Tlabel:
				Bprint(bo, strfmt, min, max, c->label);
				break;
			case Tbool:
				Bprint(bo, strfmt, min, max, (c->bool)? "True": "False");
				break;
			case Tindex:
				if(c->index < 0 || c->index >= Nstrtab)
					sysfatal("SST string out of range - corrupt file?");
				Bprint(bo, strfmt, min, max, Strtab[c->index]);
				break;
			case Terror:
				if(c->error < 0 || c->error >= nelem(Errmsgs) || !Errmsgs[c->error])
					Bprint(bo, "#ERR=%d", c->index);
				else
					Bprint(bo, strfmt, min, max, Errmsgs[c->error]);
				break;
			default:
				sysfatal("cannot happen error");
				break;
			}

			last = 1;
			for(i = n+1, c1 = c->next; c1; c1 = c1->next, i++)
				if(wanted(Columnrange, i)){
					last = 0;
					break;
				}

			if(! last){
				if(c->next->c == c->c)		// bar charts
					Bprint(bo, "=");
				else{
					Bprint(bo, "%s", Delim);
					for(i = c->c; c->next && i < c->next->c -1; i++)
						Bprint(bo, "%-*.*s%s", min, max, "", Delim);
				}
			}
		}
		if(r->next)
			for(i = r->r; i < r->next->r; i++)
				Bprint(bo, "\n");

	}
	Bprint(bo, "\n");
}

void
release(void)
{
	Row *r, *or;
	Col *c, *oc;

	r = Root;
	while(r){
		c = r->col;
		while(c){
			if(c->type == Tlabel)
				free(c->label);
			oc = c;
			c = c->next;
			free(oc);
		}
		or = r;
		r = r->next;
		free(or);
	}
	Root = nil;

	memset(Width, 0, sizeof(Width));
	Ncols = -1;
}

void
skip(Biff *b, int len)
{
	assert(len <= b->len);
	if(Bseek(b->bp, len, 1) == -1)
		sysfatal("seek failed - %r");
	b->len -= len;
}

void
gmem(Biff *b, void *p, int n)
{
	if(b->len < n)
		sysfatal("short record %d < %d", b->len, n);
	if(Bread(b->bp, p, n) != n)
		sysfatal("unexpected EOF - %r");
	b->len -= n;
}

void
xd(Biff *b)
{
	uvlong off;
	uchar buf[16];
	int addr, got, n, i, j;

	addr = 0;
	off = Boffset(b->bp);
	while(addr < b->len){
		n = (b->len >= sizeof(buf))? sizeof(buf): b->len;
		got = Bread(b->bp, buf, n);

		Bprint(bo, "	%6d  ", addr);
		addr += n;

		for(i = 0; i < got; i++)
			Bprint(bo, "%02x ", buf[i]);
		for(j = i; j < 16; j++)
			Bprint(bo, "   ");
		Bprint(bo, "  ");
		for(i = 0; i < got; i++)
			Bprint(bo, "%c", isprint(buf[i])? buf[i]: '.');
		Bprint(bo, "\n");
	}
	Bseek(b->bp, off, 0);
}

static int 
getrec(Biff *b)
{
	int c;
	if((c = Bgetc(b->bp)) == -1)
		return -1;		// real EOF
	b->op = c;
	if((c = Bgetc(b->bp)) == -1)
		sysfatal("unexpected EOF - %r");
	b->op |= c << 8;
	if((c = Bgetc(b->bp)) == -1)
		sysfatal("unexpected EOF - %r");
	b->len = c;
	if((c = Bgetc(b->bp)) == -1)
		sysfatal("unexpected EOF - %r");
	b->len |= c << 8;
	if(b->op == 0 && b->len == 0)
		return -1;
	if(Debug){
		Bprint(bo, "op=0x%x len=%d\n", b->op, b->len);
		xd(b);
	}
	return 0;
}

static uvlong
gint(Biff *b, int n)
{
	int i, c;
	uvlong vl, rc;

	if(b->len < n)
		return -1;
	rc = 0;
	for(i = 0; i < n; i++){
		if((c = Bgetc(b->bp)) == -1)
			sysfatal("unexpected EOF - %r");
		b->len--;
		vl = c;
		rc |= vl << (8*i);
	}
	return rc;
}

double
grk(Biff *b)
{
	int f;
	uvlong n;
	double d;

	n = gint(b, 4);
	f = n & 3;
	n &= ~3LL;
	if(f & 2){
		d = n / 4.0;
	}
	else{
		n <<= 32;
		memcpy(&d, &n, sizeof(d));
	}

	if(f & 1)
		d /= 100.0;
	return d;
}

double
gdoub(Biff *b)
{
	double d;
	uvlong n = gint(b, 8);
	memcpy(&d, &n, sizeof(n));
	return d;
}

char *
gstr(Biff *b, int len_width)
{
	Rune r;
	char *buf, *p;
	int nch, w, ap, ln, rt, opt;
	enum {
		Unicode = 1,
		Asian_phonetic = 4,
		Rich_text = 8,
	};

	if(b->len < len_width){
		if(getrec(b) == -1)
			sysfatal("starting STRING expected CONTINUE, got EOF");
		if(b->op != 0x03c)
			sysfatal("starting STRING expected CONTINUE, got op=0x%x", b->op);
	}

	ln = gint(b, len_width);
	if(Biffver != Ver8){
		if((buf = calloc(ln+1, sizeof(char))) == nil)
			sysfatal("no memory");
		gmem(b, buf, ln);
		return buf;
	}


	if((buf = calloc(ln+1, sizeof(char)*UTFmax)) == nil)
		sysfatal("no memory");
	p = buf;

	if(ln == 0)
		return buf;
	nch = 0;
	*buf = 0;
	opt = gint(b, 1);
	if(opt & Rich_text)
		rt = gint(b, 2);
	else
		rt = 0;
	if(opt & Asian_phonetic)
		ap = gint(b, 4);
	else
		ap = 0;
	for(;;){
		w = (opt & Unicode)? sizeof(Rune): sizeof(char);

		while(b->len > 0){
			r = gint(b, w);
			p += runetochar(p, &r);
			if(++nch >= ln){
				if(rt)
					skip(b, rt*4);
				if(ap)
					skip(b, ap);
				return buf;
			}
		}
		if(getrec(b) == -1)
			sysfatal("in STRING expected CONTINUE, got EOF");
		if(b->op != 0x03c)	
			sysfatal("in STRING expected CONTINUE, got op=0x%x", b->op);
		opt = gint(b, 1);
	}
}

void
sst(Biff *b)
{
	int n;
	
	skip(b, 4);			// total # strings
	Nstrtab = gint(b, 4);		// # unique strings
	if((Strtab = calloc(Nstrtab, sizeof(char *))) == nil)
		sysfatal("no memory");
	for(n = 0; n < Nstrtab; n++)
		Strtab[n] = gstr(b, 2);

}

void
boolerr(Biff *b)
{
	int r = gint(b, 2);		// row
	int c = gint(b, 2);		// col
	int f = gint(b, 2);		// formatting ref
	int v = gint(b, 1);		// bool value / err code
	int t = gint(b, 1);		// type
	cell(r, c, f, (t)? Terror: Tbool, &v);
}

void
rk(Biff *b)
{
	int r = gint(b, 2);		// row
	int c = gint(b, 2);		// col
	int f = gint(b, 2);		// formatting ref
	double v = grk(b);		// value
	cell(r, c, f, Tnumber, &v);
}

void
mulrk(Biff *b)
{
	int r = gint(b, 2);		// row
	int c = gint(b, 2);		// first col
	while(b->len >= 6){
		int f = gint(b, 2);	// formatting ref
		double v = grk(b);	// value
		cell(r, c++, f, Tnumber, &v);
	}
}

void
number(Biff *b)
{
	int r = gint(b, 2);		// row
	int c = gint(b, 2);		// col
	int f = gint(b, 2);		// formatting ref
	double v = gdoub(b);		// double 
	cell(r, c, f, Tnumber, &v);
}

void
label(Biff *b)
{
	int r = gint(b, 2);		// row
	int c = gint(b, 2);		// col
	int f = gint(b, 2);		// formatting ref
	char *s = gstr(b, 2);		// byte string
	cell(r, c, f, Tlabel, s);
}


void
labelsst(Biff *b)
{
	int r = gint(b, 2);		// row
	int c = gint(b, 2);		// col
	int f = gint(b, 2);		// formatting ref
	int i = gint(b, 2);		// sst string ref
	cell(r, c, f, Tindex, &i);
}

void
bof(Biff *b)
{
	Biffver = gint(b, 2);
	Content = gint(b, 2);	
}

void
defcolwidth(Biff *b)
{
	Defwidth = gint(b, 2);
}

void
datemode(Biff *b)
{
	Datemode = gint(b, 2);
}

void
eof(Biff *b)
{
	int i;
	struct {
		int n;
		char *s;
	} names[] = {
		0x005,	"Workbook globals",
		0x006,	"Visual Basic module",
		0x010,	"Worksheet",
		0x020,	"Chart",
		0x040,	"Macro sheet",
		0x100,	"Workspace file",
	};
	static int sheet = 0;

	if(! wanted(Sheetrange, ++sheet)){
		release();
		return;
	}
			
	if(Ncols != -1){
		if(All){
			for(i = 0; i < nelem(names); i++)
				if(names[i].n == Content){
					Bprint(bo, "\n# contents %s\n", names[i].s);
					dump();
				}
		}
		else 
		if(Content == 0x10)		// Worksheet
			dump();
	}
	release();
	USED(b);
}

void
colinfo(Biff *b)
{
	int c;
	int c1 = gint(b, 2);
	int c2 = gint(b, 2);
	int w  = gint(b, 2);

	if(c1 < 0)
		sysfatal("negative column number (%d)", c1);
	if(c2 >= Nwidths)
		sysfatal("too many columns (%d > %d)", c2, Nwidths);
	w /= 256;

	if(w > 100)
		w = 100;
	if(w < 0)
		w = 0;

	for(c = c1; c <= c2; c++)
		Width[c] = w;
}

void
xf(Biff *b)
{
	int fmt;
	static int nalloc = 0;

	skip(b, 2);
	fmt = gint(b, 2);
	if(nalloc >= Nxf){
		nalloc += 20;
		if((Xf = realloc(Xf, nalloc*sizeof(int))) == nil)
			sysfatal("no memory");
	}
	Xf[Nxf++] = fmt;
}

void
writeaccess(Biff *b)
{
	Bprint(bo, "# author %s\n", gstr(b, 2));
}

void
codepage(Biff *b)
{
	int codepage = gint(b, 2);
	if(codepage != 1200)				// 1200 == UTF-16
		Bprint(bo, "# codepage %d\n", codepage);
}

void
xls2csv(Biobuf *bp)
{
	int i;
	Biff biff, *b;
	struct {
		int op;
		void (*func)(Biff *);
	} dispatch[] = {
		0x000a,	eof,
		0x0022,	datemode,
		0x0042,	codepage,
		0x0055,	defcolwidth,
		0x005c,	writeaccess,
		0x007d,	colinfo,
		0x00bd,	mulrk,
		0x00fc,	sst,
		0x00fd,	labelsst,
		0x0203,	number,
		0x0204,	label,
		0x0205,	boolerr,
		0x027e,	rk,
		0x0809,	bof,
		0x00e0,	xf,
	};		
	
	b = &biff;
	b->bp = bp;
	while(getrec(b) != -1){
		for(i = 0; i < nelem(dispatch); i++)
			if(b->op == dispatch[i].op)
				(*dispatch[i].func)(b);
		skip(b, b->len);
	}
}

void
usage(void)
{
	fprint(2, "usage: %s [-Danqt] [-w worksheets] [-c columns] [-d delim] /mnt/doc/Workbook\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int i;
	Biobuf bin, bout, *bp;
	
	ARGBEGIN{
	case 'D':
		Debug = 1;
		break;
	case 'a':
		All = 1;
		break;
	case 'q':
		Doquote = 0;
		break;
	case 'd':
		Delim = EARGF(usage());
		break;
	case 'n':
		Nopad = 1;
		break;
	case 't':
		Trunc = 1;
		break;
	case 'c':
		Columnrange = EARGF(usage());
		break;
	case 'w':
		Sheetrange = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND;

	if(argc != 1)
		usage();

	bo = &bout;
	quotefmtinstall();
	Binit(bo, OWRITE, 1);

	if(argc > 0) {
		for(i = 0; i < argc; i++){
			if((bp = Bopen(argv[i], OREAD)) == nil)
				sysfatal("%s cannot open - %r", argv[i]);
			xls2csv(bp);
			Bterm(bp);
		}
	} else {
		Binit(&bin, 0, OREAD);
		xls2csv(&bin);
	}
	exits(0);
}


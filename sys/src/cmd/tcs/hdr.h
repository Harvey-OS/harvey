extern int squawk;
extern int clean;
extern char *file;
extern int verbose;
extern long ninput, noutput, nrunes, nerrors;

enum { From = 1, Table = 2, Func = 4 };

struct convert{
	char *name;
	char *chatter;
	int flags;
	void *data;
} convert[];
struct convert *conv(char *, int);
typedef void (*Infn)(int, long *, struct convert *);
typedef void (*Outfn)(Rune *, int, long *);
void outtable(Rune *, int, long *);

void utf_in(int, long *, struct convert *);
void utf_out(Rune *, int, long *);
void isoutf_in(int, long *, struct convert *);
void isoutf_out(Rune *, int, long *);

#define		N		10000

#define	OUT(out, r, n)	if(out->flags&Table) outtable(r, n, (long *)out->data);\
			else ((Outfn)out->data)(r, n, (long *)0)

Rune runes[N];
char obuf[5*N];	/* maximum bloat from N runes */

#define		BADMAP	(0xFFFD)
#define		ESC	033

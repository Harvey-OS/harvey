#include	<stdlib.h>
#include	<stdio.h>
#include	<libv.h>
#include	<string.h>
char	*strdup(char *);

#define		NNETS		5000
#define		ROWLEN		10000L
#define		PT(x,y)		((x)+ROWLEN*(y))
#define		XY(l)		(l)%ROWLEN, (l)/ROWLEN
#define		NPTS		50000

extern int verbose;
extern char *debug;
extern int oflag;
extern int eflag;
extern FILE  *UN, *RE, *NEW;

inline abs(int a) { if(a < 0) a = -a; return(a); }

#define	DEBUG(n)	(debug && ((strcmp(debug, n) == 0) || (*debug == 0)))

#define	START		04
#define	STOP		010
#define	STARTSTOP	(START|STOP)

class Wire
{
public:
	void print(int);
	void flip();
	void set(char, int, char *, char *, int, int, char *, int, int, char *);
	void pr(FILE *, char *);
	void pr(FILE *fp) { pr(fp, (char *)0); }
	int inline lev() { return(f2&03); }
	friend Wire *readwire(FILE *);
	friend Net;
private:
	int f2, x1, y1, x2, y2;
	char f1, *f3, *f4, *f5, *f6;
	char newish;
	void reverse();
	void unpr() { flip(); pr(UN); flip(); }
};

class Net
{
public:
	Net(FILE*, Wire **, int);
	int done;
	Net *next;
	void diff();
	void rm();
	void eqelim();
	int in(long);
	void pr(FILE*, char *);
	void pr(FILE *fp) { pr(fp, (char *)0); }
	friend Net *readnets(FILE *, int);
private:
	Wire *wires;
	int nwires;
	int nalloc;
	long *pts;
	char *name;
	Wire *step;
	void dumbdiff(Net *);
	void quickdiff(Net *);
	void vdiff(Net *);
	void vpr(char *);
	void order();
	Wire *havewire(Wire *);
	void addwire(Wire *);
	void delwire(Wire *);
	friend dsame(int, long *, int, long *);
};
extern Net *oldnets, *newnets;

#define	OVER(v, n)	for(v = n->wires, n->step = v+1; v < &n->wires[n->nwires]; v = (n->step)++)

class Pin;
class Pintab
{
public:
	Pintab(int);
	void stats();
	Net *look(long pt) { return(install(pt, (Net *)0)); }
	Net *install(long, Net *);
private:
	int npins;
	Pin **pins;
};
extern Pintab pins;

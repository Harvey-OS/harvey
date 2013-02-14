/* normally defined in ../port/portdat.h */
#define	ROUND(s, sz)	(((s)+(sz-1))&~(sz-1))
#define	PGROUND(s)	ROUNDUP(s, BY2PG)
#define HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))	/* ceiling */

void cgainit(void);
void cgaputc(int);
int inb(int);
void outb(int, int);

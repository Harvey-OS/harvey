#include "all.h"

/*
 * subroutines for wrap programs
 */

int
strparse(char *p, char **v, int size, char *sep)
{
	int i;

	--size;
	for(i=0, p=strtok(p, sep); p; ++i, p=strtok(0, sep))
		if(i < size)
			*v++ = p;
	*v = 0;
	return i;
}

/*
 * read one more wire entry
 */

int
wrd(void)
{
	static int nlines;
	char *l, *src[13];
	int n;

	l = Brdline(wr, '\n');
	if(l == 0)
		return -1;
	++nlines;
	l[BLINELEN(wr)-1] = 0;
	n = strparse(l, src, 13, " ");
	if(n != 12 || src[0][0] == 0 || src[0][1] != 0){
		fprint(2, "wrap file format error, line %d\n", nlines);
		exits("format");
	}
	wbuf.stype = src[0][0];
	wbuf.mark = strtol(src[1], 0, 10);
	strncpy(wbuf.sname, src[2], sizeof wbuf.sname);
	wbuf.slen = strtol(src[3], 0, 10);
	coord(&wbuf.pent[0], &src[4]);
	coord(&wbuf.pent[1], &src[8]);
	return 0;
}

void
coord(Pinent *p, char **src)
{
	char *s;

	p->x = strtol(src[0], 0, 10);
	s = strchr(src[0], '/');
	p->y = s ? strtol(s+1, 0, 10) : 0;
	strncpy(p->chname, src[1], sizeof p->chname);
	p->chsize = strtol(src[2], 0, 10);
	strncpy(p->pname, src[3], sizeof p->pname);
}

/*
 * put pins in proper sequence
 */
void
pinsort(void)
{
	Pinent *p1, *p2;
	int d;

	p1 = &wbuf.pent[0];
	p2 = &wbuf.pent[1];
	d = testdir(p1, p2, wroute[0]);
	if(d == 1 || (d == 2 && (testdir(p1, p2, wroute[1]) == 1))){
		p1 = &wbuf.pent[1];
		p2 = &wbuf.pent[0];
	}
	dirn[0] = testdir(p1, p2, 0);
	dirn[1] = testdir(p1, p2, 1);
	pptr[0] = p1;
	pptr[1] = p2;
}


/*
 * test the direction of a wire run
 * dir 	is the direction to be tested
 *	0	increasing x
 *	1	increasing y
 *	2	decreasing x
 *	3	decreasing y
 * return the following
 *	0	goes in that direction
 *	1	goes in the opposite direction
 *	2	does not move on that axis
 */

int
testdir(Pinent *pp1, Pinent *pp2, int dir)
{
	Pinent *i;

	if(dir & 2){
		i = pp1;
		pp1 = pp2;
		pp2 = i;
	}
	if(dir & 1){
		if(pp1->y < pp2->y)
			return 0;
		if(pp1->y == pp2->y)
			return 2;
	}else{
		if(pp1->x < pp2->x)
			return 0;
		if(pp1->x == pp2->x)
			return 2;
	}
	return 1;
}

/*
 * shift origin and rotate board
 * rot		anti-clockwise rotation
 *		+4 to turn board over
 * xbase/ybase	new origin
 */

#define S	1		/* swap x and y */
#define X	2		/* invert x */
#define Y	4		/* invert y */

static int permv[] = {
	0, S+Y, Y+X, S+X, S, X, S+X+Y, Y
};

void
rotate(Pinent *pp)
{
	int nx, ny;
	int c;

	c = permv[rot];
	nx = pp->x;
	ny = pp->y;
	if(c & X)
		nx = xbase - nx;
	if(c & Y)
		ny = ybase - ny;
	if(nx < 0 || ny < 0){
		fprint(2, "coordinate out of bounds\n");
		exits("bounds");
	}
	if(c & S){
		pp->x = ny;
		pp->y = nx;
	}else{
		pp->x = nx;
		pp->y = ny;
	}
}

/*
 * create temporary file (modeled on sam)
 */

Biobuf *
Btmpfile(void)
{
	int i, fd;
	char *u, tempnam[30];
	Biobuf *bp;

	u = getuser();
	i = getpid();
	do
		sprint(tempnam, "/tmp/%.4s.%d.wrap", u, i++);
	while(access(tempnam, 0) == 0);
	fd = create(tempnam, ORDWR|OCEXEC|ORCLOSE, 0000);
	if(fd < 0){
		remove(tempnam);
		fd = create(tempnam, ORDWR|OCEXEC|ORCLOSE, 0000);
	}
	bp = malloc(sizeof(Biobuf));
	Binit(bp, fd, OWRITE);
	return bp;
}

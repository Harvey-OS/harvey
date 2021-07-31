#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>

#define	HUGEINT	0x7fffffff

char *symerror = 0;			/* error message */

static	Sym	*symbols;		/* symbol table */
static	long	nsym;			/* number of symbols */

static	uchar	*spoff;			/* pc-sp state table */
static	uchar	*spoffend;		/* end of pc-sp offset table */

static	uchar	*pcline;		/* pc-line state table */
static	uchar 	*pclineend;		/* end of pc-line table */

static	long	txtstart;		/* start of text segment point */
static	long	txtend;			/* end of text segment */

/*
 *	initialize the symbol tables
 */
int
syminit(int fd, Fhdr *fp)
{
	int i;

	symerror = 0;
	if (fp->symsz == 0)
		return 0;

	txtstart = fp->txtaddr;
	switch(fp->type)
	{
	case FNONE:
		return(0);
	case FMIPS:			/* v.out */
	case FSPARC:			/* k.out */
	case F68020:			/* 2.out */
	case FI386:			/* 8.out */
	case FI960:			/* 6.out */
	case FHOBBIT:			/* z.out */
		txtstart += fp->hdrsz;
		break;
	default:			/* bootables */
		break;
	}
	txtend = fp->txtaddr+fp->txtsz;
	nsym = fp->symsz/sizeof(Sym);
	symbols = malloc(fp->symsz);
	if (symbols == 0) {
		symerror = "sym table malloc failure\n";
		return -1;
	}
	if (seek(fd, fp->symoff, 0) < 0) {
		symerror = "seek failure\n";
		return -1;
	}
	i = read(fd, symbols, fp->symsz);
	if (i != fp->symsz) {
		symerror = "error reading symbol table: %d\n", i;
		return -1;
	}
	if (fp->sppcsz) {			/* pc-sp offset table */
		spoff = (uchar *) malloc(fp->sppcsz);
		if (spoff == 0) {
			symerror = "out of pc-sp table memory\n";
			return -1;
		}
		seek(fd, fp->sppcoff, 0);
		i = read(fd, spoff, fp->sppcsz);
		if (i != fp->sppcsz){
			spoff = 0;
			symerror = "unable to read sp offset table\n";
			return -1;
		}
		spoffend = spoff+fp->sppcsz;
	}
	if (fp->lnpcsz) {			/* pc-line number table */
		pcline = (uchar *) malloc(fp->lnpcsz);
		if (pcline == 0) {
			symerror = "out of pc-line table memory\n";
			return -1;
		}
		seek(fd, fp->lnpcoff, 0);
		i = read(fd, pcline, fp->lnpcsz);
		if (i != fp->lnpcsz){
			pcline = 0;
			symerror = "unable to read pc/line table\n";
			return -1;
		}
		pclineend = pcline+fp->lnpcsz;
	}
	return nsym;
}
/*
 *	symbase: return base and size of raw symbol table
 *		(special hack for high access rate operations)
 */
Sym *
symbase(long *n)
{
	*n = nsym;
	return symbols;
}
/*
 *	Get the ith symbol table entry
 */
Sym *
getsym(int index)
{
	if (index < nsym)
		return &symbols[index];
	return 0;
}
/*
 *	find the stack frame, given the pc
 */
long
pc2sp(ulong pc)
{
	uchar *c;
	uchar u;
	ulong currpc;
	long currsp;

	if(spoff == 0)
		return -1;
	currsp = 0;
	currpc = txtstart - mach->pcquant;

	if (pc<currpc || pc>txtend)
		return -1;
	for (c = spoff; c < spoffend; c++) {
		if (currpc >= pc)
			return currsp;
		u = *c;
		if (u == 0) {
			currsp += (c[1]<<24)|(c[2]<<16)|(c[3]<<8)|c[4];
			c += 4;
		} else if (u < 65)
			currsp += 4*u;
		  else if (u < 129)
			currsp -= 4*(u-64);
		  else 
			currpc += mach->pcquant*(u-129);
		currpc += mach->pcquant;
	}
	return -1;
}
/*
 *	find the source file line number for a given value of the pc
 */
long
pc2line(ulong pc)
{
	uchar *c;
	uchar u;
	ulong currpc;
	long currline;

	if(pcline == 0)
		return -1;
	currline = 0;
	currpc = txtstart - mach->pcquant;
	if (pc<currpc || pc>txtend)
		return -1;
	for (c = pcline; c < pclineend; c++) {
		if (currpc >= pc)
			return currline;
		u = *c;
		if (u == 0) {
			currline += (c[1]<<24)|(c[2]<<16)|(c[3]<<8)|c[4];
			c += 4;
		} else if (u < 65)
			currline += u;
		  else if (u < 129)
			currline -= (u-64);
		  else 
			currpc += mach->pcquant*(u-129);
		currpc += mach->pcquant;
	}
	return -1;
}
/*
 *	find the pc associated with a line number
 *	basepc and endpc are text addresses bounding the search.
 *	if endpc == 0, the end of the table is used (i.e., no upper bound).
 *	usually, basepc and endpc contain the first text address in
 *	a file and the first text address in the following file, respectively.
 */
long
line2addr(ulong line, ulong basepc, ulong endpc)
{
	uchar *c;
	uchar u;
	ulong currpc;
	long currline;
	long delta, d;
	long pc, found;

	if(pcline == 0 || line == 0)
		return -1;
	currline = 0;
	currpc = txtstart - mach->pcquant;
	pc = -1;
	found = 0;
	delta = HUGEINT;

	for (c = pcline; c < pclineend; c++) {
		if (endpc && currpc >= endpc)	/* end of file of interest */
			break;
		if (currpc >= basepc) {		/* proper file */
			if (currline >= line) {
				d = currline-line;
				found = 1;
			} else
				d = line-currline;
			if (d < delta) {
				delta = d;
				pc = currpc;
			}
		}
		u = *c;
		if (u == 0) {
			currline += (c[1]<<24)|(c[2]<<16)|(c[3]<<8)|c[4];
			c += 4;
		} else if (u < 65)
			currline += u;
		  else if (u < 129)
			currline -= (u-64);
		  else 
			currpc += mach->pcquant*(u-129);
		currpc += mach->pcquant;
	}
	if (found)
		return pc;
	return -1;
}

/*
 *	convert an encoded file name to a string.
 */
int
fileelem(Sym **fp, uchar *cp, char *buf, int n)
{
	int i, j;
	char *c, *bp, *end;

	bp = buf;
	end = buf+n-1;
	for(i = 1; i < NNAME-1; i+=2){
		j = (cp[i]<<8)|cp[i+1];
		if(j == 0)
			break;
		c = fp[j]->name;
		if (bp != buf && bp[-1] != '/' && bp < end)
			*bp++ = '/';
		while (bp < end && *c)
			*bp++ = *c++;
	}
	*bp = 0;
	return bp-buf;
}

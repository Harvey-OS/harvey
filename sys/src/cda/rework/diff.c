#include	"rework.h"

static
nsame(int an, long *al, int bn, long *bl)
{
	register n = 0;
	register long *be = bl+bn;

	for(; --an >= 0; al++){
		while((*bl < *al) && (bl < be))
			bl++;
		if(bl >= be)
			break;
		if(*bl == *al)
			n++, bl++;
	}
	return(n);
}

void
Net::diff()
{
	register Net *o, *besto;
	register n, bestn;
	extern qflag;

fprintf(stdout, "diff on net %s\n", name);
	bestn = 0;
	for(o = oldnets; o; o = o->next){
		if(o->done)
			continue;
		if((n = nsame(nwires+1, pts, o->nwires+1, o->pts)) > bestn)
			bestn = n, besto = o;
	}
	if(bestn > 0){
		if(verbose)
			vdiff(besto);
		else
			qflag? quickdiff(besto) : dumbdiff(besto);
	} else {
		if(oflag)
			order();
		if(verbose)
			vpr("add new");
		else {
			if(DEBUG(name)) {
				fprintf(stdout, "wrap new net %s\n", name);
				fflush(stdout);
			}
			pr(NEW, name);
			pr(RE, name);
		}
	}
}

void
Net::vpr(char *s)
{
	register i;

	fprintf(stdout, "%s net %s:", s, name);
	for(i = 0; i < nwires+1; i++)
		fprintf(stdout, " %d/%d", XY(pts[i]));
	putchar('\n');
}

dsame(int an, long *al, int bn, long *bl)
{
	register long *be = bl+bn;
	int diff = 0;

	for(; --an >= 0; al++){
		while((*bl < *al) && (bl < be))
			bl++;
		if(bl >= be){
			diff = 1;
			break;
		}
		if(*bl == *al)
			*al = *bl++ = -1;
		else
			diff = 1;
	}
	return(diff == 0);
}

void
Net::vdiff(Net *old)
{
	register i;
	register long *l;

	done = 1;
	old->done = 1;
	if(dsame(nwires+1, pts, old->nwires+1, old->pts))
		return;
	fprintf(stdout, "old %s -> new %s:", old->name, name);
	for(i = 0, l = old->pts; i < old->nwires+1; i++, l++)
		if(*l >= 0) fprintf(stdout, " -%d/%d", XY(*l));
	for(i = 0, l = pts; i < nwires+1; i++, l++)
		if(*l >= 0) fprintf(stdout, " +%d/%d", XY(*l));
	putchar('\n');
}

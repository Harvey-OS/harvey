#include	"rework.h"


void
Wire::set(char F1, int F2, char *F3, char *F4, int X1, int Y1, char *F5, int X2, int Y2, char *F6)
{
	f1 = F1; f2 = F2; f3 = F3; f4 = F4; x1 = X1; y1 = Y1;
	f5 = F5; f6 = F6; x2 = X2; y2 = Y2;
	newish = 0;
}

void
Wire::flip()
{
	register i;

	i = f2 & ~3;
	f2 = 3-(f2&3);
	f2 |= i;
}

void
Wire::pr(FILE *fp, char *newname)
{
	fprintf(fp, "%c %d %s %s %d/%d %s %d/%d %s\n", f1, f2, newname? newname:f3,
		f4, x1, y1, f5, x2, y2, f6);
}

Wire *
readwire(FILE *fp)
{
	char net[256];
	char *ss[256];
	char s8[256], s9[256];
	int type, x1, y1, x2, y2;
	char buf[BUFSIZ];
	char wtype;
	register Wire *w;
	register char **f = ss, *s;

	if((s = fgets(buf, (int) sizeof(buf), fp)) == 0)
		return((Wire *)0);
	buf[strlen(buf)-1] = 0;		/* step on newline */
/*			 vv vv 4  vvvvv	   5     vvvvv     6		*/
/*	if(sscanf(s, "%c %d %s %s %d/%d %s %s %s %d/%d %s %s %s ", &wtype,
		&type, net, s7, &x1, &y1, s1, s2, s3, &x2, &y2, s4, s5, s6) != 14)
		return(-1);*/
	if((type = getfields(s, ss, 256)) != 12){
		fprintf(stderr, "read %d fields; expected 12\n", type);
		exit(1);
	}
	wtype = f[0][0];
	type = atoi(f[1]);
	x1 = atoi(s = f[4]);
	while(*s++ != '/');
	y1 = atoi(s);
	x2 = atoi(s = f[8]);
	while(*s++ != '/');
	y2 = atoi(s);
	sprintf(s8, "%s %s %s", f[5], f[6], f[7]);
	sprintf(s9, "%s %s %s", f[9], f[10], f[11]);
	w = new Wire;
	w->set(wtype, type&(~STARTSTOP), strdup(f[2]), strdup(f[3]), x1, y1,
		strdup(s8), x2, y2, strdup(s9));
	return(w);
}

void
Wire::reverse()
{
	char *s;
	int x, y;

	s = f5, x = x1, y = y1;
	f5 = f6, x1 = x2, y1 = y2;
	f6 = s, x2 = x, y2 = y;
}

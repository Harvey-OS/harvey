/*
 * Usage: resample npixels [input] [B C]
 *	horizontally scales the input (default standard input) picture
 *	to be npixels wide.  B and C are parameters of the resampling kernel,
 *	which is (1-B)*ease + B*bspline + C*crisp.
 *	Default parameters are B=1/3, C=1/3
 *
 * D. P. Mitchell  86/03/05.
 * Remangled td89.11.16 (removed Hitachi gamma-correction, tweaked for speed)
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define HALFWIDTH 2.0
#define	SCALE	4096	/* filter coefficients are scaled integers */
typedef struct Filterprog {
	short *coef;			/* Coefficients for this pixel */
	short *ecoef;			/* End of coefficient list */
	short decr;			/* how much to decrement (something) by */
	struct Filterprog *next;	/* coefficients for next pixel */
}Filterprog;
float itox, jtox, xtoi, xtoj;
int startprog;
Filterprog *prog;
int nchan;
double B = 0.3333333333;
double C = 0.3333333333;
double bspline(double x){
	if (x < 1.0)
		return 0.5*x*x*x - x*x + 2.0/3.0;
	else
		return -x*x*x/6.0 + x*x - 2.0*x + 4.0/3.0;
}
double ease(double x){
	if (x < 1.0)
		return 2.0*x*x*x - 3.0*x*x + 1.0;
	else
		return 0.0;
}
double crisp(double x){
	if (x < 1.0)
		return -x*x*x + x*x;
	else
		return -x*x*x + 5.0*x*x - 8.0*x + 4.0;
}
double filter(double x){
	if (x < 0.0) x = -x;
	if (x >= 2.0) return 0.0;
	return (1.0 -B)*ease(x) + B*bspline(x) + C*crisp(x);
}
static Filterprog *impulse_response(int j){
	int start, stop, i, count;
	float x, y;
	Filterprog *fp;
	fp = (Filterprog *)malloc(sizeof(Filterprog));
	if(fp==0){
	NoSpace:
		fprint(2, "resample: no space\n");
		exits("no space");
	}
	x = jtox*((float)j + 0.5);
	start = floor(xtoi*(x - HALFWIDTH) - 0.5);
	start++;
	stop = floor(xtoi*(x + HALFWIDTH) - 0.5);
	count=1+stop-start;
	fp->coef = (short *)malloc(count * sizeof(short));
	if(fp->coef==0) goto NoSpace;
	fp->ecoef=fp->coef+count;
	fp->decr = start*nchan;
	fp->next = 0;
	for (i = start; i <= stop; i++) {
		y = itox*((float)i + 0.5);
		fp->coef[i - start] = SCALE*filter(x - y) + 0.5;
	}
	return fp;
}
int gcd(int u, int v){
	if (v > u)
		return gcd(v, u);
	else if (v == 0)
		return u;
	else
		return gcd(v, u % v);
}
void compile_resampling_program(int m, int n){
	int j, u, period;
	Filterprog *temp, *fp, **fpp;
	float x;
	u = MIN(m, n);
	itox = (float)u/(float)m;
	jtox = (float)u/(float)n;
	xtoi = 1.0/itox;
	xtoj = 1.0/jtox;
	period = n / gcd(m, n);
	fpp = &fp;
	for (j = 0; j < period + 1; j++) {
		*fpp = impulse_response(j);
		fpp = &(*fpp)->next;
	}
	startprog = fp->decr;
	for (temp = fp, j = 0; j < period; j++) {
		temp->decr = (temp->ecoef - temp->coef)*nchan - temp->next->decr + temp->decr;
		if (j < period - 1)
			temp = temp->next;
	}
	temp->next = fp;			/* circular */
	prog = fp;
}
void resample(unsigned char input[], unsigned char *output, int m, int n){
	static init = 0;
	unsigned char *src;
	short *coef, *ecoef;
	int sig;
	short temp;
	Filterprog *fp;
	if (init == 0) {
		init = 1;
		compile_resampling_program(m, n);
	}
	fp = prog;
	src = input + startprog;
	for (fp = prog; --n >= 0; fp = fp->next) {
		coef=fp->coef;
		ecoef= fp->ecoef;
		sig=0;
		while(coef!=ecoef){
			sig += *src * *coef++;
			src+=nchan;
		}
		if(m>n) sig=itox*(double)sig;
		if(sig<0) *output=0;
		else if(sig>=255*SCALE) *output=255;
		else *output=sig/SCALE;
		output+=nchan;
		src -= fp->decr;
	}
}
main(int argc, char *argv[]){
	PICFILE *infile, *outfile;
	int outwidth, n;
	int i;
	unsigned char *inrgba, *outrgba;
	char *inname;
	argc=getflags(argc, argv, "");
	switch(argc){
	default: usage("newwidth [picture] [B C]");
	case 4:	B=atof(argv[2]);
		C=atof(argv[3]);
	case 2: inname="IN";
		break;
	case 5:	B=atof(argv[3]);
		C=atof(argv[4]);
	case 3: inname=argv[2];
		break;
	}
	outwidth=atoi(argv[1]);
	infile=picopen_r(inname);
	if(infile==0){
		perror(inname);
		exits("open input");
	}
	nchan=PIC_NCHAN(infile);
	inrgba=(unsigned char *)malloc((PIC_WIDTH(infile)+2000)*nchan);
	if(inrgba==0){
	NoSpace:
		fprint(2, "%s: no space\n", argv[0]);
		exits("no space");
	}
	memset(inrgba, 0, (PIC_WIDTH(infile)+2000)*nchan);
	inrgba+=1000*nchan;
	outrgba=(unsigned char *)malloc(outwidth * nchan);
	if(outrgba==0) goto NoSpace;
	outfile=picopen_w("OUT", infile->type,
		0, 0, outwidth, PIC_HEIGHT(infile), infile->chan, argv, infile->cmap);
	if(outfile==0){
		perror(argv[0]);
		exits("create output");
	}
	while(picread(infile, (char *)inrgba)){
		for(n=0;n!=nchan;n++)
			resample(inrgba+n, outrgba+n, PIC_WIDTH(infile), outwidth);
		picwrite(outfile, (char *)outrgba);
	}
	exits("");
}

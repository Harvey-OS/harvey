#ifndef PLAN9
#include <math.h>
#endif
#include <u.h>
#include <libc.h>
#include <stdio.h>

int AFlag = 0;
int Pflag = 0;

char *chipname = 0;
double kVCO = 0.0, kPD = 0.0, k, omega, freq, zeta, cap;
double t1, t2;
int n = 0;

void active(void);
void passive(void);
void plotit(void);
void rect_to_polar(double x, double y, double *a, double *b);
double wpo(void);

struct PLL {
	char *name;
	float k_vco;
	float k_pd;
} chips[] = {
	{"7046-1", 22440.0, 1.59154943091895},
	{"7046-2", 22440.0, 0.397887357729738},
	{"7046-3", 22440.0, 0.795774715459477},
	0};

void
main(int argc, char *argv[])
{
	int i;
	ARGBEGIN{
	case 't':
		chipname = strdup(ARGF());
		break;
	case 'A':
		AFlag++;
		break;
	case 'P':
		Pflag++;
		break;
	case 'T':
		for(i=0; chips[i].name; i++)
			fprintf(stderr, "%s: -k %f -p %f\n",
			chips[i].name, chips[i].k_vco, chips[i].k_pd);
		exits(0);
	case 'c':
		cap = atof(ARGF());
		break;
	case 'v':
		kVCO = 2.0 * PI * atof(ARGF());
		break;
	case 'p':
		kPD = atof(ARGF());
		break;
	case 'o':
		omega = 2.0 * PI * atof(ARGF());
		break;
	case 'f':
		freq = atof(ARGF());
		break;
	case 'z':
		zeta = atof(ARGF());
		break;
	case 'n':
		n = atoi(ARGF());
		break;
	default:
		fprintf(stderr, "bad option('%c')\n", ARGC());
		continue;
	}ARGEND
	USED(argc, argv);
	if (n == 0) {
		fprintf(stderr, "divider ratio (-n) zero\n");
		exits("n zero");
	}
	if (omega <= 0.0) {
		fprintf(stderr, "natural frequency (-o) zero\n");
		exits("omega zero");
	}
	if (chipname) {
	    for(i=0; chips[i].name; i++)
		if (strcmp(chips[i].name, chipname) == 0) {
			kVCO = chips[i].k_vco;
			kPD = chips[i].k_pd;
		}
	    if ((kVCO == 0.0) || (kPD == 0.0)) {
		fprintf(stderr, "%s not found\n", chipname);
		exits("not found");
	    }
	} /* end if */
	k = kVCO * kPD / (double) n;
	fprintf(stderr, "VCO gain constant = %f hz/v\n", kVCO / 2.0 / PI);
	fprintf(stderr, "Phase detector gain constant = %f hz/v\n", kPD);
	fprintf(stderr, "Natural loop frequency (omega) = %f hz\n", omega / 2.0 / PI);
	fprintf(stderr, "Reference frequency = %f hz \n", freq);
	fprintf(stderr, "Damping constant (zeta) = %f\n", zeta);
	fprintf(stderr, "Divider ratio = %d\n", n);
	if (AFlag) active();
	else
		passive();
	fprintf(stderr, "C = %f uF\n", cap * 1e6);
	fprintf(stderr, "R1 = %f\n", t1/cap);
	fprintf(stderr, "R2 = %f\n", t2/cap);
	if (Pflag) plotit();
	exits(0);
} /* end main */

void
passive(void)
{
	double a, b, c, d;
	a = k * k;
	b = 2.0 * k;
	c = 1.0 - 4.0 * zeta * zeta * k * k / omega / omega;
	d = sqrt(b * b - 4.0 * a * c);
	t2 = (-b+d) / 2.0 / a;
	while (t2 < 0.0) {
		omega = omega * 0.99; /* reduce 1% */
		c = 1.0 - 4.0 * zeta * zeta * k * k / omega / omega;
		d = sqrt(b * b - 4.0 * a * c);
		t2 = (-b+d) / 2.0 / a;
	} /* end while */
	fprintf(stderr, "New natural loop frequency (omega) = %f hz\n", omega / 2.0 / PI);
	t1 = k / omega / omega - t2;
} /* end passive */

void
active(void)
{
	t1 = k / omega / omega;
	t2 = 2.0 * zeta / omega;
} /* end active */

void
plotit(void)
{
	int i, limit;
	double f, t, l, a, b, x, y, kn;
	double magnitude[10], phase[10], noise[10], frequency[10];
	limit = ((int) ceil(log10(freq))) + 2;
	for (i=1; i<=limit; i++) {
		f = pow10(i) * 2.0 * PI;
		if (AFlag) {
			kn = kVCO * kPD;
			x = -kn / f / f / (double) n / t1;
			y = -kn * t2 / f / (double) n / t1;
		} /* end active */
		else {
			t = t1+t2;
			l = 1.0 + f*f*t*t;
			x = -t1 / l * k;
			y = -(1.0 + f*f*t2*t) / l * k / f;
		} /* end passive */
		rect_to_polar(x, y, &a, &b);
		magnitude[i] = 20.0 * log10(a);
		phase[i] = b * 180.0 / PI;
		noise[i] = 20.0 * log10(1.0 / hypot(1.0 - x, y));
		frequency[i] = f / 2.0 / PI;
	} /* end for */
	printf(".G1\n");
	printf("coord mag log x y -100,100 x 10,%d\n", (int) pow10(limit));
	printf("coord phase log x y -180,180 x 10,%d\n", (int) pow10(limit));
	printf("label right \"phase\" right .2\n");
	printf("label left \"magnitude\" left .2\n");
	printf("label bottom \"frequency\"\n");
	printf("ticks bot out from mag 10 to mag 100000 by *10\n");
	printf("ticks right out from phase -180 to 180 by 30\n");
	printf("ticks left out from mag -90 to 90 by 30\n");
	printf("draw magnitude solid\n");
	printf("\"magnitude\" ljust at mag %f, %f\n", frequency[1], magnitude[1]);
	for (i=1; i<=limit; i++)
		printf("next magnitude at mag %f, %f\n", frequency[i], magnitude[i]);
	printf("new phased dotted\n");
	printf("\"phase\" ljust at phase %f, %f\n", frequency[1], phase[1]);
	for (i=1; i<=limit; i++)
		printf("next phased at phase %f, %f\n", frequency[i], phase[i]);
	printf("new noise dashed\n");
	printf("\"noise\" ljust at mag %f, %f\n", frequency[1], noise[1]);
	for (i=1; i<=limit; i++)
		printf("next noise at mag %f, %f\n", frequency[i], noise[i]);
	printf(".G2\n");
} /* end plotit */

void
rect_to_polar(double x, double y, double *a, double *b)
{
	double h, s;
	h = hypot(x, y);
	s = 0;
	if (fabs(h) > 1e-6) s = acos(x / h);
	*b = (y < 0.0) ? -s : s;
	*a = h;
} /* end rect_to_polar */

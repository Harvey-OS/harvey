#include <u.h>
#include <libc.h>
#include	"mkpin.h"
#define	NLINE	1000
#define NARG	200
char	line[NLINE];
char	*fname;
char	*largv[NARG];
int	largc;
int	nline;

extern int errors;

int
rline(void)
{
	int i, c, prcntflg, whtflg;

	static int semiflg;

	if(!semiflg)
		nline++;
loop:
	prcntflg = semiflg = 0;
	whtflg = 1;
	largv[largc = 0] = 0;

	for(i=0; i<NLINE; i++) {
		c = fgetc(fi);
		if(c == EOF) {
			largv[largc=0] = 0;
			return(0);
		}
		if((c == '\n') || ((c == ';') && !prcntflg)) {
			if(c == ';')
				semiflg++;
			line[i] = 0;
			return(1);
		}
		if(c == '	') c = ' ';
		line[i] = c;
		if(c == ' ')
			whtflg++;
		else if(whtflg) {
			if(!prcntflg) {
				largv[largc++] = &line[i];
				if(largc >= NARG) {
					fprintf(stderr,"NARG too small");
					exit(1);
				}
			}
			whtflg = 0;
			if(c == '%')
				prcntflg++;
		}
	}
	fprintf(stderr,"NLINE (%d) too small\n", NLINE);
	for(;;) {
		c = fgetc(fi);
		if(c == EOF)
			return(1);
		if(c == '\n')
			goto loop;
	}
}

void
parse(void)
{
	int i;
	char *cp;

	largv[largc] = 0;
	for(i = 0; i < largc; i++) {
		cp = largv[i];
		if(*cp == '%')
			return;
		for( ; ;cp++) {
			if((*cp == ' ') || (*cp==0)) {	
				*cp = 0;
				break;
			}
		}
	}
}

void
diag(void)
{

	fprintf(stderr,"file: %s: line %d: ", fname, nline);
	++errors;
}


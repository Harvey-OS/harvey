#include <u.h>
#include <libc.h>
#include "mkpin.h"

#define NTOL	2000
#define NRL	250
char toline[NTOL];	/* place to save a .tp lines */
char restline[NRL];	/* place to save a DIP?? etc. */
int errors = 0;

/*
 * mkpins reads one or more .wx or .w files as input and produces
 * a "pins" file as output.  The method used is to scan the .wx files
 * for ".c" lines remembering the gate types in a symbol table.
 * The "flag" field of a symbol table entry is set to NEEDED
 * when the gate type is seen in a .c line.
 * The flag field is set to DEFINED
 * when found on a .t line.
 * .c lines must appear befor .t lines.
 *
 * Any ".tt" and ".tp" lines found in a library chip definition are copied
 * to stdout.
 */

void
main(int argc, char **argv)
{
	register struct hshtab *hp, *hhp;
	int undefcnt, ret, i;
	char b[DIRLEN];


	for(; argc>1; argc--,argv++) {
		fname = argv[1];
		if((fi = fopen(fname, "r")) == NULL) {
			fprintf(stderr, "mkchip: can't open %s\n", fname);
			exit(1);
		}
		nline = 0;
		while (rline()) {
alreadyp:
			if(line[0] != '.')
				continue;
			parse();
			if(line[2] != 0)
				continue;
			switch(line[1]) {
			case 'c':
			case 'o':
				if(largc < 3) {
					diag();
					fprintf(stderr, "bad .[co] line\n");
					continue;
				}
				hp = lookup(largv[2]);
				hp->flag |= INUSE | NEEDED;
				hp->bkwd = (struct hshtab *)0;
				continue;
			case 't':
				if(largc < 3) {
					diag();
					fprintf(stderr, "bad .t line\n");
					continue;
				}
				hp = lookup(largv[1]);
				if ((hp->flag & INUSE) == 0)
					/* it does not match one needed */
					continue;
				if(hp->flag & DEFINED)
					continue;
				hp->flag |= DEFINED;
				if ((largv[2][0] == '=') || (largv[2][1] == 0)) {
					if(largc < 4) {
						diag();
						fprintf(stderr, "bad .t = line\n");
						continue;
					}
					hhp = lookup(largv[3]);
					hhp->flag |= INUSE;
					hp->frwd = hhp;
					while(hhp->bkwd) {
						hhp = hhp->bkwd;
					}
					hhp->bkwd = hp;
					continue;
				}
				restline[0] = 0;
				for(i = 2; i < largc; i++) {
					strcat(restline, " ");
					strcat(restline, largv[i]);
					if(strlen(restline) >= NRL) {
				fprintf(stderr,"NRL too small,recompile\n");
							exit(1);
						}
				}
				toline[0] = 0;
				while(ret=rline()) {
					if(  line[0] == 0) continue;
					if ((line[0] == '.') &&(line[1]=='t')) {
						if(line[2]==' ')
							break;
						if(	(line[2]=='m') ||
							(line[2]=='n') ||
							(line[2]=='s') ||
							(line[2]=='x') ||
							(line[2]=='z'))
							continue;
					}
					strcat(toline, line);
					strcat(toline,"\n");
					if(strlen(toline) >= NTOL) {
				fprintf(stderr,"NTOL too small,recompile\n");
							exit(1);
					}
				}
				for( ; hp; hp = hp->bkwd) {
					if((hp->flag & NEEDED) == 0)
						continue;
					printf(".t %s%s\n",hp->name,restline);
					printf("%s\n",toline);
					hp->flag &= ~NEEDED;
				}
				if(ret)
					goto alreadyp;
				else
					break;
			}
		}
		fclose(fi);
	}
	/*
	 * print out missing chip definitions
	 * after processing last file.
	 */
	undefcnt = 0;
	for(hp=hshtab; hp< &hshtab[HSHSIZ]; hp++) {
		if (hp->flag & NEEDED) {
			restline[0] = 0;
			strcat(restline, hp->name);
			strcat(restline, ".w");
			if(stat(restline, b)>=0)
				hp->flag &= ~NEEDED;
			else
				undefcnt++;
		}
	}
	if(undefcnt) {
		fprintf(stderr,"%% undefined:\n");
		for(hp=hshtab; hp< &hshtab[HSHSIZ]; hp++) {
			if (hp->flag & NEEDED) {
				fprintf(stderr,".c dontcare %s\n", hp->name);
				hp->flag &= ~NEEDED;
				continue;
			}
		}
		exit(1);
	}
	exit(errors);
}

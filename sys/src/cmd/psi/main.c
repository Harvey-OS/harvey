#include <u.h>
#include <libc.h>
#include "system.h"
#ifdef Plan9
#include <libg.h>
#endif
#include "stdio.h"
#include "njerq.h"

char *outname;
int dontoutput=0, rotateflag = 0;
#ifdef FAX
extern long pg_count;
#endif
			/*
				psi - PostScript Interpreter
				By Jesse Kartus &
				   Chris Scussel
				modified & ammended by lorinda cherry
				(c) 1988 AT&T Bell Laboratories
			*/

#include "object.h"
extern int minx,miny,maxx,maxy;
char *rot="0  792 translate -90 rotate";
char usage[]="psi [-a x y] [-r] [-s] [files]\n\t-anchor ll to x,y -resolution_full -search";

main(int argc, char *argv[])
{
	struct object strobj;
	int	arg = 1, err,i;
	char	*filen=0, *s;
	char *cacheflg="-c";

	Prog_name = argv[0] ;
	anchor.x = anchor.y = 0;
	if(argc > 1){
		argv++;
		while(arg < argc){
		if(*argv[0] == '-'){
			switch(*(argv[0]+1)){
			case '?':
			default:
				fprintf(stderr,"%s\n",usage);
				exits("");
			case 'b':
				dontcache=1;
				bbflg = 1;
				break;
			case 'c': break;
			case 'd': cacheflg = argv[0];
				break;
			case 'D': dontcache=1;	/*don't cache characters*/
				break;
			case 'R': reversed = 1;
				break;
			case 'o':	/*output name for bitmap file*/
				outname = argv[1];
				arg++;
				argv++;
				break;
			case 'O': stdflag=1;
				break;
			case 'f':
				Fullbits = 1;
				break;
			case 'r':	/*full resolution for screen*/
				resolution = 1;
				break;
			case 'i':
				dontask = 1;
				break;
			case 'n':	/*don't initialize fonts*/
				nofontinit = 1;
				break;
			case 'N':
				dontoutput = 1;
				break;
			case 'a':	/*anchor lower left corner of window*/
				p_anchor.x = atoi(argv[1]);
				argv++;
				if(arg+2 >= argc){
					fprintf(stderr,"psi: -a not enough arguments\n");
					done(1);
				}
				p_anchor.y = atoi(argv[1]);
				argv++;
				arg+=2;
				resolution = 2;
				break;
			case 'p':	/*just do page n*/
				pageflag=1;
				page = atoi(argv[0]+2);
			case 'k':	/*get outline by clip clippath*/
				korean = 1;
				resolution = 1;
				/*dontask=1;*/
				fprintf(stderr,"begin font\n");
				break;
			case 'M':	/*just widths*/
				merganthal=1;	/*fall thru*/
					/*output merganthaler outlines*/
			case 'm':	/* point size 311.04 -r for scaling*/
					/*m1 for clippath type3*/
					/*m2 for clippath type1*/
				merganthal++;
				if(*(argv[0]+2) == '1')merganthal=3;
				if(*(argv[0]+2) == '2')merganthal=4;
				resolution = 1;
				dontask=1;
				fprintf(stderr,"begin font\n");
				break;
			case 's':
				searchflg = 1;
				break;
			case 'A':	/*for outlines -A after -m to vies*/
				dontask=0;
				break;
			case 'C':	/*output fontcache on showpage*/
				cacheout=1;
				if((fpcache = fopen("font.info","w")) == (FILE *)0){
					fprintf(stderr,"can't open font.info\n");
					done(0);
				}
				break;
			case 't':
				rotateflag = 1;
				break;
			}
			argv++;
			arg++;
			}
			else break;
		}
	}
	if(!outname)
		outname = "psi.out";
	init(cacheflg) ;
	if(rotateflag){
		strobj = makestring(strlen(rot));
		for(i=0,s=rot;i<strlen(rot); i++)
			strobj.value.v_string.chars[i] = *s++;
		strobj.xattr = XA_EXECUTABLE;
		execpush(strobj);
		execute();
	}
	if(arg < argc)while(arg++ < argc){
		filen = argv[0];
		argv++;
		strobj = makestring(strlen(filen));
		for(i=0,s=filen;i<strlen(filen); i++)
			strobj.value.v_string.chars[i] = *s++;
		push(strobj);
		runOP();
		execute();
	}
	else execute() ;
#ifdef Plan9
again:	if(!(merganthal||korean||dontask))if(ckmouse(2)==0){
		erasepageOP();
		restart=1;
		pageflag=1;
		if(reversed)pageno = 0;
		else pageno = 1000;
		prolog=0;
		strobj = makestring(strlen(filen));
		for(i=0,s=filen;i<strlen(filen); i++)
			strobj.value.v_string.chars[i] = *s++;
		push(strobj);
		runOP();
		execute();
		goto again;
	}
#endif
	if(!sawshow && minx<maxx && !(merganthal||korean))showpageOP();
#ifdef FAX
#ifndef SINGLE
	fixpgct();
#endif
#endif
	done(0) ;
}

/* CCITT.c - functions for CCITT compression/decompression of binary images.
   The following discussion is a summary of CCITT Recommendations T.4 and T.6
on facsimile coding schemes and coding control functions for Group 3 and Group
4 facsimile apparatus (drafted at Malaga-Torremolinos, 1984)
	The CCITT Group 3 FAX standard permits either 1-D or 2-D encoding.
It is not easy to tell from an encoding which was used.  Both 1-D and 2-D
assume fixed scanline length (in pixels), which must be known at encoding time.
	Group 3 1-D (g31) code uses modified Huffman codes for run-lengths, with
two different code tables for black and white.  Each line is assumed to begin
with a (possibly 0-length) white run.  Of course, the colors strictly alternate.
An empty (all-white) line is supposed to be spelled out with a full pixel count,
although some decoders won't complain if just a 0-length count is used.
There are provisions for fill bits at the end of lines to allow fixed-speed
receiving equipment to keep up.  Each line ends with an EOL code on which it
may be possible to synchronize after transmission error.
	Group 3 2-D (g32) encoding tries to exploit the slowly-changing nature
of artwork from line to line.  It is a mixture of 1-D-coded lines and
``2-D''-coded lines describing small local changes, relative to the prior 
``reference'' line.  1-D coding recurs every few (k) lines, so that
resynchronization is frequently possible.  K is usually 2 or 4, but the
standard permits any number, even infinity, since the coding method use on
a line is specified by which of two special EOLx codes is used to end the
previous line.
	CCITT Group 4 (g4) is like Group 3 2-D, and a lossless communications
channel is assumed.  Thus the first line is 2-D encoded (referenced to an
imaginary initial blank line), k is always infinity, and EOL's are not used at all.
It essential that the line-length be known in advance of both encoding and
decoding, so that line-breaks can be triggered when that length is exceeded.
End of FAX buffer (end of page) is signaled by a special EOFB code.
PERFORMANCE
   On 10 printed A4 pages (business letters, technical reports, etc),
at a resolution of 400 dpi:
	binary:		2020 Kbytes	CPU secs (to compress binary)
        pack:		 200 - 380	17
	bitfile(9.5):	 150 - 360	 ?
	rle:		 100 - 230	 7.5
        compress:	  90 - 155	14
	g31:		  85 - 190	13
	g32:		  55 - 110	13
	g4:		  30 -  70	13
   G4-compressed files are 15-40X smaller than binary, 5X than bitfile(9.5),
3.5X than rle, 2X than g31, and 2X than rle | pack.
*/
#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "../system.h"
#ifdef FAX
#include <ctype.h>
#include "CPU.h"
#include "stdocr.h"
#include "rle.h"
#include "../njerq.h"
#include "bitio.h"
#include "CCITT.h"

#define DEBUG	/* disables compilation of all debugging code;*/
#undef DEBUG	/* compiles but must enable particular parts below */

#ifdef DEBUG
#define MSGMAX 120
err(s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)
    char *s;
{   char m[MSGMAX];
	strcat(m,s);
	strcat(m,"\n");
	fprintf(stderr,m,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12);
	}
#endif


#ifdef TRASH
DST_table *ccitt_table()
{   DST_context cx;
	if((cx.t = (DST_table *)malloc(sizeof(DST_table)))==NULL)
		abort("CCITT.c: build_tbl: can't alloc cx.t");
	/* All tables have e[DST_white], e[DST_black], & e[DST_2d] entries */
	cx.t->mny=3;
	if((cx.t->e=(DST_entry *)malloc(cx.t->mny*sizeof(DST_entry)))==NULL)
		abort("CCITT.c: build_tbl: can't alloc cx.t->e[%d]",cx.t->mny);

	cx.s = cx.c = DST_white;
	cx.l = 0;
	cx.t->e[cx.s].p[0] = '\0';
	build_transits(cx);

	cx.s = cx.c = DST_black;
	cx.l = 0;
	cx.t->e[cx.s].p[0] = '\0';
	build_transits(cx);

	cx.s = cx.c = DST_2d;
	cx.l = 0;
	cx.t->e[cx.s].p[0] = '\0';
	build_transits(cx);

	return(cx.t);
	}
DST_state new_state(t)
   DST_table *t;
{	t->mny++;
	if((t->e=(DST_entry *)realloc(t->e,t->mny*sizeof(DST_entry)))==NULL)
		abort("can't realloc t->[%d]",t->mny);
	return(t->mny-1);
	}

/* The entry described by `cx' exists in the table, and its prefix `p' is
   setup; create its transitions, and their next entries, recursively.  */
build_transits(cx)
    DST_context cx;
#define entry cx.t->e[cx.s]
#define transit entry.t[col]
{   DST_color col;
    DST_entry ne;	/* next entry */
    DST_context ncx;	/* next Context */
    char **s,**ss;	/* for searching code */
    short *c,*cs;
    int si,matching,longer;
    char svch,col_str[2];
	switch(cx.c) {
	    case DST_white:
		ss=codewht;
		cs=bitcwht;
		break;
	    case DST_black:
		ss=codeblk;
		cs=bitcblk;
		break;
	    case DST_2d:
		ss=code2d;
		cs=bitc2d;
		break;
	    };
    	for(col=0;col<=1;col++) {
		sprintf(col_str,"%1d",col);
		ncx = cx;  ncx.l++; 
		/* build a transition */
		strcpy(ne.p,entry.p); strcat(ne.p,col_str);
		/* count those that match new prefix */
		longer=matching=0;
		for(s=ss,c=cs,si=0; (*s)!=NULL; s++,c++,si++) {
			if(*c==ncx.l) {
				if(strcmp(*s,ne.p)==0) {
					matching++;
					switch(cx.c) {
					    case DST_white:
					    case DST_black:
						transit.a = itor(si);
						break;
					    case DST_2d:
						transit.a = si;
						break;
					    };
					};
				}
			else if(*c>ncx.l) {
				svch=(*s)[ncx.l]; (*s)[ncx.l] = '\0';
				if(strcmp(*s,ne.p)==0) {
					longer++;
					};
				(*s)[ncx.l]=svch;
				};
			};
		/* analyze results */
		if(matching==0&&longer>0) {
			/* no match yet; go deeper */
			transit.a = DST_action_NULL;
			ncx.s = transit.s = new_state(cx.t);
			strcpy(cx.t->e[ncx.s].p,ne.p);
			build_transits(ncx);
			}
		else if(matching==1&&longer==0) {
			/* unique leaf: good */
			/* picked up action earlier */
			switch(cx.c) {
			    case DST_white:
			    case DST_black:
				if(transit.a<=63) /* termination code */
					transit.s = flip_color(cx.c); 
				else /* makeup code */
					transit.s = cx.c;
				break;
			    case DST_2d:
				/* legal end of code */
				transit.s = DST_state_NULL;
				break;
			    };
			}
		else {	/* illegal transition */
			transit.a = DST_action_ERROR;
			transit.s = DST_state_NULL;
			};
		};
	}

ccitt_err_tbl(t)
    DST_table *t;
{   int d;
	ccitt_err_state(DST_white,t);
	ccitt_err_state(DST_black,t);
	ccitt_err_state(DST_2d,t);
	}

ccitt_err_state(s,t)
    DST_state s;
    DST_table *t;
{	err("%03d %s%*s %d %03d,%-4d  %d %03d,%-4d",
			s,t->e[s].p,14-strlen(t->e[s].p)," ",
			0,t->e[s].t[0].s,t->e[s].t[0].a,
			1,t->e[s].t[1].s,t->e[s].t[1].a
			);
	if(t->e[s].t[0].s>1) ccitt_err_state(t->e[s].t[0].s,t);
	if(t->e[s].t[1].s>1) ccitt_err_state(t->e[s].t[1].s,t);
	}
#endif
#ifdef TRASH
/* Translate a stream of bits in CCITT FAX Group 3 (1-D) compression format
   into a sequence of RLE_Lines.  Returns one (RLE_Line *) on each call (or NULL
   if EOF or error).  The first pixel (black or white) in each g31 line is
   assigned run index 0. */
RLE_Line *g31_to_rlel(DST_table *t,BITFILE *f,boolean bof)
#define dbg_g31r_r (0)	/* trace each run/EOL/ERR_SYN */
#define dbg_g31r_c (0)	/* trace each Huffman code (or, fill+EOL)*/
#define dbg_g31r_t (0)	/* trace each state-transition */
{   static DST_context cx;
    static RLE_Line rl;
    static RLE_Run *r;	/* prior, current runs */
    int run,biti,sync,si,bitv;
    boolean fill;
/* color of 1st run in each line */
#define g31_first_color DST_white
/* reverse Black/White in output image */
#define g31_negative 0
	if(bof){if(T) {	/* sync by skipping initial FILL & EOL code */
			sync=0;  while((bitv=getb(f))==0) sync++;
			if(bitv!=1||sync<11) {
#ifdef DEBUG
				if(dbg_g31r_c) fprintf(stderr,"BOF_ERR\n");
#endif
				return(NULL);
				}
			else if(dbg_g31r_c){
				fprintf(stderr,"BOF_EOL ");
				for(si=0;si<sync;si++) fprintf(stderr,"0");
				fprintf(stderr,"1\n");
				};
			};
		/* start file expecting a run-code of a fixed color */
		cx.s = cx.c = g31_first_color;
		rl.y = -1;	/* assume first line is y==0 */
		};
	rl.y++;  rl.runs=0;  r = rl.r;  biti=run=0;
	while(T) switch(getb(f)) {
	    case 0 :  /* next bit is 0 */
		switch(t->e[cx.s].t[0].a) {
		    case DST_action_ERROR:
			/* bad code: try to resynchronize */
			if(dbg_g31r_t){
				fprintf(stderr,"%s %04d  %s0?...\n",
					(cx.c==DST_white)? "W": "B",
					t->e[cx.s].t[0].s,
					t->e[cx.s].p);
				};
			/* count trailing 0's so far (should be in table) */
			sync=1;
			si=strlen(t->e[cx.s].p)-1;
			while(si>=0&&t->e[cx.s].p[si]=='0') {sync++; si--;};
			fill = (si<0);	/* all 0's:  may be fill bits */
			if(dbg_g31r_c)fprintf(stderr,"ERR_SYN %s0?",t->e[cx.s].p);
			while(sync<11) {
				switch(bitv=getb(f)) {
					case 0:  sync++;
#ifdef DEBUG
						if(dbg_g31r_c) fprintf(stderr,"0");
#endif
						break;
					case 1:  sync=0;  fill=F;
#ifdef DEBUG
						if(dbg_g31r_c) fprintf(stderr,"1");
#endif
						break;
					case EOF:
#ifdef DEBUG
						if(dbg_g31r_c) fprintf(stderr,"<EOF>\n");
#endif
						return(NULL);  break;
					};
				};
			/* next `1' will synchronize */
			do {	switch(bitv=getb(f)) {
					case 0:
#ifdef DEBUG
						if(dbg_g31r_c) fprintf(stderr,"0");
#endif
						break;
					case 1:
#ifdef DEBUG
						if(dbg_g31r_c) fprintf(stderr,"1");
#endif
						break;
					case EOF:
#ifdef DEBUG
						if(dbg_g31r_c) fprintf(stderr,"<EOF>\n");
#endif
						return(NULL);  break;
					};
				}
			while(bitv!=1);
#ifdef DEBUG
			if(dbg_g31r_c) fprintf(stderr,"\n");
			if(dbg_g31r_r) {
				if(fill) fprintf(stderr,"FILL_EOL\n");
				else fprintf(stderr,"ERR_SYN_EOL\n");
				};
#endif
			/* start next line expecting a run-code of a fixed color */
			cx.s = cx.c = g31_first_color;
			return(&rl);
			break;
		    case DST_action_NULL:
#ifdef DEBUG
			if(dbg_g31r_t)fprintf(stderr,"%s %04d  %s0\n",
				(cx.c==DST_white)? "W": "B",
				t->e[cx.s].t[0].s,
				t->e[cx.s].p);
#endif
			cx.s = t->e[cx.s].t[0].s;
			break;
		    case DST_EOL:
#ifdef DEBUG
			if(dbg_g31r_c)fprintf(stderr,"EOL     %s0\n",
				t->e[cx.s].p);
			if(dbg_g31r_r)fprintf(stderr,"EOL\n");
#endif
			/* start next line expecting a run-code of a fixed color */
			cx.s = cx.c = g31_first_color;
			return(&rl);
			break;
		    default:
#ifdef DEBUG
			if(dbg_g31r_c)fprintf(stderr,"%s%6d %s0\n",
				(cx.c==DST_white)? "W": "B",
				t->e[cx.s].t[0].a,
				t->e[cx.s].p);
#endif
			if(t->e[cx.s].t[0].a<=63) {
				run += t->e[cx.s].t[0].a;
#ifdef DEBUG
				if(dbg_g31r_r)fprintf(stderr,"%s%6d\n",
					(cx.c==DST_white)? "W": "B",
					run);
#endif
				biti += run;
				if(cx.c==DST_black^g31_negative) {
					/* end of black run */
					r->xe = biti-1;
					r++; rl.runs++;
					}
				else {	/* end of white run */
					r->xs = biti;
					};
				cx.c = flip_color(cx.c);
				run = 0;
				}
			else run += t->e[cx.s].t[0].a;
			cx.s = t->e[cx.s].t[0].s;
			break;
		    };
		break;
	    case 1:  /* next bit is 1 */
		switch(t->e[cx.s].t[1].a) {
		    case DST_action_ERROR:
			/* bad code: try to resynchronize */
#ifdef DEBUG
			if(dbg_g31r_t){
				fprintf(stderr,"%s %04d  %s1?...\n",
					(cx.c==DST_white)? "W": "B",
					t->e[cx.s].t[1].s,
					t->e[cx.s].p);
				};
			if(dbg_g31r_c)fprintf(stderr,"ERR_SYN %s1?",t->e[cx.s].p);
#endif
			/* no trailing 0's; can't be fill bits */
			sync=0;
			while(sync<11) {
				switch(bitv=getb(f)) {
					case 0:  sync++;
#ifdef DEBUG
						if(dbg_g31r_c) fprintf(stderr,"0");
#endif
						break;
					case 1:  sync=0;
#ifdef DEBUG
						if(dbg_g31r_c) fprintf(stderr,"1");
#endif
						break;
					case EOF:
#ifdef DEBUG
						if(dbg_g31r_c)
							fprintf(stderr,"<EOF>\n");
#endif
						return(NULL);  break;
					};
				};
			/* next `1' will synchronize */
			do {	switch(bitv=getb(f)) {
					case 0:
#ifdef DEBUG
						if(dbg_g31r_c) fprintf(stderr,"0");
#endif
						break;
					case 1:
#ifdef DEBUG
						if(dbg_g31r_c) fprintf(stderr,"1");
#endif
						break;
					case EOF:
#ifdef DEBUG
						if(dbg_g31r_c)
							fprintf(stderr,"<EOF>\n");
#endif
						return(NULL);  break;
					};
				}
			while(bitv!=1);
#ifdef DEBUG
			if(dbg_g31r_c) fprintf(stderr,"\n");
			if(dbg_g31r_r) fprintf(stderr,"ERR_SYN_EOL\n");
#endif
			/* start next line expecting a run-code of a fixed color */
			cx.s = cx.c = g31_first_color;
			return(&rl);
			break;
		    case DST_action_NULL:
#ifdef DEBUG
			if(dbg_g31r_t)fprintf(stderr,"%s %04d  %s1\n",
				(cx.c==DST_white)? "W": "B",
				t->e[cx.s].t[1].s,
				t->e[cx.s].p);
#endif
			cx.s = t->e[cx.s].t[1].s;
			break;
		    case DST_EOL:
#ifdef DEBUG
			if(dbg_g31r_c)fprintf(stderr,"EOL     %s1\n",
				t->e[cx.s].p);
			if(dbg_g31r_r)fprintf(stderr,"EOL\n");
#endif
			/* start next line expecting a run-code of a fixed color */
			cx.s = cx.c = g31_first_color;
			return(&rl);
			break;
		    default:
#ifdef DEBUG
			if(dbg_g31r_c)fprintf(stderr,"%s%6d %s1\n",
				(cx.c==DST_white)? "W": "B",
				t->e[cx.s].t[1].a,
				t->e[cx.s].p);
#endif
			if(t->e[cx.s].t[1].a<=63) {
				run += t->e[cx.s].t[1].a;
#ifdef DEBUG
				if(dbg_g31r_r)fprintf(stderr,"%s%6d\n",
					(cx.c==DST_white)? "W": "B",
					run);
#endif
				biti += run;
				if(cx.c==DST_black^g31_negative) {
					/* end of black run */
					r->xe = biti-1;
					r++; rl.runs++;
					}
				else {	/* end of white run */
					r->xs = biti;
					};
				cx.c = flip_color(cx.c);
				run = 0;
				}
			else run += t->e[cx.s].t[1].a;
			cx.s = t->e[cx.s].t[1].s;
			break;
		    };
		break;
	    case EOF:
		return(NULL);
		break;
	    default:
		return(NULL);
		break;
	    };
	/* never come here:  return() variously from cases above */
	}
#endif

#ifdef G31
/* Translate a sequence of RLE_Line's (describing a binary image)
   into a file (a stream of bits) in CCITT FAX Group 3 (1-D) compression format.
   BOF_to_g31() must be called first; then call rlel_to_g31() for each line
   (including blank lines); finally, EOF_to_g31() must be called.  Each line's
   EOL and the RTC's first EOL are padded so they end on a byte boundary.
   */
/* debugging flags:  trace to stderr */
#define dbg_rg31_e (0)	/* entry */
#define dbg_rg31_r (0)	/* runs */
#define dbg_rg31_s (0)	/* bitstrings */

#ifdef DEBUG
#define bits_g31(bits) { \
	cs=(bits); while(*cs!='\0') {putb(*cs-'0',f); cs++;};  \
	if(dbg_rg31_s) fprintf(stderr,"%s",(bits)); \
	if(dbg_rg31_r) fprintf(stderr," "); \
	}
#define EOL_g31 { \
	if(dbg_rg31_r) fprintf(stderr,"EOL     "); \
	bits_g31(EOLSTRING); \
	if(dbg_rg31_r) fprintf(stderr,"\n"); \
	}
#else
#define bits_g31(bits) { \
	cs=(bits); while(*cs!='\0') {putb(*cs-'0',f); cs++;};  \
	}
#define EOL_g31 { \
	bits_g31(EOLSTRING); \
	}
#endif
void
BOF_to_g31(BITFILE *f)
{   char *cs;
	EOL_g31;
	}
void
rlel_to_g31(RLE_Line *rl,int wid,BITFILE *f)
			/* line of runs:  if NULL, then blank */
			/* width of an output line in pixels */
			/* state of output bitfile */
{   int pi;		/* input pixel index on line */
    RLE_Run *rp,*pp,*sp;
    int runl,codi;
    char *cs,*p01;
#ifdef DEBUG
#define Wrun_g31(rn) { \
	runl=(rn); \
	if(dbg_rg31_r) fprintf(stderr,"W %5d ",runl); \
	while(runl>2560) {p01=codewht[rtoi(2560)]; bits_g31(p01); runl-=2560;}; \
	p01=codewht[codi=rtoi(runl)]; bits_g31(p01); \
	if(codi>=64) {p01=codewht[runl%64]; bits_g31(p01);}; \
	if(dbg_rg31_r) fprintf(stderr,"\n"); \
	}
#else
#define Wrun_g31(rn) { \
	runl=(rn); \
	while(runl>2560) {p01=codewht[rtoi(2560)]; bits_g31(p01); runl-=2560;}; \
	p01=codewht[codi=rtoi(runl)]; bits_g31(p01); \
	if(codi>=64) {p01=codewht[runl%64]; bits_g31(p01);}; \
	}
#endif
#ifdef DEBUG
#define Brun_g31(rn) { \
	runl=(rn); \
	if(dbg_rg31_r) fprintf(stderr,"B %5d ",runl); \
	while(runl>2560) {p01=codeblk[rtoi(2560)]; bits_g31(p01); runl-=2560;}; \
	p01=codeblk[codi=rtoi(runl)]; bits_g31(p01); \
	if(codi>=64) {p01=codeblk[runl%64]; bits_g31(p01);}; \
	if(dbg_rg31_r) fprintf(stderr,"\n"); \
	}

#else
#define Brun_g31(rn) { \
	runl=(rn); \
	while(runl>2560) {p01=codeblk[rtoi(2560)]; bits_g31(p01); runl-=2560;}; \
	p01=codeblk[codi=rtoi(runl)]; bits_g31(p01); \
	if(codi>=64) {p01=codeblk[runl%64]; bits_g31(p01);}; \
	}
#endif
#ifdef DEBUG
	if(dbg_rg31_e) err("rlel_to_g31(rl[y%d,r%d])",rl->y,rl->runs);
#endif
	if(rl!=NULL&&rl->runs>0) {
		pi=0;	/* bit to write next */
#ifdef DEBUG
		if(dbg_rg31_e) err("rlel_to_g31(rl[y%d,r%d])",rl->y,rl->runs);
#endif
		for(sp=(rp=rl->r)+rl->runs; rp<sp; rp++) {
			Wrun_g31(rp->xs-pi);  pi=rp->xs;
			Brun_g31(rp->xe-pi+1);  pi=rp->xe+1;
			};
		if((--rp)->xe+1<wid) Wrun_g31(wid-rp->xe-1);
		}
	else {	
#ifdef DEBUG
		if(dbg_rg31_e) err("rlel_to_g31(rl[y?,r0])");
#endif
		Wrun_g31(wid);  /* blank (all-white) scanline */
		};
	/* fill so that EOL ends on byte boundary */
	padb(f,0,8,EOLLENGTH);
#ifdef DEBUG
	if(dbg_rg31_s) fprintf(stderr,"+0?");
#endif
	EOL_g31;
	}
void
EOF_to_g31(BITFILE *f)
{   char *cs;
	/* fill so that EOL ends on byte boundary */
	padb(f,0,8,EOLLENGTH);
#ifdef DEBUG
	if(dbg_rg31_s) fprintf(stderr,"+0?");
#endif
	/* write RTC */
	EOL_g31;
	EOL_g31;
	EOL_g31;
	EOL_g31;
	EOL_g31;
	EOL_g31;
	}
#endif
#ifdef TRASH
/* Macro for use within g32_to_rlel, to read one 1-D Modified Huffman coded
   run-length of a given color, placing the result in a given variable.
   Exceptionally, goto trap_eol, trap_eof, or trap_code_err.
  */
#ifdef DEBUG
#define g32_1d_run(C,V) { \
	run = 0; \
	do {	cx.tr.s = (C); \
	        do {	px = cx; \
			switch(bitv=getb(f)) { \
			    case 0 :  cx.tr=t->e[cx.tr.s].t[0];  break; \
			    case 1 :  cx.tr=t->e[cx.tr.s].t[1];  break; \
			    case EOF :  goto trap_eof;  break; \
			    }; \
			} \
		while(cx.tr.a==DST_action_NULL); \
		switch(cx.tr.a) { \
		    case DST_EOL : \
			if(dbg_g32r_c)fprintf(stderr,"EOL     %s\n",EOLSTRING); \
			goto trap_eol; \
			break; \
		    case DST_action_ERROR :  goto trap_code_err;  break; \
		    default : \
			if(dbg_g32r_c)fprintf(stderr,"%s%6d %s%d\n", \
				((C)==DST_white)? "W": "B", \
				cx.tr.a, \
				t->e[px.tr.s].p, \
				bitv); \
			run += cx.tr.a; \
			break; \
		    }; \
		} \
	while(cx.tr.a>63); \
	(V) = run; \
	}
#else
#define g32_1d_run(C,V) { \
	run = 0; \
	do {	cx.tr.s = (C); \
	        do {	px = cx; \
			switch(bitv=getb(f)) { \
			    case 0 :  cx.tr=t->e[cx.tr.s].t[0];  break; \
			    case 1 :  cx.tr=t->e[cx.tr.s].t[1];  break; \
			    case EOF :  goto trap_eof;  break; \
			    }; \
			} \
		while(cx.tr.a==DST_action_NULL); \
		switch(cx.tr.a) { \
		    case DST_EOL : \
			goto trap_eol; \
			break; \
		    case DST_action_ERROR :  goto trap_code_err;  break; \
		    default : \
			run += cx.tr.a; \
			break; \
		    }; \
		} \
	while(cx.tr.a>63); \
	(V) = run; \
	}
#endif

/* Translate a stream of bits in CCITT FAX Group 3 (2-D) compression format
   into a sequence of RLE_Lines.  Returns one (RLE_Line *) on each call (or NULL
   if EOF or error).  The first pixel (black or white) in each g32 line is
   assigned run index 0. */
RLE_Line *g32_to_rlel(DST_table *t,BITFILE *f,boolean bof)
#define dbg_g32r_e (0)	/* entry/exit */
#define dbg_g32r_r (0)	/* trace each run/EOL/ERR_SYN */
#define dbg_g32r_c (0)	/* trace each Huffman code (or, fill+EOL)*/
#define dbg_g32r_t (0)	/* trace each state-transition */
#define g32r_strict	/* 1 is CORRECT: explicitly code the last black pel */
{   static RLE_Line rl0,rl1,*prl,*crl;	/* prior, current run-lines */
    RLE_Line *swrl;
    int bitv;		/* the last-read bit value */
    DST_context cx,px;	/* the current/prior decoding context */
    RLE_Run *cr,*pr,*pre;	/* into current/prior rle lines */	
    RLE_Run *pra0;	/* rightmost in prior line with xe<=a0 (if none: prl->r) */
    int run,sync,si,rtc_eols;
    boolean fill;
    /* pixel indices (0,1,...):  current-line a*; prior-line b*.
       a0 is the index of the most recently completely encoded bit */
    int a0,a1,a2,b1,b2;	
    DST_color a0_color;  /* a0's color:  same as a2 & b2, opposite of a1 & b1 */
    int a01,a12;	 /* lengths of runs a0-a1 & a1-a2 */
#define g32_first_color DST_white	/* color of 1st run in each line */
#define g32_negative (0)	/* if 1, invert Black/White on output */
#define swap_rl(f,b) {swrl=(f); (f)=(b); (b)=swrl;}
/* detect b1 & b2:  sensitive to a0, a0_color, and prior runs *pr *(pr+1) */
#define g32r_find_Bb1Wb2 { \
	/* find 1st black changing pel>a0 */ \
	/* advance pra0 as far as possible s.t. pra0->xe<=a0 */ \
	while((pra0+1)<pre && (pra0+1)->xe<=a0) pra0++; \
	/* look beyond pra0 */ \
	pr=pra0;  while(pr<pre && (b1=pr->xs)<=a0) pr++; \
	/* move b2 to 1st changing white pel > b1 */ \
	if(pr<pre) b2=pr->xe+1; \
	else b1=b2=prl->len; \
	}
#define g32r_find_Wb1Bb2 { \
	/* find 1st white changing pel>a0 */ \
	/* advance pra0 as far as possible s.t. pra0->xe<=a0 */ \
	while((pra0+1)<pre && (pra0+1)->xe<=a0) pra0++; \
	/* look beyond pra0 */ \
	pr=pra0;  while(pr<pre && (b1=pr->xe+1)<=a0) pr++; \
	/* move b2 to 1st changing black pel > b1 */ \
	if(pr<pre) { \
		if((pr+1)<pre) b2=(pr+1)->xs; \
		else b2=prl->len; \
		} \
	else b1=b2=prl->len; \
	}
#define g32r_find_b1b2 {if(a0_color==DST_white) g32r_find_Bb1Wb2 else g32r_find_Wb1Bb2;}

if(dbg_g32r_e) fprintf(stderr,"g32_to_rlel(t,bf,bof%d)\n",bof);
if(bof){crl=&rl0;  crl->y=-1;  crl->len=0;  crl->runs=0;
	prl=&rl1;  prl->y=-1;  prl->len=0;  prl->runs=0;
	/* sync by skipping initial FILL & EOL code */
	sync=0;  while((bitv=getb(f))==0) sync++;
	if(bitv!=1||sync<11) {
#ifdef DEBUG
		if(dbg_g32r_c)fprintf(stderr,"BOF_ERR\n");
#endif
		return(NULL);
		}
#ifdef DEBUG
	else if(dbg_g32r_c){
		fprintf(stderr,"BOF_EOL ");
		for(si=0;si<sync;si++) fprintf(stderr,"0");
		fprintf(stderr,"1\n");
		};
#else
		;
#endif
	prl->y = -1;  crl->y = 0;
	}
else crl->y = prl->y + 1;

pre = (pra0=pr=prl->r) + prl->runs;	/* prior line */
crl->runs=0;  cr=crl->r-1;		/* current line */
/* start on an imaginary white pixel just to left of margin */
a0=-1;  a0_color = DST_white;	

/* check 1-D / 2-D bit immediately after prior EOL */
switch(bitv=getb(f)) {
    case 0 :  /* 2-dimensionally encoded line */
#ifdef DEBUG
	if(dbg_g32r_c) fprintf(stderr,"2D_LINE 0\n");
#endif
	/* start b1/b2 on prior line's first black pixel, etc;
	   if none, then place off end of line */
	if(pr<pre) {b1=pr->xs; b2=pr->xe+1;} else b1=b2=prl->len;
	/* parse a sequence of 2D codes... */
	while(T/* exited only via goto trap_* and return */) {
		cx.tr.s = DST_2d;  cx.tr.a = DST_action_NULL;
#ifdef DEBUG
		if(dbg_g32r_t)fprintf(stderr,"(%d,%d)\n",cx.tr.s,cx.tr.a);
#endif
        	do {	switch(bitv=getb(f)) {
			    case 0 :
			    case 1 :
				cx.tr=t->e[cx.tr.s].t[bitv];
				break;
			    case EOF:  goto trap_eof;  break;
			    };
#ifdef DEBUG
			if(dbg_g32r_t)fprintf(stderr,"%d->(%d,%d)\n",
					bitv,cx.tr.s,cx.tr.a);
#endif
			}
		while(cx.tr.a==DST_action_NULL);
#ifdef DEBUG
		if(dbg_g32r_t)fprintf(stderr,"%s %04d  %s0\n",
			(cx.c==DST_white)? "W": "B",
			cx.tr.s,
			t->e[cx.tr.s].p);
#endif
		switch(cx.tr.a) {
		    case i2D_V0:
#ifdef DEBUG
			if(dbg_g32r_c)
				fprintf(stderr,"V0      %s\n",code2d[cx.tr.a]);
#endif
			a1=b1;
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black)
				{ crl->runs++;  (++cr)->xs = a0;  cr->xe = a0; };
#ifdef g32r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g32r_find_b1b2;
			break;
		    case i2D_VR1:
#ifdef DEBUG
			if(dbg_g32r_c)
				fprintf(stderr,"VR1     %s\n",code2d[cx.tr.a]);
#endif
			a1=b1+1;
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black)
				{ crl->runs++;  (++cr)->xs = a0;  cr->xe = a0; };
#ifndef g32r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g32r_find_b1b2;
			break;
		    case i2D_VR2:
#ifdef DEBUG
			if(dbg_g32r_c)
				fprintf(stderr,"VR2     %s\n",code2d[cx.tr.a]);
#endif
			a1=b1+2;
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black)
				{ crl->runs++;  (++cr)->xs = a0;  cr->xe = a0; };
#ifndef g32r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g32r_find_b1b2;
			break;
		    case i2D_VR3:
#ifdef DEBUG
			if(dbg_g32r_c)
				fprintf(stderr,"VR3     %s\n",code2d[cx.tr.a]);
#endif
			a1=b1+3;
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black)
				{ crl->runs++;  (++cr)->xs = a0;  cr->xe = a0; };
#ifndef g32r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g32r_find_b1b2;
			break;
		    case i2D_VL1:
#ifdef DEBUG
			if(dbg_g32r_c)
				fprintf(stderr,"VL1     %s\n",code2d[cx.tr.a]);
#endif
			if((a1=b1-1)<0) err("g32_to_rlel: VL1 backs up to %d",a1);
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black)
				{ crl->runs++;  (++cr)->xs = a0;  cr->xe = a0; };
#ifndef g32_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g32r_find_b1b2;
			break;
		    case i2D_VL2:
#ifdef DEBUG
			if(dbg_g32r_c)
				fprintf(stderr,"VL2     %s\n",code2d[cx.tr.a]);
#endif
			if((a1=b1-2)<0) err("g32_to_rlel: VL2 backs up to %d",a1);
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black)
				{ crl->runs++;  (++cr)->xs = a0;  cr->xe = a0; };
#ifndef g32r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g32r_find_b1b2;
			break;
		    case i2D_VL3:
#ifdef DEBUG
			if(dbg_g32r_c)
				fprintf(stderr,"VL3     %s\n",code2d[cx.tr.a]);
#endif
			if((a1=b1-3)<0) err("g32_to_rlel: VL3 backs up to %d",a1);
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black)
				{ crl->runs++;  (++cr)->xs = a0;  cr->xe = a0; };
#ifndef g32r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g32r_find_b1b2;
			break;
		    case i2D_PASS:
#ifdef DEBUG
			if(dbg_g32r_c)
				fprintf(stderr,"PASS    %s\n",code2d[cx.tr.a]);
#endif
			/* move a0 to b2; no change of color */
			a0=b2;	if(a0>=prl->len) goto trap_expecting_eol;
			if(a0_color==DST_black) cr->xe = a0;
#ifndef g32r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g32r_find_b1b2;
			break;
		    case i2D_HORIZ:
#ifdef DEBUG
			if(dbg_g32r_c)
				fprintf(stderr,"HORIZ   %s\n",code2d[cx.tr.a]);
#endif
			if(a0_color==DST_white) {
				if(a0<0) a0=0;	/* first run in line starts at 0 */
				g32_1d_run(DST_white,a01);  a1 = a0 + a01;
				g32_1d_run(DST_black,a12);  a2 = a1 + a12;
				if(a12>0) /* Black run of >0 length */ {
					crl->runs++;
					(++cr)->xs = a1;
					cr->xe = a2-1;
					};
				a0 = a2;	/* still white */
				if(a0>=prl->len) goto trap_expecting_eol;
				/* encode a0 */
#ifndef g32r_strict
				if(a0==prl->len-1)
					{ a0++;  goto trap_expecting_eol; };
#endif
				g32r_find_Bb1Wb2;
				}
			else {	g32_1d_run(DST_black,a01);  a1 = a0 + a01;
				g32_1d_run(DST_white,a12);  a2 = a1 + a12;
				if(a01>0) /* Black run of >0 length */ {
					cr->xe = a1 - 1;
					}
				else {	/* 0-length: very peculiar: ignore */
					fprintf(stderr,
					   "g32_to_rlel: HORIZ B%d! W%d - ignore\n",
					   a01,a12);
					cr--; crl->runs--;
					};
				a0 = a2;	/* still black */
				if(a0>=prl->len) goto trap_expecting_eol;
				/* encode a0 */
				crl->runs++;  (++cr)->xs = a0;
#ifndef g32r_strict
				if(a0==prl->len-1) {
					cr->xe = a0;
					a0++;
					goto trap_expecting_eol;
					};
#endif
				g32r_find_Wb1Bb2;
				};
			break;
		    case i2D_EOL:
#ifdef DEBUG
			if(dbg_g32r_c)
				fprintf(stderr,"EOL     %s\n",code2d[cx.tr.a]);
#endif
			goto trap_eol;
			break;
		    case DST_action_ERROR:  goto trap_code_err;  break;
		    };
		};
	break;
    case 1 :  /* 1-dimensionally encoded line */
#ifdef DEBUG
	if(dbg_g32r_c) fprintf(stderr,"1D_LINE 1\n");
#endif
	/* read a sequence of 1-D runcodes... */
	while(T/* exit only via goto trap_X */) {
		g32_1d_run(a0_color,a01);
		a1 = ((a0>=0)? a0 : 0) + a01;
		if(a01>0) {
			/* encode a0 through a1-1 */
			if(a0_color==DST_black^g32_negative) {
				/* output-black run */
				crl->runs++;
				(++cr)->xs=((a0>=0)? a0 : 0);
				cr->xe=a1-1;
				};
			};
		a0=a1;
		a0_color=flip_color(a0_color);
		if(prl->len>0 && a0>=prl->len) goto trap_expecting_eol;
		};
	break;
    case EOF :  goto trap_eof;  break;
    };

/* come here via goto's: all these traps return() */

trap_expecting_eol:	/* come here expecting to see EOL or FILL+EOL */
	sync=0;  while((bitv=getb(f))==0) sync++;
	switch(bitv) {
	    case 1:
		if(sync==11) {
#ifdef DEBUG
			if(dbg_g32r_c){
				fprintf(stderr,"EOL     %s\n",EOLSTRING);
				};
#endif
			goto trap_eol;
			}
		else if(sync>11) {
#ifdef DEBUG
			if(dbg_g32r_c){
				fprintf(stderr,"FILLEOL ");
				for(si=0;si<sync-11;si++) fprintf(stderr,"0");
				fprintf(stderr,"+");
				fprintf(stderr,"%s\n",EOLSTRING);
				};
#endif
			goto trap_eol;
			}
		else {	
#ifdef DEBUG
			if(dbg_g32r_c){
				fprintf(stderr,"NOT EOL ");
				for(si=0;si<sync;si++) fprintf(stderr,"0");
				fprintf(stderr,"1?");
				};
#endif
			sync=0;
			goto trap_eol_err;
			};
		break;
	    case EOF:  goto trap_eof;  break;
	    };
	goto trap_eol;

trap_code_err:
	/* unexpected coding sequence:
	   'px' holds last decoding context & 'bitv' latest bit value;
	   will attempt to resynchronize on next EOL */
#ifdef DEBUG
	if(dbg_g32r_c)fprintf(stderr,"CODERR  %s%d?",
				t->e[px.tr.s].p,bitv);
#endif
	/* count trailing 0's so far (should be in table) */
	if(bitv==0) sync=1; else sync=0;
	si=strlen(t->e[px.tr.s].p)-1;
	while(si>=0&&t->e[px.tr.s].p[si]=='0') {sync++; si--;};
	fill = (si<0);	/* all 0's:  may be fill bits */
trap_eol_err:
	while(sync<11) {
		switch(bitv=getb(f)) {
			case 0:  sync++;
#ifdef DEBUG
				if(dbg_g32r_c) fprintf(stderr,"0");
#endif
				break;
			case 1:  sync=0;
#ifdef DEBUG
				if(dbg_g32r_c) fprintf(stderr,"1");
#endif
				break;
			case EOF:  goto trap_eof;  break;
			};
		};
	/* next `1' will synchronize */
	do {	switch(bitv=getb(f)) {
			case 0:
#ifdef DEBUG
				if(dbg_g32r_c) fprintf(stderr,"0");
#endif
				break;
			case 1:
#ifdef DEBUG
				if(dbg_g32r_c) fprintf(stderr,"1");
#endif
				break;
			case EOF:  goto trap_eof;  break;
			};
		}
	while(bitv!=1);
#ifdef DEBUG
	if(dbg_g32r_c) fprintf(stderr," EOL\n");
#endif
	goto trap_eol;

trap_eol:	/* come here having seen (and reported) EOL */
	/* learn/check line-length */
	if(a0>=0) crl->len=a0; else crl->len=0;
	if(crl->len==0) {
		/* no pixels coded -- may be the 1st of 6 RTC EOLs */
		/* check suffix 1 bit */
		if((bitv=getb(f))!=1) goto trap_eol_err;
		rtc_eols=1;
#ifdef DEBUG
		if(dbg_g32r_c)fprintf(stderr,"RTC_EOL +1 (%d)\n",rtc_eols);
#endif
		do {	sync=0;  while((bitv=getb(f))==0) sync++;
			switch(bitv) {
			    case 1:
				if(sync<11) {
#ifdef DEBUG
					if(dbg_g32r_c){
						fprintf(stderr,"NOT RTC ");
						for(si=0;si<sync;si++)
							fprintf(stderr,"0");
						fprintf(stderr,"1?\n");
						};
#endif
					sync=0;
					goto trap_eol_err;
					};
				break;
			    case EOF:  goto trap_eof;  break;
			    };
			/* check suffix 1 bit */
			if((bitv=getb(f))!=1) goto trap_eol_err;
			rtc_eols++;
#ifdef DEBUG
			if(dbg_g32r_c) {
				fprintf(stderr,"RTC_EOL ");
				for(si=0;si<sync;si++) fprintf(stderr,"0");
				fprintf(stderr,"1+1  (%d)\n",rtc_eols);
				};
#endif
			}
		while(rtc_eols<6);
		/* normal RTC */
#ifdef DEBUG
		if(dbg_g32r_c)fprintf(stderr,"RTC\n");
#endif
		return(NULL);
		}
	else if(prl->len==0) {
#ifdef DEBUG
		if(dbg_g32r_c)fprintf(stderr,"LINELEN %d\n",crl->len);
#endif
		}
	else if(crl->len!=prl->len) {
		err("g32_to_rlel: y%d: LINELEN changes c%d != p%d ? (force to %d)",
			crl->y,crl->len,prl->len,prl->len);
		crl->len = prl->len;
		};
	swap_rl(crl,prl);
	return(prl);

trap_eof:
#ifdef DEBUG
	if(dbg_g32r_c) fprintf(stderr,"<EOF>\n");
#endif
	return(NULL);

    }
#endif

#ifdef G32
/* Translate a sequence of RLE_Line's (describing a binary image)
   into a file (a stream of bits) in CCITT FAX Group 3 (2-D) compression format.
   BOF_to_g32() must be called first;  then call rlel_to_g32() for each line
   (including blank lines); finally, EOF_to_g32() must be called.  Each line's
   EOL and the RTC's first EOL will be padded so they end on a byte boundary.
   */
/* debugging flags:  trace to stderr */
#define dbg_rg32_e (0)	/* entry */
#define dbg_rg32_r (0)	/* runs */
#define dbg_rg32_s (0)	/* bitstrings */
#define rg32_strict 	/* 1 is CORRECT: explicitly code the last black pel */

#ifdef DEBUG
#define bits_g32(bits) { \
	cs=(bits); while(*cs!='\0') {putb(*cs-'0',f); cs++;};  \
	if(dbg_rg32_s) fprintf(stderr,"%s",(bits)); \
	if(dbg_rg32_r) fprintf(stderr," "); \
	}
#else
#define bits_g32(bits) { \
	cs=(bits); while(*cs!='\0') {putb(*cs-'0',f); cs++;};  \
	}
#endif
#ifdef DEBUG
#define EOL_g32 { \
	if(dbg_rg32_r) fprintf(stderr,"EOL     "); \
	bits_g32(EOLSTRING); \
	if(dbg_rg32_r) fprintf(stderr,"\n"); \
	}
#else
#define EOL_g32 { \
	bits_g32(EOLSTRING); \
	}
#endif

BOF_to_g32(f)
    BITFILE *f;
{   char *cs;
	/* a NOP: no header for Group 3 (2-D) */
#ifdef DEBUG
	if(dbg_rg32_e) fprintf(stderr,"BOF\n");
#endif
	};

rlel_to_g32(pl,cl,wid,f)
    RLE_Line *pl;	/* prior "reference" line: if NULL, use 1-D coding on cl */
    RLE_Line *cl;	/* current "coding" line: if NULL, is blank (all white) */
    int wid;		/* width of an output line in pixels */
    BITFILE *f;
{   int runl,codi;
    char *cs,*p01;
#ifdef DEBUG
#define Wrun_g32(rn) { \
	runl=(rn); \
	if(dbg_rg32_r) fprintf(stderr,"W %5d ",runl); \
	while(runl>2560) {p01=codewht[rtoi(2560)]; bits_g32(p01); runl-=2560;}; \
	p01=codewht[codi=rtoi(runl)]; bits_g32(p01); \
	if(codi>=64) {p01=codewht[runl%64]; bits_g32(p01);}; \
	if(dbg_rg32_r) fprintf(stderr,"\n"); \
	}
#else
#define Wrun_g32(rn) { \
	runl=(rn); \
	while(runl>2560) {p01=codewht[rtoi(2560)]; bits_g32(p01); runl-=2560;}; \
	p01=codewht[codi=rtoi(runl)]; bits_g32(p01); \
	if(codi>=64) {p01=codewht[runl%64]; bits_g32(p01);}; \
	}
#endif
#ifdef DEBUG
#define Brun_g32(rn) { \
	runl=(rn); \
	if(dbg_rg32_r) fprintf(stderr,"B %5d ",runl); \
	while(runl>2560) {p01=codeblk[rtoi(2560)]; bits_g32(p01); runl-=2560;}; \
	p01=codeblk[codi=rtoi(runl)]; bits_g32(p01); \
	if(codi>=64) {p01=codeblk[runl%64]; bits_g32(p01);}; \
	if(dbg_rg32_r) fprintf(stderr,"\n"); \
	}
#else
#define Brun_g32(rn) { \
	runl=(rn); \
	while(runl>2560) {p01=codeblk[rtoi(2560)]; bits_g32(p01); runl-=2560;}; \
	p01=codeblk[codi=rtoi(runl)]; bits_g32(p01); \
	if(codi>=64) {p01=codeblk[runl%64]; bits_g32(p01);}; \
	}
#endif
#ifdef DEBUG
#define V0_g32 { \
	if(dbg_rg32_r) fprintf(stderr,"V0      "); \
	bits_g32(code2d[i2D_V0]); \
	if(dbg_rg32_r) fprintf(stderr,"\n"); \
	}
#else
#define V0_g32 { \
	bits_g32(code2d[i2D_V0]); \
	}
#endif
#ifdef DEBUG
#define VR1_g32 { \
	if(dbg_rg32_r) fprintf(stderr,"VR1     "); \
	bits_g32(code2d[i2D_VR1]); \
	if(dbg_rg32_r) fprintf(stderr,"\n"); \
	}
#else
#define VR1_g32 { \
	bits_g32(code2d[i2D_VR1]); \
	}
#endif
#ifdef DEBUG
#define VR2_g32 { \
	if(dbg_rg32_r) fprintf(stderr,"VR2     "); \
	bits_g32(code2d[i2D_VR2]); \
	if(dbg_rg32_r) fprintf(stderr,"\n"); \
	}
#else
#define VR2_g32 { \
	bits_g32(code2d[i2D_VR2]); \
	}
#endif
#ifdef DEBUG
#define VR3_g32 { \
	if(dbg_rg32_r) fprintf(stderr,"VR3     "); \
	bits_g32(code2d[i2D_VR3]); \
	if(dbg_rg32_r) fprintf(stderr,"\n"); \
	}
#else
#define VR3_g32 { \
	bits_g32(code2d[i2D_VR3]); \
	}
#endif
#ifdef DEBUG
#define VL1_g32 { \
	if(dbg_rg32_r) fprintf(stderr,"VL1     "); \
	bits_g32(code2d[i2D_VL1]); \
	if(dbg_rg32_r) fprintf(stderr,"\n"); \
	}
#else
#define VL1_g32 { \
	bits_g32(code2d[i2D_VL1]); \
	}
#endif
#ifdef DEBUG
#define VL2_g32 { \
	if(dbg_rg32_r) fprintf(stderr,"VL2     "); \
	bits_g32(code2d[i2D_VL2]); \
	if(dbg_rg32_r) fprintf(stderr,"\n"); \
	}
#else
#define VL2_g32 { \
	bits_g32(code2d[i2D_VL2]); \
	}
#endif
#ifdef DEBUG
#define VL3_g32 { \
	if(dbg_rg32_r) fprintf(stderr,"VL3     "); \
	bits_g32(code2d[i2D_VL3]); \
	if(dbg_rg32_r) fprintf(stderr,"\n"); \
	}
#else
#define VL3_g32 { \
	bits_g32(code2d[i2D_VL3]); \
	}
#endif
#ifdef DEBUG
#define PASS_g32 { \
	if(dbg_rg32_r) fprintf(stderr,"PASS    "); \
	bits_g32(code2d[i2D_PASS]); \
	if(dbg_rg32_r) fprintf(stderr,"\n"); \
	}
#else
#define PASS_g32 { \
	bits_g32(code2d[i2D_PASS]); \
	}
#endif
#ifdef DEBUG
#define HORIZ_g32 { \
	if(dbg_rg32_r) fprintf(stderr,"HORIZ   "); \
	bits_g32(code2d[i2D_HORIZ]); \
	if(dbg_rg32_r) fprintf(stderr,"\n"); \
	}
#else
#define HORIZ_g32 { \
	bits_g32(code2d[i2D_HORIZ]); \
	}
#endif
#define detect_a1a2_BW { \
	/* find leftmost black changing pel > a0 */ \
	/* advance cra as far as possible s.t. cra->xe<=a0 */ \
	while((cra+1)<cre && (cra+1)->xe<=a0) cra++; \
	/* look beyond cra, until cr->xs>a0 */ \
	cr=cra; while(cr<cre && (a1=cr->xs)<=a0) cr++; \
	if(cr<cre) a2=cr->xe+1; \
	else a1=a2=wid; \
	}
#define detect_a1a2_WB { \
	/* find leftmost white changing pel > a0 */ \
	/* advance cra as far as possible s.t. cra->xe<=a0 */ \
	while((cra+1)<cre && (cra+1)->xe<=a0) cra++; \
	/* look beyond cra, until cr->xe+1>a0 */ \
	cr=cra;  while(cr<cre && (a1=cr->xe+1)<=a0) cr++; \
	if(cr<cre) { \
		if((cr+1)<cre) a2=(cr+1)->xs; \
		else a2=wid; \
		} \
	else a1=a2=wid; \
	}
#define detect_a1a2 {if(a0_color==DST_white) detect_a1a2_BW else detect_a1a2_WB;}
#define detect_b1b2_BW {\
	/* find leftmost black changing pel > a0 */ \
	/* advance pra as far as possible s.t. pra->xe<=a0 */ \
	while((pra+1)<pre && (pra+1)->xe<=a0) pra++; \
	/* look beyond pra */ \
	pr=pra;  while(pr<pre && (b1=pr->xs)<=a0) pr++; \
	/* move b2 to 1st changing white pel > b1 */ \
	if(pr<pre) b2=pr->xe+1; \
	else b1=b2=wid; \
	}
#define detect_b1b2_WB {\
	/* find leftmost white changing pel > a0 */ \
	/* advance pra as far as possible s.t. pra->xe<=a0 */ \
	while((pra+1)<pre && (pra+1)->xe<=a0) pra++; \
	/* look beyond pra */ \
	pr=pra;  while(pr<pre && (b1=pr->xe+1)<=a0) pr++; \
	/* move b2 to 1st changing black pel > b1 */ \
	if(pr<pre) { \
		if((pr+1)<pre) b2=(pr+1)->xs; \
		else b2=wid; \
		} \
	else b1=b2=wid; \
	}
#define detect_b1b2 {if(a0_color==DST_white) detect_b1b2_BW else detect_b1b2_WB;}

#ifdef DEBUG
	if(dbg_rg32_e) err("rlel_to_g32(pl[%d],cl[%d],w%d)",
				(pl==NULL)? -1: pl->runs,
				(cl==NULL)? -1: cl->runs,
				wid);
#endif
	/* fill so that EOL ends on byte boundary */
	padb(f,0,8,EOLLENGTH);  if(dbg_rg32_s) fprintf(stderr,"FILL  +0?");
	EOL_g32;	/* begin with EOL (sic) */
	if(pl==NULL) /* use 1-D coding for *cl */ {
	    int pi;		/* input pixel index on line */
	    RLE_Run *rp,*pp,*sp;
		putb(1,f);  if(dbg_rg32_r) fprintf(stderr,"1D_LINE 1\n");
		if(cl!=NULL&&cl->runs>0) {
			pi=0;	/* bit to write next */
			for(sp=(rp=cl->r)+cl->runs; rp<sp; rp++) {
				Wrun_g32(rp->xs-pi);  pi=rp->xs;
				Brun_g32(rp->xe-pi+1);  pi=rp->xe+1;
				};
			if((--rp)->xe+1<wid) Wrun_g32(wid-rp->xe-1);
			}
		else Wrun_g32(wid);  /* blank scanline */
		}
	else /* use 2-D coding for *cl */ {
	    RLE_Run *cr,*cre,*pr,*pre;	/* into current/prior rle lines */	
	    int a0,a1,a2,b1,b2;	 /* indices {0,1,...} of pixels */
	    DST_color a0_color;  /* a0's color:  same as a2 & b2, opp of a1 & b1 */
	    RLE_Run *cra;   /* rightmost in current st xe<=a0 (none: ==cl->r)  */
	    RLE_Run *pra;   /* rightmost in prior st xe<=a0 (none: ==pl->r) */
	    int a01,a12,a1b1;	/* lengths of runs a0-a1 a1-a2 a1-b1 */
		putb(0,f);  if(dbg_rg32_r) fprintf(stderr,"2D_LINE 0\n");
		/* start on an imaginary white pixel just to left of margin */
		a0=-1;  a0_color = DST_white;	
		pre = (pra=pl->r) + pl->runs;	/* prior line's runs */
		/* start b1/b2 on prior line's first black pixel, etc;
		   if none, then place off end of line */
		if(pra<pre) {b1=pra->xs; b2=pra->xe+1;} else b1=b2=wid;
		if(cl!=NULL&&cl->runs>0) {
			cre = (cra=cl->r) + cl->runs;
			a1=cra->xs;  a2=cra->xe+1;
#ifdef rg32_strict
			while(a0 < wid) {
#else
			while(a0 < wid-1) {
#endif
				/* a0, a1, a2, b1, b2 are as in CCITT Rec T.4 */
#ifdef DEBUG
				if(dbg_rg32_r)
				    fprintf(stderr,"f%d(%d,%d,%d) b%d(%d,%d)\n",
					cra-(cl->r),a0,a1,a2,pra-(pl->r),b1,b2);
#endif
				if(b2<a1) /* PASS mode */ {
					PASS_g32;
					a0=b2;
					/* a0-color, a1, & a2 are unchanged */
					detect_b1b2;
					}
				else if((a1b1=(a1-b1))<=3 && a1b1>=-3) {
					/* VERTICAL mode */
					switch(a1b1) {
					    case -3:  VL3_g32;  break;
					    case -2:  VL2_g32;  break;
					    case -1:  VL1_g32;  break;
					    case 0:  V0_g32;  break;
					    case 1:  VR1_g32;  break;
					    case 2:  VR2_g32;  break;
					    case 3:  VR3_g32;  break;
					    };
					a0=a1;  a0_color=flip_color(a0_color);
					detect_a1a2;
					detect_b1b2;
					}
				else {	/* HORIZONTAL mode */
					HORIZ_g32;
					a01=a1-a0; if(a0==-1) a01--;
					a12=a2-a1;
					if(a0_color==DST_white) {
						Wrun_g32(a01);
						Brun_g32(a12);
						}
					else {	Brun_g32(a01);
						Wrun_g32(a12);
						};
					a0=a2;
					/* a0_color is unchanged */
					detect_a1a2;
					detect_b1b2;
					};
				};
			}
		else /* current line is blank */ {
			a1=a2=wid;
#ifdef rg32_strict
			while(a0 < wid) {
#else
			while(a0 < wid-1) {
#endif
				/* a0, a1, a2, b1, b2 are as in CCITT Rec. T.4 */
#ifdef DEBUG
				if(dbg_rg32_r)
				    fprintf(stderr,"f(%d,%d,%d) b%d(%d,%d)\n",
					a0,a1,a2,pra-(pl->r),b1,b2);
#endif
				if(b2<a1) /* PASS mode */ {
					PASS_g32;
					a0=b2;
					/* a0-color, a1, & a2 are unchanged */
					detect_b1b2;
					}
				else if((a1b1=a1-b1)<=3 && a1b1>=-3) {
					/* VERTICAL mode */
					switch(a1b1) {
					    case -3:  VL3_g32;  break;
					    case -2:  VL2_g32;  break;
					    case -1:  VL1_g32;  break;
					    case 0:  V0_g32;  break;
					    case 1:  VR1_g32;  break;
					    case 2:  VR2_g32;  break;
					    case 3:  VR3_g32;  break;
					    };
					a0=a1;  a0_color=flip_color(a0_color);
					/* a1, & a2 are unchanged */
					detect_b1b2;
					}
				else {	/* HORIZONTAL mode */
					HORIZ_g32;
					a01=a1-a0; if(a0==-1) a01--;
					a12=a2-a1;
					if(a0_color==DST_white) {
						Wrun_g32(a01);
						Brun_g32(a12);
						}
					else {	Brun_g32(a01);
						Wrun_g32(a12);
						};
					a0=a2;
					/* a0_color, a1, & a2 are unchanged */
					detect_b1b2;
					};
				};
			};
		};
	}

EOF_to_g32(f)
    BITFILE *f;
{   char *cs;
	padb(f,0,8,EOLLENGTH);  /* fill so that EOL ends on byte boundary */
#ifdef DEBUG
	if(dbg_rg32_s) fprintf(stderr,"FILL  +0?");
#endif
	/* write RTC */
	EOL_g32;
	EOL_g32;
	EOL_g32;
	EOL_g32;
	EOL_g32;
	EOL_g32;
#ifdef DEBUG
	if(dbg_rg32_e) fprintf(stderr,"EOF\n");
#endif
	}
#endif

#ifdef TRASH
/* Macro for use within g4_to_rlel, to read one 1-D Modified Huffman coded
   run-length of color C, placing the result in variable V.
   Exceptionally, goto trap_eol, trap_eof, or trap_code_err.
  */
#ifdef DEBUG
#define g4_1d_run(C,V) { \
	run = 0; \
	do {	cx.tr.s = (C); \
	        do {	px = cx; \
			switch(bitv=getb(f)) { \
			    case 0 :  cx.tr=t->e[cx.tr.s].t[0];  break; \
			    case 1 :  cx.tr=t->e[cx.tr.s].t[1];  break; \
			    case EOF :  goto trap_eof;  break; \
			    }; \
			} \
		while(cx.tr.a==DST_action_NULL); \
		switch(cx.tr.a) { \
		    case DST_EOL : \
			if(dbg_g4r_c)fprintf(stderr,"EOL     %s\n",EOLSTRING); \
			goto trap_eol; \
			break; \
		    case DST_action_ERROR :  goto trap_code_err;  break; \
		    default : \
			if(dbg_g4r_c)fprintf(stderr,"%s%6d %s%d\n", \
				((C)==DST_white)? "W": "B", \
				cx.tr.a, \
				t->e[px.tr.s].p, \
				bitv); \
			run += cx.tr.a; \
			break; \
		    }; \
		} \
	while(cx.tr.a>63); \
	(V) = run; \
	}
#else
#define g4_1d_run(C,V) { \
	run = 0; \
	do {	cx.tr.s = (C); \
	        do {	px = cx; \
			switch(bitv=getb(f)) { \
			    case 0 :  cx.tr=t->e[cx.tr.s].t[0];  break; \
			    case 1 :  cx.tr=t->e[cx.tr.s].t[1];  break; \
			    case EOF :  goto trap_eof;  break; \
			    }; \
			} \
		while(cx.tr.a==DST_action_NULL); \
		switch(cx.tr.a) { \
		    case DST_EOL : \
			goto trap_eol; \
			break; \
		    case DST_action_ERROR :  goto trap_code_err;  break; \
		    default : \
			run += cx.tr.a; \
			break; \
		    }; \
		} \
	while(cx.tr.a>63); \
	(V) = run; \
	}
#endif

/* Translate a stream of bits in CCITT FAX Group 4 compression format, of known
   fixed line-length, into a sequence of RLE_Lines.  Returns one (RLE_Line *)
   on each call (or NULL if EOF or error).  The first pixel (black or white)
   in each g4 line is assigned run index 0.
   WARNING:  the RLE_Line returned must NOT be modified by the caller, since
   it is used to decode the next line. */
RLE_Line *g4_to_rlel(t,f,bof,len)
    DST_table *t;
    BITFILE *f;
    boolean bof;	/* beginning of file */
    int len;		/* line-length in pixels (used only on bof) */
#define dbg_g4r_e (0)	/* trace entry-to / exit-from function */
#define dbg_g4r_r (0)	/* trace each run/EOL/ERR_SYN */
#define dbg_g4r_c (0)	/* trace each Huffman code (or, fill+EOL)*/
#define dbg_g4r_t (0)	/* trace each state-transition */
#define g4r_strict (1)	/* 1 is CORRECT: explicitly code the last black pel */
{   static RLE_Line rl0,rl1,*prl,*crl;	/* prior, current run-lines */
    RLE_Line *swrl;
    int bitv;			/* the last-read bit value */
    DST_context cx,px;		/* the current/prior decoding context */
    RLE_Run *cr,*pr,*pre;	/* into current/prior rle lines */	
    RLE_Run *pra0;		/* rightmost in prior line with xe<=a0
				   (if none: prl->r) */
    int run,sync,si,rtc_eols;
    /* pixel indices (0,1,...):  current-line a*; prior-line b*.
       a0 is the index of the most recently completely encoded bit */
    int a0,a1,a2,b1,b2;	
    DST_color a0_color;  /* a0's color:  same as a2 & b2, opposite of a1 & b1 */
    int a01,a12;	/* lengths of runs a0-a1 & a1-a2 */
#define g4_first_color (DST_white)	/* color of 1st run in each line */
#define g4_negative (0)		/* if 1, invert Black/White on output */
#define swap_rl(f,b) {swrl=(f); (f)=(b); (b)=swrl;}
/* detect b1 & b2:  sensitive to a0, a0_color, and prior runs *pr *(pr+1) */
#define g4r_find_Bb1Wb2 { \
	/* find 1st black changing pel>a0 */ \
	/* advance pra0 as far as possible s.t. pra0->xe<=a0 */ \
	while((pra0+1)<pre && (pra0+1)->xe<=a0) pra0++; \
	/* look beyond pra0 */ \
	pr=pra0;  while(pr<pre && (b1=pr->xs)<=a0) pr++; \
	/* move b2 to 1st changing white pel > b1 */ \
	if(pr<pre) b2=pr->xe+1; \
	else b1=b2=prl->len; \
	}
#define g4r_find_Wb1Bb2 { \
	/* find 1st white changing pel>a0 */ \
	/* advance pra0 as far as possible s.t. pra0->xe<=a0 */ \
	while((pra0+1)<pre && (pra0+1)->xe<=a0) pra0++; \
	/* look beyond pra0 */ \
	pr=pra0;  while(pr<pre && (b1=pr->xe+1)<=a0) pr++; \
	/* move b2 to 1st changing black pel > b1 */ \
	if(pr<pre) { \
		if((pr+1)<pre) b2=(pr+1)->xs; \
		else b2=prl->len; \
		} \
	else b1=b2=prl->len; \
	}
#define g4r_find_b1b2 { \
	if(a0_color==DST_white) g4r_find_Bb1Wb2 else g4r_find_Wb1Bb2; \
	if(b1<=a0) err("g4_to_rlel: y%d: b1 %d <= a0 %d ?",crl->y,a0,b1); \
	}

	if(bof){/* initial reference line is all white, of known length */
		prl=&rl0;  prl->y=-1;  prl->len=len;  prl->runs=0;
		/* initial coding line has y-coordinate 0 */
		crl=&rl1;  crl->y=0;  crl->len=0;  crl->runs=0;
		}
	else crl->y = prl->y + 1;
#ifdef DEBUG
	if(dbg_g4r_e)
		fprintf(stderr,
			"g4_to_rlel(t,f,bof%d,len%d) y%d,l%d\n",
			bof,len,prl->y,prl->len);
#endif
	
	pre = (pra0=pr=prl->r) + prl->runs;	/* prior line */
	crl->runs=0;  cr=crl->r-1;		/* current line */
	/* start on an imaginary white pixel just to left of margin */
	a0=-1;  a0_color = DST_white;
#ifdef DEBUG
	a1=a2=b1=b2=0;  /* immaterial; looks better when debugging */
#endif

	/* start b1/b2 on prior line's first black pixel, etc;
	   if none, then place off end of line */
	if(pr<pre) {b1=pr->xs; b2=pr->xe+1;} else b1=b2=prl->len;
	/* parse a sequence of 2D codes... */
	while(T/* exit only via 'goto trap_X' */) {
		/* a0, a1, a2, b1, b2 are as defined in CCITT Rec. T.6 */
#ifdef DEBUG
		if(dbg_g4r_r)
			fprintf(stderr,"a(%d,%d,%d) b(%d,%d)\n",a0,a1,a2,b1,b2);
#endif
		cx.tr.s = DST_2d;  cx.tr.a = DST_action_NULL;
#ifdef DEBUG
		if(dbg_g4r_t)fprintf(stderr,"(%d,%d)\n",cx.tr.s,cx.tr.a);
#endif
        	do {	switch(bitv=getb(f)) {
			    case 0 :
			    case 1 :
				cx.tr=t->e[cx.tr.s].t[bitv];
				break;
			    case EOF:  goto trap_eof;  break;
			    };
#ifdef DEBUG
			if(dbg_g4r_t)fprintf(stderr,"%d->(%d,%d)\n",
					bitv,cx.tr.s,cx.tr.a);
#endif
			}
		while(cx.tr.a==DST_action_NULL);
#ifdef DEBUG
		if(dbg_g4r_t)fprintf(stderr,"%s %04d  %s0\n",
			(cx.c==DST_white)? "W": "B",
			cx.tr.s,
			t->e[cx.tr.s].p);
#endif
		switch(cx.tr.a) {
		    case i2D_V0:
#ifdef DEBUG
			if(dbg_g4r_c)
				fprintf(stderr,"V0      %s\n",code2d[cx.tr.a]);
#endif
			a1=b1;
			/* interpret (a0,a1-1] */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* interpret a0 */
			if(a0_color==DST_black) {
				if(cr->xs>cr->xe&&cr->xs<prl->len&&cr->xe>0)
					err("g4_to_rlel: y%d: r%d[%d,%d] not monotone ?",
						crl->y,crl->runs,cr->xs,cr->xe);
				crl->runs++;  (++cr)->xs = a0;  cr->xe = a0;
				};
#ifndef g4r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g4r_find_b1b2;
			break;
		    case i2D_VR1:
#ifdef DEBUG
			if(dbg_g4r_c)
				fprintf(stderr,"VR1     %s\n",code2d[cx.tr.a]);
#endif
			a1=b1+1;
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black) {
				if(cr->xs>cr->xe&&cr->xs<prl->len&&cr->xe>0)
					err("g4_to_rlel: y%d: r%d[%d,%d] not monotone ?",
						crl->y,crl->runs,cr->xs,cr->xe);
				crl->runs++;  (++cr)->xs = a0;  cr->xe = a0;
				};
#ifndef g4r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g4r_find_b1b2;
			break;
		    case i2D_VR2:
#ifdef DEBUG
			if(dbg_g4r_c)
				fprintf(stderr,"VR2     %s\n",code2d[cx.tr.a]);
#endif
			a1=b1+2;
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black) {
				if(cr->xs>cr->xe&&cr->xs<prl->len&&cr->xe>0)
					err("g4_to_rlel: y%d: r%d[%d,%d] not monotone ?",
						crl->y,crl->runs,cr->xs,cr->xe);
				crl->runs++;  (++cr)->xs = a0;  cr->xe = a0;
				};
#ifndef g4r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g4r_find_b1b2;
			break;
		    case i2D_VR3:
#ifdef DEBUG
			if(dbg_g4r_c)
				fprintf(stderr,"VR3     %s\n",code2d[cx.tr.a]);
#endif
			a1=b1+3;
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black) {
				if(cr->xs>cr->xe&&cr->xs<prl->len&&cr->xe>0)
					err("g4_to_rlel: y%d: r%d[%d,%d] not monotone ?",
						crl->y,crl->runs,cr->xs,cr->xe);
				crl->runs++;  (++cr)->xs = a0;  cr->xe = a0;
				};
#ifndef g4r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g4r_find_b1b2;
			break;
		    case i2D_VL1:
#ifdef DEBUG
			if(dbg_g4r_c)
				fprintf(stderr,"VL1     %s\n",code2d[cx.tr.a]);
#endif
			if((a1=b1-1)<=a0)
				err("g4_to_rlel: y%d: VL1 a1 %d <= a0 %d ?",
					crl->y,a1,a0);
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black) {
				if(cr->xs>cr->xe&&cr->xs<prl->len&&cr->xe>0)
					err("g4_to_rlel: y%d: r%d[%d,%d] not monotone ?",
						crl->y,crl->runs,cr->xs,cr->xe);
				crl->runs++;  (++cr)->xs = a0;  cr->xe = a0;
				};
#ifndef g4r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g4r_find_b1b2;
			break;
		    case i2D_VL2:
#ifdef DEBUG
			if(dbg_g4r_c)
				fprintf(stderr,"VL2     %s\n",code2d[cx.tr.a]);
#endif
			if((a1=b1-2)<=a0)
				err("g4_to_rlel: y%d: VL2 a1 %d <= a0 %d ?",
					crl->y,a1,a0);
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black) { 
				if(cr->xs>cr->xe&&cr->xs<prl->len&&cr->xe>0)
					err("g4_to_rlel: y%d: r%d[%d,%d] not monotone ?",
						crl->y,crl->runs,cr->xs,cr->xe);
				crl->runs++;  (++cr)->xs = a0;  cr->xe = a0;
				};
#ifndef g4r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g4r_find_b1b2;
			break;
		    case i2D_VL3:
#ifdef DEBUG
			if(dbg_g4r_c)
				fprintf(stderr,"VL3     %s\n",code2d[cx.tr.a]);
#endif
			if((a1=b1-3)<=a0)
				err("g4_to_rlel: y%d: VL3 a1 %d <= a0 %d ?",
					crl->y,a1,a0);
			/* encode a0 to a1-1 */
			if(a0_color==DST_black) cr->xe=a1-1;
			/* move a0 to a1 */
			a0=a1;  if(a0>=prl->len) goto trap_expecting_eol;
			a0_color = flip_color(a0_color);
			/* encode a0 */
			if(a0_color==DST_black) {
				if(cr->xs>cr->xe&&cr->xs<prl->len&&cr->xe>0)
					err("g4_to_rlel: y%d: r%d[%d,%d] not monotone ?",
						crl->y,crl->runs,cr->xs,cr->xe);
				crl->runs++;  (++cr)->xs = a0;  cr->xe = a0;
				};
#ifndef g4r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g4r_find_b1b2;
			break;
		    case i2D_PASS:
#ifdef DEBUG
			if(dbg_g4r_c)
				fprintf(stderr,"PASS    %s\n",code2d[cx.tr.a]);
#endif
			/* move a0 to b2; no change of color */
			a0=b2;	if(a0>=prl->len) goto trap_expecting_eol;
			if(a0_color==DST_black) cr->xe = a0;
#ifndef g4r_strict
			if(a0==prl->len-1)
				{ a0++;  goto trap_expecting_eol; };
#endif
			g4r_find_b1b2;
			break;
		    case i2D_HORIZ:
#ifdef DEBUG
			if(dbg_g4r_c)
				fprintf(stderr,"HORIZ   %s\n",code2d[cx.tr.a]);
#endif
			if(a0_color==DST_white) {
				if(a0<0) a0=0;	/* first run in line starts at 0 */
				g4_1d_run(DST_white,a01);  a1 = a0 + a01;
				g4_1d_run(DST_black,a12);  a2 = a1 + a12;
				if(a12>0) /* Black run of >0 length */ {
				if(cr->xs>cr->xe&&cr->xs<prl->len&&cr->xe>0)
					err("g4_to_rlel: y%d: r%d[%d,%d] not monotone ?",
						crl->y,crl->runs,cr->xs,cr->xe);
					crl->runs++; (++cr)->xs = a1; cr->xe = a2-1;
					};
				a0 = a2;	/* still white */
				if(a0>=prl->len) goto trap_expecting_eol;
				/* encode a0 */
#ifndef g4r_strict
				if(a0==prl->len-1)
					{ a0++;  goto trap_expecting_eol; };
#endif
				g4r_find_Bb1Wb2;
				}
			else {	g4_1d_run(DST_black,a01);  a1 = a0 + a01;
				g4_1d_run(DST_white,a12);  a2 = a1 + a12;
				if(a01>0) /* Black run of >0 length */ {
					cr->xe = a1 - 1;
					}
				else {	/* 0-length: very peculiar: ignore */
					fprintf(stderr,
					   "g4_to_rlel: HORIZ B%d! W%d - ignore\n",
					   a01,a12);
					cr--; crl->runs--;
					};
				a0 = a2;	/* still black */
				if(a0>=prl->len) goto trap_expecting_eol;
				/* encode a0 */
				crl->runs++;  (++cr)->xs = a0;
#ifndef g4r_strict
				if(a0==prl->len-1) {
					cr->xe = a0;
					a0++;
					goto trap_expecting_eol;
					};
#endif
				g4r_find_Wb1Bb2;
				};
			break;
		    case i2D_EOL:
#ifdef DEBUG
			if(dbg_g4r_c)
				fprintf(stderr,"EOL     %s\n",code2d[cx.tr.a]);
#endif
			goto trap_eol;
			break;
		    case DST_action_ERROR:  goto trap_code_err;  break;
		    };
		};

/* come here via goto's: all these traps return() */

trap_expecting_eol:	/* come here having detected (computed) end-of-line */
	/* learn/check/correct line-length */
	crl->len = (a0>0)? a0 : 0;
	if(crl->len!=prl->len) {
		err("g4_to_rlel: y%d: LINELEN changes c%d != p%d ? (force to %d)",
			crl->y,crl->len,prl->len,prl->len);
		crl->len = prl->len;
		};
	swap_rl(crl,prl);
#ifdef DEBUG
	if(dbg_g4r_e)
		fprintf(stderr,
			"exit g4_to_rlel: expg_eol y%d,l%d\n",
			prl->y,prl->len);
#endif
	return(prl);

trap_eol:  /* come here having seen an EOL code (rare in Group 4) */
	/* It must be the first of two EOL codes making up the EOB signal */
	/* look for 2nd EOL */
	sync=0;  while((bitv=getb(f))==0) sync++;
	switch(bitv) {
	    case 1:
		if(sync==11) {
#ifdef DEBUG
			if(dbg_g4r_c) {
				fprintf(stderr,"EOB_EOL %s\n",EOLSTRING);
				fprintf(stderr,"EOB\n");
				};
#endif
			}
		else {	
#ifdef DEBUG
			if(dbg_g4r_c){
				fprintf(stderr,"NOT EOB ");
				for(si=0;si<sync;si++)
					fprintf(stderr,"0");
				fprintf(stderr,"1?\n");
				fprintf(stderr,"ABORT\n");
				};
#endif
			};
		break;
	    case EOF:  goto trap_eof;  break;
	    };
#ifdef DEBUG
	if(dbg_g4r_e) fprintf(stderr,"exit g4_to_rlel: eol\n");
#endif
	return(NULL);

trap_code_err:
	/* unexpected coding sequence:
	   'px' holds last decoding context & 'bitv' latest bit value;
	   will attempt to resynchronize on next EOL */
#ifdef DEBUG
	if(dbg_g4r_c)fprintf(stderr,"CODERR  %s%d? (px.tr.s=%d)\n",
				t->e[px.tr.s].p,bitv,px.tr.s);
#endif
	err("g4_to_rlel: code error");
#ifdef DEBUG
	if(dbg_g4r_e) fprintf(stderr,"exit g4_to_rlel: code_err\n");
#endif
	return(NULL);

trap_eof:
#ifdef DEBUG
	if(dbg_g4r_c) fprintf(stderr,"<EOF>\n");
	if(dbg_g4r_e) fprintf(stderr,"exit g4_to_rlel: eof\n");
#endif
	return(NULL);

    }
#endif

#ifdef G4
/* Translate a sequence of RLE_Line's (describing a binary image)
   into a file (a stream of bits) in CCITT FAX Group 4 compression format.
   BOF_to_g4() must be called first; then call rlel_to_g4() for each line
   (including blank lines); finally, EOF_to_g4() must be called.  The EOFB
   is padded (suffixed) with 0's to a byte boundary if necessary; no other
   filling or padding is performed.
   */
/* debugging flags:  trace to stderr */
#define dbg_rg4_e (0)	/* entry */
#define dbg_rg4_r (0)	/* runs */
#define dbg_rg4_c (0)	/* codes (bitstrings) */
#define rg4_strict 	/* 1 is CORRECT: explicitly code the last black pel */

#ifdef DEBUG
#define bits_g4(bits) { \
	cs=(bits); while(*cs!='\0') {putb(*cs-'0',f); cs++;};  \
	if(dbg_rg4_c) fprintf(stderr,"%s",(bits)); \
	if(dbg_rg4_r) fprintf(stderr," "); \
	}
#else
#define bits_g4(bits) { \
	cs=(bits); while(*cs!='\0') {putb(*cs-'0',f); cs++;};  \
	}
#endif
#ifdef DEBUG
#define EOFB_g4 { \
	if(dbg_rg4_r) fprintf(stderr,"EOFB    "); \
	bits_g4(EOFB); \
	if(dbg_rg4_r) fprintf(stderr,"\n"); \
	}
#else
#define EOFB_g4 { \
	bits_g4(EOFB); \
	}
#endif

BOF_to_g4(f)
    BITFILE *f;
{   char *cs;
	/* a NOP: no header for Group 4 */
#ifdef DEBUG
	if(dbg_rg4_e) fprintf(stderr,"BOF\n");
#endif
	};

rlel_to_g4(pl,cl,wid,f)
    RLE_Line *pl;	/* prior "reference" line */
    RLE_Line *cl;	/* current "coding" line: if NULL, is blank (all white) */
    int wid;		/* width of an output line in pixels */
    BITFILE *f;
{   int runl,codi;
    char *cs,*p01;
#ifdef DEBUG
#define Wrun_g4(rn) { \
	runl=(rn); \
	if(dbg_rg4_r) fprintf(stderr,"W %5d ",runl); \
	while(runl>2560) {p01=codewht[rtoi(2560)]; bits_g4(p01); runl-=2560;}; \
	p01=codewht[codi=rtoi(runl)]; bits_g4(p01); \
	if(codi>=64) {p01=codewht[runl%64]; bits_g4(p01);}; \
	if(dbg_rg4_r) fprintf(stderr,"\n"); \
	}
#else
#define Wrun_g4(rn) { \
	runl=(rn); \
	while(runl>2560) {p01=codewht[rtoi(2560)]; bits_g4(p01); runl-=2560;}; \
	p01=codewht[codi=rtoi(runl)]; bits_g4(p01); \
	if(codi>=64) {p01=codewht[runl%64]; bits_g4(p01);}; \
	}
#endif
#ifdef DEBUG
#define Brun_g4(rn) { \
	runl=(rn); \
	if(dbg_rg4_r) fprintf(stderr,"B %5d ",runl); \
	while(runl>2560) {p01=codeblk[rtoi(2560)]; bits_g4(p01); runl-=2560;}; \
	p01=codeblk[codi=rtoi(runl)]; bits_g4(p01); \
	if(codi>=64) {p01=codeblk[runl%64]; bits_g4(p01);}; \
	if(dbg_rg4_r) fprintf(stderr,"\n"); \
	}
#else
#define Brun_g4(rn) { \
	runl=(rn); \
	while(runl>2560) {p01=codeblk[rtoi(2560)]; bits_g4(p01); runl-=2560;}; \
	p01=codeblk[codi=rtoi(runl)]; bits_g4(p01); \
	if(codi>=64) {p01=codeblk[runl%64]; bits_g4(p01);}; \
	}
#endif
#ifdef DEBUG
#define V0_g4 { \
	if(dbg_rg4_r) fprintf(stderr,"V0      "); \
	bits_g4(code2d[i2D_V0]); \
	if(dbg_rg4_r) fprintf(stderr,"\n"); \
	}
#else
#define V0_g4 { \
	bits_g4(code2d[i2D_V0]); \
	}
#endif
#ifdef DEBUG
#define VR1_g4 { \
	if(dbg_rg4_r) fprintf(stderr,"VR1     "); \
	bits_g4(code2d[i2D_VR1]); \
	if(dbg_rg4_r) fprintf(stderr,"\n"); \
	}
#else
#define VR1_g4 { \
	bits_g4(code2d[i2D_VR1]); \
	}
#endif
#ifdef DEBUG
#define VR2_g4 { \
	if(dbg_rg4_r) fprintf(stderr,"VR2     "); \
	bits_g4(code2d[i2D_VR2]); \
	if(dbg_rg4_r) fprintf(stderr,"\n"); \
	}
#else
#define VR2_g4 { \
	bits_g4(code2d[i2D_VR2]); \
	}
#endif
#ifdef DEBUG
#define VR3_g4 { \
	if(dbg_rg4_r) fprintf(stderr,"VR3     "); \
	bits_g4(code2d[i2D_VR3]); \
	if(dbg_rg4_r) fprintf(stderr,"\n"); \
	}
#else
#define VR3_g4 { \
	bits_g4(code2d[i2D_VR3]); \
	}
#endif
#ifdef DEBUG
#define VL1_g4 { \
	if(dbg_rg4_r) fprintf(stderr,"VL1     "); \
	bits_g4(code2d[i2D_VL1]); \
	if(dbg_rg4_r) fprintf(stderr,"\n"); \
	}
#else
#define VL1_g4 { \
	bits_g4(code2d[i2D_VL1]); \
	}
#endif
#ifdef DEBUG
#define VL2_g4 { \
	if(dbg_rg4_r) fprintf(stderr,"VL2     "); \
	bits_g4(code2d[i2D_VL2]); \
	if(dbg_rg4_r) fprintf(stderr,"\n"); \
	}
#else
#define VL2_g4 { \
	bits_g4(code2d[i2D_VL2]); \
	}
#endif
#ifdef DEBUG
#define VL3_g4 { \
	if(dbg_rg4_r) fprintf(stderr,"VL3     "); \
	bits_g4(code2d[i2D_VL3]); \
	if(dbg_rg4_r) fprintf(stderr,"\n"); \
	}
#else
#define VL3_g4 { \
	bits_g4(code2d[i2D_VL3]); \
	}
#endif
#ifdef DEBUG
#define PASS_g4 { \
	if(dbg_rg4_r) fprintf(stderr,"PASS    "); \
	bits_g4(code2d[i2D_PASS]); \
	if(dbg_rg4_r) fprintf(stderr,"\n"); \
	}
#else
#define PASS_g4 { \
	bits_g4(code2d[i2D_PASS]); \
	}
#endif
#ifdef DEBUG
#define HORIZ_g4 { \
	if(dbg_rg4_r) fprintf(stderr,"HORIZ   "); \
	bits_g4(code2d[i2D_HORIZ]); \
	if(dbg_rg4_r) fprintf(stderr,"\n"); \
	}
#else
#define HORIZ_g4 { \
	bits_g4(code2d[i2D_HORIZ]); \
	}
#endif
#define detect_a1a2_BW { \
	/* find leftmost black changing pel > a0 */ \
	/* advance cra as far as possible s.t. cra->xe<=a0 */ \
	while((cra+1)<cre && (cra+1)->xe<=a0) cra++; \
	/* look beyond cra, until cr->xs>a0 */ \
	cr=cra; while(cr<cre && (a1=cr->xs)<=a0) cr++; \
	if(cr<cre) a2=cr->xe+1; \
	else a1=a2=wid; \
	}
#define detect_a1a2_WB { \
	/* find leftmost white changing pel > a0 */ \
	/* advance cra as far as possible s.t. cra->xe<=a0 */ \
	while((cra+1)<cre && (cra+1)->xe<=a0) cra++; \
	/* look beyond cra, until cr->xe+1>a0 */ \
	cr=cra;  while(cr<cre && (a1=cr->xe+1)<=a0) cr++; \
	if(cr<cre) { \
		if((cr+1)<cre) a2=(cr+1)->xs; \
		else a2=wid; \
		} \
	else a1=a2=wid; \
	}
#define detect_a1a2 {if(a0_color==DST_white) detect_a1a2_BW else detect_a1a2_WB;}
#define detect_b1b2_BW {\
	/* find leftmost black changing pel > a0 */ \
	/* advance pra as far as possible s.t. pra->xe<=a0 */ \
	while((pra+1)<pre && (pra+1)->xe<=a0) pra++; \
	/* look beyond pra */ \
	pr=pra;  while(pr<pre && (b1=pr->xs)<=a0) pr++; \
	/* move b2 to 1st changing white pel > b1 */ \
	if(pr<pre) b2=pr->xe+1; \
	else b1=b2=wid; \
	}
#define detect_b1b2_WB {\
	/* find leftmost white changing pel > a0 */ \
	/* advance pra as far as possible s.t. pra->xe<=a0 */ \
	while((pra+1)<pre && (pra+1)->xe<=a0) pra++; \
	/* look beyond pra */ \
	pr=pra;  while(pr<pre && (b1=pr->xe+1)<=a0) pr++; \
	/* move b2 to 1st changing black pel > b1 */ \
	if(pr<pre) { \
		if((pr+1)<pre) b2=(pr+1)->xs; \
		else b2=wid; \
		} \
	else b1=b2=wid; \
	}
#define detect_b1b2 {if(a0_color==DST_white) detect_b1b2_BW else detect_b1b2_WB;}

    RLE_Run *cr,*cre,*pr,*pre;	/* into current/prior rle lines */	
    int a0,a1,a2,b1,b2;	 /* indices {0,1,...} of pixels */
    DST_color a0_color;  /* a0's color:  same as a2 & b2, opp of a1 & b1 */
    RLE_Run *cra;   /* rightmost in current st xe<=a0 (none: ==cl->r)  */
    RLE_Run *pra;   /* rightmost in prior st xe<=a0 (none: ==pl->r) */
    int a01,a12,a1b1;	/* lengths of runs a0-a1 a1-a2 a1-b1 */
#ifdef DEBUG
	if(dbg_rg4_e) err("rlel_to_g4(pl[%d],cl[%d],w%d)",
				(pl==NULL)? -1: pl->runs,
				(cl==NULL)? -1: cl->runs,
				wid);
#endif
	/* start on an imaginary white pixel just to left of margin */
	a0=-1;  a0_color = DST_white;	
	pre = (pra=pl->r) + pl->runs;	/* prior line's runs */
	/* start b1/b2 on prior line's first black pixel, etc;
	   if none, then place off end of line */
	if(pra<pre) {b1=pra->xs; b2=pra->xe+1;} else b1=b2=wid;
	if(cl!=NULL&&cl->runs>0) {
		cre = (cra=cl->r) + cl->runs;
		a1=cra->xs;  a2=cra->xe+1;
#ifdef rg4_strict
		while( a0 < wid ) {
#else
		while( a0 < wid-1 ) {
#endif
			/* a0, a1, a2, b1, b2 are as defined in CCITT Rec. T.6 */
#ifdef DEBUG
			if(dbg_rg4_r)
			    fprintf(stderr,"f%d(%d,%d,%d) b%d(%d,%d)\n",
				cra-(cl->r),a0,a1,a2,pra-(pl->r),b1,b2);
#endif
			if(b2<a1) /* PASS mode */ {
				PASS_g4;
				a0=b2;
				/* a0-color, a1, & a2 are unchanged */
				detect_b1b2;
				}
			else if((a1b1=(a1-b1))<=3 && a1b1>=-3) {
				/* VERTICAL mode */
				switch(a1b1) {
				    case -3:  VL3_g4;  break;
				    case -2:  VL2_g4;  break;
				    case -1:  VL1_g4;  break;
				    case 0:  V0_g4;  break;
				    case 1:  VR1_g4;  break;
				    case 2:  VR2_g4;  break;
				    case 3:  VR3_g4;  break;
				    };
				a0=a1;  a0_color=flip_color(a0_color);
				detect_a1a2;
				detect_b1b2;
				}
			else {	/* HORIZONTAL mode */
				HORIZ_g4;
				a01=a1-a0; if(a0==-1) a01--;
				a12=a2-a1;
				if(a0_color==DST_white) {
					Wrun_g4(a01);
					Brun_g4(a12);
					}
				else {	Brun_g4(a01);
					Wrun_g4(a12);
					};
				a0=a2;
				/* a0_color is unchanged */
				detect_a1a2;
				detect_b1b2;
				};
			};
		}
	else /* current line is blank */ {
		a1=a2=wid;
#ifdef rg4_strict
		while( a0 < wid ) {
#else
		while( a0 < wid-1 ) {
#endif
			/* a0, a1, a2, b1, b2 are as defined in CCITT Rec. T.6 */
#ifdef DEBUG
			if(dbg_rg4_r)
			    fprintf(stderr,"f(%d,%d,%d) b%d(%d,%d)\n",
				a0,a1,a2,pra-(pl->r),b1,b2);
#endif
			if(b2<a1) /* PASS mode */ {
				PASS_g4;
				a0=b2;
				/* a0-color, a1, & a2 are unchanged */
				detect_b1b2;
				}
			else if((a1b1=a1-b1)<=3 && a1b1>=-3) {
				/* VERTICAL mode */
				switch(a1b1) {
				    case -3:  VL3_g4;  break;
				    case -2:  VL2_g4;  break;
				    case -1:  VL1_g4;  break;
				    case 0:  V0_g4;  break;
				    case 1:  VR1_g4;  break;
				    case 2:  VR2_g4;  break;
				    case 3:  VR3_g4;  break;
				    };
				a0=a1;  a0_color=flip_color(a0_color);
				/* a1, & a2 are unchanged */
				detect_b1b2;
				}
			else {	/* HORIZONTAL mode */
				HORIZ_g4;
				a01=a1-a0; if(a0==-1) a01--;
				a12=a2-a1;
				if(a0_color==DST_white) {
					Wrun_g4(a01);
					Brun_g4(a12);
					}
				else {	Brun_g4(a01);
					Wrun_g4(a12);
					};
				a0=a2;
				/* a0_color, a1, & a2 are unchanged */
				detect_b1b2;
				};
			};
		};
	}

EOF_to_g4(f)
    BITFILE *f;
{   char *cs;
	/* write EOFB */
	EOFB_g4;
#ifdef DEBUG
	if(dbg_rg4_e) fprintf(stderr,"EOF\n");
#endif
	}

#endif
#endif

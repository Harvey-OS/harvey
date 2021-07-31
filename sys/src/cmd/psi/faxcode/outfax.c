/* Main program illustrating use of bitmap to ccitt fax codes.

   cc -c outbitmap.c
   cc CCITT.o jslr.o outbitmap.o

   All *.h and *.o files are found in /n/pipe/usr/hsb/ocr

   */
#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "../system.h"
#ifdef FAX
#include "CPU.h"
#define MAIN 1		/* some include files are sensitive to this */
#include "limits.h"
#include "../njerq.h"

#define boolean int
#define T 1
#define F 0
typedef struct Pp {
	float x,y;
	} Pp;
#define Radians float

#include "Coord.h"
#include "rle.h"
#include "bitio.h"
#define CMDNAME "outbitmap"
Sp zero_Sp = Init_Zero_Sp;
Sps empty_Sps = Init_Sps;
Spa empty_Spa = Init_Spa;
Bbx empty_Bbx = Init_Bbx;
Bbx max_Bbx = Init_Max_Bbx;
Bbx null_Bbx = {Init_Max_Sp,Init_Min_Sp};
Bbxs empty_Bbxs = Init_Bbxs;
RLE_Lines empty_RLE_Lines = Init_RLE_Lines;
Transform_rlel_arg empty_Transform_rlel_arg = Init_Transform_rlel_arg;

BITFILE *bopen_rw(FILE *, char *);
unsigned long mybclose(BITFILE *);

/* Code common to all formats: */
BITFILE empty_BITFILE = Init_BITFILE;
BITFILE out_BITFILE = Init_BITFILE;

BITFILE *bopen_rw(FILE *s,char *t)
 {   BITFILE *f;
/*	if((f=(BITFILE *)malloc(sizeof(BITFILE)))==NULL) {
		err("bopen: can't alloc");
		return(NULL);
		};
	*f = empty_BITFILE;*/
	if(*t != 'w'){
		fprintf(stderr,"bopen: read not implemented");
		return(NULL);
	}
	f = &out_BITFILE;
	f->nb = 0;
	f->fp = s;
	f->type = *t;
	return(f);
	}

unsigned long mybclose(BITFILE *f)
{   unsigned long nb;
	if(f->type=='w') { bflush(f); fflush(f->fp); };
	nb=f->nb;
/*	free(f);*/
	return(nb);
	}
#ifdef w_1

BITFILE *bopen_w_1(FILE *s)
{   BITFILE *f;
	if((f=bopen_rw(s,"w"))!=NULL) {
		f->i.ss.s0.cc.c0=0000;
		f->cm=0200;
		};
	return(f);
	}
#endif

#ifdef G31
unsigned long fwr_bm_g31(FILE *,Bitmap *);
/* Write a bitmap to a stream in CCITT Group 3 1-dim encoding.
   Don't write picfile(5) header. */
unsigned long
fwr_bm_g31(FILE *fp,Bitmap *bm)
{   BITFILE *bf;
    int wid,y, widm;
    Word *l;
    RLE_Line rl;
    register unsigned char *cp,*cq;
	if((bf=bopen(fp,"w"))==(BITFILE *)NULL){
		exits("fwr_bm_g31: can't open output bitfile");
	}
	wid=bm->r.max.x-bm->r.min.x;
	widm = wid-1;
	BOF_to_g31(bf);
	/* for each line... */
	for(y=bm->r.min.y,l=bm->base;y<bm->r.max.y;y++,l+=bm->width) {
		/* run-length encode the binary raster line - little-endian/big-endian compiled in */
		rl.runs = rlbr((char *)l,widm,(short *)rl.r);
		rl.y=y;
		rl.len=wid;
		rlel_to_g31(&rl,wid,bf);
		};
	EOF_to_g31(bf);
	return(mybclose(bf));
	}
#endif
#ifdef G32
/* Write a bitmap to a stream in CCITT Group 3 2-dim encoding with a given `k'.
   Don't write picfile(5) header. */
fwr_bm_g32(FILE *fp,Bitmap *bm,int k)
{   BITFILE *bf;
    int wid,y;
    Word *l;
    static RLE_Line prl;
    RLE_Line rl,*pl;
    register unsigned char *cp,*cq;
	if((bf=bopen(fp,"w"))==NULL)
		abort("fwr_bm_g32: can't open output bitfile");
	wid=bm->rect.corner.x-bm->rect.origin.x;
	widm = wid-1;
	BOF_to_g32(bf);
	/* for each line... */
	for(y=bm->rect.origin.y,l=bm->base;y<bm->rect.corner.y;y++,l+=bm->width) {
		rl.runs = rlbr(l,widm,rl.r);  rl.y=y;  rl.len=wid;
		if(((rl.y-bm->rect.origin.y)%k)==0) pl=NULL; else pl=&prl;
		rlel_to_g32(pl,&rl,wid,bf);
		prl.y=rl.y;  prl.len=rl.len;  prl.runs=rl.runs;
		memcpy(prl.r,rl.r,2*prl.runs*sizeof(short));
		};
	EOF_to_g32(bf);
	mybclose(bf);
	}
#endif
#endif

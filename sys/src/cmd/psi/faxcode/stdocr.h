/* stdocr.h -- conventional OCGR constants, typedefs, and file formats
 NOTE: sensitive to prior `#define MAIN 1' & `#define CPU ??'  */

#ifndef MAIN
#define MAIN 0
#endif

#ifdef FCRAY
#define DIRSIZ (17)
#else
#ifdef V9_OS
#define DIRSIZ 14
#else
#ifdef SUN_OS
#define DIRSIZ (28)	/* not really correct, but better than 14 */
#ifdef SV_OS
#define DIRSIZ (28)	/* not really correct, but better than 14 */
#endif
#endif
#endif
#endif
#ifdef FSUN
#define sgn(v) (((v)>0)? 1: (((v)<0)? -1: 0))
#define swapshortin(x) (((x)<<8)&0xff00)|(((x)>>8)&0xff)
#define swapintin(x) \
		((((x)<<24)&0xff000000)| \
		 (((x)<<8) &0x00ff0000)| \
		 (((x)>>8) &0x0000ff00)| \
		 (((x)>>24)&0x000000ff))
/* The order of bytes in an int */
#define INDEX0 3
#define INDEX1 2
#define INDEX2 1
#define INDEX3 0
/* This controls stuff in skewlib.c */
#define USESHIFT
#else
#ifdef FATT3B
#define sgn(v) (((v)>0)? 1: (((v)<0)? -1: 0))
#define swapshortin(x) (((x)<<8)&0xff00)|(((x)>>8)&0xff)
#define swapintin(x) \
		((((x)<<24)&0xff000000)| \
		 (((x)<<8) &0x00ff0000)| \
		 (((x)>>8) &0x0000ff00)| \
		 (((x)>>24)&0x000000ff))
/* The order of bytes in an int */
#define INDEX0 3
#define INDEX1 2
#define INDEX2 1
#define INDEX3 0
/* This controls stuff in skewlib.c */
#define USESHIFT
#else
#define swapshortin(x) (x)
#define swapintin(x) (x)
#define INDEX0 0
#define INDEX1 1
#define INDEX2 2
#define INDEX3 3
#endif
#endif

#ifndef PI
#define PI 3.1415926535
#endif

#include "boole.h"
#include "limits.h"
#include "Units.h"
#include "Coord.h"

extern int Readvax;	/* T. Thompson flag:  see Coord.c */

/* 5620 dimensions, in dots */
#define Width5620 800
#define Height5620 1024

/* used (in clc.c) to compute absolute maximum value of natural 
   log values used in Bayesian binary weights */
#define LOG_ABS_MAX 3.0

#define Merit float		/* takes on values in [0,1] */
#define Pts float		/* text ``points'' units (~1/72) inch */
#define Ems float		/* em-space units */

/* class name - long enough to be a unix file name */
#define Cln_len (DIRSIZ)	/* maximum no. chars */
typedef char Cln[Cln_len+1];	/* class name */

/* font name */
#define Fontn_len (Cln_len)
typedef char Fontn[Fontn_len];

typedef struct ClassId {
	Fontn f;	/* font name */
	Pts s;		/* size in points */
	Cln c;		/* printable symbol name (``class-name'') */
	short v;	/* symbol variant no. */
	} ClassId;

#define Init_ClassId {"",0.0,"",0}
#ifdef MAIN
ClassId empty_ClassId = Init_ClassId;
#else
extern ClassId empty_ClassId;
#endif
#define eq_classid(a,b) ((!strcmp((a).f,(b).f))&&((a).s==(b).s)&&(!strcmp((a).c,(b).c))&&((a).v==(b).v))

/* parametric values: */
#define Pval float

typedef struct {	/* parametric point */
	Pval x;
	Pval y;
	} Pp;

#define Init_Zero_Pp {0.0,0.0}
#ifdef MAIN
Pp zero_Pp = Init_Zero_Pp;
#else
extern Pp zero_Pp;
#endif

/* metrics:  ABSS, EUCL, MAXV */
#define ABSS 'a'
#define EUCL 'e'
#define MAXV 'm'
typedef short Metric;

/* a vectorized blob file is a sequence of blob-descriptions.
   a blob-description is a sequence of "vector" records; there are two
   types of blob descriptions:
	DOT:
		'D'-vector (with bounding box)
	CHAR:
		'C'-vector, followed by:
			one 'B'-vector (bounding-box), and
			any number of: 'S' (stroke),
				       'G' (edge),
				       'O' (hole),
			               'A' (arc),
				       'C' (corner),
				       'P' (end-point), and
				       'V' & 'W' (boundary angles - 1 each),
			and terminated with one 'E' vector (giving classname)
   */

#define Vrec_len 5
typedef short Vrec[Vrec_len];
   
/* subshape values */
/* #define U 0     uninitialized */
#define NA (SHRT_MAX)	/* deliberately not-assigned (classifier failure) */

#define Dim short	/* specifies dimension 0,1,..,DIM-1 */

#define MaxDim 4

#define Seq int		/* sequence nos;  indices into tables */
#define FISeq 0		/* first value */
#define NLSeq -1	/* conventional NULL value */
#define S1Seq 0200000000  /* special bit 1 of Seq */
#define S2Seq 0100000000  /* special bit 2 of Seq */
#define NSSeq 0077777777  /* non-special bits */

typedef short Liv;	/* limit value: scaled and truncated from floating-pt */
#define Liv_MAX (SHRT_MAX)
#define Liv_MIN (SHRT_MIN)

typedef short Lit;	/* limit type, two flavors: */
#define MN 0		/* 	min:  MN < v */
#define MX 1		/*	max:  v <= MX */

typedef Liv Ivl[2];	/* interval:  ( Ivl[MN], Ivl[MX] ] */

typedef Ivl Rec[MaxDim];   /* rectangular parallelopiped of limits in DIM-space */

/* a normalized blob file is a sequence of:
	Nb_h	header, followed by Nb_h.ss of:
		Nb_s	contour shapes */
typedef struct {
	ClassId ci;	/* class name */
	Bbx bb;		/* bounding box */
	Pval rsz;	/* raw character size (Ems) */
	Pval bht;	/* height-above-baseline (Ems) */
	Pval rwd;	/* width (Ems) */
	Pval rar;	/* area (square-Ems) */
	Pval rpe;	/* perimiter (Ems) */
	Pval asp;	/* aspect ratio (h/w) */
	Pval blk;	/* fraction of Bbx area that is black */
	Pval per;	/* perimeter of blob as multiple of Bbx perim */
	Pval gale;	/* Gale's feature:  incl. angle 'tween 2 longest sides */
	short ss;	/* no. of shapes to follow */
	} Nb_h;

#define MAX_SHAPES_EACH	1024	/* Max no. shapes per Blob/Char/item */

/* Shape is tiny (below threshold, may be pruned):  a flag ORed into shape type */
#define Sh_tiny (0x80)

/* Shape types: */
#define U 0	/* uninitialized */
#define Sh_FI 1	/* first shape no. */
#define Sh_B 1	/* blob (connected black region) */
#define Sh_H 2	/* hole (connected white region) */
#define Sh_S 3	/* stroke (undirected line-segment) */
#define Sh_E 4	/* edge (directed line-segment) */
#define Sh_C 5  /* concavity (intrusion from convex hull) */
#define Sh_V 6  /* convexity (extrusion along exterior boundary) */
#define Sh_A 7	/* arc (along bdy) */
#define Sh_D 8  /* detail (along bdy) */
#define Sh_P 9	/* endpoint (0-junction) */
#define Sh_T 10	/* tee (3-junction) */
#define Sh_X 11 /* crossing (4-junction) */
#define Sh_Y 12 /* global scalar variables, combined */
#define Sh_Z 13 /* more global scalar variables, combined */
#define Sh_LA 13 /* last of variable-no-of-occurences shape-types */
#define Sh_MNY (Sh_LA+1)
#define SHS_MNY (Sh_MNY)

#ifdef MAIN
	/* shape-names, given shape-type */
	char Sh_nam[SHS_MNY] =
	{'U', 'B', 'H', 'S', 'E', 'C', 'V', 'A', 'D', 'P', 'T', 'X', 'Y', 'Z'};
	/* "dimension" -- no parameters in normalized form */
	short Sh_dim[SHS_MNY] =
	{ 0,   3,   3,   4,   4,   4,   4,   4,   4,   4,   2,   2,   3,   1};
	/* absolute minimum parametric values possible (see norm.c) */
	Pval Sh_MN[SHS_MNY][MaxDim]
		   = { { 0.0,  0.0,  0.0,  0.0},	/* U */
		       {-0.5, -0.5, -0.5/*see mkd*/},   /* B */
		       {-0.5, -0.5, -0.5/*see mkd*/},   /* H */
		       {-0.5, -0.5, -0.5, -0.5},	/* S */
		       {-0.5, -0.5, -0.5, -0.5},	/* E */
		       {-0.5, -0.5, -0.5, -0.5},	/* C */
		       {-0.5, -0.5, -0.5, -0.5},	/* V */
		       {-0.5, -0.5, -0.5, -0.5},	/* A */
		       {-0.5, -0.5, -0.5, -0.5},	/* D */
		       {-0.5, -0.5, -0.5, -0.5},	/* P */
		       {-0.5, -0.5},  			/* T */
		       {-0.5, -0.5},  			/* X */
		       {-0.5, -0.5, -0.5},	  	/* Y */
		       {-0.5}			  	/* Z */
			 };
	/* absolute maximum parametric value possible (see norm.c) */
	Pval Sh_MX[SHS_MNY][MaxDim]
		   = { { 0.0,  0.0,  0.0,  0.0},	/* U */
		       {0.5, 0.5, 1.0/*see mkd*/},      /* B */
		       {0.5, 0.5, 1.0/*see mkd*/},      /* H */
		       {0.5, 0.5, 0.5, 0.5},	        /* S */
		       {0.5, 0.5, 0.5, 0.5},	        /* E */
		       {0.5, 0.5, 0.5, 0.5},	        /* C */
		       {0.5, 0.5, 0.5, 0.5},	        /* V */
		       {0.5, 0.5, 0.5, 0.5},	        /* A */
		       {0.5, 0.5, 0.5, 0.5},	        /* D */
		       {0.5, 0.5, 0.5, 0.5},	        /* P */
		       {0.5, 0.5},  			/* T */
		       {0.5, 0.5},  			/* X */
		       {0.5, 0.5, 0.5},	  		/* Y */
		       {0.5}			  	/* Z */
			 };
	/* minimum magnitude of (r,i) part of certain shapes */
	Pval A_ri_minmag = 0.25;
	Pval P_ri_minmag = 0.25;
#else
	extern char Sh_nam[];
	extern short Sh_dim[];
	extern Pval Sh_MN[][MaxDim];
	extern Pval Sh_MX[][MaxDim];
	extern Pval A_ri_minmag;
	extern Pval P_ri_minmag;
#endif

#define MIN_SD 0.001	/* minimum std-dev permitted */

typedef Pval Spar[MaxDim];	/* shape parameters */

typedef struct Nb_s {
	short t;	/* shape type: one of U S O A C etc, perhaps |Sh_tiny */
	Spar p;		/* parametric values */
	} Nb_s;

/* indices into parametric values */
/* blobs */
#define B_x 0	/* (x,y) location of dot wrt bounding-box */
#define B_y 1
#define B_r 2	/* size of dot */
/* holes */
#define H_x 0	/* (x,y) location of center wrt bounding-box */
#define H_y 1
#define H_r 2	/* size of hole */
/* strokes */
#define S_x 0	/* (x,y) location of stroke center wrt bounding box */
#define S_y 1
#define S_r 2	/* (r,i) real-imag parts of rotation-length vector; */
#define S_i 3   /*   rotation angle *2 since strokes have only [0,PI] range */
/* edges */
#define E_x 0	/* (x,y) location of edge center wrt bounding box */
#define E_y 1
#define E_r 2	/* (r,i) real-imag parts of direction-length vector */
#define E_i 3 
/* concavities */
#define C_x 0	/* (x,y) location of center of hull-bdy wrt bounding-box */
#define C_y 1
#define C_r 2	/* (r,i) real-imag parts of direction-depth vector;*/
#define C_i 3
/* convexities */
#define V_x 0	/* (x,y) location of center of hull-bdy wrt bounding-box */
#define V_y 1
#define V_r 2	/* (r,i) real-imag parts of direction-depth vector;*/
#define V_i 3
/* arcs */
#define A_x 0	/* (x,y) location of bending-point wrt bounding-box */
#define A_y 1
#define A_r 2	/* (r,i) real-imag parts of rotation-length vector; */
#define A_i 3   /*   leave rotation angle alone, full [0,2PI] range */
/* endpoint */
#define P_x 0	/* (x,y) location of center of hull-bdy wrt bounding-box */
#define P_y 1
#define P_r 2	/* (r,i) real-imag parts of direction-depth vector;*/
#define P_i 3
/* crossings */
#define X_x 0	/* (x,y) location of crossing wrt bounding-box */
#define X_y 1
/* global scalars, combined */
#define Y_a 0	/* log(aspect_ratio=hgt/wid) */
#define Y_b 1	/* ratio of area to bbx_area */
#define Y_p 2	/* ratio of perimeter to bbx_perimeter */
/* global scalars, combined (more) */
#define Z_g 0	/* Gale's feature: included angle between two longest bdy edges */

/* scalar features field indices */
#define SF_RSZ 0	/* relative size (height in ems) */
#define SF_BHT 1	/* height above baseline (ems) */
#define SF_RWD 2	/* relative width (ems) */
#define SF_RAR 3	/* relative area (square-ems) */
#define SF_RPE 4	/* relative perimeter (ems) */
#define SF_ASP 5	/* aspect-ratio */
#define SF_BLK 6	/* fraction of Bbx that is black */
#define SF_PER 7	/* ratio of perimeter to BBx per */
#define SF_GALE 8	/* Gale's feature: incl. ang. between 2 longest edges */
#define SF_N 8		/* index of last var that's not an occurrence-count */
#define SF_MNY (SF_N+Sh_LA+1)

#ifdef MAIN
	/* ``Is this feature available early enough to be used in fast scalar-
	   feature preclassifier?'' (Must not depend on shape analysis.) */
	boolean SFfeature[SF_MNY] = {
		F, F, F, F, F, T, T, T, F,
		T, T, T, T, T, T, T, T, T, T, T, F, F };
	/* ``Is this feature discrete (integer-valued)?'' (Affects construction
	   of scalar-decision tree preclassifier.) */
	boolean SFdiscrete[SF_MNY] = {
		F, F, F, F, F, F, F, F, F,
		T, T, T, T, T, T, T, T, T, T, T, T, T };
#else
	extern boolean SFfeature[];
	extern boolean SFdiscrete[];
#endif

typedef Pval SFv[SF_MNY];

/* blob tracer (boundary angles) features field indices */
#define TR_MNY 8

typedef Pval TRv[TR_MNY];

/* tracer decision-tree header */
typedef struct TRtr {
	struct SFdnode *TRd;	/* array of decision-nodes */
	struct Cl ***clist;	/* list of (class-ptr)-lists */
	} TRtr;

/* Ss - sub-shape list-item */
typedef struct Ss {
	Seq seq;	/* globally unique sequence no */
	Seq shs;	/* shape-hdr seq no */
	Seq cls;	/* class sequence no */
	short no;	/* sub-shape number (0,1,2..., NA) */
	float focc;	/* fraction of training set w/ >=1 occurrence */
	float fdup;	/* fraction of training set w/ >=2 occurrence */
	Spar me;	/* mean parameters */
	Spar sd;	/* std-dev parameters */
	Spar min;	/* min limits (for fast checking) */
	Spar max;	/* max limits (for fast checking) */
	Rec r;		/* rectangular parallelopiped in Liv space */
	Pval fr;	/* fraction of training set covered */
	short nuse;	/* for current blob, no. uses */
	float nocc;	/* occurrences: no. blobs with >=1 of these */
	float ndup;	/* duplicates: no. blobs with >=2 of these */
	float mind;	/* minimum scaled distance from cluster among uses */
	float merit;	/* merit score resulting from 'nuse' matches to this */
	struct Ss *ne;	/* ptr to next in list */
	} Ss;

#define MAX_SS 8192	/* see 'kdt.h' for reasons */

/* BMask:  1-d, variable-length packed bitstring (N = no. bits).  Bitstring
   is stored in an array of (unsigned int).  It is often accessed as (unsigned
   short).
      BMask_ni(N)    no. unsigned ints of bit-data
      BMask_si(N)    no. unsigned shorts of bit-data
      BMask_ci(N)    no. unsigned chars of bit-data
      BMask_size(N)  total no. bytes in, or pointed to by BMask:
			sizeof(n) + sizeof(mny) + sizeof(r) + sizeof(malloc space) 
      */
#define BMask_ni(N) (((N)+(8*sizeof(unsigned int))-1)/(8*sizeof(int)))
#define BMask_si(N) (((N)+(8*sizeof(unsigned short))-1)/(8*sizeof(short)))
#define BMask_ci(N) (sizeof(unsigned int)*BMask_ni((N)))
#define BMask_size(N) (3*sizeof(unsigned int)+(BMask_ci((N))))

typedef struct BMask {
	unsigned int n;			/* no. of bits in structure (==N above) */
	unsigned int mny;		/* no. bits set to 1 */
	unsigned int r;			/* represents `r' BMasks altogether */
	union {	/* packed bits: 0th bit is 01 */
		unsigned int *i;	/* malloc space: int [Bmask_ni] */
		unsigned short *s;	/* malloc space: short [Bmask_si] */
		int ii;			/* index into (unsigned int)[] array */
		int si;			/* index into (unsigned short)[] array */
		} u;
	} BMask;

#define Init_BMask {0,0,0,}
#ifdef MAIN
	BMask empty_BMask = Init_BMask;
#else
	extern empty_BMask;
#endif


typedef struct BMasks {
	short mny;		/* no. BMasks */
	short shallow;		/* 0 <= shallow <= mny */
	short alloc;		/* no. items allocated in .a[] */
	union {	BMask *a;	/* array[mny] of BMasks */
		int bi;		/* index into (BMask)[] array */
		} u;
	} BMasks;

#define Init_BMasks {0,0,0,}
#ifdef MAIN
	BMasks empty_BMasks = Init_BMasks;
#else
	extern empty_BMasks;
#endif

/* Sh - shape list-item */
typedef struct Sh {
	Seq seq;	/* unique sequence no. */
	short t;	/* shape type: S, H, X, etc */
	short nss;	/* no. sub-shapes in the list */
	float mess;	/* mean no. sub-shapes in training set */
	float sdss;	/* std-dev of sub-shapes in training set */
	float NAocc;	/* prob at least one shape not-assigned */
	float NAdup;	/* prob more than one not-assigned */
	Ss NAss;	/* "not-assigned" data */
	Ss *fi;		/* first sub-shape  */
	Ss *la;		/* last sub-shape */
	struct Sh *ne;	/* next shape */
	} Sh;

/* Cl - class list-item */
typedef struct Cl {
	Seq seq;	/* unique sequence no. 0,1,... */
	Cln c;		/* class name */
	float shNA;	/* fraction of shapes unassigned */
	float blDP;	/* fraction of blobs w/ >=1 duplicate shape-match*/
	float blAL;	/* average extra alternate shape-matches / blob */
	SFv sf_me;	/* scalar-features:  means, std-devs */
	SFv sf_sd;
	BMask bm;	/* canonical BMask */
	BMasks bms;	/* additional representative BMask records */
	Sh *sha[Sh_MNY];	/* table of pointers to shape-types */
	Sh *fi;			/* first owned shape */
	Sh *la;			/* last owned shape */
	short unmat[Sh_MNY];	/* counts of unmatched shapes/shape-type */
	struct Ssm *ssmfi;	/* first sub-shape match for this class */
	TRtr *tr;		/* pointer to tracer decision tree (if any) */
	TRv tr_me;		/* tracer-features:  means, std-devs */
	TRv tr_sd;
	float m_me;		/* mean, std-err of merit (in training set) */
	float m_sd;
	float merit;	/* Bayesian merit: a posteriori log-probability */
	Merit m01;	/* merit in range [0,1] */
	short pass;	/* the last classification method this class passed */
	int ch_mny;	/* No. Chars of this class in a subset */
	int ss_mny;	/* No. Chars with given feature */
	struct Cl *ne;	/* next class */
	} Cl;

/* global max no. classes -- can be upped safely */
#ifdef FSUN
#define MAX_CL 128
#else
#define MAX_CL 1000
#endif
#define MAX_SH (MAX_CL*SH_MNY)

typedef struct Classes {
	int mny;		/* number of items in array */
	Cl *clpa[MAX_CL+1];	/* NULL-terminated array of pointers */
	} Classes;

typedef struct Ssm {	/* sub-shape match record */
	Seq isn;	/* input shape sequence no (w/in blob) */
	Cl *clp;	/* owning class, shape, sub-shape... */
	Sh *shp;
	Ss *ssp;
	float mind;	/* distance to closest assigned sub-shape */
	boolean alt;	/* T if this is an alternative (not the first) */
	struct Ssm *ne;	/* next, prior ptrs in list */
	struct Ssm *pr;
	} Ssm;


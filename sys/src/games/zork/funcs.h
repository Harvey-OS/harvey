/* funcs.h -- functions for dungeon */

#ifndef FUNCS_H
#define FUNCS_H

/* If __STDC__ is not defined, don't use function prototypes, void, or
 * const.
 */

#ifdef __STDC__
#define P(x) x
#else
#define P(x) ()
#define void int
#define const
#endif

/* Try to guess whether we need "rb" to open files in binary mode.
 * If this is unix, it doesn't matter.  Otherwise, assume that if
 * __STDC__ is defined we can use "rb".  Otherwise, assume that we
 * had better use "r" or fopen will fail.
 */

#define BINREAD "r"
#define BINWRITE "w"

typedef int integer;
typedef int logical;

#define TRUE_ (1)
#define FALSE_ (0)

#undef abs
#define abs(x) ((x) >= 0 ? (x) : -(x))
#undef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#undef max
#define max(a,b) ((a) >= (b) ? (a) : (b))

extern logical
	protected P((void)),
	wizard P((void));

extern void
	more_init P((void)),
	more_output P((const char *)),
	more_input P((void));

extern void
	bug_ P((integer, integer)),
	cevapp_ P((integer)),
	cpgoto_ P((integer)),
	cpinfo_ P((integer, integer)),
	encryp_ P((const char *, char *)),
	exit_ P((void)),
	fightd_ P((void)),
	game_ P((void)),
	gdt_ P((void)),
	gttime_ P((integer *)),
	invent_ P((integer)),
	itime_ P((integer *, integer *, integer *)),
	jigsup_ P((integer)),
	newsta_ P((integer, integer, integer, integer, integer)),
	orphan_ P((integer, integer, integer, integer, integer)),
	princo_ P((integer, integer)),
	princr_ P((logical, integer)),
	rdline_ P((char *, integer)),
	rspeak_ P((integer)),
	rspsb2_ P((integer, integer, integer)),
	rspsub_ P((integer, integer)),
	rstrgm_ P((void)),
	savegm_ P((void)),
	score_ P((logical)),
	scrupd_ P((integer)),
	swordd_ P((void)),
	thiefd_ P((void)),
	valuac_ P((integer));
extern integer
	blow_ P((integer, integer, integer, logical, integer)),
	fights_ P((integer, logical)),
	fwim_ P((integer, integer, integer, integer, integer, logical)),
	getobj_ P((integer, integer, integer)),
	schlst_ P((integer, integer, integer, integer,  integer, integer)),
	mrhere_ P((integer)),
	oactor_ P((integer)),
	rnd_ P((integer)),
	robadv_ P((integer, integer, integer, integer)), 
	robrm_ P((integer, integer, integer, integer, integer)),
	sparse_ P((const integer *, integer, logical)),
	vilstr_ P((integer)),
	weight_ P((integer, integer, integer));
extern logical
	aappli_ P((integer)),
	ballop_ P((integer)),
	clockd_ P((void)),
	cyclop_ P((integer)),
	drop_ P((logical)),
	findxt_ P((integer, integer)),
	ghere_ P((integer, integer)),
	init_ P((void)),
	lightp_ P((integer)),
	lit_ P((integer)),
	moveto_ P((integer, integer)),
	nobjs_ P((integer, integer)),
	oappli_ P((integer, integer)),
	objact_ P((void)),
	opncls_ P((integer, integer, integer)),
	parse_ P((char *, logical)),
	prob_ P((integer, integer)),
	put_ P((logical)),
	rappli_ P((integer)),
	rappl1_ P((integer)),
	rappl2_ P((integer)),
	rmdesc_ P((integer)),
	sobjs_ P((integer, integer)),
	sverbs_ P((integer)),
	synmch_ P((void)),
	take_ P((logical)),
	thiefp_ P((integer)),
	trollp_ P((integer)),
	qempty_ P((integer)),
	qhere_ P((integer, integer)),
	vappli_ P((integer)),
	walk_ P((void)),
	winnin_ P((integer, integer)),
	yesno_ P((integer, integer, integer));

#endif

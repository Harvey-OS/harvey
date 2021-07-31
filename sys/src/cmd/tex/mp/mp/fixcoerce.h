/*
 * The C compiler ignores most unnecessary casts (i.e., casts of something
 * to its own type).  However, for structures, it doesn't.  Therefore,
 * we have to redefine this macro so that it doesn't try to cast
 * the argument (a memoryword) as a memoryword.
 */
#undef	printword
#define	printword(x)	zprintword(x)
#undef	tfmqqqq
#define	tfmqqqq(x)	ztfmqqqq(x)
#undef finishdimen
#define finishdimen(a,b) zfinishdimen((fontnumber)(a),(b))


/*
 * These external routines need casting of the arguments
 */
#define	testaccess(a,b,c,d)	ztestaccess(a, b, (integer) c, (integer) d)

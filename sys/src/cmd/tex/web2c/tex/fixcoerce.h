/*
 * The C compiler ignores most unnecessary casts (i.e., casts of something
 * to its own type).  However, for structures, it doesn't.  Therefore,
 * we have to redefine these two macros so that they don't try to cast
 * the argument (a memoryword) as a memoryword.
 */
#undef	eqdestroy
#define	eqdestroy(x)	zeqdestroy(x)
#undef	printword
#define	printword(x)	zprintword(x)

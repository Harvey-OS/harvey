/* Some definitions that get appended to the `coerce.h' file that web2c
   outputs.  */

/* The C compiler ignores most unnecessary casts (i.e., casts of
   something to its own type).  However, for structures, it doesn't.
   Therefore, we have to redefine these macros so they don't cast
   cast their argument (of type memoryword or fourquarters,
   respectively).  */
#undef	printword
#define	printword(x)	zprintword (x)

#undef	tfmqqqq
#define tfmqqqq(x)	ztfmqqqq (x)

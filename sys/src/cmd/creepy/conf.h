

#define TESTING

enum
{
	KiB = 1024UL,
	MiB = KiB * 1024UL,
	GiB = MiB * 1024UL,

	Incr = 16,
	Fsysmem = 2*GiB,		/* size for in-memory block array */

	/* disk parameters; don't change */
	Dblksz = 16*KiB,		/* disk block size */
	Ndptr = 8,		/* # of direct data pointers */
	Niptr = 4,		/* # of indirect data pointers */

	Syncival = 60,		/* desired sync intervals (s) */

	Mmaxdirtypcent = 50,	/* Max % of blocks dirty in mem */
	Mminfree = 50,		/* # blocks when low on mem blocks */
	Dminfree = 1000,		/* # blocks when low on disk blocks */
	Dminattrsz = Dblksz/2,	/* min size for attributes */

	Nahead = 10 * Dblksz,	/* # of bytes to read ahead */

	/*
	 * Caution: Errstack also limits the max tree depth,
	 * because of recursive routines (in the worst case).
	 */
	Stack = 64*KiB,		/* stack size for threads */
	Errstack = 128,		/* max # of nested error labels */
	Fhashsz = 7919,		/* size of file hash (plan9 has 35454 files). */
	Fidhashsz = 97,		/* size of the fid hash size */
	Uhashsz = 97,

	Rpcspercli = 0,		/* != 0 places a limit */

	Nlstats = 1009,		/* # of lock profiling entries */

	Mmaxfree = 2*Mminfree,		/* high on mem blocks */
	Dmaxfree = 2*Dminfree,		/* high on disk blocks */
	Mzerofree = 10,			/* out of memory blocks */
	Dzerofree = 10,			/* out of disk blocks */

	Unamesz = 20,
	Statsbufsz = 1024,

};


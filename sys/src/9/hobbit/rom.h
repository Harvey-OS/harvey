enum {
	Nenv		= 11,		/* max. number of environment variables */
	Nnamelen	= 15,		/* Max length of environment name */
	Nvaluelen	= 47,		/* Max length of environment value */

	Ncmdlen		= 220,		/* Max length of command line */
	Nargc		= 8,		/* Max number of command line arguments */
};

typedef struct {
	char	name[Nnamelen+1];
	char	value[Nvaluelen+1];
} Entry;

typedef struct {
	int	nentry;
	uchar	sum[4];
	Entry	entry[Nenv];
} Env;

typedef struct {
	void	(*vectors[16])(void);	/* offset 0x0000: vector table */

	ulong	msp;			/* offset 0x0040: saved registers on reset */
	ulong	isp;			/* offset 0x0044: */
	ulong	sp;			/* offset 0x0048: */
	ulong	config;			/* offset 0x004C: */
	ulong	psw;			/* offset 0x0050: */
	ulong	shad;			/* offset 0x0054: */
	ulong	vb;			/* offset 0x0058: */
	ulong	stb;			/* offset 0x005C: */
	ulong	fault;			/* offset 0x0060: */
	ulong	id;			/* offset 0x0064: */
	ulong	r0;			/* offset 0x0068: */
	ulong	r16;			/* offset 0x006C: */
	ulong	spare0x0070[4];

	ulong	memsize;		/* offset 0x0080: memory size */
	void	(*fpi)(void);		/* offset 0x0084: fp interpreter vector */
	ulong	spare0x0088[30];

	int	argc;			/* offset 0x0200: boot args */
	char	*argv[Nargc];		/* offset 0x0204 */
	char	argbuf[Ncmdlen];	/* offset 0x0224 */

	Env	env;			/* offset 0x0300: environment */
} PPage0;

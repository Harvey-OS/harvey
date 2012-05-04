/*
 * Exceptions thrown by the interpreter
 */	
extern	char exAlt[];		/* alt send/recv on same chan */
extern	char exBusy[];		/* channel busy */
extern	char exModule[];	/* module not loaded */
extern	char exCompile[];	/* compile failed */
extern	char exCrange[];	/* constant range */
extern	char exCctovflw[];	/* constant table overflow */
extern	char exCphase[];	/* compiler phase error */
extern	char exType[];		/* type not constructed correctly */
extern	char exZdiv[];		/* zero divide */
extern	char exHeap[];		/* out of memory: heap */
extern	char	exImage[];	/* out of memory: image */
extern	char exItype[];		/* inconsistent type */
extern	char exMathia[];	/* invalid math argument */
extern	char exBounds[];	/* array bounds error */
extern	char exNegsize[];	/* negative array size */
extern	char exNomem[];		/* out of memory: main */
extern	char exSpawn[];		/* spawn a builtin module */
extern	char exOp[];		/* illegal dis instruction */
extern	char exTcheck[];	/* type check */
extern	char exInval[];		/* invalid argument */
extern	char exNilref[];	/* dereference of nil */
extern	char exRange[];		/* value out of range */
extern  char exLoadedmod[];     /* need newmod */

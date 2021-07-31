void	deflateinit(void);
int	deflate(void *wr, int (*w)(void*, void*, int), void *rr, int (*r)(void*, void*, int), int level, int debug);

void	inflateinit(void);
int	inflate(void *wr, int (*w)(void*, void*, int), void *getr, int (*get)(void*));

void	mkcrctab(ulong);
ulong	blockcrc(ulong, void*, int);

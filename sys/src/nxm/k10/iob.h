
/* io bufs */
void	iobufinit(void);
Block*	va2block(void*);
void*	io2alloc(uint);
Block*	bigalloc(void);
Block*	sbigalloc(void);
int	isbigblock(Block*);

/* bal.c */
physaddr	bal(usize);
void		bfree(physaddr, usize);
void		balinit(physaddr, usize);
void		balfreephys(physaddr, usize);
void	baldump(void);

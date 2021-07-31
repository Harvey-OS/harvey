void*	emalloc(uint);
void*	erealloc(void*, uint);
void		error(byte*, ...);
void		run(byte**, int, chan(int));

#define	USED(x)			{if(x);}

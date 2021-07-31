int		openlockfile(byte*);
void		run(byte**, chan(int), int*);
void		error(byte*, ...);
int		tryopen(byte*, int);

#define	USED(x)			{if(x);}

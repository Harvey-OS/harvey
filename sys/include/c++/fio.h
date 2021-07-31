#pragma lib "ape/libap.a"
#pragma lib "c++/libfio.a"

#define	FIOBSIZE	4096

typedef struct
{
	char	*next;			/* next char to be used */
	char	*end;			/* first invalid char */
	char	*lnext;			/* previous value of next */
	short	flags;			/* see FIO_.* below */
	short	oflush;			/* if >= 0 fd to flush on read */
	long	offset;			/* seek of end */
	char	buf[FIOBSIZE];
} Fbuffer;
extern Fbuffer		**Ffb;

#define	FIOSET(f, fd)	f = _F_set(fd)
/*
 * FIOLINELEN is length of last input
 */
#define	FIOLINELEN(fd)	((Ffb[fd]->next - Ffb[fd]->lnext) - 1)
/*
 * FIOSEEK is lseek of next char to be processed
 */
#define	FIOSEEK(fd)	(Ffb[fd]->offset - (Ffb[fd]->end - Ffb[fd]->next))
#define	FIOFLUSH(fd)	if((fd >= 0) && Ffb[fd])\
				Fflush(fd)

#define	FIO_RDLAST	0x0001		/* true if last input was rdline */
#define	FIO_WRITING	0x0002		/* true if last action was write */
#define	FIO_MALLOCED	0x0004		/* set if Finit did the malloc */
#define	FIO_GETC	0x0008		/* ok to do getc macro */

#ifdef __cplusplus
extern "C"
{
#endif
	void	Finit(int, void*);
	void	Fclose(int);
	char*	Frdline(int);
	void	Fundo(int);
	int	Fgetc(int);
	double	Fgetd(int);
	int	Fputc(int, int);
	long	Fread(int, void*, long);
	long	Fwrite(int, void*, long);
	long	Fseek(int, long, int);
	int	Fflush(int);

	int	_F_fillbuf(int);
	void	Fexit(int);
	int	_F_flush(Fbuffer*, int);
	void	Ftie(int, int);
	Fbuffer*	_F_set(int);
	void	_F_reset(Fbuffer*);

	int	Fprint(int, char *, ...);
#ifdef __cplusplus
};
#endif

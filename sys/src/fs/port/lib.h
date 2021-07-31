/*
 * functions (possibly) linked in, complete, from libc.
 */

/*
 * mem routines
 */
extern	void*	memset(void*, int, long);
extern	int	memcmp(void*, void*, long);
extern	void*	memmove(void*, void*, long);
extern	void*	memchr(void*, int, long);

/*
 * string routines
 */
extern	char*	strcat(char*, char*);
extern	char*	strchr(char*, char);
extern	int	strcmp(char*, char*);
extern	char*	strcpy(char*, char*);
extern	char*	strncat(char*, char*, long);
extern	char*	strncpy(char*, char*, long);
extern	int	strncmp(char*, char*, long);
extern	long	strlen(char*);

/*
 * utf routines
 */
extern	char*	utfrune(char*, long);
extern	char*	utfrrune(char*, long);

/*
 * print routines
 */

#define	FLONG	(1<<0)
#define	FSHORT	(1<<1)
#define	FUNSIGN	(1<<2)

typedef struct Op	Op;
struct Op
{
	char	*p;
	char	*ep;
	void	*argp;
	int	f1;
	int	f2;
	int	f3;
};
extern	void	strconv(char*, Op*, int, int);
extern	int	numbconv(Op*, int);
extern	char*	donprint(char*, char*, char*, void*);
extern	int	fmtinstall(char, int(*)(Op*));
extern	int	sprint(char*, char*, ...);
extern	int	print(char*, ...);

/*
 * one-of-a-kind
 */
extern	void	qsort(void*, long, long, int (*)(void*, void*));
extern	char	end[];

#define DESKEYLEN	7		/* length of a des key for encrypt/decrypt */
extern	int	decrypt(void*, void*, int);
extern	int	encrypt(void*, void*, int);
extern	int	nrand(int);

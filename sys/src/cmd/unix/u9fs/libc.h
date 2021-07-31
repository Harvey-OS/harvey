#ifndef	NEEDPROTO

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#else
/*
 * mem routines
 */
void*	memset(void*, int, Ulong);
int	memcmp(void*, void*, Ulong);
void*	memmove(void*, void*, Ulong);
void*	memchr(void*, int, Ulong);
void*	memccpy(void*, void*, int, Ulong);

/*
 * string routines
 */
char*	strcat(char*, char*);
char*	strchr(char*, char);
int	strcmp(char*, char*);
char*	strcpy(char*, char*);
char*	strncat(char*, char*, long);
char*	strncpy(char*, char*, long);
int	strncmp(char*, char*, long);
char*	strrchr(char*, char);
char*	strtok(char*, char*);
long	strlen(char*);
long	strspn(char*, char*);
long	strcspn(char*, char*);
char*	strpbrk(char*, char*);
char*	strdup(char*);

/*
 * malloc
 */
void*	malloc(long);
void	free(void*);
void*	realloc(void*, long);

/*
 * one-of-a-kind
 */
void	abort(void);

extern	int errno;
extern	char *sys_errlist[];

int	chmod(const char*, mode_t);
int	chown(char*, int, int);
int	close(int);
int	creat(char*, int);
int	fchmod(int, mode_t);
int	fchown(int, int, int);
int	getpid(void);
int	link(char*, char*);
long	lseek(int, long, int);
int	mkdir(const char*, mode_t);
int	open(char*, int, ...);
long	read(int, void*, int);
int	rmdir(char*);
int	stat(const char*, struct stat*);
int	unlink(char*);
long	write(int, void*, int);
#endif

/* A really simple argument processor from Plan 9 */

#define USED(x)	if(x);else
#define SET(x) (x)=0
#define	ARGBEGIN	for(argv++,argc--;\
			    argv[0] && argv[0][0]=='-' && argv[0][1];\
			    argc--, argv++) {\
				char *_args, *_argt;\
				char _argc;\
				_args = &argv[0][1];\
				if(_args[0]=='-' && _args[1]==0){\
					argc--; argv++; break;\
				}\
				_argc = 0;\
				while(*_args && (_argc = (*_args++)))\
				switch(_argc)
#define	ARGEND		SET(_argt);USED(_argt);USED(_argc);USED(_args);}USED(argv);USED(argc);
#define	ARGF()		(_argt=_args, _args="",\
				(*_argt? _argt: argv[1]? (argc--, *++argv): 0))
#define	ARGC()		_argc

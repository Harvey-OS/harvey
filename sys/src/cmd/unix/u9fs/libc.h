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

#ifdef	NEEDPROTO
int	chmod(const char*, int);
int	chown(char*, int, int);
int	close(int);
int	creat(char*, int);
int	fchmod(int, int);
int	fchown(int, int, int);
int	getpid(void);
int	link(char*, char*);
long	lseek(int, long, int);
int	mkdir(const char*, int);
int	open(char*, int, ...);
long	read(int, void*, int);
int	rmdir(char*);
int	stat(const char*, struct stat*);
int	unlink(char*);
long	write(int, void*, int);
#endif

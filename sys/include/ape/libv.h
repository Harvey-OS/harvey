#ifndef __LIBV_H
#define __LIBV_H
#ifndef _RESEARCH_SOURCE
   This header file is not defined in ANSI or POSIX
#endif
#pragma lib "/$M/lib/ape/libv.a"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef stdin
extern FILE	*popen(char *, char *);
extern int	pclose(FILE *);
#endif

#ifdef __DIRENT_H
#ifdef __TYPES_H
extern off_t	telldir(DIR *);
extern void	seekdir(DIR *, off_t);
#endif
#endif

#ifdef __STAT_H
extern int	lstat(char *, struct stat *);
extern int	symlink(char *, char *);
extern int	readlink(char *, char*, int);
#endif

extern void	srand(unsigned int);
extern int	rand(void);
extern int	nrand(int);
extern long	lrand(void);
extern double	frand(void);

extern int	getopt(int, char**, char*);
extern int	opterr;
extern int	optind;
extern int	optopt;
extern char	*optarg;

extern char	*getpass(char *);
extern int	tty_echoon(int);
extern int	tty_echooff(int);

extern int	min(int, int);
extern int	max(int, int);

extern void	_perror(char *);
extern char	*_progname;

extern int	nap(int);

extern char	*setfields(char *);
extern int	getfields(char *, char **, int);
extern int	getmfields(char *, char **, int);

#ifdef __cplusplus
};
#endif

/*
 *  FTW stuff
 */

typedef struct FTW Ftw;

#ifdef __cplusplus
extern "C"{
#endif

extern int	ftw(char *path,
		    int (*fn)(char*, struct stat *, int, Ftw *),
		    int depth);

#ifdef __cplusplus
};
#endif

/*
 *	Codes for the third argument to the user-supplied function
 *	which is passed as the second argument to ftw...
 */

#define	FTW_F	0	/* file */
#define	FTW_D	1	/* directory */
#define	FTW_DNR	2	/* directory without read permission */
#define	FTW_NS	3	/* unknown type, stat failed */
#define FTW_DP	4	/* directory, postorder visit */
#define FTW_SL	5	/* symbolic link */
#define FTW_NSL 6	/* stat failed (errno = ENOENT) on symbolic link */

/*	Values the user-supplied function may wish to assign to
	component quit of struct FTW...
 */

#define FTW_SKD 1	/* skip this directory (2nd par = FTW_D) */
#define FTW_SKR 2	/* skip rest of current directory */
#define FTW_FOLLOW 3	/* follow symbolic link */

struct FTW { int quit, base, level;
#ifndef FTW_more_to_come
	};
#endif


#endif /* __LIBV_H */

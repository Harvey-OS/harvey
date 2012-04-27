#ifndef __STAT_H
#define __STAT_H

#ifndef __TYPES_H
#include <sys/types.h>
#endif

#pragma lib "/$M/lib/ape/libap.a"

/*
 * stat structure, used by stat(2) and fstat(2)
 */
struct	stat {
	dev_t	st_dev;
	ino_t	st_ino;
	mode_t 	st_mode;
	nlink_t	st_nlink;
	uid_t 	st_uid;
	gid_t 	st_gid;
	off_t	st_size;
	time_t	st_atime;
	time_t	st_mtime;
	time_t	st_ctime;
};

#define	S__MASK		     0170000
#ifdef _RESEARCH_SOURCE
#define S_ISLNK(m)	(((m)&S__MASK) == 0120000)
#endif
#define S_ISREG(m)	(((m)&S__MASK) == 0100000)
#define S_ISDIR(m)	(((m)&S__MASK) == 0040000)
#define S_ISCHR(m)	(((m)&S__MASK) == 0020000)
#define S_ISBLK(m)	(((m)&S__MASK) == 0060000)
#define S_ISFIFO(m)	(((m)&S__MASK) == 0010000)

#define	S_ISUID	04000		/* set user id on execution */
#define	S_ISGID	02000		/* set group id on execution */
#define	S_IRWXU	00700		/* read, write, execute: owner */
#define	S_IRUSR	00400		/* read permission: owner */
#define	S_IWUSR	00200		/* write permission: owner */
#define	S_IXUSR	00100		/* execute permission: owner */
#define	S_IRWXG	00070		/* read, write, execute: group */
#define	S_IRGRP	00040		/* read permission: group */
#define	S_IWGRP	00020		/* write permission: group */
#define	S_IXGRP	00010		/* execute permission: group */
#define	S_IRWXO	00007		/* read, write, execute: other */
#define	S_IROTH	00004		/* read permission: other */
#define	S_IWOTH	00002		/* write permission: other */
#define	S_IXOTH	00001		/* execute permission: other */

#ifdef _BSD_EXTENSION
#define S_IFMT S__MASK
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFBLK 0060000
#define S_IFREG 0100000
#define S_IFIFO 0010000
#define S_IFLNK 0120000
#define S_IFSOCK S_IFIFO
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern mode_t umask(mode_t);
extern int mkdir(const char *, mode_t);
extern int mkfifo(const char *, mode_t);
extern int stat(const char *, struct stat *);
extern int fstat(int, struct stat *);
extern int chmod(const char *, mode_t);

#ifdef _BSD_EXTENSION
#pragma lib "/$M/lib/ape/libbsd.a"
extern int	lstat(char *, struct stat *);
extern int	symlink(char *, char *);
extern int	readlink(char *, char*, int);
#endif

#ifdef __cplusplus
}
#endif

#endif

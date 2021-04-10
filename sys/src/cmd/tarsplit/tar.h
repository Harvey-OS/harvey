/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* tar archive format definitions and functions */

#define islink(lf)	(isreallink(lf) || issymlink(lf))
#define isreallink(lf)	((lf) == Lflink)
#define issymlink(lf)	((lf) == Lfsymlink1 || (lf) == Lfsymlink2)

#define HOWMANY(a, size)	(((a) + (size) - 1) / (size))
#define ROUNDUP(a, size)	(HOWMANY(a, size) * (size))

#define TAPEBLKS(bytes)		HOWMANY(bytes, Tblock)

enum {
	Tblock = 512u,
	Namesz = 100,

	/* link flags */
	Lfplain1 = '\0',
	Lfplain2 = '0',
	Lflink,
	Lfsymlink1,
	Lfchr,
	Lfblk,
	Lfdir,
	Lffifo,
	Lfcontig,
	Lfsymlink2 = 's',
};

typedef struct {
	char	name[Namesz];
	char	mode[8];
	char	uid[8];
	char	gid[8];
	char	size[12];
	char	mtime[12];
	char	chksum[8];
	char	linkflag;
	char	linkname[Namesz];
} Header;

typedef union {
	u8	dummy[Tblock];
	Header header;
} Hblock;

/* tarsub.c */
char *thisnm, *lastnm;

unsigned checksum(Hblock *hp);
int	closeout(int outf, char *, int prflag);
int	getdir(Hblock *, int in, i64 *);
u32	otoi(char *s);
void	newarch(void);
u64	passtar(Hblock *hp, int in, int outf, i64 bytes);
void	putempty(int out);
void	readtar(int in, char *buffer, i32 size);
u64	writetar(int outf, char *buffer, u32 size);

/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Data	Data;
typedef struct Page	Page;
typedef struct Proc	Proc;
typedef struct Seg	Seg;

enum {
	Psegment = 0,
	Pfd,
	Pfpregs,
	Pkregs,
	Pnoteid,
	Pns,
	Pproc,
	Pregs,
	Pstatus,
	Npfile,

	Pagesize = 1024,	/* need not relate to kernel */
};

struct Data {
	uint32_t len;
	char data[1];
};

struct Seg {
	char*	name;
	uint64_t	offset;
	uint64_t	 len;
	Page**	pg;
	int	npg;
};

struct Page {
	Page*	link;
	uint32_t	len;
	char*	data;

	/* when page is written, these hold the ptr to it */
	int	written;
	int	type;
	uint32_t	pid;
	uint64_t	offset;
};

struct Proc {
	Proc *link;
	int32_t	pid;
	Data*	d[Npfile];
	Seg**	seg;	/* memory segments */
	int	nseg;
	Seg*	text;	/* text file */
};

extern char *pfile[Npfile];

Proc*	snap(int32_t pid, int usetext);
void*	emalloc(uint32_t);
void*	erealloc(void*, uint32_t);
char*	estrdup(char*);
void	writesnap(Biobuf*, Proc*);
Page*	datapage(char *p, int32_t len);
Proc*	readsnap(Biobuf *b);
Page*	findpage(Proc *plist, int32_t pid, int type, uint64_t off);

int	debug;

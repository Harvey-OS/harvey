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
	u32 len;
	char data[1];
};

struct Seg {
	char*	name;
	u64	offset;
	u64	 len;
	Page**	pg;
	int	npg;
};

struct Page {
	Page*	link;
	u32	len;
	char*	data;

	/* when page is written, these hold the ptr to it */
	int	written;
	int	type;
	u32	pid;
	u64	offset;
};

struct Proc {
	Proc *link;
	i32	pid;
	Data*	d[Npfile];
	Seg**	seg;	/* memory segments */
	int	nseg;
	Seg*	text;	/* text file */
};

extern char *pfile[Npfile];

Proc*	snap(i32 pid, int usetext);
void*	emalloc(u32);
void*	erealloc(void*, u32);
char*	estrdup(char*);
void	writesnap(Biobuf*, Proc*);
Page*	datapage(char *p, i32 len);
Proc*	readsnap(Biobuf *b);
Page*	findpage(Proc *plist, i32 pid, int type, u64 off);

int	debug;

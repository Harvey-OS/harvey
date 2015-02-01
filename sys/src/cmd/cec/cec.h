/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct {
	uchar	dst[6];
	uchar	src[6];
	ushort	etype;
	uchar	type;
	uchar	conn;
	uchar	seq;
	uchar	len;
	uchar	data[1500];
} Pkt;

enum {
	Fkbd,
	Fcec,
	Ffatal,
};

typedef struct Mux Mux;
#pragma incomplete Mux;

enum{
	Iowait		= 2000,
	Etype 		= 0xbcbc,
};
int debug;

Mux	*mux(int fd[2]);
void	muxfree(Mux*);
int	muxread(Mux*, Pkt*);

int	netget(void *, int);
int	netopen(char *name);
int	netsend(void *, int);

void	dump(uchar*, int);
void	exits0(char*);
void	rawoff(void);
void	rawon(void);

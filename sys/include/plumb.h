/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma	lib	"libplumb.a"
#pragma	src	"/sys/src/libplumb"

/*
 * Message format:
 *	source application\n
 *	destination port\n
 *	working directory\n
 *	type\n
 *	attributes\n
 *	nbytes\n
 *	n bytes of data
 */

typedef struct Plumbattr Plumbattr;
typedef struct Plumbmsg Plumbmsg;

struct Plumbmsg
{
	int8_t		*src;
	int8_t		*dst;
	int8_t		*wdir;
	int8_t		*type;
	Plumbattr	*attr;
	int		ndata;
	int8_t		*data;
};

struct Plumbattr
{
	int8_t		*name;
	int8_t		*value;
	Plumbattr	*next;
};

int			plumbsend(int, Plumbmsg*);
int			plumbsendtext(int, int8_t*, int8_t*, int8_t*,
					 int8_t*);
Plumbmsg*	plumbrecv(int);
int8_t*		plumbpack(Plumbmsg*, int*);
Plumbmsg*	plumbunpack(int8_t*, int);
Plumbmsg*	plumbunpackpartial(int8_t*, int, int*);
int8_t*		plumbpackattr(Plumbattr*);
Plumbattr*	plumbunpackattr(int8_t*);
Plumbattr*	plumbaddattr(Plumbattr*, Plumbattr*);
Plumbattr*	plumbdelattr(Plumbattr*, int8_t*);
void			plumbfree(Plumbmsg*);
int8_t*		plumblookup(Plumbattr*, int8_t*);
int			plumbopen(int8_t*, int);
int			eplumb(int, int8_t*);

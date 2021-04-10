/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Field Field;
typedef struct Filter Filter;
typedef struct Msg Msg;
typedef struct Mux Mux;
typedef struct Proto Proto;

#define NetS(x) ((((u8*)x)[0]<<8) | ((u8*)x)[1])
#define Net3(x) ((((u8*)x)[0]<<16) | (((u8*)x)[1]<<8) | ((u8*)x)[2])
#define NetL(x) ((((u8*)x)[0]<<24) | (((u8*)x)[1]<<16) | (((u8*)x)[2]<<8) | ((u8*)x)[3])

/*
 *  one per protocol module
 */
struct Proto
{
	char*	name;
	void	(*compile)(Filter*);
	int	(*filter)(Filter*, Msg*);
	int	(*seprint)(Msg*);
	Mux*	mux;
	char*	valfmt;
	Field*	field;
	int	(*framer)(int, u8*, int);
};
extern Proto *protos[];

/*
 *  one per protocol module, pointed to by Proto.mux
 */
struct Mux
{
	char*	name;
	u32	val;
	Proto*	pr;
};

/*
 *  a field defining a comparison filter
 */
struct Field
{
	char*	name;
	int	ftype;
	int	subop;
	char*	help;
};

/*
 *  the status of the current message walk
 */
struct Msg
{
	u8	*ps;	/* packet ptr */
	u8	*pe;	/* packet end */

	char	*p;	/* buffer start */
	char	*e;	/* buffer end */

	int	needroot;	/* pr is root, need to see in expression */
	Proto	*pr;	/* current/next protocol */
};

enum
{
	Fnum,		/* just a number */
	Fether,		/* ethernet address */
	Fv4ip,		/* v4 ip address */
	Fv6ip,		/* v6 ip address */
	Fba,		/* byte array */
};

/*
 *  a node in the filter tree
 */
struct Filter {
	int	op;	/* token type */
	char	*s;	/* string */
	Filter	*l;
	Filter	*r;

	Proto	*pr;	/* next protocol;*/

	/* protocol specific */
	int	subop;
	u32	param;
	union {
		u32	ulv;
		i64	vlv;
		u8	a[32];
	};
};

extern void	yyinit(char*);
extern int	yyparse(void);
extern Filter*	newfilter(void);
extern void	compile_cmp(char*, Filter*, Field*);
extern void	demux(Mux*, u32, u32, Msg*, Proto*);
extern int	defaultframer(int, u8*, int);

extern int Mflag;
extern int Nflag;
extern int dflag;
extern int Cflag;

typedef Filter *Filterptr;
#define YYSTYPE Filterptr
extern Filter *filter;

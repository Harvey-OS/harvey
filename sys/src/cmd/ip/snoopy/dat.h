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

#define NetS(x) ((((uint8_t*)x)[0]<<8) | ((uint8_t*)x)[1])
#define Net3(x) ((((uint8_t*)x)[0]<<16) | (((uint8_t*)x)[1]<<8) | ((uint8_t*)x)[2])
#define NetL(x) ((((uint8_t*)x)[0]<<24) | (((uint8_t*)x)[1]<<16) | (((uint8_t*)x)[2]<<8) | ((uint8_t*)x)[3])

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
	int	(*framer)(int, uint8_t*, int);
};
extern Proto *protos[];

/*
 *  one per protocol module, pointed to by Proto.mux
 */
struct Mux
{
	char*	name;
	uint32_t	val;
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
	uint8_t	*ps;	/* packet ptr */
	uint8_t	*pe;	/* packet end */

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
	uint32_t	param;
	union {
		uint32_t	ulv;
		int64_t	vlv;
		uint8_t	a[32];
	};
};

extern void	yyinit(char*);
extern int	yyparse(void);
extern Filter*	newfilter(void);
extern void	compile_cmp(char*, Filter*, Field*);
extern void	demux(Mux*, uint32_t, uint32_t, Msg*, Proto*);
extern int	defaultframer(int, uint8_t*, int);

extern int Mflag;
extern int Nflag;
extern int dflag;
extern int Cflag;

typedef Filter *Filterptr;
#define YYSTYPE Filterptr
extern Filter *filter;

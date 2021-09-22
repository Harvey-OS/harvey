#include <u.h>
#include <libc.h>
#include <bio.h>
#include "sphere.h"

enum
{
	Olist	= 1,
	Opoint,
	Odefn,
	Oxcc,
	Oxcl,
	Oxll,
	Ocirc,
	Oline,
	Odme,
};

typedef	struct	Pair	Pair;
typedef	struct	Node	Node;
typedef	struct	String	String;

struct	Pair
{
	long	len;
	long	val;
};
struct	Node
{
	int	op;
	union
	{
		Node*	nleft;
		double	vleft;
		int	ileft;
	};
	union
	{
		Node*	nright;
		double	vright;
		int	iright;
	};
};
struct	String
{
	char	val[100];
	short	len;
};

Biobuf*	bin;
long	lineno;
Node*	top;
int	lastchar;
int	nerror;
Node*	basep;
Node*	basev;
String	gname;
char*	header;
double	grain;
int	outline;

Node*	new(int);
Node*	lookupp(int);
double	lookupv(int);
void	definep(int, Node*);
void	definev(int, double);
Node*	eval(Node*);
Node*	xcc(Node*);
Node*	xcl(Node*);
Node*	xll(Node*);
Node*	dme(Node*, double, double);
double	dist(Node*, Node*);
double	angl(Node*, Node*);
double	xsqrt(double);
void	outstring(String*);
void	outsegment(Node*, Node*);
void	outarc(int, Node*, Node*, Node*);
void	outcirc(Node*, double);
double	anorm(double);
double	adiff(double);

int	yylex(void);
void	yyerror(char*, ...);
int	yyparse(void);

typedef void (*Inst)(void);
#define	STOP	(Inst) 0

typedef struct Symbol {	/* symbol table entry */
	char	*name;
	long	type;
	union {
		double	val;		/* VAR */
		double	(*ptr)(double);	/* BLTIN */
		Inst	*defn;		/* FUNCTION, PROCEDURE */
		char	*str;		/* STRING */
	} u;
	struct Symbol	*next;	/* to link to another */
} Symbol;
Symbol	*install(char*, int, double), *lookup(char*);

typedef union Datum {	/* interpreter stack type */
	double	val;
	Symbol	*sym;
} Datum;
extern	double Fgetd(int);
extern	int moreinput(void);
extern	void execerror(char*, char*);
extern	void define(Symbol*), verify(Symbol*);
extern	Datum pop(void);
extern	void initcode(void), push(Datum), xpop(void), constpush(void);
extern	void varpush(void);
extern	void eval(void), add(void), sub(void), mul(void), div(void), mod(void);
extern	void negate(void), power(void);
extern	void addeq(void), subeq(void), muleq(void), diveq(void), modeq(void);

extern	Inst *progp, *progbase, prog[], *code(Inst);
extern	void assign(void), bltin(void), varread(void);
extern	void prexpr(void), prstr(void);
extern	void gt(void), lt(void), eq(void), ge(void), le(void), ne(void);
extern	void and(void), or(void), not(void);
extern	void ifcode(void), whilecode(void), forcode(void);
extern	void call(void), arg(void), argassign(void);
extern	void funcret(void), procret(void);
extern	void preinc(void), predec(void), postinc(void), postdec(void);
extern	void argaddeq(void), argsubeq(void), argmuleq(void);
extern	void argdiveq(void), argmodeq(void);
extern	void execute(Inst*);
extern	void printtop(void);

extern double	Log(double), Log10(double), Gamma(double), Sqrt(double), Exp(double);
extern double	Asin(double), Acos(double), Sinh(double), Cosh(double), integer(double);
extern double	Pow(double, double);

extern	void init(void);
extern	int yyparse(void);
extern	void execerror(char*, char*);
extern	void *emalloc(unsigned);

void initialize(void);
#define initialize_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void println(void);
#define println_regmem
void printchar(ASCIIcode s);
#define printchar_regmem register memoryword *eqtb=zeqtb;
void print(integer s);
#define print_regmem register memoryword *eqtb=zeqtb;
void slowprint(integer s);
#define slowprint_regmem register memoryword *eqtb=zeqtb;
void printnl(strnumber s);
#define printnl_regmem
void printesc(strnumber s);
#define printesc_regmem register memoryword *eqtb=zeqtb;
void printthedigs(eightbits k);
#define printthedigs_regmem
void printint(integer n);
#define printint_regmem
void printcs(integer p);
#define printcs_regmem register memoryword *eqtb=zeqtb;
void sprintcs(halfword p);
#define sprintcs_regmem
void printfilename(integer n,integer a,integer e);
#define printfilename_regmem
void printsize(integer s);
#define printsize_regmem
void printwritewhatsit(strnumber s,halfword p);
#define printwritewhatsit_regmem register memoryword *mem=zmem;
void jumpout(void);
#define jumpout_regmem
void error(void);
#define error_regmem
void fatalerror(strnumber s);
#define fatalerror_regmem
void overflow(strnumber s,integer n);
#define overflow_regmem
void confusion(strnumber s);
#define confusion_regmem
boolean initterminal(void);
#define initterminal_regmem
strnumber makestring(void);
#define makestring_regmem
boolean streqbuf(strnumber s,integer k);
#define streqbuf_regmem
boolean streqstr(strnumber s,strnumber t);
#define streqstr_regmem
boolean getstringsstarted(void);
#define getstringsstarted_regmem
void printtwo(integer n);
#define printtwo_regmem
void printhex(integer n);
#define printhex_regmem
void printromanint(integer n);
#define printromanint_regmem
void printcurrentstring(void);
#define printcurrentstring_regmem
void terminput(void);
#define terminput_regmem
void interror(integer n);
#define interror_regmem
void normalizeselector(void);
#define normalizeselector_regmem
void pauseforinstructions(void);
#define pauseforinstructions_regmem
integer half(integer x);
#define half_regmem
scaled rounddecimals(smallnumber k);
#define rounddecimals_regmem
void printscaled(scaled s);
#define printscaled_regmem
scaled multandadd(integer n,scaled x,scaled y,scaled maxanswer);
#define multandadd_regmem
scaled xovern(scaled x,integer n);
#define xovern_regmem
scaled xnoverd(scaled x,integer n,integer d);
#define xnoverd_regmem
halfword badness(scaled t,scaled s);
#define badness_regmem
void printword(memoryword w);
#define printword_regmem
void showtokenlist(integer p,integer q,integer l);
#define showtokenlist_regmem register memoryword *mem=zmem;
void runaway(void);
#define runaway_regmem register memoryword *mem=zmem;
halfword getavail(void);
#define getavail_regmem register memoryword *mem=zmem;
void flushlist(halfword p);
#define flushlist_regmem register memoryword *mem=zmem;
halfword getnode(integer s);
#define getnode_regmem register memoryword *mem=zmem;
void freenode(halfword p,halfword s);
#define freenode_regmem register memoryword *mem=zmem;
void sortavail(void);
#define sortavail_regmem register memoryword *mem=zmem;
halfword newnullbox(void);
#define newnullbox_regmem register memoryword *mem=zmem;
halfword newrule(void);
#define newrule_regmem register memoryword *mem=zmem;
halfword newligature(quarterword f,quarterword c,halfword q);
#define newligature_regmem register memoryword *mem=zmem;
halfword newligitem(quarterword c);
#define newligitem_regmem register memoryword *mem=zmem;
halfword newdisc(void);
#define newdisc_regmem register memoryword *mem=zmem;
halfword newmath(scaled w,smallnumber s);
#define newmath_regmem register memoryword *mem=zmem;
halfword newspec(halfword p);
#define newspec_regmem register memoryword *mem=zmem;
halfword newparamglue(smallnumber n);
#define newparamglue_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
halfword newglue(halfword q);
#define newglue_regmem register memoryword *mem=zmem;
halfword newskipparam(smallnumber n);
#define newskipparam_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
halfword newkern(scaled w);
#define 
void initialize AA((void));
#define initialize_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void println AA((void));
#define println_regmem
void zprintvisiblechar AA((ASCIIcode s));
#define printvisiblechar(s) zprintvisiblechar((ASCIIcode) (s))
#define printvisiblechar_regmem
void zprintchar AA((ASCIIcode s));
#define printchar(s) zprintchar((ASCIIcode) (s))
#define printchar_regmem register memoryword *eqtb=zeqtb;
void zprint AA((integer s));
#define print(s) zprint((integer) (s))
#define print_regmem
void zprintnl AA((strnumber s));
#define printnl(s) zprintnl((strnumber) (s))
#define printnl_regmem
void zprintesc AA((strnumber s));
#define printesc(s) zprintesc((strnumber) (s))
#define printesc_regmem register memoryword *eqtb=zeqtb;
void zprintthedigs AA((eightbits k));
#define printthedigs(k) zprintthedigs((eightbits) (k))
#define printthedigs_regmem
void zprintint AA((integer n));
#define printint(n) zprintint((integer) (n))
#define printint_regmem
void zprintcs AA((integer p));
#define printcs(p) zprintcs((integer) (p))
#define printcs_regmem register memoryword *eqtb=zeqtb;
void zsprintcs AA((halfword p));
#define sprintcs(p) zsprintcs((halfword) (p))
#define sprintcs_regmem
void zprintfilename AA((integer n,integer a,integer e));
#define printfilename(n, a, e) zprintfilename((integer) (n), (integer) (a), (integer) (e))
#define printfilename_regmem
void zprintsize AA((integer s));
#define printsize(s) zprintsize((integer) (s))
#define printsize_regmem
void zprintwritewhatsit AA((strnumber s,halfword p));
#define printwritewhatsit(s, p) zprintwritewhatsit((strnumber) (s), (halfword) (p))
#define printwritewhatsit_regmem register memoryword *mem=zmem;
void zprintcsnames AA((integer hstart,integer hfinish));
#define printcsnames(hstart, hfinish) zprintcsnames((integer) (hstart), (integer) (hfinish))
#define printcsnames_regmem
void jumpout AA((void));
#define jumpout_regmem
void error AA((void));
#define error_regmem
void zfatalerror AA((strnumber s));
#define fatalerror(s) zfatalerror((strnumber) (s))
#define fatalerror_regmem
void zoverflow AA((strnumber s,integer n));
#define overflow(s, n) zoverflow((strnumber) (s), (integer) (n))
#define overflow_regmem
void zconfusion AA((strnumber s));
#define confusion(s) zconfusion((strnumber) (s))
#define confusion_regmem
boolean initterminal AA((void));
#define initterminal_regmem
strnumber makestring AA((void));
#define makestring_regmem
boolean zstreqbuf AA((strnumber s,integer k));
#define streqbuf(s, k) zstreqbuf((strnumber) (s), (integer) (k))
#define streqbuf_regmem
boolean zstreqstr AA((strnumber s,strnumber t));
#define streqstr(s, t) zstreqstr((strnumber) (s), (strnumber) (t))
#define streqstr_regmem
strnumber zsearchstring AA((strnumber search));
#define searchstring(search) zsearchstring((strnumber) (search))
#define searchstring_regmem
strnumber slowmakestring AA((void));
#define slowmakestring_regmem
boolean getstringsstarted AA((void));
#define getstringsstarted_regmem
void zprinttwo AA((integer n));
#define printtwo(n) zprinttwo((integer) (n))
#define printtwo_regmem
void zprinthex AA((integer n));
#define printhex(n) zprinthex((integer) (n))
#define printhex_regmem
void zprintromanint AA((integer n));
#define printromanint(n) zprintromanint((integer) (n))
#define printromanint_regmem
void printcurrentstring AA((void));
#define printcurrentstring_regmem
void terminput AA((void));
#define terminput_regmem
void zinterror AA((integer n));
#define interror(n) zinterror((integer) (n))
#define interror_regmem
void normalizeselector AA((void));
#define normalizeselector_regmem
void pauseforinstructions AA((void));
#define pauseforinstructions_regmem
integer zhalf AA((integer x));
#define half(x) zhalf((integer) (x))
#define half_regmem
scaled zrounddecimals AA((smallnumber k));
#define rounddecimals(k) zrounddecimals((smallnumber) (k))
#define rounddecimals_regmem
void zprintscaled AA((scaled s));
#define printscaled(s) zprintscaled((scaled) (s))
#define printscaled_regmem
scaled zmultandadd AA((integer n,scaled x,scaled y,scaled maxanswer));
#define multandadd(n, x, y, maxanswer) zmultandadd((integer) (n), (scaled) (x), (scaled) (y), (scaled) (maxanswer))
#define multandadd_regmem
scaled zxovern AA((scaled x,integer n));
#define xovern(x, n) zxovern((scaled) (x), (integer) (n))
#define xovern_regmem
scaled zxnoverd AA((scaled x,integer n,integer d));
#define xnoverd(x, n, d) zxnoverd((scaled) (x), (integer) (n), (integer) (d))
#define xnoverd_regmem
halfword zbadness AA((scaled t,scaled s));
#define badness(t, s) zbadness((scaled) (t), (scaled) (s))
#define badness_regmem
void zprintword AA((memoryword w));
#define printword(w) zprintword((memoryword) (w))
#define printword_regmem
void zshowtokenlist AA((integer p,integer q,integer l));
#define showtokenlist(p, q, l) zshowtokenlist((integer) (p), (integer) (q), (integer) (l))
#define showtokenlist_regmem register memoryword *mem=zmem;
void runaway AA((void));
#define runaway_regmem register memoryword *mem=zmem;
halfword getavail AA((void));
#define getavail_regmem register memoryword *mem=zmem;
void zflushlist AA((halfword p));
#define flushlist(p) zflushlist((halfword) (p))
#define flushlist_regmem register memoryword *mem=zmem;
halfword zgetnode AA((integer s));
#define getnode(s) zgetnode((integer) (s))
#define getnode_regmem register memoryword *mem=zmem;
void zfreenode AA((halfword p,halfword s));
#define freenode(p, s) zfreenode((halfword) (p), (halfword) (s))
#define freenode_regmem register memoryword *mem=zmem;
void sortavail AA((void));
#define sortavail_regmem register memoryword *mem=zmem;
halfword newnullbox AA((void));
#define newnullbox_regmem register memoryword *mem=zmem;
halfword newrule AA((void));
#define newrule_regmem register memoryword *mem=zmem;
halfword znewligature AA((internalfontnumber f,quarterword c,halfword q));
#define newligature(f, c, q) znewligature((internalfontnumber) (f), (quarterword) (c), (halfword) (q))
#define newligature_regmem register memoryword *mem=zmem;
halfword znewligitem AA((quarterword c));
#define newligitem(c) znewligitem((quarterword) (c))
#define newligitem_regmem register memoryword *mem=zmem;
halfword newdisc AA((void));
#define newdisc_regmem register memoryword *mem=zmem;
halfword znewmath AA((scaled w,smallnumber s));
#define newmath(w, s) znewmath((scaled) (w), (smallnumber) (s))
#define newmath_regmem register memoryword *mem=zmem;
halfword znewspec AA((halfword p));
#define newspec(p) znewspec((halfword) (p))
#define newspec_regmem register memoryword *mem=zmem;
halfword znewparamglue AA((smallnumber n));
#define newparamglue(n) znewparamglue((smallnumber) (n))
#define newparamglue_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
halfword znewglue AA((halfword q));
#define newglue(q) znewglue((halfword) (q))
#define newglue_regmem register memoryword *mem=zmem;
halfword znewskipparam AA((smallnumber n));
#define newskipparam(n) znewskipparam((smallnumber) (n))
#define newskipparam_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
halfword znewkern AA((scaled w));
#define newkern(w) znewkern((scaled) (w))
#define newkern_regmem register memoryword *mem=zmem;
halfword znewpenalty AA((integer m));
#define newpenalty(m) znewpenalty((integer) (m))
#define newpenalty_regmem register memoryword *mem=zmem;
void zcheckmem AA((boolean printlocs));
#define checkmem(printlocs) zcheckmem((boolean) (printlocs))
#define checkmem_regmem register memoryword *mem=zmem;
void zsearchmem AA((halfword p));
#define searchmem(p) zsearchmem((halfword) (p))
#define searchmem_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zshortdisplay AA((integer p));
#define shortdisplay(p) zshortdisplay((integer) (p))
#define shortdisplay_regmem register memoryword *mem=zmem;
void zprintfontandchar AA((integer p));
#define printfontandchar(p) zprintfontandchar((integer) (p))
#define printfontandchar_regmem register memoryword *mem=zmem;
void zprintmark AA((integer p));
#define printmark(p) zprintmark((integer) (p))
#define printmark_regmem register memoryword *mem=zmem;
void zprintruledimen AA((scaled d));
#define printruledimen(d) zprintruledimen((scaled) (d))
#define printruledimen_regmem
void zprintglue AA((scaled d,integer order,strnumber s));
#define printglue(d, order, s) zprintglue((scaled) (d), (integer) (order), (strnumber) (s))
#define printglue_regmem
void zprintspec AA((integer p,strnumber s));
#define printspec(p, s) zprintspec((integer) (p), (strnumber) (s))
#define printspec_regmem register memoryword *mem=zmem;
void zprintfamandchar AA((halfword p));
#define printfamandchar(p) zprintfamandchar((halfword) (p))
#define printfamandchar_regmem register memoryword *mem=zmem;
void zprintdelimiter AA((halfword p));
#define printdelimiter(p) zprintdelimiter((halfword) (p))
#define printdelimiter_regmem register memoryword *mem=zmem;
void zprintsubsidiarydata AA((halfword p,ASCIIcode c));
#define printsubsidiarydata(p, c) zprintsubsidiarydata((halfword) (p), (ASCIIcode) (c))
#define printsubsidiarydata_regmem register memoryword *mem=zmem;
void zprintstyle AA((integer c));
#define printstyle(c) zprintstyle((integer) (c))
#define printstyle_regmem
void zprintskipparam AA((integer n));
#define printskipparam(n) zprintskipparam((integer) (n))
#define printskipparam_regmem
void zshownodelist AA((integer p));
#define shownodelist(p) zshownodelist((integer) (p))
#define shownodelist_regmem register memoryword *mem=zmem;
void zshowbox AA((halfword p));
#define showbox(p) zshowbox((halfword) (p))
#define showbox_regmem register memoryword *eqtb=zeqtb;
void zdeletetokenref AA((halfword p));
#define deletetokenref(p) zdeletetokenref((halfword) (p))
#define deletetokenref_regmem register memoryword *mem=zmem;
void zdeleteglueref AA((halfword p));
#define deleteglueref(p) zdeleteglueref((halfword) (p))
#define deleteglueref_regmem register memoryword *mem=zmem;
void zflushnodelist AA((halfword p));
#define flushnodelist(p) zflushnodelist((halfword) (p))
#define flushnodelist_regmem register memoryword *mem=zmem;
halfword zcopynodelist AA((halfword p));
#define copynodelist(p) zcopynodelist((halfword) (p))
#define copynodelist_regmem register memoryword *mem=zmem;
void zprintmode AA((integer m));
#define printmode(m) zprintmode((integer) (m))
#define printmode_regmem
void pushnest AA((void));
#define pushnest_regmem
void popnest AA((void));
#define popnest_regmem register memoryword *mem=zmem;
void showactivities AA((void));
#define showactivities_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zprintparam AA((integer n));
#define printparam(n) zprintparam((integer) (n))
#define printparam_regmem
void begindiagnostic AA((void));
#define begindiagnostic_regmem register memoryword *eqtb=zeqtb;
void zenddiagnostic AA((boolean blankline));
#define enddiagnostic(blankline) zenddiagnostic((boolean) (blankline))
#define enddiagnostic_regmem
void zprintlengthparam AA((integer n));
#define printlengthparam(n) zprintlengthparam((integer) (n))
#define printlengthparam_regmem
void zprintcmdchr AA((quarterword cmd,halfword chrcode));
#define printcmdchr(cmd, chrcode) zprintcmdchr((quarterword) (cmd), (halfword) (chrcode))
#define printcmdchr_regmem
void zshoweqtb AA((halfword n));
#define showeqtb(n) zshoweqtb((halfword) (n))
#define showeqtb_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
halfword zidlookup AA((integer j,integer l));
#define idlookup(j, l) zidlookup((integer) (j), (integer) (l))
#define idlookup_regmem
void zprimitive AA((strnumber s,quarterword c,halfword o));
#define primitive(s, c, o) zprimitive((strnumber) (s), (quarterword) (c), (halfword) (o))
#define primitive_regmem register memoryword *eqtb=zeqtb;
void znewsavelevel AA((groupcode c));
#define newsavelevel(c) znewsavelevel((groupcode) (c))
#define newsavelevel_regmem
void zeqdestroy AA((memoryword w));
#define eqdestroy(w) zeqdestroy((memoryword) (w))
#define eqdestroy_regmem register memoryword *mem=zmem;
void zeqsave AA((halfword p,quarterword l));
#define eqsave(p, l) zeqsave((halfword) (p), (quarterword) (l))
#define eqsave_regmem register memoryword *eqtb=zeqtb;
void zeqdefine AA((halfword p,quarterword t,halfword e));
#define eqdefine(p, t, e) zeqdefine((halfword) (p), (quarterword) (t), (halfword) (e))
#define eqdefine_regmem register memoryword *eqtb=zeqtb;
void zeqworddefine AA((halfword p,integer w));
#define eqworddefine(p, w) zeqworddefine((halfword) (p), (integer) (w))
#define eqworddefine_regmem register memoryword *eqtb=zeqtb;
void zgeqdefine AA((halfword p,quarterword t,halfword e));
#define geqdefine(p, t, e) zgeqdefine((halfword) (p), (quarterword) (t), (halfword) (e))
#define geqdefine_regmem register memoryword *eqtb=zeqtb;
void zgeqworddefine AA((halfword p,integer w));
#define geqworddefine(p, w) zgeqworddefine((halfword) (p), (integer) (w))
#define geqworddefine_regmem register memoryword *eqtb=zeqtb;
void zsaveforafter AA((halfword t));
#define saveforafter(t) zsaveforafter((halfword) (t))
#define saveforafter_regmem
void zrestoretrace AA((halfword p,strnumber s));
#define restoretrace(p, s) zrestoretrace((halfword) (p), (strnumber) (s))
#define restoretrace_regmem
void unsave AA((void));
#define unsave_regmem register memoryword *eqtb=zeqtb;
void preparemag AA((void));
#define preparemag_regmem register memoryword *eqtb=zeqtb;
void ztokenshow AA((halfword p));
#define tokenshow(p) ztokenshow((halfword) (p))
#define tokenshow_regmem register memoryword *mem=zmem;
void printmeaning AA((void));
#define printmeaning_regmem
void showcurcmdchr AA((void));
#define showcurcmdchr_regmem
void showcontext AA((void));
#define showcontext_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zbegintokenlist AA((halfword p,quarterword t));
#define begintokenlist(p, t) zbegintokenlist((halfword) (p), (quarterword) (t))
#define begintokenlist_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void endtokenlist AA((void));
#define endtokenlist_regmem
void backinput AA((void));
#define backinput_regmem register memoryword *mem=zmem;
void backerror AA((void));
#define backerror_regmem
void inserror AA((void));
#define inserror_regmem
void beginfilereading AA((void));
#define beginfilereading_regmem
void endfilereading AA((void));
#define endfilereading_regmem
void clearforerrorprompt AA((void));
#define clearforerrorprompt_regmem
void checkoutervalidity AA((void));
#define checkoutervalidity_regmem register memoryword *mem=zmem;
void getnext AA((void));
#define getnext_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void firmuptheline AA((void));
#define firmuptheline_regmem register memoryword *eqtb=zeqtb;
void gettoken AA((void));
#define gettoken_regmem
void macrocall AA((void));
#define macrocall_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void insertrelax AA((void));
#define insertrelax_regmem
void expand AA((void));
#define expand_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void getxtoken AA((void));
#define getxtoken_regmem
void xtoken AA((void));
#define xtoken_regmem
void scanleftbrace AA((void));
#define scanleftbrace_regmem
void scanoptionalequals AA((void));
#define scanoptionalequals_regmem
boolean zscankeyword AA((strnumber s));
#define scankeyword(s) zscankeyword((strnumber) (s))
#define scankeyword_regmem register memoryword *mem=zmem;
void muerror AA((void));
#define muerror_regmem
void scaneightbitint AA((void));
#define scaneightbitint_regmem
void scancharnum AA((void));
#define scancharnum_regmem
void scanfourbitint AA((void));
#define scanfourbitint_regmem
void scanfifteenbitint AA((void));
#define scanfifteenbitint_regmem
void scantwentysevenbitint AA((void));
#define scantwentysevenbitint_regmem
void scanfontident AA((void));
#define scanfontident_regmem register memoryword *eqtb=zeqtb;
void zfindfontdimen AA((boolean writing));
#define findfontdimen(writing) zfindfontdimen((boolean) (writing))
#define findfontdimen_regmem
void zscansomethinginternal AA((smallnumber level,boolean negative));
#define scansomethinginternal(level, negative) zscansomethinginternal((smallnumber) (level), (boolean) (negative))
#define scansomethinginternal_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void scanint AA((void));
#define scanint_regmem
void zscandimen AA((boolean mu,boolean inf,boolean shortcut));
#define scandimen(mu, inf, shortcut) zscandimen((boolean) (mu), (boolean) (inf), (boolean) (shortcut))
#define scandimen_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zscanglue AA((smallnumber level));
#define scanglue(level) zscanglue((smallnumber) (level))
#define scanglue_regmem register memoryword *mem=zmem;
halfword scanrulespec AA((void));
#define scanrulespec_regmem register memoryword *mem=zmem;
halfword zstrtoks AA((poolpointer b));
#define strtoks(b) zstrtoks((poolpointer) (b))
#define strtoks_regmem register memoryword *mem=zmem;
halfword thetoks AA((void));
#define thetoks_regmem register memoryword *mem=zmem;
void insthetoks AA((void));
#define insthetoks_regmem register memoryword *mem=zmem;
void convtoks AA((void));
#define convtoks_regmem register memoryword *mem=zmem;
halfword zscantoks AA((boolean macrodef,boolean xpand));
#define scantoks(macrodef, xpand) zscantoks((boolean) (macrodef), (boolean) (xpand))
#define scantoks_regmem register memoryword *mem=zmem;
void zreadtoks AA((integer n,halfword r));
#define readtoks(n, r) zreadtoks((integer) (n), (halfword) (r))
#define readtoks_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void passtext AA((void));
#define passtext_regmem
void zchangeiflimit AA((smallnumber l,halfword p));
#define changeiflimit(l, p) zchangeiflimit((smallnumber) (l), (halfword) (p))
#define changeiflimit_regmem register memoryword *mem=zmem;
void conditional AA((void));
#define conditional_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void beginname AA((void));
#define beginname_regmem
boolean zmorename AA((ASCIIcode c));
#define morename(c) zmorename((ASCIIcode) (c))
#define morename_regmem
void endname AA((void));
#define endname_regmem
void zpackfilename AA((strnumber n,strnumber a,strnumber e));
#define packfilename(n, a, e) zpackfilename((strnumber) (n), (strnumber) (a), (strnumber) (e))
#define packfilename_regmem
void zpackbufferedname AA((smallnumber n,integer a,integer b));
#define packbufferedname(n, a, b) zpackbufferedname((smallnumber) (n), (integer) (a), (integer) (b))
#define packbufferedname_regmem
strnumber makenamestring AA((void));
#define makenamestring_regmem
strnumber zamakenamestring AA((alphafile f));
#define amakenamestring(f) zamakenamestring((alphafile) (f))
#define amakenamestring_regmem
strnumber zbmakenamestring AA((bytefile f));
#define bmakenamestring(f) zbmakenamestring((bytefile) (f))
#define bmakenamestring_regmem
strnumber zwmakenamestring AA((wordfile f));
#define wmakenamestring(f) zwmakenamestring((wordfile) (f))
#define wmakenamestring_regmem
void scanfilename AA((void));
#define scanfilename_regmem
void zpackjobname AA((strnumber s));
#define packjobname(s) zpackjobname((strnumber) (s))
#define packjobname_regmem
void zpromptfilename AA((strnumber s,strnumber e));
#define promptfilename(s, e) zpromptfilename((strnumber) (s), (strnumber) (e))
#define promptfilename_regmem
void openlogfile AA((void));
#define openlogfile_regmem register memoryword *eqtb=zeqtb;
void startinput AA((void));
#define startinput_regmem register memoryword *eqtb=zeqtb;
integer zeffectivechar AA((boolean errp,internalfontnumber f,quarterword c));
#define effectivechar(errp, f, c) zeffectivechar((boolean) (errp), (internalfontnumber) (f), (quarterword) (c))
#define effectivechar_regmem register memoryword *eqtb=zeqtb;
fourquarters zeffectivecharinfo AA((internalfontnumber f,quarterword c));
#define effectivecharinfo(f, c) zeffectivecharinfo((internalfontnumber) (f), (quarterword) (c))
#define effectivecharinfo_regmem register memoryword *eqtb=zeqtb;
internalfontnumber zreadfontinfo AA((halfword u,strnumber nom,strnumber aire,scaled s));
#define readfontinfo(u, nom, aire, s) zreadfontinfo((halfword) (u), (strnumber) (nom), (strnumber) (aire), (scaled) (s))
#define readfontinfo_regmem register memoryword *eqtb=zeqtb;
void zcharwarning AA((internalfontnumber f,eightbits c));
#define charwarning(f, c) zcharwarning((internalfontnumber) (f), (eightbits) (c))
#define charwarning_regmem register memoryword *eqtb=zeqtb;
halfword znewcharacter AA((internalfontnumber f,eightbits c));
#define newcharacter(f, c) znewcharacter((internalfontnumber) (f), (eightbits) (c))
#define newcharacter_regmem register memoryword *mem=zmem;
void dviswap AA((void));
#define dviswap_regmem
void zdvifour AA((integer x));
#define dvifour(x) zdvifour((integer) (x))
#define dvifour_regmem
void zdvipop AA((integer l));
#define dvipop(l) zdvipop((integer) (l))
#define dvipop_regmem
void zdvifontdef AA((internalfontnumber f));
#define dvifontdef(f) zdvifontdef((internalfontnumber) (f))
#define dvifontdef_regmem
void zmovement AA((scaled w,eightbits o));
#define movement(w, o) zmovement((scaled) (w), (eightbits) (o))
#define movement_regmem register memoryword *mem=zmem;
void zprunemovements AA((integer l));
#define prunemovements(l) zprunemovements((integer) (l))
#define prunemovements_regmem register memoryword *mem=zmem;
void zspecialout AA((halfword p));
#define specialout(p) zspecialout((halfword) (p))
#define specialout_regmem register memoryword *mem=zmem;
void zwriteout AA((halfword p));
#define writeout(p) zwriteout((halfword) (p))
#define writeout_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zoutwhat AA((halfword p));
#define outwhat(p) zoutwhat((halfword) (p))
#define outwhat_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void hlistout AA((void));
#define hlistout_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void vlistout AA((void));
#define vlistout_regmem register memoryword *mem=zmem;
void zshipout AA((halfword p));
#define shipout(p) zshipout((halfword) (p))
#define shipout_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zscanspec AA((groupcode c,boolean threecodes));
#define scanspec(c, threecodes) zscanspec((groupcode) (c), (boolean) (threecodes))
#define scanspec_regmem
halfword zhpack AA((halfword p,scaled w,smallnumber m));
#define hpack(p, w, m) zhpack((halfword) (p), (scaled) (w), (smallnumber) (m))
#define hpack_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
halfword zvpackage AA((halfword p,scaled h,smallnumber m,scaled l));
#define vpackage(p, h, m, l) zvpackage((halfword) (p), (scaled) (h), (smallnumber) (m), (scaled) (l))
#define vpackage_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zappendtovlist AA((halfword b));
#define appendtovlist(b) zappendtovlist((halfword) (b))
#define appendtovlist_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
halfword newnoad AA((void));
#define newnoad_regmem register memoryword *mem=zmem;
halfword znewstyle AA((smallnumber s));
#define newstyle(s) znewstyle((smallnumber) (s))
#define newstyle_regmem register memoryword *mem=zmem;
halfword newchoice AA((void));
#define newchoice_regmem register memoryword *mem=zmem;
void showinfo AA((void));
#define showinfo_regmem register memoryword *mem=zmem;
halfword zfractionrule AA((scaled t));
#define fractionrule(t) zfractionrule((scaled) (t))
#define fractionrule_regmem register memoryword *mem=zmem;
halfword zoverbar AA((halfword b,scaled k,scaled t));
#define overbar(b, k, t) zoverbar((halfword) (b), (scaled) (k), (scaled) (t))
#define overbar_regmem register memoryword *mem=zmem;
halfword zcharbox AA((internalfontnumber f,quarterword c));
#define charbox(f, c) zcharbox((internalfontnumber) (f), (quarterword) (c))
#define charbox_regmem register memoryword *mem=zmem;
void zstackintobox AA((halfword b,internalfontnumber f,quarterword c));
#define stackintobox(b, f, c) zstackintobox((halfword) (b), (internalfontnumber) (f), (quarterword) (c))
#define stackintobox_regmem register memoryword *mem=zmem;
scaled zheightplusdepth AA((internalfontnumber f,quarterword c));
#define heightplusdepth(f, c) zheightplusdepth((internalfontnumber) (f), (quarterword) (c))
#define heightplusdepth_regmem
halfword zvardelimiter AA((halfword d,smallnumber s,scaled v));
#define vardelimiter(d, s, v) zvardelimiter((halfword) (d), (smallnumber) (s), (scaled) (v))
#define vardelimiter_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
halfword zrebox AA((halfword b,scaled w));
#define rebox(b, w) zrebox((halfword) (b), (scaled) (w))
#define rebox_regmem register memoryword *mem=zmem;
halfword zmathglue AA((halfword g,scaled m));
#define mathglue(g, m) zmathglue((halfword) (g), (scaled) (m))
#define mathglue_regmem register memoryword *mem=zmem;
void zmathkern AA((halfword p,scaled m));
#define mathkern(p, m) zmathkern((halfword) (p), (scaled) (m))
#define mathkern_regmem register memoryword *mem=zmem;
void flushmath AA((void));
#define flushmath_regmem register memoryword *mem=zmem;
halfword zcleanbox AA((halfword p,smallnumber s));
#define cleanbox(p, s) zcleanbox((halfword) (p), (smallnumber) (s))
#define cleanbox_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zfetch AA((halfword a));
#define fetch(a) zfetch((halfword) (a))
#define fetch_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zmakeover AA((halfword q));
#define makeover(q) zmakeover((halfword) (q))
#define makeover_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zmakeunder AA((halfword q));
#define makeunder(q) zmakeunder((halfword) (q))
#define makeunder_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zmakevcenter AA((halfword q));
#define makevcenter(q) zmakevcenter((halfword) (q))
#define makevcenter_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zmakeradical AA((halfword q));
#define makeradical(q) zmakeradical((halfword) (q))
#define makeradical_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zmakemathaccent AA((halfword q));
#define makemathaccent(q) zmakemathaccent((halfword) (q))
#define makemathaccent_regmem register memoryword *mem=zmem;
void zmakefraction AA((halfword q));
#define makefraction(q) zmakefraction((halfword) (q))
#define makefraction_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
scaled zmakeop AA((halfword q));
#define makeop(q) zmakeop((halfword) (q))
#define makeop_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zmakeord AA((halfword q));
#define makeord(q) zmakeord((halfword) (q))
#define makeord_regmem register memoryword *mem=zmem;
void zmakescripts AA((halfword q,scaled delta));
#define makescripts(q, delta) zmakescripts((halfword) (q), (scaled) (delta))
#define makescripts_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
smallnumber zmakeleftright AA((halfword q,smallnumber style,scaled maxd,scaled maxh));
#define makeleftright(q, style, maxd, maxh) zmakeleftright((halfword) (q), (smallnumber) (style), (scaled) (maxd), (scaled) (maxh))
#define makeleftright_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void mlisttohlist AA((void));
#define mlisttohlist_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void pushalignment AA((void));
#define pushalignment_regmem register memoryword *mem=zmem;
void popalignment AA((void));
#define popalignment_regmem register memoryword *mem=zmem;
void getpreambletoken AA((void));
#define getpreambletoken_regmem register memoryword *eqtb=zeqtb;
void initalign AA((void));
#define initalign_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zinitspan AA((halfword p));
#define initspan(p) zinitspan((halfword) (p))
#define initspan_regmem
void initrow AA((void));
#define initrow_regmem register memoryword *mem=zmem;
void initcol AA((void));
#define initcol_regmem register memoryword *mem=zmem;
boolean fincol AA((void));
#define fincol_regmem register memoryword *mem=zmem;
void finrow AA((void));
#define finrow_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void finalign AA((void));
#define finalign_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void alignpeek AA((void));
#define alignpeek_regmem
halfword zfiniteshrink AA((halfword p));
#define finiteshrink(p) zfiniteshrink((halfword) (p))
#define finiteshrink_regmem register memoryword *mem=zmem;
void ztrybreak AA((integer pi,smallnumber breaktype));
#define trybreak(pi, breaktype) ztrybreak((integer) (pi), (smallnumber) (breaktype))
#define trybreak_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zpostlinebreak AA((integer finalwidowpenalty));
#define postlinebreak(finalwidowpenalty) zpostlinebreak((integer) (finalwidowpenalty))
#define postlinebreak_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
smallnumber zreconstitute AA((smallnumber j,smallnumber n,halfword bchar,halfword hchar));
#define reconstitute(j, n, bchar, hchar) zreconstitute((smallnumber) (j), (smallnumber) (n), (halfword) (bchar), (halfword) (hchar))
#define reconstitute_regmem register memoryword *mem=zmem;
void hyphenate AA((void));
#define hyphenate_regmem register memoryword *mem=zmem;
trieopcode znewtrieop AA((smallnumber d,smallnumber n,trieopcode v));
#define newtrieop(d, n, v) znewtrieop((smallnumber) (d), (smallnumber) (n), (trieopcode) (v))
#define newtrieop_regmem
triepointer ztrienode AA((triepointer p));
#define trienode(p) ztrienode((triepointer) (p))
#define trienode_regmem
triepointer zcompresstrie AA((triepointer p));
#define compresstrie(p) zcompresstrie((triepointer) (p))
#define compresstrie_regmem
void zfirstfit AA((triepointer p));
#define firstfit(p) zfirstfit((triepointer) (p))
#define firstfit_regmem
void ztriepack AA((triepointer p));
#define triepack(p) ztriepack((triepointer) (p))
#define triepack_regmem
void ztriefix AA((triepointer p));
#define triefix(p) ztriefix((triepointer) (p))
#define triefix_regmem
void newpatterns AA((void));
#define newpatterns_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void inittrie AA((void));
#define inittrie_regmem
void zlinebreak AA((integer finalwidowpenalty));
#define linebreak(finalwidowpenalty) zlinebreak((integer) (finalwidowpenalty))
#define linebreak_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void newhyphexceptions AA((void));
#define newhyphexceptions_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
halfword zprunepagetop AA((halfword p));
#define prunepagetop(p) zprunepagetop((halfword) (p))
#define prunepagetop_regmem register memoryword *mem=zmem;
halfword zvertbreak AA((halfword p,scaled h,scaled d));
#define vertbreak(p, h, d) zvertbreak((halfword) (p), (scaled) (h), (scaled) (d))
#define vertbreak_regmem register memoryword *mem=zmem;
halfword zvsplit AA((eightbits n,scaled h));
#define vsplit(n, h) zvsplit((eightbits) (n), (scaled) (h))
#define vsplit_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void printtotals AA((void));
#define printtotals_regmem
void zfreezepagespecs AA((smallnumber s));
#define freezepagespecs(s) zfreezepagespecs((smallnumber) (s))
#define freezepagespecs_regmem register memoryword *eqtb=zeqtb;
void zboxerror AA((eightbits n));
#define boxerror(n) zboxerror((eightbits) (n))
#define boxerror_regmem register memoryword *eqtb=zeqtb;
void zensurevbox AA((eightbits n));
#define ensurevbox(n) zensurevbox((eightbits) (n))
#define ensurevbox_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zfireup AA((halfword c));
#define fireup(c) zfireup((halfword) (c))
#define fireup_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void buildpage AA((void));
#define buildpage_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void appspace AA((void));
#define appspace_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void insertdollarsign AA((void));
#define insertdollarsign_regmem
void youcant AA((void));
#define youcant_regmem
void reportillegalcase AA((void));
#define reportillegalcase_regmem
boolean privileged AA((void));
#define privileged_regmem
boolean itsallover AA((void));
#define itsallover_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void appendglue AA((void));
#define appendglue_regmem register memoryword *mem=zmem;
void appendkern AA((void));
#define appendkern_regmem register memoryword *mem=zmem;
void offsave AA((void));
#define offsave_regmem register memoryword *mem=zmem;
void extrarightbrace AA((void));
#define extrarightbrace_regmem
void normalparagraph AA((void));
#define normalparagraph_regmem register memoryword *eqtb=zeqtb;
void zboxend AA((integer boxcontext));
#define boxend(boxcontext) zboxend((integer) (boxcontext))
#define boxend_regmem register memoryword *mem=zmem;
void zbeginbox AA((integer boxcontext));
#define beginbox(boxcontext) zbeginbox((integer) (boxcontext))
#define beginbox_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zscanbox AA((integer boxcontext));
#define scanbox(boxcontext) zscanbox((integer) (boxcontext))
#define scanbox_regmem
void zpackage AA((smallnumber c));
#define package(c) zpackage((smallnumber) (c))
#define package_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
smallnumber znormmin AA((integer h));
#define normmin(h) znormmin((integer) (h))
#define normmin_regmem
void znewgraf AA((boolean indented));
#define newgraf(indented) znewgraf((boolean) (indented))
#define newgraf_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void indentinhmode AA((void));
#define indentinhmode_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void headforvmode AA((void));
#define headforvmode_regmem
void endgraf AA((void));
#define endgraf_regmem register memoryword *eqtb=zeqtb;
void begininsertoradjust AA((void));
#define begininsertoradjust_regmem
void makemark AA((void));
#define makemark_regmem register memoryword *mem=zmem;
void appendpenalty AA((void));
#define appendpenalty_regmem register memoryword *mem=zmem;
void deletelast AA((void));
#define deletelast_regmem register memoryword *mem=zmem;
void unpackage AA((void));
#define unpackage_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void appenditaliccorrection AA((void));
#define appenditaliccorrection_regmem register memoryword *mem=zmem;
void appenddiscretionary AA((void));
#define appenddiscretionary_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void builddiscretionary AA((void));
#define builddiscretionary_regmem register memoryword *mem=zmem;
void makeaccent AA((void));
#define makeaccent_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void alignerror AA((void));
#define alignerror_regmem
void noalignerror AA((void));
#define noalignerror_regmem
void omiterror AA((void));
#define omiterror_regmem
void doendv AA((void));
#define doendv_regmem
void cserror AA((void));
#define cserror_regmem
void zpushmath AA((groupcode c));
#define pushmath(c) zpushmath((groupcode) (c))
#define pushmath_regmem
void initmath AA((void));
#define initmath_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void starteqno AA((void));
#define starteqno_regmem register memoryword *eqtb=zeqtb;
void zscanmath AA((halfword p));
#define scanmath(p) zscanmath((halfword) (p))
#define scanmath_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void zsetmathchar AA((integer c));
#define setmathchar(c) zsetmathchar((integer) (c))
#define setmathchar_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void mathlimitswitch AA((void));
#define mathlimitswitch_regmem register memoryword *mem=zmem;
void zscandelimiter AA((halfword p,boolean r));
#define scandelimiter(p, r) zscandelimiter((halfword) (p), (boolean) (r))
#define scandelimiter_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void mathradical AA((void));
#define mathradical_regmem register memoryword *mem=zmem;
void mathac AA((void));
#define mathac_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void appendchoices AA((void));
#define appendchoices_regmem register memoryword *mem=zmem;
halfword zfinmlist AA((halfword p));
#define finmlist(p) zfinmlist((halfword) (p))
#define finmlist_regmem register memoryword *mem=zmem;
void buildchoices AA((void));
#define buildchoices_regmem register memoryword *mem=zmem;
void subsup AA((void));
#define subsup_regmem register memoryword *mem=zmem;
void mathfraction AA((void));
#define mathfraction_regmem register memoryword *mem=zmem;
void mathleftright AA((void));
#define mathleftright_regmem register memoryword *mem=zmem;
void aftermath AA((void));
#define aftermath_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void resumeafterdisplay AA((void));
#define resumeafterdisplay_regmem register memoryword *eqtb=zeqtb;
void getrtoken AA((void));
#define getrtoken_regmem
void trapzeroglue AA((void));
#define trapzeroglue_regmem register memoryword *mem=zmem;
void zdoregistercommand AA((smallnumber a));
#define doregistercommand(a) zdoregistercommand((smallnumber) (a))
#define doregistercommand_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void alteraux AA((void));
#define alteraux_regmem
void alterprevgraf AA((void));
#define alterprevgraf_regmem
void alterpagesofar AA((void));
#define alterpagesofar_regmem
void alterinteger AA((void));
#define alterinteger_regmem
void alterboxdimen AA((void));
#define alterboxdimen_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void znewfont AA((smallnumber a));
#define newfont(a) znewfont((smallnumber) (a))
#define newfont_regmem register memoryword *eqtb=zeqtb;
void newinteraction AA((void));
#define newinteraction_regmem
void prefixedcommand AA((void));
#define prefixedcommand_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void doassignments AA((void));
#define doassignments_regmem
void openorclosein AA((void));
#define openorclosein_regmem
void issuemessage AA((void));
#define issuemessage_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void shiftcase AA((void));
#define shiftcase_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void showwhatever AA((void));
#define showwhatever_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void storefmtfile AA((void));
#define storefmtfile_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void znewwhatsit AA((smallnumber s,smallnumber w));
#define newwhatsit(s, w) znewwhatsit((smallnumber) (s), (smallnumber) (w))
#define newwhatsit_regmem register memoryword *mem=zmem;
void znewwritewhatsit AA((smallnumber w));
#define newwritewhatsit(w) znewwritewhatsit((smallnumber) (w))
#define newwritewhatsit_regmem register memoryword *mem=zmem;
void doextension AA((void));
#define doextension_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void fixlanguage AA((void));
#define fixlanguage_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void handlerightbrace AA((void));
#define handlerightbrace_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void maincontrol AA((void));
#define maincontrol_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void giveerrhelp AA((void));
#define giveerrhelp_regmem register memoryword *eqtb=zeqtb;
boolean openfmtfile AA((void));
#define openfmtfile_regmem
boolean loadfmtfile AA((void));
#define loadfmtfile_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void closefilesandterminate AA((void));
#define closefilesandterminate_regmem register memoryword *eqtb=zeqtb;
void finalcleanup AA((void));
#define finalcleanup_regmem register memoryword *mem=zmem;
void initprim AA((void));
#define initprim_regmem register memoryword *eqtb=zeqtb;
void debughelp AA((void));
#define debughelp_regmem register memoryword *mem=zmem, *eqtb=zeqtb;
void mainbody AA((void));
#define mainbody_regmem register memoryword *eqtb=zeqtb;
/*
 * The C compiler ignores most unnecessary casts (i.e., casts of something
 * to its own type).  However, for structures, it doesn't.  Therefore,
 * we have to redefine these two macros so that they don't try to cast
 * the argument (a memoryword) as a memoryword.
 */
#undef	eqdestroy
#define	eqdestroy(x)	zeqdestroy(x)
#undef	printword
#define	printword(x)	zprintword(x)

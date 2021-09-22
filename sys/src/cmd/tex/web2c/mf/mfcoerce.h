void initialize AA((void));
void println AA((void));
void zprintvisiblechar AA((ASCIIcode s));
#define printvisiblechar(s) zprintvisiblechar((ASCIIcode) (s))
void zprintchar AA((ASCIIcode s));
#define printchar(s) zprintchar((ASCIIcode) (s))
void zprint AA((integer s));
#define print(s) zprint((integer) (s))
void zprintnl AA((strnumber s));
#define printnl(s) zprintnl((strnumber) (s))
void zprintthedigs AA((eightbits k));
#define printthedigs(k) zprintthedigs((eightbits) (k))
void zprintint AA((integer n));
#define printint(n) zprintint((integer) (n))
void zprintscaled AA((scaled s));
#define printscaled(s) zprintscaled((scaled) (s))
void zprinttwo AA((scaled x,scaled y));
#define printtwo(x, y) zprinttwo((scaled) (x), (scaled) (y))
void zprinttype AA((smallnumber t));
#define printtype(t) zprinttype((smallnumber) (t))
void begindiagnostic AA((void));
void zenddiagnostic AA((boolean blankline));
#define enddiagnostic(blankline) zenddiagnostic((boolean) (blankline))
void zprintdiagnostic AA((strnumber s,strnumber t,boolean nuline));
#define printdiagnostic(s, t, nuline) zprintdiagnostic((strnumber) (s), (strnumber) (t), (boolean) (nuline))
void zprintfilename AA((integer n,integer a,integer e));
#define printfilename(n, a, e) zprintfilename((integer) (n), (integer) (a), (integer) (e))
void zflushstring AA((strnumber s));
#define flushstring(s) zflushstring((strnumber) (s))
void jumpout AA((void));
void error AA((void));
void zfatalerror AA((strnumber s));
#define fatalerror(s) zfatalerror((strnumber) (s))
void zoverflow AA((strnumber s,integer n));
#define overflow(s, n) zoverflow((strnumber) (s), (integer) (n))
void zconfusion AA((strnumber s));
#define confusion(s) zconfusion((strnumber) (s))
boolean initterminal AA((void));
strnumber makestring AA((void));
boolean zstreqbuf AA((strnumber s,integer k));
#define streqbuf(s, k) zstreqbuf((strnumber) (s), (integer) (k))
integer zstrvsstr AA((strnumber s,strnumber t));
#define strvsstr(s, t) zstrvsstr((strnumber) (s), (strnumber) (t))
boolean getstringsstarted AA((void));
void zprintdd AA((integer n));
#define printdd(n) zprintdd((integer) (n))
void terminput AA((void));
void normalizeselector AA((void));
void pauseforinstructions AA((void));
void zmissingerr AA((strnumber s));
#define missingerr(s) zmissingerr((strnumber) (s))
void cleararith AA((void));
integer zslowadd AA((integer x,integer y));
#define slowadd(x, y) zslowadd((integer) (x), (integer) (y))
scaled zrounddecimals AA((smallnumber k));
#define rounddecimals(k) zrounddecimals((smallnumber) (k))
fraction zmakefraction AA((integer p,integer q));
#define makefraction(p, q) zmakefraction((integer) (p), (integer) (q))
integer ztakefraction AA((integer q,fraction f));
#define takefraction(q, f) ztakefraction((integer) (q), (fraction) (f))
integer ztakescaled AA((integer q,scaled f));
#define takescaled(q, f) ztakescaled((integer) (q), (scaled) (f))
scaled zmakescaled AA((integer p,integer q));
#define makescaled(p, q) zmakescaled((integer) (p), (integer) (q))
fraction zvelocity AA((fraction st,fraction ct,fraction sf,fraction cf,scaled t));
#define velocity(st, ct, sf, cf, t) zvelocity((fraction) (st), (fraction) (ct), (fraction) (sf), (fraction) (cf), (scaled) (t))
integer zabvscd AA((integer a,integer b,integer c,integer d));
#define abvscd(a, b, c, d) zabvscd((integer) (a), (integer) (b), (integer) (c), (integer) (d))
scaled zsquarert AA((scaled x));
#define squarert(x) zsquarert((scaled) (x))
integer zpythadd AA((integer a,integer b));
#define pythadd(a, b) zpythadd((integer) (a), (integer) (b))
integer zpythsub AA((integer a,integer b));
#define pythsub(a, b) zpythsub((integer) (a), (integer) (b))
scaled zmlog AA((scaled x));
#define mlog(x) zmlog((scaled) (x))
scaled zmexp AA((scaled x));
#define mexp(x) zmexp((scaled) (x))
angle znarg AA((integer x,integer y));
#define narg(x, y) znarg((integer) (x), (integer) (y))
void znsincos AA((angle z));
#define nsincos(z) znsincos((angle) (z))
void newrandoms AA((void));
void zinitrandoms AA((scaled seed));
#define initrandoms(seed) zinitrandoms((scaled) (seed))
scaled zunifrand AA((scaled x));
#define unifrand(x) zunifrand((scaled) (x))
scaled normrand AA((void));
void zprintword AA((memoryword w));
#define printword(w) zprintword((memoryword) (w))
void zshowtokenlist AA((integer p,integer q,integer l,integer nulltally));
#define showtokenlist(p, q, l, nulltally) zshowtokenlist((integer) (p), (integer) (q), (integer) (l), (integer) (nulltally))
void runaway AA((void));
halfword getavail AA((void));
halfword zgetnode AA((integer s));
#define getnode(s) zgetnode((integer) (s))
void zfreenode AA((halfword p,halfword s));
#define freenode(p, s) zfreenode((halfword) (p), (halfword) (s))
void sortavail AA((void));
void zflushlist AA((halfword p));
#define flushlist(p) zflushlist((halfword) (p))
void zflushnodelist AA((halfword p));
#define flushnodelist(p) zflushnodelist((halfword) (p))
void zcheckmem AA((boolean printlocs));
#define checkmem(printlocs) zcheckmem((boolean) (printlocs))
void zsearchmem AA((halfword p));
#define searchmem(p) zsearchmem((halfword) (p))
void zprintop AA((quarterword c));
#define printop(c) zprintop((quarterword) (c))
void fixdateandtime AA((void));
halfword zidlookup AA((integer j,integer l));
#define idlookup(j, l) zidlookup((integer) (j), (integer) (l))
void zprimitive AA((strnumber s,halfword c,halfword o));
#define primitive(s, c, o) zprimitive((strnumber) (s), (halfword) (c), (halfword) (o))
halfword znewnumtok AA((scaled v));
#define newnumtok(v) znewnumtok((scaled) (v))
void zflushtokenlist AA((halfword p));
#define flushtokenlist(p) zflushtokenlist((halfword) (p))
void zdeletemacref AA((halfword p));
#define deletemacref(p) zdeletemacref((halfword) (p))
void zprintcmdmod AA((integer c,integer m));
#define printcmdmod(c, m) zprintcmdmod((integer) (c), (integer) (m))
void zshowmacro AA((halfword p,integer q,integer l));
#define showmacro(p, q, l) zshowmacro((halfword) (p), (integer) (q), (integer) (l))
void zinitbignode AA((halfword p));
#define initbignode(p) zinitbignode((halfword) (p))
halfword idtransform AA((void));
void znewroot AA((halfword x));
#define newroot(x) znewroot((halfword) (x))
void zprintvariablename AA((halfword p));
#define printvariablename(p) zprintvariablename((halfword) (p))
boolean zinteresting AA((halfword p));
#define interesting(p) zinteresting((halfword) (p))
halfword znewstructure AA((halfword p));
#define newstructure(p) znewstructure((halfword) (p))
halfword zfindvariable AA((halfword t));
#define findvariable(t) zfindvariable((halfword) (t))
void zprintpath AA((halfword h,strnumber s,boolean nuline));
#define printpath(h, s, nuline) zprintpath((halfword) (h), (strnumber) (s), (boolean) (nuline))
void zprintweight AA((halfword q,integer xoff));
#define printweight(q, xoff) zprintweight((halfword) (q), (integer) (xoff))
void zprintedges AA((strnumber s,boolean nuline,integer xoff,integer yoff));
#define printedges(s, nuline, xoff, yoff) zprintedges((strnumber) (s), (boolean) (nuline), (integer) (xoff), (integer) (yoff))
void zunskew AA((scaled x,scaled y,smallnumber octant));
#define unskew(x, y, octant) zunskew((scaled) (x), (scaled) (y), (smallnumber) (octant))
void zprintpen AA((halfword p,strnumber s,boolean nuline));
#define printpen(p, s, nuline) zprintpen((halfword) (p), (strnumber) (s), (boolean) (nuline))
void zprintdependency AA((halfword p,smallnumber t));
#define printdependency(p, t) zprintdependency((halfword) (p), (smallnumber) (t))
void zprintdp AA((smallnumber t,halfword p,smallnumber verbosity));
#define printdp(t, p, verbosity) zprintdp((smallnumber) (t), (halfword) (p), (smallnumber) (verbosity))
halfword stashcurexp AA((void));
void zunstashcurexp AA((halfword p));
#define unstashcurexp(p) zunstashcurexp((halfword) (p))
void zprintexp AA((halfword p,smallnumber verbosity));
#define printexp(p, verbosity) zprintexp((halfword) (p), (smallnumber) (verbosity))
void zdisperr AA((halfword p,strnumber s));
#define disperr(p, s) zdisperr((halfword) (p), (strnumber) (s))
halfword zpplusfq AA((halfword p,integer f,halfword q,smallnumber t,smallnumber tt));
#define pplusfq(p, f, q, t, tt) zpplusfq((halfword) (p), (integer) (f), (halfword) (q), (smallnumber) (t), (smallnumber) (tt))
halfword zpoverv AA((halfword p,scaled v,smallnumber t0,smallnumber t1));
#define poverv(p, v, t0, t1) zpoverv((halfword) (p), (scaled) (v), (smallnumber) (t0), (smallnumber) (t1))
void zvaltoobig AA((scaled x));
#define valtoobig(x) zvaltoobig((scaled) (x))
void zmakeknown AA((halfword p,halfword q));
#define makeknown(p, q) zmakeknown((halfword) (p), (halfword) (q))
void fixdependencies AA((void));
void ztossknotlist AA((halfword p));
#define tossknotlist(p) ztossknotlist((halfword) (p))
void ztossedges AA((halfword h));
#define tossedges(h) ztossedges((halfword) (h))
void ztosspen AA((halfword p));
#define tosspen(p) ztosspen((halfword) (p))
void zringdelete AA((halfword p));
#define ringdelete(p) zringdelete((halfword) (p))
void zrecyclevalue AA((halfword p));
#define recyclevalue(p) zrecyclevalue((halfword) (p))
void zflushcurexp AA((scaled v));
#define flushcurexp(v) zflushcurexp((scaled) (v))
void zflusherror AA((scaled v));
#define flusherror(v) zflusherror((scaled) (v))
void putgeterror AA((void));
void zputgetflusherror AA((scaled v));
#define putgetflusherror(v) zputgetflusherror((scaled) (v))
void zflushbelowvariable AA((halfword p));
#define flushbelowvariable(p) zflushbelowvariable((halfword) (p))
void zflushvariable AA((halfword p,halfword t,boolean discardsuffixes));
#define flushvariable(p, t, discardsuffixes) zflushvariable((halfword) (p), (halfword) (t), (boolean) (discardsuffixes))
smallnumber zundtype AA((halfword p));
#define undtype(p) zundtype((halfword) (p))
void zclearsymbol AA((halfword p,boolean saving));
#define clearsymbol(p, saving) zclearsymbol((halfword) (p), (boolean) (saving))
void zsavevariable AA((halfword q));
#define savevariable(q) zsavevariable((halfword) (q))
void zsaveinternal AA((halfword q));
#define saveinternal(q) zsaveinternal((halfword) (q))
void unsave AA((void));
halfword zcopyknot AA((halfword p));
#define copyknot(p) zcopyknot((halfword) (p))
halfword zcopypath AA((halfword p));
#define copypath(p) zcopypath((halfword) (p))
halfword zhtapypoc AA((halfword p));
#define htapypoc(p) zhtapypoc((halfword) (p))
fraction zcurlratio AA((scaled gamma,scaled atension,scaled btension));
#define curlratio(gamma, atension, btension) zcurlratio((scaled) (gamma), (scaled) (atension), (scaled) (btension))
void zsetcontrols AA((halfword p,halfword q,integer k));
#define setcontrols(p, q, k) zsetcontrols((halfword) (p), (halfword) (q), (integer) (k))
void zsolvechoices AA((halfword p,halfword q,halfword n));
#define solvechoices(p, q, n) zsolvechoices((halfword) (p), (halfword) (q), (halfword) (n))
void zmakechoices AA((halfword knots));
#define makechoices(knots) zmakechoices((halfword) (knots))
void zmakemoves AA((scaled xx0,scaled xx1,scaled xx2,scaled xx3,scaled yy0,scaled yy1,scaled yy2,scaled yy3,smallnumber xicorr,smallnumber etacorr));
#define makemoves(xx0, xx1, xx2, xx3, yy0, yy1, yy2, yy3, xicorr, etacorr) zmakemoves((scaled) (xx0), (scaled) (xx1), (scaled) (xx2), (scaled) (xx3), (scaled) (yy0), (scaled) (yy1), (scaled) (yy2), (scaled) (yy3), (smallnumber) (xicorr), (smallnumber) (etacorr))
void zsmoothmoves AA((integer b,integer t));
#define smoothmoves(b, t) zsmoothmoves((integer) (b), (integer) (t))
void zinitedges AA((halfword h));
#define initedges(h) zinitedges((halfword) (h))
void fixoffset AA((void));
void zedgeprep AA((integer ml,integer mr,integer nl,integer nr));
#define edgeprep(ml, mr, nl, nr) zedgeprep((integer) (ml), (integer) (mr), (integer) (nl), (integer) (nr))
halfword zcopyedges AA((halfword h));
#define copyedges(h) zcopyedges((halfword) (h))
void yreflectedges AA((void));
void xreflectedges AA((void));
void zyscaleedges AA((integer s));
#define yscaleedges(s) zyscaleedges((integer) (s))
void zxscaleedges AA((integer s));
#define xscaleedges(s) zxscaleedges((integer) (s))
void znegateedges AA((halfword h));
#define negateedges(h) znegateedges((halfword) (h))
void zsortedges AA((halfword h));
#define sortedges(h) zsortedges((halfword) (h))
void zculledges AA((integer wlo,integer whi,integer wout,integer win));
#define culledges(wlo, whi, wout, win) zculledges((integer) (wlo), (integer) (whi), (integer) (wout), (integer) (win))
void xyswapedges AA((void));
void zmergeedges AA((halfword h));
#define mergeedges(h) zmergeedges((halfword) (h))
integer ztotalweight AA((halfword h));
#define totalweight(h) ztotalweight((halfword) (h))
void beginedgetracing AA((void));
void traceacorner AA((void));
void endedgetracing AA((void));
void ztracenewedge AA((halfword r,integer n));
#define tracenewedge(r, n) ztracenewedge((halfword) (r), (integer) (n))
void zlineedges AA((scaled x0,scaled y0,scaled x1,scaled y1));
#define lineedges(x0, y0, x1, y1) zlineedges((scaled) (x0), (scaled) (y0), (scaled) (x1), (scaled) (y1))
void zmovetoedges AA((integer m0,integer n0,integer m1,integer n1));
#define movetoedges(m0, n0, m1, n1) zmovetoedges((integer) (m0), (integer) (n0), (integer) (m1), (integer) (n1))
void zskew AA((scaled x,scaled y,smallnumber octant));
#define skew(x, y, octant) zskew((scaled) (x), (scaled) (y), (smallnumber) (octant))
void zabnegate AA((scaled x,scaled y,smallnumber octantbefore,smallnumber octantafter));
#define abnegate(x, y, octantbefore, octantafter) zabnegate((scaled) (x), (scaled) (y), (smallnumber) (octantbefore), (smallnumber) (octantafter))
fraction zcrossingpoint AA((integer a,integer b,integer c));
#define crossingpoint(a, b, c) zcrossingpoint((integer) (a), (integer) (b), (integer) (c))
void zprintspec AA((strnumber s));
#define printspec(s) zprintspec((strnumber) (s))
void zprintstrange AA((strnumber s));
#define printstrange(s) zprintstrange((strnumber) (s))
void zremovecubic AA((halfword p));
#define removecubic(p) zremovecubic((halfword) (p))
void zsplitcubic AA((halfword p,fraction t,scaled xq,scaled yq));
#define splitcubic(p, t, xq, yq) zsplitcubic((halfword) (p), (fraction) (t), (scaled) (xq), (scaled) (yq))
void quadrantsubdivide AA((void));
void octantsubdivide AA((void));
void makesafe AA((void));
void zbeforeandafter AA((scaled b,scaled a,halfword p));
#define beforeandafter(b, a, p) zbeforeandafter((scaled) (b), (scaled) (a), (halfword) (p))
scaled zgoodval AA((scaled b,scaled o));
#define goodval(b, o) zgoodval((scaled) (b), (scaled) (o))
scaled zcompromise AA((scaled u,scaled v));
#define compromise(u, v) zcompromise((scaled) (u), (scaled) (v))
void xyround AA((void));
void diaground AA((void));
void znewboundary AA((halfword p,smallnumber octant));
#define newboundary(p, octant) znewboundary((halfword) (p), (smallnumber) (octant))
halfword zmakespec AA((halfword h,scaled safetymargin,integer tracing));
#define makespec(h, safetymargin, tracing) zmakespec((halfword) (h), (scaled) (safetymargin), (integer) (tracing))
void zendround AA((scaled x,scaled y));
#define endround(x, y) zendround((scaled) (x), (scaled) (y))
void zfillspec AA((halfword h));
#define fillspec(h) zfillspec((halfword) (h))
void zdupoffset AA((halfword w));
#define dupoffset(w) zdupoffset((halfword) (w))
halfword zmakepen AA((halfword h));
#define makepen(h) zmakepen((halfword) (h))
halfword ztrivialknot AA((scaled x,scaled y));
#define trivialknot(x, y) ztrivialknot((scaled) (x), (scaled) (y))
halfword zmakepath AA((halfword penhead));
#define makepath(penhead) zmakepath((halfword) (penhead))
void zfindoffset AA((scaled x,scaled y,halfword p));
#define findoffset(x, y, p) zfindoffset((scaled) (x), (scaled) (y), (halfword) (p))
void zsplitforoffset AA((halfword p,fraction t));
#define splitforoffset(p, t) zsplitforoffset((halfword) (p), (fraction) (t))
void zfinoffsetprep AA((halfword p,halfword k,halfword w,integer x0,integer x1,integer x2,integer y0,integer y1,integer y2,boolean rising,integer n));
#define finoffsetprep(p, k, w, x0, x1, x2, y0, y1, y2, rising, n) zfinoffsetprep((halfword) (p), (halfword) (k), (halfword) (w), (integer) (x0), (integer) (x1), (integer) (x2), (integer) (y0), (integer) (y1), (integer) (y2), (boolean) (rising), (integer) (n))
void zoffsetprep AA((halfword c,halfword h));
#define offsetprep(c, h) zoffsetprep((halfword) (c), (halfword) (h))
void zskewlineedges AA((halfword p,halfword w,halfword ww));
#define skewlineedges(p, w, ww) zskewlineedges((halfword) (p), (halfword) (w), (halfword) (ww))
void zdualmoves AA((halfword h,halfword p,halfword q));
#define dualmoves(h, p, q) zdualmoves((halfword) (h), (halfword) (p), (halfword) (q))
void zfillenvelope AA((halfword spechead));
#define fillenvelope(spechead) zfillenvelope((halfword) (spechead))
halfword zmakeellipse AA((scaled majoraxis,scaled minoraxis,angle theta));
#define makeellipse(majoraxis, minoraxis, theta) zmakeellipse((scaled) (majoraxis), (scaled) (minoraxis), (angle) (theta))
scaled zfinddirectiontime AA((scaled x,scaled y,halfword h));
#define finddirectiontime(x, y, h) zfinddirectiontime((scaled) (x), (scaled) (y), (halfword) (h))
void zcubicintersection AA((halfword p,halfword pp));
#define cubicintersection(p, pp) zcubicintersection((halfword) (p), (halfword) (pp))
void zpathintersection AA((halfword h,halfword hh));
#define pathintersection(h, hh) zpathintersection((halfword) (h), (halfword) (hh))
void zopenawindow AA((windownumber k,scaled r0,scaled c0,scaled r1,scaled c1,scaled x,scaled y));
#define openawindow(k, r0, c0, r1, c1, x, y) zopenawindow((windownumber) (k), (scaled) (r0), (scaled) (c0), (scaled) (r1), (scaled) (c1), (scaled) (x), (scaled) (y))
void zdispedges AA((windownumber k));
#define dispedges(k) zdispedges((windownumber) (k))
fraction zmaxcoef AA((halfword p));
#define maxcoef(p) zmaxcoef((halfword) (p))
halfword zpplusq AA((halfword p,halfword q,smallnumber t));
#define pplusq(p, q, t) zpplusq((halfword) (p), (halfword) (q), (smallnumber) (t))
halfword zptimesv AA((halfword p,integer v,smallnumber t0,smallnumber t1,boolean visscaled));
#define ptimesv(p, v, t0, t1, visscaled) zptimesv((halfword) (p), (integer) (v), (smallnumber) (t0), (smallnumber) (t1), (boolean) (visscaled))
halfword zpwithxbecomingq AA((halfword p,halfword x,halfword q,smallnumber t));
#define pwithxbecomingq(p, x, q, t) zpwithxbecomingq((halfword) (p), (halfword) (x), (halfword) (q), (smallnumber) (t))
void znewdep AA((halfword q,halfword p));
#define newdep(q, p) znewdep((halfword) (q), (halfword) (p))
halfword zconstdependency AA((scaled v));
#define constdependency(v) zconstdependency((scaled) (v))
halfword zsingledependency AA((halfword p));
#define singledependency(p) zsingledependency((halfword) (p))
halfword zcopydeplist AA((halfword p));
#define copydeplist(p) zcopydeplist((halfword) (p))
void zlineareq AA((halfword p,smallnumber t));
#define lineareq(p, t) zlineareq((halfword) (p), (smallnumber) (t))
halfword znewringentry AA((halfword p));
#define newringentry(p) znewringentry((halfword) (p))
void znonlineareq AA((integer v,halfword p,boolean flushp));
#define nonlineareq(v, p, flushp) znonlineareq((integer) (v), (halfword) (p), (boolean) (flushp))
void zringmerge AA((halfword p,halfword q));
#define ringmerge(p, q) zringmerge((halfword) (p), (halfword) (q))
void zshowcmdmod AA((integer c,integer m));
#define showcmdmod(c, m) zshowcmdmod((integer) (c), (integer) (m))
void showcontext AA((void));
void zbegintokenlist AA((halfword p,quarterword t));
#define begintokenlist(p, t) zbegintokenlist((halfword) (p), (quarterword) (t))
void endtokenlist AA((void));
void zencapsulate AA((halfword p));
#define encapsulate(p) zencapsulate((halfword) (p))
void zinstall AA((halfword r,halfword q));
#define install(r, q) zinstall((halfword) (r), (halfword) (q))
void zmakeexpcopy AA((halfword p));
#define makeexpcopy(p) zmakeexpcopy((halfword) (p))
halfword curtok AA((void));
void backinput AA((void));
void backerror AA((void));
void inserror AA((void));
void beginfilereading AA((void));
void endfilereading AA((void));
void clearforerrorprompt AA((void));
boolean checkoutervalidity AA((void));
void getnext AA((void));
void firmuptheline AA((void));
halfword zscantoks AA((commandcode terminator,halfword substlist,halfword tailend,smallnumber suffixcount));
#define scantoks(terminator, substlist, tailend, suffixcount) zscantoks((commandcode) (terminator), (halfword) (substlist), (halfword) (tailend), (smallnumber) (suffixcount))
void getsymbol AA((void));
void getclearsymbol AA((void));
void checkequals AA((void));
void makeopdef AA((void));
void zcheckdelimiter AA((halfword ldelim,halfword rdelim));
#define checkdelimiter(ldelim, rdelim) zcheckdelimiter((halfword) (ldelim), (halfword) (rdelim))
halfword scandeclaredvariable AA((void));
void scandef AA((void));
void zprintmacroname AA((halfword a,halfword n));
#define printmacroname(a, n) zprintmacroname((halfword) (a), (halfword) (n))
void zprintarg AA((halfword q,integer n,halfword b));
#define printarg(q, n, b) zprintarg((halfword) (q), (integer) (n), (halfword) (b))
void zscantextarg AA((halfword ldelim,halfword rdelim));
#define scantextarg(ldelim, rdelim) zscantextarg((halfword) (ldelim), (halfword) (rdelim))
void zmacrocall AA((halfword defref,halfword arglist,halfword macroname));
#define macrocall(defref, arglist, macroname) zmacrocall((halfword) (defref), (halfword) (arglist), (halfword) (macroname))
void expand AA((void));
void getxnext AA((void));
void zstackargument AA((halfword p));
#define stackargument(p) zstackargument((halfword) (p))
void passtext AA((void));
void zchangeiflimit AA((smallnumber l,halfword p));
#define changeiflimit(l, p) zchangeiflimit((smallnumber) (l), (halfword) (p))
void checkcolon AA((void));
void conditional AA((void));
void zbadfor AA((strnumber s));
#define badfor(s) zbadfor((strnumber) (s))
void beginiteration AA((void));
void resumeiteration AA((void));
void stopiteration AA((void));
void beginname AA((void));
boolean zmorename AA((ASCIIcode c));
#define morename(c) zmorename((ASCIIcode) (c))
void endname AA((void));
void zpackfilename AA((strnumber n,strnumber a,strnumber e));
#define packfilename(n, a, e) zpackfilename((strnumber) (n), (strnumber) (a), (strnumber) (e))
void zpackbufferedname AA((smallnumber n,integer a,integer b));
#define packbufferedname(n, a, b) zpackbufferedname((smallnumber) (n), (integer) (a), (integer) (b))
strnumber makenamestring AA((void));
strnumber zamakenamestring AA((alphafile f));
#define amakenamestring(f) zamakenamestring((alphafile) (f))
strnumber zbmakenamestring AA((bytefile f));
#define bmakenamestring(f) zbmakenamestring((bytefile) (f))
strnumber zwmakenamestring AA((wordfile f));
#define wmakenamestring(f) zwmakenamestring((wordfile) (f))
void scanfilename AA((void));
void zpackjobname AA((strnumber s));
#define packjobname(s) zpackjobname((strnumber) (s))
void zpromptfilename AA((strnumber s,strnumber e));
#define promptfilename(s, e) zpromptfilename((strnumber) (s), (strnumber) (e))
void openlogfile AA((void));
void startinput AA((void));
void zbadexp AA((strnumber s));
#define badexp(s) zbadexp((strnumber) (s))
void zstashin AA((halfword p));
#define stashin(p) zstashin((halfword) (p))
void backexpr AA((void));
void badsubscript AA((void));
void zobliterated AA((halfword q));
#define obliterated(q) zobliterated((halfword) (q))
void zbinarymac AA((halfword p,halfword c,halfword n));
#define binarymac(p, c, n) zbinarymac((halfword) (p), (halfword) (c), (halfword) (n))
void materializepen AA((void));
void knownpair AA((void));
halfword newknot AA((void));
smallnumber scandirection AA((void));
void zdonullary AA((quarterword c));
#define donullary(c) zdonullary((quarterword) (c))
boolean znicepair AA((integer p,quarterword t));
#define nicepair(p, t) znicepair((integer) (p), (quarterword) (t))
void zprintknownorunknowntype AA((smallnumber t,integer v));
#define printknownorunknowntype(t, v) zprintknownorunknowntype((smallnumber) (t), (integer) (v))
void zbadunary AA((quarterword c));
#define badunary(c) zbadunary((quarterword) (c))
void znegatedeplist AA((halfword p));
#define negatedeplist(p) znegatedeplist((halfword) (p))
void pairtopath AA((void));
void ztakepart AA((quarterword c));
#define takepart(c) ztakepart((quarterword) (c))
void zstrtonum AA((quarterword c));
#define strtonum(c) zstrtonum((quarterword) (c))
scaled pathlength AA((void));
void ztestknown AA((quarterword c));
#define testknown(c) ztestknown((quarterword) (c))
void zdounary AA((quarterword c));
#define dounary(c) zdounary((quarterword) (c))
void zbadbinary AA((halfword p,quarterword c));
#define badbinary(p, c) zbadbinary((halfword) (p), (quarterword) (c))
halfword ztarnished AA((halfword p));
#define tarnished(p) ztarnished((halfword) (p))
void zdepfinish AA((halfword v,halfword q,smallnumber t));
#define depfinish(v, q, t) zdepfinish((halfword) (v), (halfword) (q), (smallnumber) (t))
void zaddorsubtract AA((halfword p,halfword q,quarterword c));
#define addorsubtract(p, q, c) zaddorsubtract((halfword) (p), (halfword) (q), (quarterword) (c))
void zdepmult AA((halfword p,integer v,boolean visscaled));
#define depmult(p, v, visscaled) zdepmult((halfword) (p), (integer) (v), (boolean) (visscaled))
void zhardtimes AA((halfword p));
#define hardtimes(p) zhardtimes((halfword) (p))
void zdepdiv AA((halfword p,scaled v));
#define depdiv(p, v) zdepdiv((halfword) (p), (scaled) (v))
void zsetuptrans AA((quarterword c));
#define setuptrans(c) zsetuptrans((quarterword) (c))
void zsetupknowntrans AA((quarterword c));
#define setupknowntrans(c) zsetupknowntrans((quarterword) (c))
void ztrans AA((halfword p,halfword q));
#define trans(p, q) ztrans((halfword) (p), (halfword) (q))
void zpathtrans AA((halfword p,quarterword c));
#define pathtrans(p, c) zpathtrans((halfword) (p), (quarterword) (c))
void zedgestrans AA((halfword p,quarterword c));
#define edgestrans(p, c) zedgestrans((halfword) (p), (quarterword) (c))
void zbilin1 AA((halfword p,scaled t,halfword q,scaled u,scaled delta));
#define bilin1(p, t, q, u, delta) zbilin1((halfword) (p), (scaled) (t), (halfword) (q), (scaled) (u), (scaled) (delta))
void zaddmultdep AA((halfword p,scaled v,halfword r));
#define addmultdep(p, v, r) zaddmultdep((halfword) (p), (scaled) (v), (halfword) (r))
void zbilin2 AA((halfword p,halfword t,scaled v,halfword u,halfword q));
#define bilin2(p, t, v, u, q) zbilin2((halfword) (p), (halfword) (t), (scaled) (v), (halfword) (u), (halfword) (q))
void zbilin3 AA((halfword p,scaled t,scaled v,scaled u,scaled delta));
#define bilin3(p, t, v, u, delta) zbilin3((halfword) (p), (scaled) (t), (scaled) (v), (scaled) (u), (scaled) (delta))
void zbigtrans AA((halfword p,quarterword c));
#define bigtrans(p, c) zbigtrans((halfword) (p), (quarterword) (c))
void zcat AA((halfword p));
#define cat(p) zcat((halfword) (p))
void zchopstring AA((halfword p));
#define chopstring(p) zchopstring((halfword) (p))
void zchoppath AA((halfword p));
#define choppath(p) zchoppath((halfword) (p))
void zpairvalue AA((scaled x,scaled y));
#define pairvalue(x, y) zpairvalue((scaled) (x), (scaled) (y))
void zsetupoffset AA((halfword p));
#define setupoffset(p) zsetupoffset((halfword) (p))
void zsetupdirectiontime AA((halfword p));
#define setupdirectiontime(p) zsetupdirectiontime((halfword) (p))
void zfindpoint AA((scaled v,quarterword c));
#define findpoint(v, c) zfindpoint((scaled) (v), (quarterword) (c))
void zdobinary AA((halfword p,quarterword c));
#define dobinary(p, c) zdobinary((halfword) (p), (quarterword) (c))
void zfracmult AA((scaled n,scaled d));
#define fracmult(n, d) zfracmult((scaled) (n), (scaled) (d))
void gfswap AA((void));
void zgffour AA((integer x));
#define gffour(x) zgffour((integer) (x))
void zgftwo AA((integer x));
#define gftwo(x) zgftwo((integer) (x))
void zgfthree AA((integer x));
#define gfthree(x) zgfthree((integer) (x))
void zgfpaint AA((integer d));
#define gfpaint(d) zgfpaint((integer) (d))
void zgfstring AA((strnumber s,strnumber t));
#define gfstring(s, t) zgfstring((strnumber) (s), (strnumber) (t))
void zgfboc AA((integer minm,integer maxm,integer minn,integer maxn));
#define gfboc(minm, maxm, minn, maxn) zgfboc((integer) (minm), (integer) (maxm), (integer) (minn), (integer) (maxn))
void initgf AA((void));
void zshipout AA((eightbits c));
#define shipout(c) zshipout((eightbits) (c))
void ztryeq AA((halfword l,halfword r));
#define tryeq(l, r) ztryeq((halfword) (l), (halfword) (r))
void zmakeeq AA((halfword lhs));
#define makeeq(lhs) zmakeeq((halfword) (lhs))
void doequation AA((void));
void doassignment AA((void));
void dotypedeclaration AA((void));
void dorandomseed AA((void));
void doprotection AA((void));
void defdelims AA((void));
void dointerim AA((void));
void dolet AA((void));
void donewinternal AA((void));
void doshow AA((void));
void disptoken AA((void));
void doshowtoken AA((void));
void doshowstats AA((void));
void zdispvar AA((halfword p));
#define dispvar(p) zdispvar((halfword) (p))
void doshowvar AA((void));
void doshowdependencies AA((void));
void doshowwhatever AA((void));
boolean scanwith AA((void));
void zfindedgesvar AA((halfword t));
#define findedgesvar(t) zfindedgesvar((halfword) (t))
void doaddto AA((void));
scaled ztfmcheck AA((smallnumber m));
#define tfmcheck(m) ztfmcheck((smallnumber) (m))
void doshipout AA((void));
void dodisplay AA((void));
boolean zgetpair AA((commandcode c));
#define getpair(c) zgetpair((commandcode) (c))
void doopenwindow AA((void));
void docull AA((void));
void domessage AA((void));
eightbits getcode AA((void));
void zsettag AA((halfword c,smallnumber t,halfword r));
#define settag(c, t, r) zsettag((halfword) (c), (smallnumber) (t), (halfword) (r))
void dotfmcommand AA((void));
void dospecial AA((void));
void storebasefile AA((void));
void dostatement AA((void));
void maincontrol AA((void));
halfword zsortin AA((scaled v));
#define sortin(v) zsortin((scaled) (v))
integer zmincover AA((scaled d));
#define mincover(d) zmincover((scaled) (d))
scaled zthresholdfn AA((integer m));
#define thresholdfn(m) zthresholdfn((integer) (m))
integer zskimp AA((integer m));
#define skimp(m) zskimp((integer) (m))
void ztfmwarning AA((smallnumber m));
#define tfmwarning(m) ztfmwarning((smallnumber) (m))
void fixdesignsize AA((void));
integer zdimenout AA((scaled x));
#define dimenout(x) zdimenout((scaled) (x))
void fixchecksum AA((void));
void ztfmqqqq AA((fourquarters x));
#define tfmqqqq(x) ztfmqqqq((fourquarters) (x))
boolean openbasefile AA((void));
boolean loadbasefile AA((void));
void scanprimary AA((void));
void scansuffix AA((void));
void scansecondary AA((void));
void scantertiary AA((void));
void scanexpression AA((void));
void getboolean AA((void));
void printcapsule AA((void));
void tokenrecycle AA((void));
void closefilesandterminate AA((void));
void finalcleanup AA((void));
void initprim AA((void));
void inittab AA((void));
void debughelp AA((void));
/* Some definitions that get appended to the `coerce.h' file that web2c
   outputs.  */

/* The C compiler ignores most unnecessary casts (i.e., casts of
   something to its own type).  However, for structures, it doesn't.
   Therefore, we have to redefine these macros so they don't cast
   cast their argument (of type memoryword or fourquarters,
   respectively).  */
#undef	printword
#define	printword(x)	zprintword (x)

#undef	tfmqqqq
#define tfmqqqq(x)	ztfmqqqq (x)

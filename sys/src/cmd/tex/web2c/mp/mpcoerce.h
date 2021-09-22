void initialize AA((void));
void println AA((void));
void zprintvisiblechar AA((ASCIIcode s));
#define printvisiblechar(s) zprintvisiblechar((ASCIIcode) (s))
void zprintchar AA((ASCIIcode k));
#define printchar(k) zprintchar((ASCIIcode) (k))
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
integer trueline AA((void));
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
void zdocompaction AA((poolpointer needed));
#define docompaction(needed) zdocompaction((poolpointer) (needed))
void unitstrroom AA((void));
strnumber makestring AA((void));
void zchoplaststring AA((poolpointer p));
#define choplaststring(p) zchoplaststring((poolpointer) (p))
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
void zprpath AA((halfword h));
#define prpath(h) zprpath((halfword) (h))
void zprintpath AA((halfword h,strnumber s,boolean nuline));
#define printpath(h, s, nuline) zprintpath((halfword) (h), (strnumber) (s), (boolean) (nuline))
void zprpen AA((halfword h));
#define prpen(h) zprpen((halfword) (h))
void zprintpen AA((halfword h,strnumber s,boolean nuline));
#define printpen(h, s, nuline) zprintpen((halfword) (h), (strnumber) (s), (boolean) (nuline))
scaled zsqrtdet AA((scaled a,scaled b,scaled c,scaled d));
#define sqrtdet(a, b, c, d) zsqrtdet((scaled) (a), (scaled) (b), (scaled) (c), (scaled) (d))
scaled zgetpenscale AA((halfword p));
#define getpenscale(p) zgetpenscale((halfword) (p))
void zprintcompactnode AA((halfword p,smallnumber k));
#define printcompactnode(p, k) zprintcompactnode((halfword) (p), (smallnumber) (k))
void zprintobjcolor AA((halfword p));
#define printobjcolor(p) zprintobjcolor((halfword) (p))
scaled zdashoffset AA((halfword h));
#define dashoffset(h) zdashoffset((halfword) (h))
void zprintedges AA((halfword h,strnumber s,boolean nuline));
#define printedges(h, s, nuline) zprintedges((halfword) (h), (strnumber) (s), (boolean) (nuline))
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
void zflushdashlist AA((halfword h));
#define flushdashlist(h) zflushdashlist((halfword) (h))
halfword ztossgrobject AA((halfword p));
#define tossgrobject(p) ztossgrobject((halfword) (p))
void ztossedges AA((halfword h));
#define tossedges(h) ztossedges((halfword) (h))
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
fraction zcrossingpoint AA((integer a,integer b,integer c));
#define crossingpoint(a, b, c) zcrossingpoint((integer) (a), (integer) (b), (integer) (c))
scaled zevalcubic AA((halfword p,halfword q,fraction t));
#define evalcubic(p, q, t) zevalcubic((halfword) (p), (halfword) (q), (fraction) (t))
void zboundcubic AA((halfword p,halfword q,smallnumber c));
#define boundcubic(p, q, c) zboundcubic((halfword) (p), (halfword) (q), (smallnumber) (c))
void zpathbbox AA((halfword h));
#define pathbbox(h) zpathbbox((halfword) (h))
scaled zsolverisingcubic AA((scaled a,scaled b,scaled c,scaled x));
#define solverisingcubic(a, b, c, x) zsolverisingcubic((scaled) (a), (scaled) (b), (scaled) (c), (scaled) (x))
scaled zarctest AA((scaled dx0,scaled dy0,scaled dx1,scaled dy1,scaled dx2,scaled dy2,scaled v0,scaled v02,scaled v2,scaled agoal,scaled tol));
#define arctest(dx0, dy0, dx1, dy1, dx2, dy2, v0, v02, v2, agoal, tol) zarctest((scaled) (dx0), (scaled) (dy0), (scaled) (dx1), (scaled) (dy1), (scaled) (dx2), (scaled) (dy2), (scaled) (v0), (scaled) (v02), (scaled) (v2), (scaled) (agoal), (scaled) (tol))
scaled zdoarctest AA((scaled dx0,scaled dy0,scaled dx1,scaled dy1,scaled dx2,scaled dy2,scaled agoal));
#define doarctest(dx0, dy0, dx1, dy1, dx2, dy2, agoal) zdoarctest((scaled) (dx0), (scaled) (dy0), (scaled) (dx1), (scaled) (dy1), (scaled) (dx2), (scaled) (dy2), (scaled) (agoal))
scaled zgetarclength AA((halfword h));
#define getarclength(h) zgetarclength((halfword) (h))
scaled zgetarctime AA((halfword h,scaled arc0));
#define getarctime(h, arc0) zgetarctime((halfword) (h), (scaled) (arc0))
void zmoveknot AA((halfword p,halfword q));
#define moveknot(p, q) zmoveknot((halfword) (p), (halfword) (q))
halfword zconvexhull AA((halfword h));
#define convexhull(h) zconvexhull((halfword) (h))
halfword zmakepen AA((halfword h,boolean needhull));
#define makepen(h, needhull) zmakepen((halfword) (h), (boolean) (needhull))
halfword zgetpencircle AA((scaled diam));
#define getpencircle(diam) zgetpencircle((scaled) (diam))
void zmakepath AA((halfword h));
#define makepath(h) zmakepath((halfword) (h))
void zfindoffset AA((scaled x,scaled y,halfword h));
#define findoffset(x, y, h) zfindoffset((scaled) (x), (scaled) (y), (halfword) (h))
void zpenbbox AA((halfword h));
#define penbbox(h) zpenbbox((halfword) (h))
halfword znewfillnode AA((halfword p));
#define newfillnode(p) znewfillnode((halfword) (p))
halfword znewstrokednode AA((halfword p));
#define newstrokednode(p) znewstrokednode((halfword) (p))
void beginname AA((void));
boolean zmorename AA((ASCIIcode c));
#define morename(c) zmorename((ASCIIcode) (c))
void endname AA((void));
void zpackfilename AA((strnumber n,strnumber a,strnumber e));
#define packfilename(n, a, e) zpackfilename((strnumber) (n), (strnumber) (a), (strnumber) (e))
void zstrscanfile AA((strnumber s));
#define strscanfile(s) zstrscanfile((strnumber) (s))
fontnumber zreadfontinfo AA((strnumber fname));
#define readfontinfo(fname) zreadfontinfo((strnumber) (fname))
fontnumber zfindfont AA((strnumber f));
#define findfont(f) zfindfont((strnumber) (f))
void zlostwarning AA((fontnumber f,poolpointer k));
#define lostwarning(f, k) zlostwarning((fontnumber) (f), (poolpointer) (k))
void zsettextbox AA((halfword p));
#define settextbox(p) zsettextbox((halfword) (p))
halfword znewtextnode AA((strnumber f,strnumber s));
#define newtextnode(f, s) znewtextnode((strnumber) (f), (strnumber) (s))
halfword znewboundsnode AA((halfword p,smallnumber c));
#define newboundsnode(p, c) znewboundsnode((halfword) (p), (smallnumber) (c))
void zinitbbox AA((halfword h));
#define initbbox(h) zinitbbox((halfword) (h))
void zinitedges AA((halfword h));
#define initedges(h) zinitedges((halfword) (h))
halfword zcopyobjects AA((halfword p,halfword q));
#define copyobjects(p, q) zcopyobjects((halfword) (p), (halfword) (q))
halfword zprivateedges AA((halfword h));
#define privateedges(h) zprivateedges((halfword) (h))
halfword zskip1component AA((halfword p));
#define skip1component(p) zskip1component((halfword) (p))
void xretraceerror AA((void));
halfword zmakedashes AA((halfword h));
#define makedashes(h) zmakedashes((halfword) (h))
void zadjustbbox AA((halfword h));
#define adjustbbox(h) zadjustbbox((halfword) (h))
void zboxends AA((halfword p,halfword pp,halfword h));
#define boxends(p, pp, h) zboxends((halfword) (p), (halfword) (pp), (halfword) (h))
void zsetbbox AA((halfword h,boolean toplevel));
#define setbbox(h, toplevel) zsetbbox((halfword) (h), (boolean) (toplevel))
void zsplitcubic AA((halfword p,fraction t));
#define splitcubic(p, t) zsplitcubic((halfword) (p), (fraction) (t))
void zremovecubic AA((halfword p));
#define removecubic(p) zremovecubic((halfword) (p))
halfword zpenwalk AA((halfword w,integer k));
#define penwalk(w, k) zpenwalk((halfword) (w), (integer) (k))
void zfinoffsetprep AA((halfword p,halfword w,integer x0,integer x1,integer x2,integer y0,integer y1,integer y2,integer rise,integer turnamt));
#define finoffsetprep(p, w, x0, x1, x2, y0, y1, y2, rise, turnamt) zfinoffsetprep((halfword) (p), (halfword) (w), (integer) (x0), (integer) (x1), (integer) (x2), (integer) (y0), (integer) (y1), (integer) (y2), (integer) (rise), (integer) (turnamt))
integer zgetturnamt AA((halfword w,scaled dx,scaled dy,boolean ccw));
#define getturnamt(w, dx, dy, ccw) zgetturnamt((halfword) (w), (scaled) (dx), (scaled) (dy), (boolean) (ccw))
halfword zoffsetprep AA((halfword c,halfword h));
#define offsetprep(c, h) zoffsetprep((halfword) (c), (halfword) (h))
void zprintspec AA((halfword curspec,halfword curpen,strnumber s));
#define printspec(curspec, curpen, s) zprintspec((halfword) (curspec), (halfword) (curpen), (strnumber) (s))
halfword zinsertknot AA((halfword q,scaled x,scaled y));
#define insertknot(q, x, y) zinsertknot((halfword) (q), (scaled) (x), (scaled) (y))
halfword zmakeenvelope AA((halfword c,halfword h,smallnumber ljoin,smallnumber lcap,scaled miterlim));
#define makeenvelope(c, h, ljoin, lcap, miterlim) zmakeenvelope((halfword) (c), (halfword) (h), (smallnumber) (ljoin), (smallnumber) (lcap), (scaled) (miterlim))
scaled zfinddirectiontime AA((scaled x,scaled y,halfword h));
#define finddirectiontime(x, y, h) zfinddirectiontime((scaled) (x), (scaled) (y), (halfword) (h))
void zcubicintersection AA((halfword p,halfword pp));
#define cubicintersection(p, pp) zcubicintersection((halfword) (p), (halfword) (pp))
void zpathintersection AA((halfword h,halfword hh));
#define pathintersection(h, hh) zpathintersection((halfword) (h), (halfword) (hh))
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
boolean beginmpxreading AA((void));
void endmpxreading AA((void));
void clearforerrorprompt AA((void));
boolean checkoutervalidity AA((void));
void getnext AA((void));
void firmuptheline AA((void));
void tnext AA((void));
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
boolean ztryextension AA((strnumber ext));
#define tryextension(ext) ztryextension((strnumber) (ext))
void startinput AA((void));
void zcopyoldname AA((strnumber s));
#define copyoldname(s) zcopyoldname((strnumber) (s))
void startmpxinput AA((void));
boolean zstartreadinput AA((strnumber s,readfindex n));
#define startreadinput(s, n) zstartreadinput((strnumber) (s), (readfindex) (n))
void zopenwritefile AA((strnumber s,readfindex n));
#define openwritefile(s, n) zopenwritefile((strnumber) (s), (readfindex) (n))
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
void knownpair AA((void));
halfword newknot AA((void));
smallnumber scandirection AA((void));
void finishread AA((void));
void zdonullary AA((quarterword c));
#define donullary(c) zdonullary((quarterword) (c))
boolean znicepair AA((integer p,quarterword t));
#define nicepair(p, t) znicepair((integer) (p), (quarterword) (t))
boolean znicecolororpair AA((integer p,quarterword t));
#define nicecolororpair(p, t) znicecolororpair((integer) (p), (quarterword) (t))
void zprintknownorunknowntype AA((smallnumber t,integer v));
#define printknownorunknowntype(t, v) zprintknownorunknowntype((smallnumber) (t), (integer) (v))
void zbadunary AA((quarterword c));
#define badunary(c) zbadunary((quarterword) (c))
void znegatedeplist AA((halfword p));
#define negatedeplist(p) znegatedeplist((halfword) (p))
void pairtopath AA((void));
void ztakepart AA((quarterword c));
#define takepart(c) ztakepart((quarterword) (c))
void ztakepictpart AA((quarterword c));
#define takepictpart(c) ztakepictpart((quarterword) (c))
void zstrtonum AA((quarterword c));
#define strtonum(c) zstrtonum((quarterword) (c))
scaled pathlength AA((void));
scaled pictlength AA((void));
scaled zcountturns AA((halfword c));
#define countturns(c) zcountturns((halfword) (c))
void ztestknown AA((quarterword c));
#define testknown(c) ztestknown((quarterword) (c))
void zpairvalue AA((scaled x,scaled y));
#define pairvalue(x, y) zpairvalue((scaled) (x), (scaled) (y))
boolean getcurbbox AA((void));
void zdoreadorclose AA((quarterword c));
#define doreadorclose(c) zdoreadorclose((quarterword) (c))
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
void zdopathtrans AA((halfword p));
#define dopathtrans(p) zdopathtrans((halfword) (p))
void zdopentrans AA((halfword p));
#define dopentrans(p) zdopentrans((halfword) (p))
halfword zedgestrans AA((halfword h));
#define edgestrans(h) zedgestrans((halfword) (h))
void zdoedgestrans AA((halfword p,quarterword c));
#define doedgestrans(p, c) zdoedgestrans((halfword) (p), (quarterword) (c))
void scaleedges AA((void));
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
void zsetupoffset AA((halfword p));
#define setupoffset(p) zsetupoffset((halfword) (p))
void zsetupdirectiontime AA((halfword p));
#define setupdirectiontime(p) zsetupdirectiontime((halfword) (p))
void zfindpoint AA((scaled v,quarterword c));
#define findpoint(v, c) zfindpoint((scaled) (v), (quarterword) (c))
void zdoinfont AA((halfword p));
#define doinfont(p) zdoinfont((halfword) (p))
void zdobinary AA((halfword p,quarterword c));
#define dobinary(p, c) zdobinary((halfword) (p), (quarterword) (c))
void zfracmult AA((scaled n,scaled d));
#define fracmult(n, d) zfracmult((scaled) (n), (scaled) (d))
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
void zscanwithlist AA((halfword p));
#define scanwithlist(p) zscanwithlist((halfword) (p))
halfword zfindedgesvar AA((halfword t));
#define findedgesvar(t) zfindedgesvar((halfword) (t))
halfword zstartdrawcmd AA((quarterword sep));
#define startdrawcmd(sep) zstartdrawcmd((quarterword) (sep))
void dobounds AA((void));
void doaddto AA((void));
scaled ztfmcheck AA((smallnumber m));
#define tfmcheck(m) ztfmcheck((smallnumber) (m))
void readpsnametable AA((void));
void openoutputfile AA((void));
void zpspairout AA((scaled x,scaled y));
#define pspairout(x, y) zpspairout((scaled) (x), (scaled) (y))
void zpsprint AA((strnumber s));
#define psprint(s) zpsprint((strnumber) (s))
void zpspathout AA((halfword h));
#define pspathout(h) zpspathout((halfword) (h))
void zunknowngraphicsstate AA((scaled c));
#define unknowngraphicsstate(c) zunknowngraphicsstate((scaled) (c))
boolean zcoordrangeOK AA((halfword h,smallnumber zoff,scaled dz));
#define coordrangeOK(h, zoff, dz) zcoordrangeOK((halfword) (h), (smallnumber) (zoff), (scaled) (dz))
boolean zsamedashes AA((halfword h,halfword hh));
#define samedashes(h, hh) zsamedashes((halfword) (h), (halfword) (hh))
void zfixgraphicsstate AA((halfword p));
#define fixgraphicsstate(p) zfixgraphicsstate((halfword) (p))
void zstrokeellipse AA((halfword h,boolean fillalso));
#define strokeellipse(h, fillalso) zstrokeellipse((halfword) (h), (boolean) (fillalso))
void zpsfillout AA((halfword p));
#define psfillout(p) zpsfillout((halfword) (p))
void zdoouterenvelope AA((halfword p,halfword h));
#define doouterenvelope(p, h) zdoouterenvelope((halfword) (p), (halfword) (h))
scaled zchoosescale AA((halfword p));
#define choosescale(p) zchoosescale((halfword) (p))
void zpsstringout AA((strnumber s));
#define psstringout(s) zpsstringout((strnumber) (s))
boolean zispsname AA((strnumber s));
#define ispsname(s) zispsname((strnumber) (s))
void zpsnameout AA((strnumber s,boolean lit));
#define psnameout(s, lit) zpsnameout((strnumber) (s), (boolean) (lit))
void zunmarkfont AA((fontnumber f));
#define unmarkfont(f) zunmarkfont((fontnumber) (f))
void zmarkstringchars AA((fontnumber f,strnumber s));
#define markstringchars(f, s) zmarkstringchars((fontnumber) (f), (strnumber) (s))
void zhexdigitout AA((smallnumber d));
#define hexdigitout(d) zhexdigitout((smallnumber) (d))
halfword zpsmarksout AA((fontnumber f,eightbits c));
#define psmarksout(f, c) zpsmarksout((fontnumber) (f), (eightbits) (c))
boolean zcheckpsmarks AA((fontnumber f,integer c));
#define checkpsmarks(f, c) zcheckpsmarks((fontnumber) (f), (integer) (c))
quarterword zsizeindex AA((fontnumber f,scaled s));
#define sizeindex(f, s) zsizeindex((fontnumber) (f), (scaled) (s))
scaled zindexedsize AA((fontnumber f,quarterword j));
#define indexedsize(f, j) zindexedsize((fontnumber) (f), (quarterword) (j))
void clearsizes AA((void));
void zshipout AA((halfword h));
#define shipout(h) zshipout((halfword) (h))
void doshipout AA((void));
void znostringerr AA((strnumber s));
#define nostringerr(s) znostringerr((strnumber) (s))
void domessage AA((void));
void dowrite AA((void));
eightbits getcode AA((void));
void zsettag AA((halfword c,smallnumber t,halfword r));
#define settag(c, t, r) zsettag((halfword) (c), (smallnumber) (t), (halfword) (r))
void dotfmcommand AA((void));
void dospecial AA((void));
void storememfile AA((void));
void dostatement AA((void));
void maincontrol AA((void));
halfword zsortin AA((scaled v));
#define sortin(v) zsortin((scaled) (v))
integer zmincover AA((scaled d));
#define mincover(d) zmincover((scaled) (d))
scaled zcomputethreshold AA((integer m));
#define computethreshold(m) zcomputethreshold((integer) (m))
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
boolean openmemfile AA((void));
boolean loadmemfile AA((void));
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

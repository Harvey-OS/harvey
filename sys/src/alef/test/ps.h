	/****** rationals ******/

aggr rat { int num, den; };

int gcd(int u, int v);
rat ratmk(int i, int j);
rat ratneg(rat r);
rat ratadd(rat r, rat s);
rat ratsub(rat r, rat s);
rat ratinv(rat r);
rat ratmul(rat r, rat s);
void ratprint(rat r);

	/****** power series ******/

enum unit { UNIT };

aggr ps { chan(unit) req; chan(rat) dat; };

ps psmk();
(ps, ps) psdup(ps F);
void psprint(ps F);
void psprintn(ps F, int n);
ps psspy(byte*m, ps F);
ps Ones();
ps psmon(int n);
ps psneg(ps F);
ps psadd(ps F, ps G);
ps pssub(ps F, ps G);
ps pssca(rat c, int n, ps F);
ps pscmul(rat c, ps F);
ps psxmul(ps F);
ps psdif(ps F);
ps psint(ps F, rat c);
ps psmul(ps F, ps G);
ps pscom(ps F, ps G);
ps psexp(ps F);
ps psinv(ps F);
ps psrev(ps F);
ps pssqu(ps F);

#define pscmul(c, F) pssca(c, 0, F)
#define psxmul(F) pssca((1,1), 1, F)

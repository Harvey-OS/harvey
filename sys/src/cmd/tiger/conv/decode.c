#include	<all.h>

void
decode1(char *p)
{
	Rec1 *r;

	if(nrec1 >= mrec1) {
		mrec1 = mrec1*2;
		if(mrec1 < xrec1)
			mrec1 = xrec1;
		rec1 = realloc(rec1, mrec1*sizeof(Rec1));
	}
	r = rec1+nrec1;
	nrec1++;

	r->version = rec(p, N, 4, 2);	/* 0.00 -- version number */
	r->tlid = num(p, N, 10, 6);	/* 1.00 -- record number (permanent) */
	r->side1 = rec(p, N, 1, 16);	/* 0.00 -- single-side segment code */
					/*	(value "1" signifies */
					/*	data exist only for one */
					/*	side of the segment) */
	r->source = rec(p, A, 1, 17);	/* 0.00 -- source code */
	r->fedirp = rec(p, A, 2, 18);	/* 0.00 -- feature direction, prefix */
	r->fename = rec(p, A, 30, 20);	/* 0.09 -- feature name */
	r->fetype = rec(p, A, 4, 50);	/* 0.00 -- feature type */
	r->fedirs = rec(p, A, 2, 54);	/* 0.00 -- feature direction, suffix */
	r->cfcc = rec(p, A, 3, 56);	/* 0.00 -- census feature class code */
	r->fraddl = rec(p, A, 11, 59);	/* 0.00 -- from address left */
	r->toaddl = rec(p, A, 11, 70);	/* 0.00 -- to address left */
	r->fraddr = rec(p, A, 11, 81);	/* 0.00 -- from address right */
	r->toaddr = rec(p, A, 11, 92);	/* 0.00 -- to address right */
	r->friaddfl = rec(p, N, 1, 103);/* 0.00 -- from imputed address flag left */
	r->toiaddfl = rec(p, N, 1, 104);/* 0.00 -- to imputed address flag left */
	r->friaddfr = rec(p, N, 1, 105);/* 0.00 -- from imputed address flag right */
	r->toiaddfr = rec(p, N, 1, 106);/* 0.00 -- to imputed address flag right */
	r->zipl = num(p, N, 5, 107);	/* 0.00 -- zip code left (only when an */
					/*	address range is present) */
	r->zipr = num(p, N, 5, 112);	/* 0.00 -- zip code right (only when an */
					/*	address range is present) */
	r->airl = rec(p, N, 5, 117);	/* 0.00 -- fips pub 55 code left */
					/*	(american indian reservation */
					/*	(air), alaska native village */
					/*	statistical area (anvsa), */
					/*	tribal jurisdiction */
					/*	statistical area (tjsa), */
					/*	tribal designated */
					/*	statistical area (tdsa))
	r->airr = rec(p, N, 5, 122);	/* 0.00 -- fips pub 55 code right */
					/*	(american indian reservation */
					/*	(air), alaska native village */
					/*	statistical area (anvsa), */
					/*	tribal jurisdiction */
					/*	statistical area (tjsa), */
					/*	tribal designated */
					/*	statistical area (tdsa)) */
	r->anrcl = rec(p, N, 2, 127);	/* 0.00 -- alaska native regional */
					/*	corporation code left */
	r->anrcr = rec(p, N, 2, 129);	/* 0.00 -- alaska native regional */
					/*	corporation code right */
	r->statel = num(p, N, 2, 131);	/* 0.00 -- fips state code left */
	r->stater = num(p, N, 2, 133);	/* 0.00 -- fips state code right */
	r->countyl = num(p, N, 3, 135);	/* 0.00 -- fips county code left */
	r->countyr = num(p, N, 3, 138);	/* 0.00 -- fips county code right */
	r->fmcdl = num(p, N, 5, 141);	/* 0.00 -- fips pub 55 code left (mcd/ccd) */
	r->fmcdr = num(p, N, 5, 146);	/* 0.00 -- fips pub 55 code right (mcd/ccd) */
	r->fsmcdl = num(p, N, 5, 151);	/* 0.00 -- fips pub 55 code left (sub-mcd) */
	r->fsmcdr = num(p, N, 5, 156);	/* 0.00 -- fips pub 55 code right (sub-mcd) */
	r->fpll = num(p, N, 5, 161);	/* 0.00 -- fips pub 55 code left (place) */
	r->fplr = num(p, N, 5, 166);	/* 0.00 -- fips pub 55 code right (place) */
	r->ctbnal = rec(p, N, 6, 171);	/* 0.00 -- census tract/bna (4+2) code left */
	r->ctbnar = rec(p, N, 6, 177);	/* 0.00 -- census tract/bna (4+2) code right */
	r->blkl = rec(p, A, 4, 183);	/* 0.04 -- tabulation block number (3+1) left */
	r->blkr = rec(p, A, 4, 187);	/* 0.04 -- tabulation block number (3+1) right */
	r->frlng = lat(p, N, 10, 191);	/* 0.60 -- longitude from (implied 6 decimal places) */
	r->frlat = lat(p, N, 9, 201);	/* 0.59 -- latitude from (implied 6 decimal places) */
	r->tolng = lat(p, N, 10, 210);	/* 0.68 -- longitude to (implied 6 decimal places) */
	r->tolat = lat(p, N, 9, 220);	/* 0.68 -- latitude to (implied 6 decimal places) */
}

void
decode2(char *p)
{
	int i;
	Rec2 *r;

	if(nrec2 >= mrec2) {
		mrec2 = mrec2*2;
		if(mrec2 < xrec2)
			mrec2 = xrec2;
		rec2 = realloc(rec2, mrec2*sizeof(Rec2));
	}
	r = rec2+nrec2;
	nrec2++;

	r->version = rec(p, N, 4, 2);	/* 0.00 -- version number */
	r->tlid = num(p, N, 10, 6);	/* 0.88 -- record number (permanent) */
	r->rtsq = num(p, N, 3, 16);	/* 0.00 -- record sequence number */
	for(i=0; i<10; i++) {
		r->lng[i] = lat(p, N, 10, 19+i*19);
					/* 0.98 -- longitude coordinate for shape point */
		r->lat[i] = lat(p, N, 9, 29+i*19);
					/* 0.97 -- latitude coordinate for shape point */
	}
}

void
decode3(char *p)
{
#ifdef REC3
	Rec3 *r;

	if(nrec3 >= mrec3) {
		mrec3 = mrec3*2;
		if(mrec3 < xrec3)
			mrec3 = xrec3;
		rec3 = realloc(rec3, mrec3*sizeof(Rec3));
	}
	r = rec3+nrec3;
	nrec3++;

	r->version = rec(p, N, 4, 2);	/* 0.00 -- version number */
	r->tlid = num(p, N, 10, 6);	/* 1.00 -- record number (permanent) */
	r->statel = num(p, N, 2, 16);	/* 0.00 -- 1980 fips state code left */
	r->stater = num(p, N, 2, 18);	/* 0.00 -- 1980 fips state code right */
	r->counl = num(p, N, 3, 20);	/* 0.00 -- 1980 fips county code left */
	r->counr = num(p, N, 3, 23);	/* 0.00 -- 1980 fips county code right */
	r->fmcdl = num(p, N, 5, 26);	/* 0.00 -- 1980 fips pub 55 code left (mcd/ccd) */
	r->fmcdr = num(p, N, 5, 31);	/* 0.00 -- 1980 fips pub 55 code right (mcd/ccd) */
	r->fpll = num(p, N, 5, 36);	/* 0.00 -- 1980 fips pub 55 code left (place) */
	r->fplr = num(p, N, 5, 41);	/* 0.00 -- 1980 fips pub 55 code right (place) */
	r->ctbnal = rec(p, N, 6, 46);	/* 0.00 -- 1980 census tract/bna code left 4+2 */
	r->ctbnar = rec(p, N, 6, 52);	/* 0.00 -- 1980 census tract/bna code right 4+2 */
	r->blkl = rec(p, N, 3, 58);	/* 0.00 -- 1980 block number left */
	r->blkr = rec(p, N, 3, 61);	/* 0.00 -- 1980 block number right */
	r->mcdl80 = rec(p, N, 3, 64);	/* 0.00 -- 1980 minor civil */
						/* division/census county */
						/* division census code left */
	r->mcdr80 = rec(p, N, 3, 67);	/* 0.00 -- 1980 minor civil */
						/* division/census county */
						/* division census code right */
	r->pll80 = rec(p, N, 4, 70);	/* 0.00 -- 1980 census place code left */
	r->plr80 = rec(p, N, 4, 74);	/* 0.00 -- 1980 census place code right */
	r->airl = rec(p, N, 4, 78);	/* 0.00 -- 1990 american indian */
						/* reservation/tjsa/tdsa/anvsa */
						/* census code left */
	r->airr = rec(p, N, 4, 82);	/* 0.00 -- 1990 american indian */
						/* reservation/tjsa/tdsa/anvsa */
						/* census code right */
	r->mcdl90 = rec(p, N, 3, 86);	/* 0.00 -- 1990 census mcd/ccd code left */
	r->mcdr90 = rec(p, N, 3, 89);	/* 0.00 -- 1990 census mcd/ccd code right */
	r->smcdl = rec(p, N, 2, 92);	/* 0.00 -- 1990 census sub-mcd code left */
	r->smcdr = rec(p, N, 2, 94);	/* 0.00 -- 1990 census sub-mcd code right */
	r->pll90 = rec(p, N, 4, 96);	/* 0.00 -- 1990 census place code left */
	r->plr90 = rec(p, N, 4, 100);	/* 0.00 -- 1990 census place code right */
	r->vtdl = rec(p, A, 4, 104);	/* 0.00 -- 1990 voting district code left */
	r->vtdr = rec(p, A, 4, 108);	/* 0.00 -- 1990 voting district code right */
#endif
	USED(p);
}

void
decode4(char *p)
{
	int i;
	Rec4 *r;

	if(nrec4 >= mrec4) {
		mrec4 = mrec4*2;
		if(mrec4 < xrec4)
			mrec4 = xrec4;
		rec4 = realloc(rec4, mrec4*sizeof(Rec4));
	}
	r = rec4+nrec4;
	nrec4++;

	r->version = rec(p, N, 4, 2);	/* 0.00 -- version number */
	r->tlid = num(p, N, 10, 6);	/* 1.00 -- record number (permanent) */
	r->rtsq = num(p, N, 3, 16);	/* 0.00 -- record sequence number */
	for(i=0; i<5; i++) {
		r->feat[i] = num(p, N, 8, 19+i*8);
					/* 0.09 -- alternate feature id code */
	}
}

void
decode5(char *p)
{
	Rec5 *r;

	if(nrec5 >= mrec5) {
		mrec5 = mrec5*2;
		if(mrec5 < xrec5)
			mrec5 = xrec5;
		rec5 = realloc(rec5, mrec5*sizeof(Rec5));
	}
	r = rec5+nrec5;
	nrec5++;

	r->state = num(p, N, 2, 2);	/* 0.00 -- fips state code for file */
	r->county = num(p, N, 3, 4);	/* 0.00 -- fips county code for file */
	r->feat = num(p, N, 8, 7);	/* 1.00 -- alternate feature id code */
	r->fedirp = rec(p, A, 2, 15);	/* 0.00 -- feature direction, prefix */
	r->fename = rec(p, A, 30, 17);	/* 0.89 -- feature name */
	r->fetype = rec(p, A, 4, 47);	/* 0.01 -- feature type */
	r->fedirs = rec(p, A, 2, 51);	/* 0.00 -- feature direction, suffix */
}

void
decode6(char *p)
{
	Rec6 *r;

	if(nrec6 >= mrec6) {
		mrec6 = mrec6*2;
		if(mrec6 < xrec6)
			mrec6 = xrec6;
		rec6 = realloc(rec6, mrec6*sizeof(Rec6));
	}
	r = rec6+nrec6;
	nrec6++;

	r->version = rec(p, N, 4, 2);	/* 0.00 -- version number */
	r->tlid = num(p, N, 10, 6);	/* 0.00 -- record number */
	r->rtsq = num(p, N, 3, 16);	/* 0.00 -- record sequence number */
	r->fraddl = rec(p, A, 11, 19);	/* 0.00 -- from address left */
	r->toaddl = rec(p, A, 11, 30);	/* 0.00 -- to address left */
	r->fraddr = rec(p, A, 11, 41);	/* 0.00 -- from address right */
	r->toaddr = rec(p, A, 11, 52);	/* 0.00 -- to address right */
	r->friaddfl = rec(p, N, 1, 63);	/* 0.00 -- from imputed address flag left */
	r->toiaddfl = rec(p, N, 1, 64);	/* 0.00 -- to imputed address flag left */
	r->friaddfr = rec(p, N, 1, 65);	/* 0.00 -- from imputed address flag right */
	r->toiaddfr = rec(p, N, 1, 66);	/* 0.00 -- to imputed address flag right */
	r->zipl = num(p, N, 5, 67);	/* 0.00 -- zip code left */
	r->zipr = num(p, N, 5, 72);	/* 0.00 -- zip code right */
}

void
decode7(char *p)
{
	Rec7 *r;

	if(nrec7 >= mrec7) {
		mrec7 = mrec7*2;
		if(mrec7 < xrec7)
			mrec7 = xrec7;
		rec7 = realloc(rec7, mrec7*sizeof(Rec7));
	}
	r = rec7+nrec7;
	nrec7++;

	r->version = rec(p, N, 4, 2);	/* 0.00 -- version number */
	r->state = num(p, N, 2, 6);	/* 0.00 -- fips state code for file */
	r->county = num(p, N, 3, 8);	/* 0.00 -- fips county code for file */
	r->land = num(p, N, 10, 11);	/* 1.00 -- landmark identification number */
	r->source = rec(p, A, 1, 21);	/* 0.00 -- source code */
	r->cfcc = rec(p, A, 3, 22);	/* 0.03 -- cfcc code */
	r->laname = rec(p, A, 30, 25);	/* 0.49 -- landmark feature name */
	r->lng = lat(p, N, 10, 55);	/* 0.33 -- longitude (point only) */
	r->lat = lat(p, N, 9, 65);	/* 0.33 -- latitude (point only) */
}

void
decode8(char *p)
{
	Rec8 *r;

	if(nrec8 >= mrec8) {
		mrec8 = mrec8*2;
		if(mrec8 < xrec8)
			mrec8 = xrec8;
		rec8 = realloc(rec8, mrec8*sizeof(Rec8));
	}
	r = rec8+nrec8;
	nrec8++;

	r->version = rec(p, N, 4, 2);	/* 0.00 -- version number */
	r->state = num(p, N, 2, 6);	/* 0.00 -- fips code for file */
	r->county = num(p, N, 3, 8);	/* 0.00 -- fips county code for file */
	r->cenid = num(p, N, 5, 11);	/* 0.00 -- census file identification code */
	r->polyid = num(p, N, 10, 16);	/* 1.00 -- polygon identification code */
	r->land = num(p, N, 10, 26);	/* 0.70 -- landmark identification number */
}

void
decodea(char *p)
{
	Reca *r;

	if(nreca >= mreca) {
		mreca = mreca*2;
		if(mreca < xreca)
			mreca = xreca;
		reca = realloc(reca, mreca*sizeof(Reca));
	}
	r = reca+nreca;
	nreca++;

	r->version = rec(p, N, 4, 2);	/* 0.00 -- version number */
	r->state = num(p, N, 2, 6);	/* 0.00 -- fips state code for file */
	r->county = num(p, N, 3, 8);	/* 0.00 -- fips county code for file */
	r->cenid = num(p, N, 5, 11);	/* 0.00 -- census file identification code */
	r->polyid = num(p, N, 10, 16);	/* 1.00 -- polygon identification code */
	r->fair = rec(p, N, 5, 26);	/* 0.00 -- fips pub 55 code (american indian reservation/
						/* tjsa/tdsa/anvsa) */
	r->fmcd = num(p, N, 5, 31);	/* 0.00 -- fips pub 55 code (mcd/ccd) */
	r->fpl = num(p, N, 5, 36);	/* 0.00 -- fips pub 55 code (place) */
	r->ctbna = rec(p, N, 6, 41);	/* 0.00 -- census tract/bna code */
	r->blk = rec(p, A, 4, 47);	/* 0.13 -- tabulation block number */
	r->cd101 = rec(p, N, 2, 51);	/* 0.00 -- 101st congressional district code */
	r->cd103 = rec(p, N, 2, 53);	/* 0.00 -- 103rd congressional district code
						/* (reserved for possible future use) */
	r->sdelm = rec(p, A, 5, 55);	/* 0.00 -- elementary school district
						/* (reserved for possible future use) */
	r->sdmid = rec(p, N, 5, 60);	/* 0.00 -- middle school district
						/* (reserved for possible future use) */
	r->sdsec = rec(p, N, 5, 65);	/* 0.00 -- secondary school district
						/* (reserved for possible future use) */
	r->sduni = rec(p, N, 5, 70);	/* 0.00 -- unified school district
						/* (reserved for possible future use) */
	r->taz = rec(p, A, 6, 75);	/* 0.00 -- traffic analysis zone
						/* (reserved for possible future use) */
	r->ua = rec(p, N, 4, 81);	/* 0.00 -- 1990 urbanized area code
						/* (reserved for possible future use) */
	r->urbflag = rec(p, N, 1, 85);	/* 0.00 -- urban flag
						/* (reserved for possible future use) */
	r->rs = rec(p, N, 13, 86);	/* 0.00 -- reserved space
						/* (reserved for possible future use) */
}

void
decodei(char *p)
{
	Reci *r;

	if(nreci >= mreci) {
		mreci = mreci*2;
		if(mreci < xreci)
			mreci = xreci;
		reci = realloc(reci, mreci*sizeof(Reci));
	}
	r = reci+nreci;
	nreci++;

	r->version = rec(p, N, 4, 2);	/* 0.00 -- version number */
	r->tlid = num(p, N, 10, 6);	/* 1.00 -- record number (permanent) */
	r->state = num(p, N, 2, 16);	/* 0.00 -- fips state code for file */
	r->county = num(p, N, 3, 18);	/* 0.00 -- fips county code for file */
	r->rtpoint = rec(p, A, 1, 21);	/* 0.00 -- area pointer type code
						/* (p=polygon identification code) */
	r->cenidl = num(p, N, 5, 22);	/* 0.00 -- left census file identification code */
	r->polyl = num(p, N, 10, 27);	/* 0.26 -- left polygon identification code (temporary) */
	r->cenidr = num(p, N, 5, 37);	/* 0.00 -- right census file identification code */
	r->polyr = num(p, N, 10, 42);	/* 0.26 -- right polygon identification code (temporary) */
}

void
decodep(char *p)
{
	Recp *r;

	if(nrecp >= mrecp) {
		mrecp = mrecp*2;
		if(mrecp < xrecp)
			mrecp = xrecp;
		recp = realloc(recp, mrecp*sizeof(Recp));
	}
	r = recp+nrecp;
	nrecp++;

	r->version = rec(p, N, 4, 2);	/* 0.00 -- version number */
	r->state = num(p, N, 2, 6);	/* 0.00 -- fips state code for file */
	r->county = num(p, N, 3, 8);	/* 0.00 -- fips county code for file */
	r->cenid = num(p, N, 5, 11);	/* 0.00 -- census file identification code */
	r->polyid = num(p, N, 10, 16);	/* 1.00 -- polygon identification code */
	r->lng = lat(p, N, 10, 26);	/* 0.99 -- longitude of polygon interior point
						/* (implied 6 decimal places) */
	r->lat = lat(p, N, 9, 36);	/* 0.99 -- latitude of polygon interior point
						/* (implied 6 decimal places) */
}

void
decoder(char *p)
{
	Recr *r;

	if(nrecr >= mrecr) {
		mrecr = mrecr*2;
		if(mrecr < xrecr)
			mrecr = xrecr;
		recr = realloc(recr, mrecr*sizeof(Recr));
	}
	r = recr+nrecr;
	nrecr++;

	r->version = rec(p, N, 4, 2);	/* 0.00 -- version number */
	r->state = num(p, N, 2, 6);	/* 0.00 -- fips state code for file */
	r->county = num(p, N, 3, 8);	/* 0.00 -- fips county code for file */
	r->cenid = rec(p, N, 5, 11);	/* 0.00 -- census file identification code */
	r->maxid = rec(p, N, 10, 16);	/* 0.00 -- maximum value for cenid */
	r->minid = rec(p, N, 10, 26);	/* 0.00 -- minimum value for cenid */
	r->highid = rec(p, N, 10, 36);	/* 0.00 -- highest used value for this */
						/* cenid in this file version */
}

void
decodex(char *p)
{
	Recx *x;
	int n, t;
	long l;

	if(nrecx >= mrecx) {
		mrecx = mrecx*2;
		if(mrecx < xrecx)
			mrecx = xrecx;
		recx = realloc(recx, mrecx*sizeof(Recx));
	}
	x = recx+nrecx;
	nrecx++;

	n = strchr(p, '\n') - p;
	t = p[1] - '0';
	x->type = t;

	switch(t) {
	default:
		fprint(2, "%s: unknown fips code\n", p);
		break;

	case 1:	/* state */
		l = num(p, N, 2, 3);
		laststate = l;
		x->state = 0;
		x->fips = l;
		x->name = rec(p, A, n-4, 5);
		break;

	case 2:	/* county */
		l = num(p, N, 3, 3);
		if(laststate == 0)
			fprint(2, "county with no state %ld\n", l);
		x->state = laststate;
		x->fips = l;
		x->name = rec(p, A, n-5, 6);
		break;

	case 3:	/* mcd/ccd */
	case 4:	/* sub-mcd/ccd */
	case 5:	/* place */
		l = num(p, N, 5, 3);
		if(laststate == 0)
			fprint(2, "city with no state %ld\n", l);
		x->state = laststate;
		x->fips = l;
		x->name = rec(p, A, n-7, 8);
		break;
	}
}

#include	"l.h"

uchar	ynone[] =
{
	Ynone,	Ynone,	Ynone,	Znone,	1,
	0
};
uchar	ytext[] =
{
	Ym,	Ynone,	Yi32,	Zpseudo,0,
	0
};
uchar	ypseudo[] =
{
	Ynone,	Ynone,	Ynone,	Zpseudo,0,
	Ynone,	Ynone,	Yr,	Zpseudo,0,
	0
};
uchar	ynop[] =
{
	Ynone,	Ynone,	Ynone,	Zpseudo,1,
	Ynone,	Ynone,	Yr,	Zpseudo,1,
	0
};
uchar	yadd[] =
{
	Yri5,	Ynri5,	Yr,	Zrrr,	0,
	Yi32,	Ynri5,	Yr,	Zirr,	1,
	0
};
uchar	ycmp[] =
{
	Yri5,	Ynone,	Yri5,	Zrrx,	0,
	Yi32,	Ynone,	Yr,	Zirx,	1,
	0
};
uchar	ymova[] =
{
	Ym,	Ynone,	Yr,	Zmr,	1,
	0
};
uchar	ymov[] =
{
	Yri5,	Ynone,	Yr,	Zrxr,	0,
	Yi32,	Ynone,	Yr,	Zir,	1,
	Ym,	Ynone,	Yr,	Zmr,	1,
	Yi32,	Ynone,	Ym,	Zim,	0,
	Yr,	Ynone,	Ym,	Zrm,	1,
	0
};
uchar	ybcond[] =
{
	Ynone,	Ynone,	Ybr,	Zbr,	1,
	0
};
uchar	ycall[] =
{
	Ynone,	Ynone,	Ybr,	Zbr,	1,
	Ynone,	Ynone,	Ym,	Zmbr,	1,
	0
};
uchar	ylong[] =
{
	Yi32,	Ynone,	Ynone,	Zlong,	1,
	0
};
uchar	ysysctl[] =
{
	Yr,	Yr,	Yr,	Zrrr,	1,
	0
};

Optab optab[] =
/*	as, ytab, andproto, opcode */
{
	{ AXXX },
	{ AADJSP },
	{ ABYTE },
	{ ADATA },
	{ AGLOBL },
	{ AGOK },
	{ AHISTORY },
	{ ALONG,	ylong },
	{ ANAME },
	{ ANOP,		ypseudo },
	{ ARTS },
	{ ATEXT,	ytext },
	{ ASYSCALL },
	{ ASYSCTL,	ysysctl, 0x659 },
	{ AMOV,		ymov, 0x5cc, 0x90, 0x92 },
	{ AMOVA,	ymova, 0x8c },
	{ AMOVIB,	ymov, 0x5cc, 0xc0, 0x82 },
	{ AMOVIS,	ymov, 0x5cc, 0xc8, 0x8a },
	{ AMOVOB,	ymov, 0x5cc, 0x80, 0x82 },
	{ AMOVOS,	ymov, 0x5cc, 0x88, 0x8a },
	{ AMOVQ,	ymov, 0x5fc, 0xb0, 0xb2 },
	{ AMOVT,	ymov, 0x5ec, 0xa0, 0xa2 },
	{ AMOVV },
	{ ASYNMOV },
	{ ASYNMOVV },
	{ ASYNMOVQ },
	{ AADDC,	yadd, 0x5b0 },
	{ AADDI,	yadd, 0x591 },
	{ AADDO,	yadd, 0x590 },
	{ AALTERBIT },
	{ AAND,		yadd, 0x581 },
	{ AANDNOT,	yadd, 0x582 },
	{ AATADD },
	{ AATMOD },
	{ ABAL,		ycall, 0x0b, 0x85 },
	{ AB,		ycall, 0x08, 0x84 },
	{ ABBC },
	{ ABBS },
	{ ABE,		ybcond, 0x12 },
	{ ABNE,		ybcond, 0x15 },
	{ ABL,		ybcond, 0x14 },
	{ ABLE,		ybcond, 0x16 },
	{ ABG,		ybcond, 0x11 },
	{ ABGE,		ybcond, 0x13 },
	{ ABO,		ybcond, 0x17 },
	{ ABNO,		ybcond, 0x10 },
	{ ACALL },
	{ ACALLS },
	{ ACHKBIT },
	{ ACLRBIT },
	{ ACMPI,	ycmp, 0x5a1 },
	{ ACMPO,	ycmp, 0x5a0 },
	{ ACMPDECI },
	{ ACMPDECO },
	{ ACMPINCI },
	{ ACMPINCO },
	{ ACMPIBE },
	{ ACMPIBNE },
	{ ACMPIBL },
	{ ACMPIBLE },
	{ ACMPIBG },
	{ ACMPIBGE },
	{ ACMPIBO },
	{ ACMPIBNO },
	{ ACMPOBE },
	{ ACMPOBNE },
	{ ACMPOBL },
	{ ACMPOBLE },
	{ ACMPOBG },
	{ ACMPOBGE },
	{ ACONCMPI },
	{ ACONCMPO },
	{ ADADDC },
	{ ADIVI,	yadd, 0x74b },
	{ ADIVO,	yadd, 0x70b },
	{ ADMOVT },
	{ ADSUBC },
	{ AEDIV },
	{ AEMUL },
	{ AEXTRACT },
	{ AFAULTE,	ynone, 0x1a0 },
	{ AFAULTNE,	ynone, 0x1d0 },
	{ AFAULTL,	ynone, 0x1c0 },
	{ AFAULTLE,	ynone, 0x1e0 },
	{ AFAULTG,	ynone, 0x190 },
	{ AFAULTGE,	ynone, 0x1b0 },
	{ AFAULTO,	ynone, 0x1f0 },
	{ AFAULTNO,	ynone, 0x180 },
	{ AFLUSHREG },
	{ AFMARK,	ynone, 0x66c },
	{ AMARK },
	{ AMODAC },
	{ AMODI },
	{ AMODIFY },
	{ AMODPC,	ysysctl, 0x655 },
	{ AMODTC },
	{ AMULI,	yadd, 0x741 },
	{ AMULO,	yadd, 0x701 },
	{ ANAND,	yadd, 0x58e },
	{ ANOR,		yadd, 0x588 },
	{ ANOT },
	{ ANOTAND,	yadd, 0x584 },
	{ ANOTBIT },
	{ ANOTOR,	yadd, 0x58d },
	{ AOR,		yadd, 0x587 },
	{ AORNOT,	yadd, 0x58b },
	{ AREMI,	yadd, 0x748 },
	{ AREMO,	yadd, 0x708 },
	{ ARET },
	{ AROTATE,	yadd, 0x59d },
	{ ASCANBIT },
	{ ASCANBYTE },
	{ ASETBIT },
	{ ASHLO,	yadd, 0x59c },
	{ ASHRO,	yadd, 0x598 },
	{ ASHLI,	yadd, 0x59e },
	{ ASHRI,	yadd, 0x59b },
	{ ASHRDI,	yadd, 0x59a },
	{ ASPANBIT },
	{ ASUBC,	yadd, 0x5b2 },
	{ ASUBI,	yadd, 0x593 },
	{ ASUBO,	yadd, 0x592 },
	{ ASYNCF },
	{ ATESTE },
	{ ATESTNE },
	{ ATESTL },
	{ ATESTLE },
	{ ATESTG },
	{ ATESTGE },
	{ ATESTO },
	{ ATESTNO },
	{ AXNOR,	yadd, 0x589 },
	{ AXOR,		yadd, 0x586 },
	{ AEND },
	0,
};

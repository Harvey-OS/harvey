#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

typedef struct Optab Optab;
struct Optab {
	uchar op;
	uchar a1, a2, a3;
};

static Optab optab[256] = {
//00
  {OADD,  AEb, AGb}, {OADD,  AEv, AGv}, {OADD,  AGb, AEb}, {OADD,  AGv, AEv},
  {OADD,  AAL, AIb}, {OADD,  AAX, AIv}, {OPUSH, AES,    }, {OPOP,  AES     },
  {OOR,   AEb, AGb}, {OOR,   AEv, AGv}, {OOR,   AGb, AEb}, {OOR,   AGv, AEv},
  {OOR,   AAL, AIb}, {OOR,   AAX, AIv}, {OPUSH, ACS,    }, {O0F,           },
//10
  {OADC,  AEb, AGb}, {OADC,  AEv, AGv}, {OADC,  AGb, AEb}, {OADC,  AGv, AEv},
  {OADC,  AAL, AIb}, {OADC,  AAX, AIv}, {OPUSH, ASS,    }, {OPOP,  ASS,    },
  {OSBB,  AEb, AGb}, {OSBB,  AEv, AGv}, {OSBB,  AGb, AEb}, {OSBB,  AGv, AEv},
  {OSBB,  AAL, AIb}, {OSBB,  AAX, AIv}, {OPUSH, ADS,    }, {OPOP,  ADS,    },
//20
  {OAND,  AEb, AGb}, {OAND,  AEv, AGv}, {OAND,  AGb, AEb}, {OAND,  AGv, AEv},
  {OAND,  AAL, AIb}, {OAND,  AAX, AIv}, {OSEG,  AES,    }, {ODAA,          },
  {OSUB,  AEb, AGb}, {OSUB,  AEv, AGv}, {OSUB,  AGb, AEb}, {OSUB,  AGv, AEv},
  {OSUB,  AAL, AIb}, {OSUB,  AAX, AIv}, {OSEG,  ACS,    }, {ODAS,          },
//30
  {OXOR,  AEb, AGb}, {OXOR,  AEv, AGv}, {OXOR,  AGb, AEb}, {OXOR,  AGv, AEv},
  {OXOR,  AAL, AIb}, {OXOR,  AAX, AIv}, {OSEG,  ASS,    }, {OAAA,          },
  {OCMP,  AEb, AGb}, {OCMP,  AEv, AGv}, {OCMP,  AGb, AEb}, {OCMP,  AGv, AEv},
  {OCMP,  AAL, AIb}, {OCMP,  AAX, AIv}, {OSEG,  ADS,    }, {OAAS,          },
//40
  {OINC,  AAX,    }, {OINC,  ACX,    }, {OINC,  ADX,    }, {OINC,  ABX,    },
  {OINC,  ASP,    }, {OINC,  ABP,    }, {OINC,  ASI,    }, {OINC,  ADI,    },
  {ODEC,  AAX,    }, {ODEC,  ACX,    }, {ODEC,  ADX,    }, {ODEC,  ABX,    },
  {ODEC,  ASP,    }, {ODEC,  ABP,    }, {ODEC,  ASI,    }, {ODEC,  ADI,    },
//50
  {OPUSH, AAX,    }, {OPUSH, ACX,    }, {OPUSH, ADX,    }, {OPUSH, ABX,    },
  {OPUSH, ASP,    }, {OPUSH, ABP,    }, {OPUSH, ASI,    }, {OPUSH, ADI,    },
  {OPOP,  AAX,    }, {OPOP,  ACX,    }, {OPOP,  ADX,    }, {OPOP,  ABX,    },
  {OPOP,  ASP,    }, {OPOP,  ABP,    }, {OPOP,  ASI,    }, {OPOP,  ADI,    },
//60
  {OPUSHA,        }, {OPOPA,         }, {OBOUND,AGv,AMa,AMa2}, {OARPL, AEw, AGw},
  {OSEG,  AFS,    }, {OSEG,  AGS,    }, {OOSIZE,        }, {OASIZE,        },
  {OPUSH, AIv,    }, {OIMUL,AGv,AEv,AIv},{OPUSH, AIb,   }, {OIMUL,AGv,AEv,AIb},
  {OINS,  AYb, ADX}, {OINS,  AYv, ADX}, {OOUTS, ADX, AXb}, {OOUTS, ADX, AXv},
//70
  {OJUMP, AJb,    }, {OJUMP, AJb,    }, {OJUMP, AJb,    }, {OJUMP, AJb,    },
  {OJUMP, AJb,    }, {OJUMP, AJb,    }, {OJUMP, AJb,    }, {OJUMP, AJb,    },
  {OJUMP, AJb,    }, {OJUMP, AJb,    }, {OJUMP, AJb,    }, {OJUMP, AJb,    },
  {OJUMP, AJb,    }, {OJUMP, AJb,    }, {OJUMP, AJb,    }, {OJUMP, AJb,    },
//80
  {OGP1,  AEb, AIb}, {OGP1,  AEv, AIv}, {OGP1,  AEb, AIb}, {OGP1,  AEv, AIc},
  {OTEST, AEb, AGb}, {OTEST, AEv, AGv}, {OXCHG, AEb, AGb}, {OXCHG, AEv, AGv},
  {OMOV,  AEb, AGb}, {OMOV,  AEv, AGv}, {OMOV,  AGb, AEb}, {OMOV,  AGv, AEv},
  {OMOV,  AEw, ASw}, {OLEA,  AGv, AM }, {OMOV,  ASw, AEw}, {OGP10,    },
//90
  {ONOP,          }, {OXCHG, ACX, AAX}, {OXCHG, ADX, AAX}, {OXCHG, ABX, AAX},
  {OXCHG, ASP, AAX}, {OXCHG, ABP, AAX}, {OXCHG, ASI, AAX}, {OXCHG, ADI, AAX},
  {OCBW,          }, {OCWD,          }, {OCALL, AAp,    }, {OWAIT,         },
  {OPUSHF,AFv,    }, {OPOPF, AFv,    }, {OSAHF, AAH,    }, {OLAHF, AAH,    },
//A0
  {OMOV,  AAL, AOb}, {OMOV,  AAX, AOv}, {OMOV,  AOb, AAL}, {OMOV,  AOv, AAX},
  {OMOVS, AYb, AXb}, {OMOVS, AYv, AXv}, {OCMPS, AYb, AXb}, {OCMPS, AYv, AXv},
  {OTEST, AAL, AIb}, {OTEST, AAX, AIv}, {OSTOS, AYb, AAL}, {OSTOS, AYv, AAX},
  {OLODS, AAL, AXb}, {OLODS, AAX, AXv}, {OSCAS, AYb, AAL}, {OSCAS, AYv, AAX},
//B0
  {OMOV,  AAL, AIb}, {OMOV,  ACL, AIb}, {OMOV,  ADL, AIb}, {OMOV,  ABL, AIb},
  {OMOV,  AAH, AIb}, {OMOV,  ACH, AIb}, {OMOV,  ADH, AIb}, {OMOV,  ABH, AIb},
  {OMOV,  AAX, AIv}, {OMOV,  ACX, AIv}, {OMOV,  ADX, AIv}, {OMOV,  ABX, AIv},
  {OMOV,  ASP, AIv}, {OMOV,  ABP, AIv}, {OMOV,  ASI, AIv}, {OMOV,  ADI, AIv},
//C0
  {OGP2,  AEb, AIb}, {OGP2,  AEv, AIb}, {ORET,  AIw,    }, {ORET,  A0,     },
  {OLFP,AES,AGv,AMp},{OLFP,ADS,AGv,AMp},{OGP12, AEb, AIb}, {OGP12, AEv, AIv},
  {OENTER,AIw, AIb}, {OLEAVE,        }, {ORETF, AIw,    }, {ORETF, A0,     },
  {OINT,  A3,     }, {OINT,  AIb,    }, {OINT,  A4,     }, {OIRET,         },
//D0
  {OGP2,  AEb, A1 }, {OGP2,  AEv, A1 }, {OGP2,  AEb, ACL}, {OGP2,  AEv, ACL},
  {OAAM,  AIb,    }, {OAAD,  AIb,    }, {OBAD,          }, {OXLAT, AAL, ABX},
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
//E0
  {OLOOPNZ,AJb,   }, {OLOOPZ,AJb,    }, {OLOOP, AJb,    }, {OJUMP, AJb,    },
  {OIN,   AAL, AIb}, {OIN,   AAX, AIb}, {OOUT,  AIb, AAL}, {OOUT,  AIb, AAX},
  {OCALL, AJv,    }, {OJUMP, AJv,    }, {OJUMP, AAp,    }, {OJUMP, AJb,    },
  {OIN,   AAL, ADX}, {OIN,   AAX, ADX}, {OOUT,  ADX, AAL}, {OOUT,  ADX, AAX},
//F0
  {OLOCK,         }, {OBAD,          }, {OREPNE,        }, {OREPE,         },
  {OHLT,          }, {OCMC,          }, {OGP3b,         }, {OGP3v,         },
  {OCLC,          }, {OSTC,          }, {OCLI,          }, {OSTI,          },
  {OCLD,          }, {OSTD,          }, {OGP4,          }, {OGP5,          },
};

static Optab optab0F[256] = {
//00
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
//10 - mostly floating point and quadword moves
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
//20 - doubleword <-> control register moves, other arcana
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
//30 - wrmsr, rdtsc, rdmsr, rdpmc, sysenter, sysexit
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
//40 - conditional moves
  {OCMOV, AGv, AEv}, {OCMOV, AGv, AEv}, {OCMOV, AGv, AEv}, {OCMOV, AGv, AEv},
  {OCMOV, AGv, AEv}, {OCMOV, AGv, AEv}, {OCMOV, AGv, AEv}, {OCMOV, AGv, AEv},
  {OCMOV, AGv, AEv}, {OCMOV, AGv, AEv}, {OCMOV, AGv, AEv}, {OCMOV, AGv, AEv},
  {OCMOV, AGv, AEv}, {OCMOV, AGv, AEv}, {OCMOV, AGv, AEv}, {OCMOV, AGv, AEv},
//50 - floating point, mmx stuff
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
//60 - floating point, mmx stuff
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
//70 - floating point, mmx stuff
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
//80 - long-displacement jumps
  {OJUMP, AJv,    }, {OJUMP, AJv,    }, {OJUMP, AJv,    }, {OJUMP, AJv,    },
  {OJUMP, AJv,    }, {OJUMP, AJv,    }, {OJUMP, AJv,    }, {OJUMP, AJv,    },
  {OJUMP, AJv,    }, {OJUMP, AJv,    }, {OJUMP, AJv,    }, {OJUMP, AJv,    },
  {OJUMP, AJv,    }, {OJUMP, AJv,    }, {OJUMP, AJv,    }, {OJUMP, AJv,    },
//90 - conditional byte set
  {OSET,  AEb,    }, {OSET,  AEb,    }, {OSET,  AEb,    }, {OSET,  AEb,    },
  {OSET,  AEb,    }, {OSET,  AEb,    }, {OSET,  AEb,    }, {OSET,  AEb,    },
  {OSET,  AEb,    }, {OSET,  AEb,    }, {OSET,  AEb,    }, {OSET,  AEb,    },
  {OSET,  AEb,    }, {OSET,  AEb,    }, {OSET,  AEb,    }, {OSET,  AEb,    },
//A0
  {OPUSH, AFS,    }, {OPOP,  AFS,    }, {OCPUID,        }, {OBT,   AEv, AGv},
  {OSHLD,AEv,AGv,AIb}, {OSHLD,AEv,AGv,ACL}, {OBAD,      }, {OBAD,          },
  {OPUSH, AGS,    }, {OPOP,  AGS,    }, {OBAD,          }, {OBTS,  AEv, AGv},
  {OSHRD,AEv,AGv,AIb}, {OSHRD,AEv,AGv,ACL}, {OBAD,      }, {OIMUL, AGv,AGv,AEv},
//B0 - mostly arcana
  {OBAD,          }, {OBAD,          }, {OLFP,ASS,AGv,AMp},{OBTR,AEv,AGv   },
  {OLFP,AFS,AGv,AMp},{OLFP,AGS,AGv,AMp},{OMOVZX,AGv,AEb }, {OMOVZX,AGv,AEw },
  {OBAD,          }, {OBAD,          }, {OGP8,          }, {OBAD,          },
  {OBSF,AGv,AEv   }, {OBSR,AGv,AEv   }, {OMOVSX,AGv,AEb }, {OMOVSX,AGv,AEw },
//C0 - more arcana
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
//D0 - mmx
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
//E0 - mmx
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
//F0 - mmx
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
};

/* some operands map to whole groups; group numbers from intel opcode map */
/* args filled in already (in OGP1 entries) */
static Optab optabgp1[8] = {
  {OADD,          }, {OOR,           }, {OADC,          }, {OSBB,          },
  {OAND,          }, {OSUB,          }, {OXOR,          }, {OCMP,          },
};

/* args filled in already (in OGP2 entries) */
static Optab optabgp2[8] = {
  {OROL,          }, {OROR,          }, {ORCL,          }, {ORCR,          },
  {OSHL,          }, {OSHR,          }, {OBAD,          }, {OSAR,          },
};

static Optab optabgp3b[8] = {
  {OTEST, AEb, AIb}, {OBAD,          }, {ONOT,  AEb,    }, {ONEG,  AEb,    },
  {OMUL,AAX,AAL,AEb},{OIMUL,AAX,AAL,AEb},{ODIV, AEb,    }, {OIDIV, AEb,    },
};

static Optab optabgp3v[8] = {
  {OTEST, AEv, AIv}, {OBAD,          }, {ONOT,  AEv,    }, {ONEG,  AEv,    },
  {OMUL,AAX,AAX,AEv},{OIMUL,AAX,AAX,AEv},{ODIV, AEv,    }, {OIDIV, AEv,    },
};

static Optab optabgp4[8] = {
  {OINC,  AEb,    }, {ODEC,  AEb,    }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
};

static Optab optabgp5[8] = {
  {OINC,  AEv,    }, {ODEC,  AEv,    }, {OCALL,  AEv,   }, {OCALL,  AMp    },
  {OJUMP, AEv,    }, {OJUMP, AMp,    }, {OPUSH,  AEv,   }, {OBAD,          },
};

static Optab optabgp8[8] = {
  {OMOV,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBT, AEv, AIb  }, {OBTS, AEv, AIb }, {OBTR, AEv, AIb }, {OBTC, AEv, AIb },
};

static Optab optabgp10[8] = {
  {OPOP, AEv,     }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
};

static Optab optabgp12[8] = {
  {OMOV,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
  {OBAD,          }, {OBAD,          }, {OBAD,          }, {OBAD,          },
};

/* optabg6  unimplemented - mostly segment manipulation */
/* optabg7 unimplemented - more segment manipulation */
/* optabg8 unimplemented - bit tests */

/*
 * most of optabg9 - optabg16 decode differently depending on the mod value of
 * the modrm byte.  they're mostly arcane instructions so they're not
 * implemented.
 */

static Optab *optabgp[NUMOP] = {
	[OGP1]	optabgp1,
	[OGP2]	optabgp2,
	[OGP3b]	optabgp3b,
	[OGP3v]	optabgp3v,
	[OGP4]	optabgp4,
	[OGP5]	optabgp5,
	[OGP8]	optabgp8,
	[OGP10]	optabgp10,
	[OGP12] optabgp12,
};

static uchar modrmarg[NATYPE] = {
	[AEb] 1,
	[AEw] 1,
	[AEv] 1,
	[AGb] 1,
	[AGw] 1,
	[AGv] 1,
	[AM]  1,
	[AMp] 1,
	[AMa] 1,
	[AMa2] 1,
	[ASw] 1,
	[AJr] 1,
};

static void
getmodrm16(Iarg *ip, Inst *i)
{
	Iarg *p;
	uchar b;

	b = ar(ip); ip->off++;

	i->mod = b>>6;
	i->reg = (b>>3)&7;
	i->rm = b&7;

	if(i->mod == 3)
		return;

	switch(i->rm){
	case 0:
		i->off = ar(areg(ip->cpu, 2, RBX)) + ar(areg(ip->cpu, 2, RSI));
		i->off &= 0xFFFF;
		break;
	case 1:
		i->off = ar(areg(ip->cpu, 2, RBX)) + ar(areg(ip->cpu, 2, RDI));
		i->off &= 0xFFFF;
		break;
	case 2:
		i->dsreg = RSS;
		i->off = ar(areg(ip->cpu, 2, RBP)) + ar(areg(ip->cpu, 2, RSI));
		i->off &= 0xFFFF;
		break;
	case 3:
		i->dsreg = RSS;
		i->off = ar(areg(ip->cpu, 2, RBP)) + ar(areg(ip->cpu, 2, RDI));
		i->off &= 0xFFFF;
		break;
	case 4:
		i->off = ar(areg(ip->cpu, 2, RSI));
		break;
	case 5:
		i->off = ar(areg(ip->cpu, 2, RDI));
		break;
	case 6:
		if(i->mod == 0){
			p = adup(ip); ip->off += 2;
			p->len = 2;
			i->off = ar(p);
			return;
		}
		i->dsreg = RSS;
		i->off = ar(areg(ip->cpu, 2, RBP));
		break;
	case 7:
		i->off = ar(areg(ip->cpu, 2, RBX));
		break;
	}
	switch(i->mod){
	case 1:
		i->off += (i->disp = ars(ip)); ip->off++;
		i->off &= 0xFFFF;
		break;
	case 2:
		p = adup(ip); ip->off += 2;
		p->len = 2;
		i->off += (i->disp = ars(p));
		i->off &= 0xFFFF;
		break;
	}
}

static void
getmodrm32(Iarg *ip, Inst *i)
{
	static uchar scaler[] = {1, 2, 4, 8};
	Iarg *p;
	uchar b;

	b = ar(ip); ip->off++;

	i->mod = b>>6;
	i->reg = (b>>3)&7;
	i->rm = b&7;

	if(i->mod == 3)
		return;

	switch(i->rm){
	case 0:
	case 1:
	case 2:
	case 3:
	case 6:
	case 7:
		i->off = ar(areg(ip->cpu, 4, i->rm + RAX));
		break;
	case 4:
		b = ar(ip); ip->off++;
		i->scale = b>>6;
		i->index = (b>>3)&7;
		i->base = b&7;

		if(i->base != 5){
			i->off = ar(areg(ip->cpu, 4, i->base + RAX));
			break;
		}
	case 5:
		if(i->mod == 0){
			p = adup(ip); ip->off += 4;
			p->len = 4;
			i->off = ar(p);
		} else {
			i->dsreg = RSS;
			i->off = ar(areg(ip->cpu, 4, RBP));
		}
		break;
	}

	if(i->rm == 4 && i->index != 4)
		i->off += ar(areg(ip->cpu, 4, i->index + RAX)) * scaler[i->scale];

	switch(i->mod){
	case 1:
		i->off += (i->disp = ars(ip)); ip->off++;
		break;
	case 2:
		p = adup(ip); ip->off += 4;
		p->len = 4;
		i->off += (i->disp = ars(p));
		break;
	}
}

static Iarg*
getarg(Iarg *ip, Inst *i, uchar atype)
{
	Iarg *a;
	uchar len, reg;

	len = i->olen;
	switch(atype){
	default:
		abort();

	case A0:
	case A1:
	case A2:
	case A3:
	case A4:
		a = acon(ip->cpu, len, atype - A0);
		break;

	case AEb:
		len = 1;
		if(0){
	case AEw:
		len = 2;
		}
	case AEv:
		if(i->mod == 3){
			reg = i->rm;
			goto REG;
		}
		goto MEM;

	case AM:
	case AMp:
	case AMa:
	case AMa2:
		if(i->mod == 3)
			trap(ip->cpu, EBADOP);
	MEM:
		a = amem(ip->cpu, len, i->sreg, i->off);
		if(atype == AMa2)
			a->off += i->olen;
		break;

	case AGb:
		len = 1;
		if(0){
	case AGw:
		len = 2;
		}
	case AGv:
		reg = i->reg;
	REG:
		a = areg(ip->cpu, len, reg + RAX);
		if(len == 1 && reg >= 4){
			a->reg -= 4;
			a->tag |= TH;
		}
		break;

	case AIb:
	case AIc:
		len = 1;
		if(0){
	case AIw:
		len = 2;
		}
	case AIv:
		a = adup(ip); ip->off += len;
		a->len = len;
		break;

	case AJb:
		len = 1;
	case AJv:
		a = adup(ip); ip->off += len;
		a->len = len;
		a->off = ip->off + ars(a);
		break;

	case AJr:
		if(i->mod != 3)
			trap(ip->cpu, EBADOP);
		a = adup(ip);
		a->off = ar(areg(ip->cpu, i->olen, i->rm + RAX));
		break;

	case AAp:
		a = afar(ip, ip->len, len); ip->off += 2+len;
		break;

	case AOb:
		len = 1;
	case AOv:
		a = adup(ip); ip->off += i->alen;
		a->len = i->alen;
		a = amem(ip->cpu, len, i->sreg, ar(a));
		break;

	case ASw:
		reg = i->reg;
	SREG:
		a = areg(ip->cpu, 2, reg + RES);
		break;

	case AXb:
		len = 1;
	case AXv:
		a = amem(ip->cpu, len, i->sreg, ar(areg(ip->cpu, i->alen, RSI)));
		break;

	case AYb:
		len = 1;
	case AYv:
		a = amem(ip->cpu, len, RES, ar(areg(ip->cpu, i->alen, RDI)));
		break;

	case AFv:
		a = areg(ip->cpu, len, RFL);
		break;

	case AAL:
	case ACL:
	case ADL:
	case ABL:
	case AAH:
	case ACH:
	case ADH:
	case ABH:
		len = 1;
		reg = atype - AAL;
		goto REG;

	case AAX:
	case ACX:
	case ADX:
	case ABX:
	case ASP:
	case ABP:
	case ASI:
	case ADI:
		reg = atype - AAX;
		goto REG;

	case AES:
	case ACS:
	case ASS:
	case ADS:
	case AFS:
	case AGS:
		reg = atype - AES;
		goto SREG;
	}
	a->atype = atype;
	return a;
}

static int
otherlen(int a)
{
	if(a == 2)
		return 4;
	else if(a == 4)
		return 2;
	abort();
	return 0;
}

void
decode(Iarg *ip, Inst *i)
{
	Optab *t, *t2;
	Cpu *cpu;

	cpu = ip->cpu;

	i->op = 0;
	i->rep = 0;
	i->sreg = 0;
	i->dsreg = RDS;
	i->olen = cpu->olen;
	i->alen = cpu->alen;
	i->a1 = i->a2 = i->a3 = nil;

	for(;;){
		i->code = ar(ip); ip->off++;
		t = optab + i->code;
		switch(t->op){
		case OOSIZE:
			i->olen = otherlen(cpu->olen);
			continue;
		case OASIZE:
			i->alen = otherlen(cpu->alen);
			continue;
		case OREPE:
		case OREPNE:
			i->rep = t->op;
			continue;
		case OLOCK:
			continue;
		case OSEG:
			i->sreg = t->a1-AES+RES;
			continue;
		case O0F:
			i->code = ar(ip); ip->off++;
			t = optab0F + i->code;
			break;
		}
		break;
	}
	t2 = optabgp[t->op];
	if(t2 || modrmarg[t->a1] || modrmarg[t->a2] || modrmarg[t->a3])
		if(i->alen == 2)
			getmodrm16(ip, i);
		else
			getmodrm32(ip, i);
	if(i->sreg == 0)
		i->sreg = i->dsreg;

	for(;;){
		if(t->a1)
			i->a1 = getarg(ip, i, t->a1);
		if(t->a2)
			i->a2 = getarg(ip, i, t->a2);
		if(t->a3)
			i->a3 = getarg(ip, i, t->a3);
		if(t2 == nil)
			break;
		t = t2 + i->reg;
		t2 = nil;
	}
	i->op = t->op;
}

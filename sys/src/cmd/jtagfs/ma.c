#include <u.h>
#include <libc.h>
#include <ctype.h> /* BUG: yes I know... ascii*/
#include <bio.h>
#include "debug.h"
#include "tap.h"
#include "jtag.h"
#include "icert.h"	/* BUG: separate medium and ice */
#include "mpsse.h"

/*
 *	Mpsse assembler
 *	Based on AN2232C-01 Command Processor for MPSSE and MCU Host Bus
 *	and various magical commands from source code all around.
 */


enum{
	/* MPSSE  */

	/*
	 *	This are used only to make opcodes
	 *	for Clocking in/out data to TD0 to TDI
	 */
	OpModOut = 0x10,
	OpTmsCs = 0x4a,			/* clock data to TmsCs */

	/* Modifiers for both */
	OpModShort = 0x02,		/* len only one byte */
	OpModOutFalling = 0x01,
	OpModInFalling = 0x04,
	OpModLSB = 0x08,
	OpModIn = 0x20,

	OpModBitsH = 0x02,
	OpModBitsIn = 0x01,
	OpSetBits = 0x80,			/* value(?)/direction on pins*/

	OpLoop = 0x84,			/* loop connect TDI/DO with TDO/DI */
	OpBreakLoop = 0x85,		/* break loop */
	OpTckSkDiv = 0x86,			/* valL valH,  ?? description of reset */

	OpDiv5ClkEnab = 0x8b,
	OpDiv5ClkDisab = 0x8a,
	

	/* MCU host emulation */
	OpModLong = 0x01,			/* addrH addrL */

	OpMCURd = 0x90,			/* addr */
	OpMCUWr = 0x92,			/* addr Data */

	OpBad = 0,				/* made this up */

	/* Both modes */
	OpSendImm = 0x87,		/* flush buffer back to PC  ??? */
	OpWaitIOHigh = 0x88,		/* IO high, GPIOH in MPSSE, I/O1 on MCU */
	OpWaitIOLow = 0x89,		/* IO low, GPIOH in MPSSE, I/O1 on MCU */

	OpAdaptClkEnab = 0x96,
	OpAdaptClkDisab = 0x97,
	RBad = 0xFA,				/* followed by the bad command */

	MaxDataSz = 0xffff+1,
};

typedef struct Inst Inst;
struct Inst{
	int hasresp;
	int isshort;
	int islsb;	/* bits come/go LSB */
	int nedge;
	uchar edge[2];
	uchar opcode;
	int lensz;
	uchar len[2];
	int immsz;
	uchar imm[3];
	int datasz;
	uchar data[1];
};

enum{
	DataIn,
	DataOut,
	DataOutIn,
	TmsCsOut,
	TmsCsOutIn,
	SetBitsL,
	SetBitsH,
	TckSkDiv,
	MCURd,	/* from this down, no param */
	MCUWr,
	SendImm,
	WaitIOHigh,
	WaitIOLow,
	AdaptClkEnab,
	AdaptClkDisab,
	Div5ClkEnab,
	Div5ClkDisab,
	GetBitsL,
	GetBitsH,
	Loop,
	BreakLoop,
	ILast,
};

static char *inst[] = {
	[DataIn]			"DataIn",
	[DataOut]			"DataOut",
	[DataOutIn]		"DataOutIn",
	[TmsCsOut]		"TmsCsOut",
	[TmsCsOutIn]		"TmsCsOutIn",
	[SetBitsL]			"SetBitsL",
	[SetBitsH]			"SetBitsH",
	[TckSkDiv]		"TckSkDiv",
	[MCURd]			"MCURd",
	[MCUWr]			"MCUWr",
	[SendImm]		"SendImm",
	[WaitIOHigh]		"WaitIOHigh",
	[WaitIOLow]		"WaitIOLow",
	[AdaptClkEnab]		"AdaptClkEnab",
	[AdaptClkDisab]	"AdaptClkDisab",
	[Div5ClkEnab]		"Div5ClkEnab",
	[Div5ClkDisab]		"Div5ClkDisab",
	[GetBitsL]			"GetBitsL",
	[GetBitsH]			"GetBitsH",
	[Loop]			"Loop",
	[BreakLoop]		"BreakLoop",
	[ILast]			nil,
	
};


enum{
	EdgeNone,
	EdgeUp,
	EdgeDown,
	EdgeBad,
};

static uchar
dataopcode(Inst *inst, int instid, int issh, int islsb)
{
	uchar opcode = 0;

	switch(instid){
	case DataOutIn:
		if(inst->edge[1] == EdgeDown)
				opcode = OpModInFalling;
		/* fall through */
	case DataOut:
		opcode |= OpModOut;
		if (inst->edge[0] == EdgeDown)
			opcode |= OpModOutFalling;
		break;
	case DataIn:
		if (inst->edge[0] == EdgeDown)
			opcode |= OpModInFalling;
		break;
	}
	
	opcode |= inst->hasresp? OpModIn : 0;
	opcode |= issh? OpModShort : 0;
	opcode |= islsb? OpModLSB : 0;

	return opcode;
}


static uchar
opcode(Inst *inst, int instid)
{
	int issh, islsb;
	uchar opcode;

	opcode = 0;
	issh = inst->isshort;
	islsb = inst->islsb;

	if(instid != TmsCsOutIn && instid != TmsCsOut&& instid != DataOutIn)
		if(inst->edge[1] != EdgeNone){
			werrstr("should not have edge");
			return OpBad;
		}
	
	switch(instid){
	case DataOutIn:
	case DataOut:
	case DataIn:
		opcode = dataopcode(inst, instid, issh, islsb);
		break;

	case TmsCsOutIn:
		opcode = OpModIn;
		if (inst->edge[1] == EdgeDown)
			opcode |= OpModInFalling;
	/* fall through */
	case TmsCsOut:
		opcode |= OpTmsCs;
		if(!issh || (inst->len[0] + 1) > 7){
			werrstr("len too big: %d, issh:%d",
				inst->len[0]+1, issh);
			return OpBad;
		}
		if (inst->edge[0] == EdgeDown)
			opcode |= OpModOutFalling;
		opcode |= OpModShort;
		opcode |= OpModLSB;
		break;
	case MCURd:
		opcode = OpMCURd;
		opcode |= !issh? OpModLong : 0;
		break;
	case MCUWr:
		opcode = OpMCUWr;
		opcode |= !issh? OpModLong : 0;
		break;
	case SendImm:
		opcode = OpSendImm;
		break;
	case WaitIOHigh:
		opcode = OpWaitIOHigh;
		break;
	case TckSkDiv:
		opcode = OpTckSkDiv;
		break;
	case WaitIOLow:
		opcode = OpWaitIOLow;
		break;
	case AdaptClkEnab:
		opcode = OpAdaptClkEnab;
		break;
	case AdaptClkDisab:
		opcode = OpAdaptClkDisab;
		break;
	case Div5ClkEnab:
		opcode = OpDiv5ClkEnab;
		break;
	case Div5ClkDisab:
		opcode = OpDiv5ClkDisab;
		break;
	case GetBitsH:
	case SetBitsH:
		opcode = OpModBitsH;
	case GetBitsL:
	/* fall through */
		if(instid == GetBitsL || instid == GetBitsH)
			opcode |= OpModBitsIn;
	case SetBitsL:
		opcode |= OpSetBits;
		break;
	case Loop:
		opcode = OpLoop;
		break;
	case BreakLoop:
		opcode = OpBreakLoop;
		break;
	default:
		werrstr("unknown instruction");
		return OpBad;
	}
	return opcode;
}

enum {
	ParLen	=	'L',		/* Data len 2 bytes or 1 byte (bit len) */
	ParShLen	=	'l', 		/* bit len Bnumber */
	ParData	=	'D',		/* data or @ to place data later */
	ParShData = 	'd',		/* short data, 1 byte at most */
	ParImm 	=	'I',		/* immediate operator, 2/1 bytes */
	ParLImm	=	'i',		/* immediate operator, 2 bytes */
	ParShImm =	'c',		/* immediate operator, 1 byte */
	ParEdge	=	'E',		/* EdgeUp or EdgeDown */
	ParEndian	=	'N',		/* LSB or MSB */
	ParLsb	=	'n',		/* LSB */
};

typedef struct InstDesc InstDesc;
struct InstDesc
{
	char *name;
	char *desc;
	int hasresp;
};

static InstDesc idesc[] = {
	[DataIn]			{"DataIn",	"ENL", 1},
	[DataOut]			{"DataOut",	"ENLD", 0},
	[DataOutIn]		{"DataOutIn",	"EENLD", 1},
	[TmsCsOut]		{"TmsCsOut",	"ENld", 0},
	[TmsCsOutIn]		{"TmsCsOutIn",	"EENld", 1},
	[MCURd]			{"MCURd",	"I", 1},
	[MCUWr]			{"MCUWr",	"Id", 0},
	[SendImm]		{"SendImm",	"", 0},
	[WaitIOHigh]		{"WaitIOHigh",	"", 0},
	[WaitIOLow]		{"WaitIOLow",	"", 0},
	[AdaptClkEnab]		{"AdaptClkEnab",	"", 0},
	[AdaptClkDisab]	{"AdaptClkDisab",	"", 0},
	[Div5ClkEnab]		{"Div5ClkEnab",	"", 0},
	[Div5ClkDisab]		{"Div5ClkDisab",	"", 0},
	[SetBitsL]			{"SetBitsL",	"cc", 0},
	[GetBitsL]			{"GetBitsL",	"c", 0},
	[SetBitsH]			{"SetBitsH",	"cc", 0},
	[GetBitsH]			{"GetBitsH",	"c", 0},
	[TckSkDiv]		{"TckSkDiv",	"i", 0},
	[Loop]			{"Loop",	"", 0},
	[BreakLoop]		{"BreakLoop",	"", 0},
	[ILast]			{nil,		nil, 0},
};


static int
nametoid(char *name)
{
	int i;
	for (i = 0; i < ILast; i++){
		if(strcmp(inst[i], name) == 0)
			break;
	}
	if(i == ILast)
		return -1;
	return i;
}

static int
edgetoid(char *edgename)
{
	if(strcmp(edgename, "EdgeUp") == 0)
		return EdgeUp;
	else if(strcmp(edgename, "EdgeDown") == 0)
		return EdgeDown;

	return EdgeBad;
}

static int
isendian(char *endianname)
{
	if(strcmp(endianname, "LSB") == 0)
		return 1;
	else if(strcmp(endianname, "MSB") == 0)
		return 1;

	return 0;
}


static int
datalen(Inst *inst, char *p)
{
	int len;
	if(p[0] == 'B'){
		inst->isshort++;
		p++;
	}
	else
		inst->isshort = 0;
	len = atoi(p); //strtol and check
	if(inst->isshort && len > 8)
		return -1;
	if(len > MaxDataSz)
		return -1;
	return len;
}

static void
setlen(Inst *inst, int dlen)
{
	if(dlen != 0){
		inst->lensz++;
		inst->len[0] = (dlen - 1 ) & 0xff;
		if(!inst->isshort){
			inst->len[1] = 0xff & ((dlen - 1) >> 8);
			inst->lensz++;
		}
	}
}


static int
setdata(Inst *inst, char *nm, char *p, char *te, int bdlen)
{
	int i;
	char *e;

	if(p == nil){
		werrstr("%s: should have data", nm);
		return -1;
	}
	if(*p == '@'){
		inst->datasz = bdlen;
		return 0;
	}
	
	if(te != p)
		p[strlen(p)] = ' '; /* untokenize */

	for (i = 0; i < bdlen; i++){
		inst->data[i] = strtol(p, &e, 0);
		if(!isspace(*e) && i != bdlen-1)
			return -1;
		p = e;
	}
	inst->datasz = i;
	return 0;

}

static int
hasresp(int instid)
{
	return idesc[instid].hasresp;
}


static int
hasnoparam(int instid)
{
	return strlen(idesc[instid].desc) == 0;
}

static char *
endfield(char *p)
{
	char *s;

	s = p;
	if(*p =='\0')
		return nil;
	while(*p != '\0'){
		if(isspace(*p)){
			if(s == p)
				return nil;
			return p;
		}
		p++;
	}
	return p;
}

enum{
	LongSz,
	ShortSz,
	AnySz,
};

static int
setimm(Inst *inst, int imm, int kindsz)
{	
	int isl, immh, imml;

	isl = imm&~0xff;

	if(isl && kindsz == ShortSz)
		return -1;


	if(isl || kindsz == LongSz){
		immh = (imm >> 8)&0xff;
		inst->imm[inst->immsz++] = immh; /* High, then low */
	}
	imml = imm&0xff;
	inst->imm[inst->immsz++] = imml;
	return 0;
}

static Inst*
parseinst(int instid, char *pars, char *idname)
{
	char *tok[15], *err, *e;
	char tf;
	int i, ntok, tnf, dlen, bdlen, imm, ndata;
	Inst *inst;
	
	tf = '\0';
	dlen = bdlen = 0;
	err = "";
	inst = mallocz(sizeof(Inst) + 1, 1);
	if(inst == nil)
		return nil;
	tnf = strlen(idesc[instid].desc);
	e = pars + strlen(pars) + 1;
	ntok = tokenize(pars, tok, tnf);
	if(tnf != ntok){
		werrstr("%s: bad nr param %d!=%d", idname, tnf, ntok);
		return nil;
	}
	
	for(i = 0; i < tnf; i++){
		tf = idesc[instid].desc[i];
		switch(tf){	
		case ParLen:
			dlen = datalen(inst, tok[i]);
			if(dlen < 0){
				err = "bad len";
				goto Err;
			}
			if(inst->isshort)
				bdlen = 1;
			else {
				bdlen = dlen;
				inst = realloc(inst, sizeof(Inst) + bdlen);
				if(inst == nil)
					return nil;
			}
			break;
		case ParShLen:
			dlen = datalen(inst, tok[i]);
			if(dlen < 0){
				err = "bad len";
				goto Err;
			}
			bdlen = 1;
			if(!inst->isshort) {
				err = "should be short";
				goto Err;
			}
			break;
		case ParData:
			if(bdlen != 0){
				ndata = setdata(inst, idname, tok[i], e, bdlen);
				if(ndata < 0){
					err = "short data";
					goto Err;
				}
			}
			else {
				err = "no data";
				goto Err;
			}
			break;
		case ParShData:
			if(bdlen != 0){
				ndata = setdata(inst, idname, tok[i], e, bdlen);
				if(ndata < 0){
					err = "short data";
					goto Err;
				}
			}
			else {
				err = "no data";
				goto Err;
			}
			if(ndata > 1){
				err = "more than 1 byte data";
				goto Err;
			}
			break;
		case ParImm:
			imm = atoi(tok[i]);
			if(setimm(inst, imm, AnySz) < 0)
				goto Err;
			break;
		case ParLImm:
			imm = atoi(tok[i]);
			if(setimm(inst, imm, LongSz) < 0)
				goto Err;
			break;
		case ParShImm:
			imm = atoi(tok[i]);
			if(setimm(inst, imm, ShortSz) < 0)
				goto Err;
			break;
		case ParEdge:
			inst->edge[inst->nedge] = edgetoid(tok[i]);
			if(inst->edge[inst->nedge] == EdgeBad)
				goto Err;
			inst->nedge++;
			break;
		case ParEndian:
			inst->islsb = strcmp(tok[i], "LSB") == 0;
			if(!isendian(tok[i])){
				err = "bad endianness";
				goto Err;
			}
			break;
		case ParLsb:
			inst->islsb = strcmp(tok[i], "LSB") == 0;
			if(inst->islsb){
				err = "bad endianness: not LSB";
				goto Err;
			}
			break;
		default:
			err = "bad param";
			goto Err;
		}
		
		if(bdlen > MaxDataSz || (inst->isshort && dlen > 8)){
			err = "len too big";
			goto Err;
		}
	}
	
	setlen(inst, dlen);
	inst->hasresp = hasresp(instid);
	inst->opcode = opcode(inst, instid);
	if(inst->opcode == OpBad){
		err = "bad opcode";
		goto Err;
	}
	return inst;
Err:
	werrstr("inst:%s, nf:%d, ftype:%c, fval:%s:%s\n", idname, i, tf, tok[i], err);
	free(inst);
	return nil;	
}

static Inst *
parseassln(char *assline)
{
	int instid, lnsz;
	Inst *inst;
	char *ef, inm[32];

	dprint(Dassln, "%s\n", assline);

	lnsz = strlen(assline);
	if(lnsz == 0){
	 	werrstr("%s: line does not match instruction", assline);
		return nil;
	}

	ef = endfield(assline);
	if(ef == nil){
	 	werrstr("%s: empty inst", assline);
		return nil;
	}
	strncpy(inm, assline, ef-assline);
	inm[ef-assline] = '\0';
	instid = nametoid(inm);
	if(instid < 0){
		werrstr("unrecognized instruction %s", inm);
		return nil;
	}
	inst = parseinst(instid, ef, inm);
	return inst;
}

static int
unpacklen(Inst *inst)
{
	int len;

	if (inst->lensz == 0)
		return 0;
	len =  ((inst->len[1]<<8) | inst->len[0]) + 1;

	return len;
}

static int
instpacksz(Inst *inst)
{
	return 1 + inst->immsz + inst->lensz + inst->datasz;
}

static void
sdumpinst(char *s, int ssz, Inst *inst)
{
	int i;
	char *e, *te;

	e = s + ssz - 1;
	te = e;

	e = seprint(s, te, "op: %#2.2ux\n", inst->opcode);
	if(e >= te)
		return;
	if(inst->hasresp){
		e = seprint(e, te, "hasresp: %d\n", inst->hasresp);
		if(e >= te)
			return;
	}
	if(inst->isshort){
		e = seprint(e, te, "isshort: %d\n", inst->isshort);
		if(e >= te)
			return;
	}
	e = seprint(e, te, "islsb: %d\n", inst->islsb);
	if(e >= te)
		return;

	for(i = 0; i < inst->immsz; i++){
		e = seprint(e, te, "imm%d: %#2.2ux\n", i, inst->imm[i]);
		if(e >= te)
			return;
	}

	if(inst->lensz != 0){	
		e = seprint(e, te, "lensz: %d\n", inst->lensz);
		if(e >= te)
			return;
		e = seprint(e, te, "len: %d\n", unpacklen(inst));
		if(e >= te)
			return;
		e = seprint(e, te, "datasz: %d\n", inst->datasz);
		if(e >= te)
			return;
		for(i = 0; i < inst->datasz; i++){
			e = seprint(e, te, "%#2.2ux ", inst->data[i]);
			if(e >= te)
				return;
		}
	}
}

static void
debpack(Inst *inst)
{
	int i;
	if(!debug[Dmach] && !debug[DAll])
		return;
	fprint(2, "%#2.2ux ", inst->opcode);

	for(i = 0; i <  inst->immsz; i++)
		fprint(2, "%#2.2ux ", inst->imm[i]);
	for(i = 0; i <  inst->lensz; i++)
		fprint(2, "%#2.2ux ", inst->len[i]);
	fprint(2, "\n");
	dumpbuf(Dmach, inst->data, inst->datasz);
	fprint(2, "\n");
}


static int
instpack(Mpsse *mpsse, Inst *inst)
{
	int n;
	int sz;
	Biobufhdr *bp;

	bp = &mpsse->bout;

	sz = instpacksz(inst);
	if(sz + Bbuffered(bp) > MpsseBufSz){
		n = mpsseflush(mpsse);
		if(n < 0)
			return -1;
	}
	debpack(inst);
	n = Bwrite(bp, &inst->opcode, 1);
	if(n < 0)
		return -1;
	n = Bwrite(bp, inst->imm, inst->immsz);
	if(n < 0)
		return -1;
	n = Bwrite(bp, inst->len, inst->lensz);
	if(n < 0)
		return -1;
	n = Bwrite(bp, inst->data, inst->datasz);
	if(n < 0)
		return -1;
	return 0;
}

int
pushcmd(Mpsse *mpsse, char *ln)
{
	Inst *inst;
	int res;
	char *dmp, *lln;

	res = 0;
	lln = strdup(ln);
	inst = parseassln(lln);
	free(lln);

	if(inst == nil)
		return -1;
	if(debug[Dinst]){
		dmp = malloc(255);
		if(dmp == nil)
			return -1;
		sdumpinst(dmp, 255, inst);
		fprint(2, "%s\n", dmp);
		free(dmp);
	}

	if(instpack(mpsse, inst) < 0)
		res = -1;
	free(inst);
	return res;
}

int
pushcmdwdata(Mpsse *mpsse, char *ln, uchar *buf, int buflen)
{
	Inst *inst;
	int res;
	char *dmp, *lln;

	res = 0;
	lln = strdup(ln);
	inst = parseassln(lln);
	free(lln);

	if(inst == nil)
		return -1;
	if(inst->datasz != buflen){
		werrstr("wrong data in cmd %d != %d", inst->datasz, buflen);
		return -1;
	}
	memmove(inst->data, buf, inst->datasz);
	if(debug[Dinst]){
		dmp = malloc(255);
		if(dmp == nil)
			return -1;
		sdumpinst(dmp, 255, inst);
		fprint(2, "%s\n", dmp);
		free(dmp);
	}

	if(instpack(mpsse, inst) < 0)
		res = -1;
	free(inst);
	return res;
}


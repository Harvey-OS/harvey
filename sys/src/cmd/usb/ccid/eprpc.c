#include <u.h>
#include <libc.h>
#include <thread.h>
#include "le.h"
#include "usb.h"
#include "usbfs.h"
#include "ccid.h"
#include "eprpc.h"
#include "tagrd.h"

/* table 6 in iso7816-3  translates the clock and data rates, if only I had it... */


/*
 * protocol on the pipe, defined in USB CCID standard
 */

char *errmsgs[] ={
	[CmdNotSupp] = "command not supported",
	[Indexstart] = nil,	/* start */
	[Indexend] = "index not supported/incorrect msg",
	[CmdSlotBusy] = "slot busy",
	[PinCancelled] = "pin cancelled",
	[PinTimeout] = "pin timeout",
	[AutoseqBusy] = "there an autoseq already going on",
	[DeactProto] = "deactivated protocol",
	[ProcByteConflic] = "procedure byte conflict",
	[IccClassNotSup] = "icc class not supported",
	[IccProtoNotSup] = "icc protocol not supported",
	[BadAtrTck] = "bad atr tck",
	[BadAtrTs] = "bad atr ts",
	[HwError] = "hw armageddon",
	[XfrOverrun] = "overrun talking to the icc",
	[XfrParError] = "parity error talking to the icc",
	[IccMute] = "CCID timed out talking to the icc",
	[Aborted] = "host aborted the current activity",
};

int
packpinmod(uchar *b, int bsz, Pinop *p)
{
	uchar * s;

	s = b;
	if(bsz < 19)
		return -1;
	*b++ = p->pinoperation;
	*b++ = p->tout;
	*b++ = p->fmtstr;
	*b++ = p->pinblkstr;
	*b++ = p->pinlenfmt;
	*b++ = p->insertoffold;
	*b++ = p->insertoffnew;
	hиputs(b, p->pinxdigitmax);
	b+= sizeof p->pinxdigitmax;
	*b++ = p->confirmpin;
	*b++ = p->entryvalcond;
	*b++ = p->nmsg;
	*b++ = p->langid;
	*b++ = p->msgidx1;
	*b++ = p->msgidx2;
	*b++ = p->msgidx3;
	*b++ = p->teoprolog[0];
	*b++ = p->teoprolog[1];
	*b++ = p->teoprolog[2];
	return b-s;
}

int
packpinver(uchar *b, int bsz, Pinop *p)
{
	uchar * s;

	s = b;
	if(bsz < 14)
		return -1;
	*b++ = p->pinoperation;
	*b++ = p->tout;
	*b++ = p->fmtstr;
	*b++ = p->pinblkstr;
	*b++ = p->pinlenfmt;
	hиputs(b, p->pinxdigitmax);
	b+= sizeof p->pinxdigitmax;
	*b++ = p->entryvalcond;
	*b++ = p->nmsg;
	*b++ = p->langid;
	*b++ = p->msgidx;
	*b++ = p->teoprolog[0];
	*b++ = p->teoprolog[1];
	*b++ = p->teoprolog[2];
	return b-s;
}

int
isresponse(uchar type)
{
	if(type & 0x80)
		return 1;
	else
		return 0;
}


/* this packs the structure not the ICC payload or the pinapdu */

int
packmsg(uchar *b, int bsz, Cmsg *c)
{
	uchar *s;
	Pinop *pinop;
	Param *par;
	Clockdata *clkd;
	int r;
	s = b;
	
	r = 0;

	c->isresp = isresponse(c->type);
	if(bsz < Hdrsz+c->len)
		return -1;

	*b++ = c->type;
	
	hиputl(b, c->len);
	b += sizeof c->len;
	*b++ = c->bslot;
	*b++ = c->bseq;

	if(c->isresp){
		*b++ = c->status;
		*b++ = c->error;
	}
	switch(c->type){
		case IccPowerOnTyp:
			*b++ = c->powsel;
			break;
		case XfrBlockTyp:	/* fall */
		case SecureTyp:
			*b++ = c->bwi;
			hиputs(b, c->levelparam);
			b += sizeof c->levelparam;
			break;
		case ParamTypR:	/* fall */
		case SetParamTyp:
			*b++ = c->protoid;
			break;

		case IccClockTyp:
			*b++ = c->clockcmd;
			break;
		case T0ApduTyp:
			*b++ = c->mchanges;
			*b++ = c->classgetresp;
			*b++ = c->classenv;
			break;
		case MechanTyp:
			*b++ = c->function;
			break;
		case DataBlockTypR:
			*b++ = c->chainparam;
			break;
		case SlotStatusTypR:
			fprint(2, "ooooor:%ld len:%lud\n", b-s, c->len);
			*b++ = c->clockstatus;
			break;
		default:
			break;
	}

	memset(b, 0, Hdrsz - (b - s));
	b += Hdrsz - (b - s);
	switch(c->type){
		case SetParamTyp:
			par = (Param *)c->data;
			*b++ = (par->clkrate << 4)|(par->bitrate & 0xf);
			*b++ = par->tcck;
			*b++ = par->guardtime;
			*b++ = par->waitingint;
			*b++ = par->clockstop;
			if(c->protoid == T1){
				*b++ = par->ifsc;
				*b++ = par->nadvalue;
			}
			break;

		case SecureTyp:
			pinop = (Pinop *)c->data;
			if(pinop->pinoperation == Pinverop)
				r = packpinver(b, bsz-(b-s), pinop);
			else if(pinop->pinoperation == Pinmodop)
				r = packpinmod(b, bsz-(b-s), pinop);
			if(r < 0)
				return -1;
			b+=r;
			break;
		case DataRateClkTypR:
			clkd = (Clockdata *)c->data;
			hиputl(b, clkd->clockfreq);
			b += sizeof clkd->clockfreq;
			hиputl(b, clkd->datarate);
			b += sizeof clkd->datarate;
			break;
		default:
			if(c->len && c->data){
				memmove(b, c->data, c->len);
				b += c->len;
			}
			break;
	}
	if(b-s > bsz)
		sysfatal("bad size for buffer");
	if(b-s != Hdrsz + c->len)
		fprint(2, "bad size for packing r:%ld len:%lud\n", b-s, Hdrsz + c->len);
	return b-s;
}



int
unpackpinmod(Pinop *p, uchar *b, int bsz)
{
	uchar * s;

	s = b;
	if(bsz < 19)
		return -1;
	p->pinoperation = *b++;
	p->tout = *b++;
	p->fmtstr = *b++;
	p->pinblkstr = *b++;
	p->pinlenfmt = *b++;
	p->insertoffold = *b++;
	p->insertoffnew = *b++;
	p->pinxdigitmax= иhgets(b);
	b+= sizeof p->pinxdigitmax;
	p->confirmpin = *b++;
	p->entryvalcond = *b++;
	p->nmsg = *b++;
	p->langid = *b++;
	p->msgidx1 = *b++;
	p->msgidx2 = *b++;
	p->msgidx2 = *b++;
	p->teoprolog[0] = *b++;
	p->teoprolog[1] = *b++;
	p->teoprolog[2] = *b++;
	return b-s;
}

int
unpackpinver(Pinop *p, uchar *b, int bsz)
{
	uchar * s;

	if(bsz < 14)
		return -1;
	s = b;
	p->pinoperation = *b++;
	p->tout = *b++;
	p->fmtstr = *b++;
	p->pinblkstr = *b++;
	p->pinlenfmt = *b++;
	p->pinxdigitmax= иhgets(b);
	b+= sizeof p->pinxdigitmax;
	p->entryvalcond = *b++;
	p->nmsg = *b++;
	p->langid = *b++;
	p->msgidx = *b++;
	p->teoprolog[0] = *b++;
	p->teoprolog[1] = *b++;
	p->teoprolog[2] = *b++;
	return b-s;
}

/*
 *	this unpacks the structure not the ICC payload or the pinapdu (other layer)
*/

int
unpackmsg(Cmsg *c, uchar *b, int bsz)
{
	uchar *s;
	Pinop *pinop;
	Param *par;
	Clockdata *clkd;
	int r;
	s = b;
	
	r = 0;
	if(bsz < Hdrsz){
		werrstr("unpack: len < Hdr");
		return -1;
	}

	c->type = *b++;
	c->isresp = isresponse(c->type);
	c->len= иhgets(b);
	b += sizeof c->len;
	c->bslot = *b++;
	c->bseq = *b++;

	if(c->isresp){
		c->status = *b++;
		c->error = *b++;
	}
	if(bsz < Hdrsz+c->len && c->len > Maxpayload){
		werrstr("unpack: len < c->len");
		return -1;
	}

	switch(c->type){
		case IccPowerOnTyp:
			c->powsel = *b++;
			break;
		case XfrBlockTyp:	/* fall */
		case SecureTyp:
			c->bwi = *b++;
			c->levelparam= иhgets(b);
			b += sizeof c->levelparam;
			break;
		case ParamTypR:	/* fall */
		case SetParamTyp:
			c->protoid = *b++;
			break;
		case IccClockTyp:
			c->clockcmd = *b++;
			break;
		case T0ApduTyp:
			c->mchanges = *b++;
			c->classgetresp = *b++;
			c->classenv = *b++;
			break;
		case MechanTyp:
			c->function = *b++;
			break;
		case DataBlockTypR:
			c->chainparam = *b++;
			break;
		case SlotStatusTypR:
			c->clockstatus = *b++;
			break;
		default:
			;
	}

	b += Hdrsz - (b - s);	/*skip RFU and gaps... */

	if(c->len+1 > Maxpayload){
		werrstr("unpack: Maxpay < c->len+1 %lud", c->len+1);
		c->data = nil;
		return -1;
	}
	c->data = c->unpkdata;
	switch(c->type){
		case SetParamTyp:
			par = (Param *)c->data;
			par->clkrate = *b>>4;
			par->bitrate =  (*b++)&0xf;
			par->tcck = *b++;
			par->guardtime = *b++;
			par->waitingint = *b++;
			par->clockstop = *b++;
			if(c->protoid == T1){
				par->ifsc = *b++;
				par->nadvalue = *b++;
			}
			break;
		case SecureTyp:
			pinop = (Pinop *)c->data;
			if(pinop->pinoperation == Pinverop)
				r = unpackpinver(pinop, b, bsz-(b-s));
			else if(pinop->pinoperation == Pinmodop)
				r = unpackpinmod(pinop, b, bsz-(b-s));
			if(r < 0)
				return -1;
			b+=r;
			break;
		case DataRateClkTypR:
			clkd = (Clockdata *)c->data;
			clkd->clockfreq = иhgetl(b);
			b += sizeof(clkd->clockfreq);
			clkd->datarate = иhgetl(b);
			b += sizeof(clkd->datarate);
			break;
		default:
			if(c->len != 0){
				memmove((uchar *)c->unpkdata, b, c->len);
				b += c->len;
			}
			break;
	}
	c->unpkdata[c->len] = '\0';
	if(b-s > bsz)
		sysfatal("bad size for buffer");
	if(b-s != Hdrsz + c->len)
		werrstr("bad size for unpacking %ld %lud\n", b-s, Hdrsz +c->len);
	return b-s;
}

int
unpackimsg(Imsg *im, uchar *b, int bsz)
{
	uchar *s;
	int i;

	s = b;
	
	if(bsz < IHdrsz)
		return -1;

	im->type = *b++;

	switch(im->type){
		case HardwareErrorTypI:
			im->bslot = *b++;
			im->bseq = *b++;
			im->hwerrcode = *b++;
			break;
		case NotifySlotChangeTypI:
			for(i = 0; i< bsz - (b - s); i++){
				im->sloticcstate = *b++;
			}
			break;
		default:
			return -1;
	}
	if(b-s > bsz)
		sysfatal("bad size for buffer");
	return b-s;
}





static void
debbufprint(uchar *buf, int sz)
{
	int i;
	for(i = 0; i < sz; i++)
			dcprint(2, "%2.02x ", buf[i]);
	dcprint(2, "\n");
}

/*	BUG only one slot supported (has anyone more than one?)
 *	called with the lock...
 */
Cmsg *
iccmsgrpc(Ccid *ccid, void *blk, int sz, uchar type, uchar twait)
{
	int n, nw;
	uchar buf[64];
	Cmsg *c;
	int fdin, fdout;

	fdin = ccid->epin->dfd;
	fdout = ccid->epout->dfd;

	c = mallocz(sizeof (Cmsg), 1);
	if(c == nil)
		goto Error;
	c->isresp = 0;
	c->type = type;
	c->len = sz;
	c->bslot = 0;	/* BUG only one slot supported */
	if(ccid->sl == nil)	/* for initialization */
		c->bseq = 0;
	else
		c->bseq = ccid->sl->nreq;
	c->powsel = 0;
	c->data = blk;
	n = packmsg(buf, sizeof buf, c);
	if(n < 0){
		fprint(2, "packing msg\n");
		goto Error;
	}

	dcprint(2, "%d->", n);
	debbufprint(buf, n);
	nw = write(fdout, buf, n);
	if(nw !=  n){
		fprint(2, "writing to  fout %r\n");
		goto Error;
	}

	if(twait != 0)
		sleep(twait);
		
	n = read(fdin, buf, sizeof buf);
	if(n < 0){
		fprint(2, "reading from  fin %r\n");
		goto Error;
	}
	dcprint(2, "%d<-", n);
	debbufprint(buf, n);

	
	if(ccid->sl == nil){	/* for initialization */
		c->bseq = 0;
	}
	else
		c->bseq = ccid->sl->nreq++;

	ccid->recover = 0; /* I have done an RPC, so state is back ok */
	n = unpackmsg(c, buf, sizeof buf);
	
	if(n < 0){
		fprint(2, "error unpacking msg %r\n");
		goto Error;
	}
	dcprint(2, "\n");

	if( c->type != SlotStatusTypR && c->status != 0){
		if(c->error >= Indexstart && c->error <= Indexend)
			werrstr(errmsgs[Indexend]);
		else
			werrstr(errmsgs[c->error]);
		goto Error;
	}
	return c;
Error:
	free(c);
	return nil;	
}

int
isiccpresent(Ccid *ccid)
{
	int iccpres;
	Cmsg *c;

	iccpres = 1;
	c = iccmsgrpc(ccid, nil, 0, GetSlotStatusTyp, 0);

	if( c != nil && (c->status&NoIccpresent))
		iccpres = 0;
	free(c);
	return iccpres;
}

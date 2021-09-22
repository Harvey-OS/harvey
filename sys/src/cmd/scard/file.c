#include <u.h>
#include <libc.h>
#include <ip.h>
#include "icc.h"
#include "tlv.h"
#include "file.h"

/* named after the DF/EF */
char senam[] = "rmweatd";

int
tlv2fref(Fref *fr, Tlv *tlv)
{
	int i;
	uchar *p;

	if(tlv->tag != TagFref)
		return -1;
	assert(tlv->len < MaxPath);
	fr->type = 0;
	if(tlv->len == 0)
		fr->type |= IsRoot;
	else if(tlv->len == 1){
		fr->type |= IsShort;
		fr->shortid = tlv->data[0];
	}
	else if(tlv->len == 2){
		fr->type |= IsLong;
		fr->longid = nhgets(tlv->data);
	}
	else if(tlv->len % 2 == 0){
		fr->type |= IsPath;
		fr->pathlen = tlv->len/2;
		p =  tlv->data;
		for(i = 0; i < fr->pathlen; i++){
			fr->path[i] = nhgets(p);
			p += sizeof(ushort);
		}
		if(fr->path[0] != MFid)
			fr->type |= IsRel;
	}
	else{
		fr->type |= IsPath|IsQual;
		fr->pathlen = (tlv->len - 1)/2;
		p =  tlv->data;
		for(i = 0; i < fr->pathlen; i++){
			fr->path[i] = nhgets(p);
			p += sizeof(ushort);
		}
		fr->p1 = p[i];
	}
	return 0;
}

int
fref2tlv(Tlv *tlv, Fref *fr)
{
	int i;
	
	tlv->kind = TSimple;
	tlv->tagclass = 0;	/* ?? */
	tlv->isconstr = 0;
	
	tlv->tag = TagFref;
	tlv->data = tlv->datapack; /* BUG could use fr */
	tlv->len = fr->pathlen * 2;
	for(i = 0; i < fr->pathlen; i++)
		hnputs(tlv->data, fr->path[i]);
	if(fr->type &  IsQual){
		tlv->len++;
		tlv->data[i] = fr->p1;
	}
	return 0;
}

static ushort
fref2p(Fref *f)
{
	if(f->current){
		if(f->isdir)
			return 0x3FFF;
		else
			return 0x0000;
	}
	if(f->type&IsShort)
		return f->shortid;
	else if (f->type&IsLong)
		return f->longid;
	else 
		return 0x0000;
}

/* put data and others, similar */
CmdiccR *
getdata(int fd, Fref *f, uchar ins, ushort p12, uchar chan, int howmany)
{
	int rpcsz, res;
	Cmdicc c;
	CmdiccR *cr;
	Tlv t;

	rpcsz = 5;	/* cla ins p1 p2 ne */
	memset(&c, 0, sizeof(Cmdicc));
	/* TODO: for the moment, no chaining? */

	c.cla = chan2cla(chan);
	c.ne = howmany;

	if(f == nil )
		c.ins = ins;
	else if(f != nil ){
		p12 = fref2p(f);
		if(p12 == 0x0000 && !f->current){
			c.cdata = c.cdatapack;
			fref2tlv(&t, f);
			res = packtlv(&t, c.cdata, Maxcmd);
			if(res < 0){
				werrstr("getdata: packing");
				return nil;
			}
			c.nc = res;
			rpcsz += res+1;	/* nc + data */
		}
	}
	else{
		werrstr("getdata: bad request");
		return nil;
	}
	c.p1 = p12 >> 8;
	c.p2 = p12 & 0xff;
	cr = iccrpc(fd, &c, rpcsz);	/* TODO issue additional get response?? */
	if(cr == nil){
		werrstr("getdata:  %r\n");
		return nil;
	}
	return cr;
}

/*	select == walk, can be by DF name
 *	or by id (for EF and DF), by path, (list ids)
 *	by short EF identifier, 5 bits not all equal
 *	0x00000
 */
/* put data and others, similar */
CmdiccR *
selectfid(int fd, Fref *f, ushort id, uchar chan, uchar p1)
{
	int rpcsz;
	Cmdicc c;
	CmdiccR *cr;

	rpcsz = 5;	/* cla ins p1 p2 nc EF*/
	memset(&c, 0, sizeof(Cmdicc));
	/* TODO: for the moment, no chaining? */

	c.cla = chan2cla(chan);
	c.ins = InsSelect;

	c.p1 = p1;
	c.p2 = 0;
	c.cdata = c.cdatapack;	

	if(f == nil ){
		c.nc = 2;
		c.cdata[0] = id >> 8;
		c.cdata[1] = id & 0xff;
		rpcsz += 2;
	}
	else if(f != nil ){

		if(f->shortid){
			werrstr("tried to select a short id");
			return nil;
		}
		c.p1 = 0;
		if(f->pathlen != 0){	/* always absolute and without MF */
			c.p1 |= 1<<3;
			c.nc =  f->pathlen;
			memmove(c.cdata, f->path, f->pathlen);
			rpcsz += f->pathlen;
		}
		else{
			if(f->isdir)
				c.p1 |= 1;
				c.nc = 2;
				c.cdata[0] = f->longid >> 8;
				c.cdata[1] = f->longid & 0xff;
		}
	}
	else{
		werrstr("getdata: bad request");
		return nil;
	}
	cr = iccrpc(fd, &c, rpcsz);	/* TODO issue additional get response?? */
	if(cr == nil){
		werrstr("ttag: raw rpc error %r\n");
		return nil;
	}
	return cr;
}

static void
parsecond(Am *am, Tlv *t)
{
	Tlv tin;
	int l, res, i, authidx;

	i = 0;
	authidx = 0;
	for(l = 0; l< t->len; l += res){
		res = unpacktlv(&tin, t->data+l, t->len-l, TSimple);
		if(res < 0)
			sysfatal("unpacking tag: %r");
		switch(tin.tag){
		case	TagIdKey:
			am->sc[i] = tin.data[0];
			am->nsc++;
			i++;
			break;
		case	TagAuthVer:
			for(;authidx<i; authidx++)
				am->isauth[authidx] =   (tin.data[0]&0x08) != 0;
			break;
		}
	}
}

static void
parseexp(Am *am, Tlv *t)
{
	Tlv tin;
	int l, res, i, j;
	uchar c;

	res = unpacktlv(&tin, t->data, t->len, TSimple);
	if(res < 0)
		sysfatal("unpacking tag: %r");
	c = tin.tag & 0x0f;
	j = 0;
	for(i = 0; i < 4; i++){
		if(c&(1<<(3-i))){
			am->comm[i] = tin.data[j];
			am->iscomm[i] = 1;
			j++;
		}
		else
			am->iscomm[i] = 0;
	}

	am->nsc = 0;
	am->isallowed = 1;
	for(l = res; l< t->len; l += res){
		res = unpacktlv(&tin, t->data+l, t->len-l, TSimple);
		if(res < 0)
			sysfatal("unpacking tag: %r");
		switch(tin.tag){
		case	TagAlways:
			am->isallowed++;
			break;
		case	TagNever:
			am->isallowed = 0;
			break;
		case	TagAuthTemp:
			am->op = OneOp;
			break;
		case	TagAllowifOr:
			am->op = OrOp;
			break;
		case	TagAllowifNot:
			am->op = NotOp;
			break;
		case	TagAllowifAnd:
			am->op = AndOp;
			break;
		case TagScSe:
			am->op = SOp;
			break;
		default:
			break;
		}
		if(am->op != 0 && am->op != SOp){
			parsecond(am, &tin);
		}
	}

}


int
tlv2fcp(Fcp *fcp, uchar *buf, int bufsz)
{
	Tlv t, tin;
	int l, j, res;
	uchar c;

	unpacktlv(&t, buf, bufsz, TSimple);
	if(t.tag != TagFci && t.tag != TagFmd && t.tag != TagFci)
		return -1;
	fcp->kindtag = t.tag;
	for(l = 0; l< t.len; l += res){ 
		res = unpacktlv(&tin, t.data+l, t.len-l, TSimple);
		if(res < 0)
			sysfatal("unpacking tag: %r");
		switch(tin.tag){
		case TagFid:
			if(tin.len != 2)
				break;
			fcp->f.longid = nhgets(tin.data);
			break;
		case TagShort:
			if(tin.len != 1)
				break;
			fcp->f.shortid = tin.data[0];
			break;
		case	TagLcs:
			if(tin.len != 1)
				break;
			fcp->lcs = tin.data[0];
			break;
		case TagFd:
			break;
		case TagEFSecEnv:
			if(tin.len != 2)
				break;
			fcp->sefid = nhgets(tin.data);
			break;
		case TagDFname:
			if(tin.len > 64){
				tin.len = 64;
				fprint(2, "tlv2fcp: warning, name too long %lud\n", tin.len);
			}
			fcp->name[0] = 0;
			memmove(fcp->name, tin.data, tin.len);
			break;
		case TagSecAttrComp:
			c = tin.data[0];
			for(j = 0; j < tin.len-1; c >>= 1){
				if(c & 1){
					fcp->sc[j] = tin.data[j+1];
					fcp->scidx[j] = j;
					j++;
				}
			}
			fcp->nsc = j;
			break;
		case TagSectAttrExp:
			if(tin.len > 2)
				parseexp(&fcp->am, &tin);
			else
				memset(&fcp->am, 0, sizeof(Am));
			break;
		default:
			break;
		};
	}
	return 0;
}


uchar iccnam[] = "cipP";
uchar opsnam[] = "_&|!oS";

void
printfcp(Fcp *fcp)
{
	int i;
	Am *am;

	print("kindtag: %ux\n", fcp->kindtag);
	print("fref: %4.4ux\n", fcp->f.longid);
	print("lcs: %2.2ux\n", fcp->lcs);
	print("sefid: %4.4ux\n", fcp->sefid);
	print("name: %s\n", fcp->name);
	print("perm: ");
	for(i = 0; i < fcp->nsc; i++){
		if(fcp->scidx[i] < strlen(senam))
			print("%c[%2.2ux]",senam[fcp->scidx[i]], fcp->sc[i]);
		else
			print("N[%2.2ux]", fcp->sc[i]);
	}
	print("\n");
	print("extperm: ");
	am = &fcp->am;
	for(i = 0; i < 4; i++){
		if(am->iscomm[i] != 0)
			print("%c: %2.2ux ", iccnam[i], am->comm[i]);
	}
	print("%s ", am->isallowed?"allow":"never");
	print("op %c ", opsnam[am->op]);
	for(i = 0; i < am->nsc; i++)
		print("%c[%2.2ux]", am->isauth?'a':'v', am->sc[i]);
	print("\n");
}
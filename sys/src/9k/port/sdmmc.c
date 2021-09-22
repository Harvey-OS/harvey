/*
 * mmc / sd memory card / sdio interface
 *
 * Copyright © 2012 Richard Miller <r.miller@acm.org>
 *
 */

#include "u.h"
#include "../port/lib.h"
#include "../port/error.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "../port/sd.h"

#define CSD(end, start)	rbits(csd, start, (end)-(start)+1)
#define CMD(op, data)	io->cmd(unit->subno, (op), (data), r)

typedef struct Ctlr Ctlr;
typedef struct Card Card;

enum {
	Inittimeout	= 15,
	Multiblock	= 1,
	ExtCSDsize	= 512,

	/* Commands */
	GO_IDLE_STATE	= 0,
	SEND_OP_COND	= 1,
	ALL_SEND_CID	= 2,
	SEND_RELATIVE_ADDR= 3,
	IO_SEND_OP_COND=	5,
	MMC_SWITCH = 6,
	SELECT_CARD	= 7,
	SD_SEND_IF_COND	= 8,
	MMC_SEND_EXTENDED_CSD = 8,
	SEND_CSD	= 9,
	STOP_TRANSMISSION= 12,
	SEND_STATUS	= 13,
	SET_BLOCKLEN	= 16,
	READ_SINGLE_BLOCK= 17,
	READ_MULTIPLE_BLOCK= 18,
	WRITE_BLOCK	= 24,
	WRITE_MULTIPLE_BLOCK= 25,
	APP_CMD		= 55,	/* prefix for following app-specific commands */
	SET_BUS_WIDTH	= 6,
	SD_SEND_OP_COND	= 41,

	/* Command arguments */
	/* SD_SEND_IF_COND */
	Voltage		= 1<<8,
	Checkpattern	= 0x42,

	/* SELECT_CARD */
	Rcashift	= 16,

	/* SD_SEND_OP_COND */
	Hcs	= 1<<30,	/* host supports SDHC & SDXC */
	Ccs	= 1<<30,	/* card is SDHC or SDXC */
	V3_3	= 3<<20,	/* 3.2-3.4 volts */

	/* SET_BUS_WIDTH */
	Width1	= 0<<0,
	Width4	= 2<<0,

	/* MMC_SWITCH */
	MMCWidth4 = 0x03B70100,

	/* OCR (operating conditions register) */
	Powerup	= 1<<31,
};

struct Ctlr {
	SDev	*dev;
	SDio	*io;
	Card	**card;
};

struct Card {
	/* SD card registers */
	u16int	rca;
	u32int	ocr;
	u32int	cid[4];
	u32int	csd[4];
};

extern SDifc sdmmcifc;
extern SDio sdio;

static uint
rbits(u32int *p, uint start, uint len)
{
	uint w, off, v;

	w   = start / 32;
	off = start % 32;
	if(off == 0)
		v = p[w];
	else
		v = p[w] >> off | p[w+1] << (32-off);
	if(len < 32)
		return v & ((1<<len) - 1);
	else
		return v;
}

static int
identify(SDunit *unit, u32int *csd)
{
	uint csize, mult;

	unit->secsize = 1 << CSD(83, 80);
	switch(CSD(127, 126)){
	case 3:				/* MMC extended CSD */
		return -1;
	case 0:				/* SD CSD version 1 */
	case 2:				/* MMC CSD v1.2 */
		csize = CSD(73, 62);
		mult = CSD(49, 47);
		unit->sectors = (csize+1) * (1<<(mult+2));
		break;
	case 1:				/* SD CSD version 2 or MMC CSD v1.1 */
		csize = CSD(69, 48);
		unit->sectors = (csize+1) * 512LL*KB / unit->secsize;
		break;
	}
	if(unit->secsize == 1024){
		unit->sectors <<= 1;
		unit->secsize = 512;
	}
	return 0;
}

static void
identifyext(SDunit *unit, uchar *csdext)
{
	uchar *s;

	s = &csdext[212];
	unit->sectors = s[0] | s[1]<<8 | s[2]<<16 | s[3]<<24;
}

static SDev*
mmcpnp(void)
{
	SDev *sdev;
	Ctlr *ctl;
	int nunit;

	if((nunit = sdio.init()) <= 0)
		return nil;
	sdev = malloc(sizeof(SDev));
	if(sdev == nil)
		return nil;
	ctl = malloc(sizeof(Ctlr) + nunit*sizeof(Card*));
	if(ctl == nil){
		free(sdev);
		return nil;
	}
	ctl->card = (Card**)((char*)ctl + sizeof(Ctlr));
	sdev->idno = 'M';
	sdev->ifc = &sdmmcifc;
	sdev->nunit = nunit;
	sdev->ctlr = ctl;
	ctl->dev = sdev;
	ctl->io = &sdio;
	return sdev;
}

static int
mmcverify(SDunit *unit)
{
	int n;
	Ctlr *ctl;
	Card *card;

	ctl = unit->dev->ctlr;
	n = ctl->io->inquiry(unit->subno, (char*)&unit->inquiry[8], sizeof(unit->inquiry)-8);
	if(n < 0)
		return 0;
	card = malloc(sizeof(Card));
	if(card == nil)
		return 0;
	ctl->card[unit->subno] = card;
	unit->inquiry[0] = SDperdisk;
	unit->inquiry[1] = SDinq1removable;
	unit->inquiry[4] = sizeof(unit->inquiry)-4;
	return 1;
}

static int
mmcenable(SDev* dev)
{
	Ctlr *ctl;

	ctl = dev->ctlr;
	ctl->io->enable();
	return 1;
}

static int
mmconline(SDunit *unit)
{
	int hcs, mmc, i;
	u32int r[4];
	Ctlr *ctl;
	SDio *io;
	Card *card;
	uchar *csdext;

	ctl = unit->dev->ctlr;
	io = ctl->io;
	card = ctl->card[unit->subno];

	if(waserror()){
		unit->sectors = 0;
		return 0;
	}
	if(unit->sectors != 0){
		CMD(SEND_STATUS, card->rca<<Rcashift);
		poperror();
		return 1;
	}
	CMD(GO_IDLE_STATE, 0);
	hcs = 0;
	mmc = 0;
	if(!waserror()){
		CMD(SD_SEND_IF_COND, Voltage|Checkpattern);
		if(r[0] == (Voltage|Checkpattern))	/* SD 2.0 or above */
			hcs = Hcs;
		poperror();
	}
	if(!waserror()){
		for(i = 0; i < Inittimeout; i++){
			delay(100);
			CMD(APP_CMD, 0);
			CMD(SD_SEND_OP_COND, hcs|V3_3);
			if(r[0] & Powerup)
				break;
		}
		poperror();
	}else{
		mmc = 1;
		CMD(GO_IDLE_STATE, 0);
		for(i = 0; i < Inittimeout; i++){
			delay(100);
			CMD(SEND_OP_COND, Hcs|V3_3);
			if(r[0] & Powerup)
				break;
		}
	}
	if(i == Inittimeout){
		print("sdmmc: card won't power up\n");
		poperror();
		return 2;
	}
	card->ocr = r[0];
	CMD(ALL_SEND_CID, 0);
	memmove(card->cid, r, sizeof card->cid);
	if(mmc){
		card->rca = 1;
		CMD(SEND_RELATIVE_ADDR, card->rca<<Rcashift);
	}else{
		CMD(SEND_RELATIVE_ADDR, 0);
		card->rca = r[0]>>16;
	}
	CMD(SEND_CSD, card->rca<<Rcashift);
	memmove(card->csd, r, sizeof card->csd);
	CMD(SELECT_CARD, card->rca<<Rcashift);
	if (identify(unit, card->csd) < 0){
		csdext = malloc(ExtCSDsize);
		io->iosetup(unit->subno, 0, csdext, ExtCSDsize, 1);
		if(waserror()){
			free(csdext);
			nexterror();
		}
		CMD(MMC_SEND_EXTENDED_CSD, 0);
		io->io(unit->subno, 0, csdext, ExtCSDsize);
		poperror();
		identifyext(unit, csdext);
		free(csdext);
	}
	CMD(SET_BLOCKLEN, unit->secsize);
	if(!mmc){
		CMD(APP_CMD, card->rca<<Rcashift);
		CMD(SET_BUS_WIDTH, Width4);
	}else{
		if(!waserror()){
			CMD(MMC_SWITCH, MMCWidth4);
			poperror();
		}
	}
	poperror();
	return 1;
}

static int
mmcrctl(SDunit *unit, char *p, int l)
{
	Ctlr *ctl;
	Card *card;
	int i, n;

	ctl = unit->dev->ctlr;
	card = ctl->card[unit->subno];
	if(unit->sectors == 0){
		mmconline(unit);
		if(unit->sectors == 0)
			return 0;
	}
	n = snprint(p, l, "rca %4.4ux ocr %8.8ux\ncid ", card->rca, card->ocr);
	for(i = nelem(card->cid)-1; i >= 0; i--)
		n += snprint(p+n, l-n, "%8.8ux", card->cid[i]);
	n += snprint(p+n, l-n, " csd ");
	for(i = nelem(card->csd)-1; i >= 0; i--)
		n += snprint(p+n, l-n, "%8.8ux", card->csd[i]);
	n += snprint(p+n, l-n, "\ngeometry %llud %ld\n",
		unit->sectors, unit->secsize);
	return n;
}

static long
mmcbio(SDunit *unit, int lun, int write, void *data, long nb, uvlong bno)
{
	int len, tries;
	ulong b;
	u32int r[4];
	uchar *buf;
	Ctlr *ctl;
	Card *card;
	SDio *io;

	USED(lun);
	ctl = unit->dev->ctlr;
	card = ctl->card[unit->subno];
	io = ctl->io;
	if(unit->sectors == 0)
		error("media change");
	buf = data;
	len = unit->secsize;
	if(Multiblock){
		b = bno;
		tries = 0;
		while(waserror())
			if(++tries == 3)
				nexterror();
		io->iosetup(unit->subno, write, buf, len, nb);
		if(waserror()){
			CMD(STOP_TRANSMISSION, 0);
			nexterror();
		}
		CMD(write? WRITE_MULTIPLE_BLOCK: READ_MULTIPLE_BLOCK,
			card->ocr & Ccs? b: b * len);
		io->io(unit->subno, write, buf, nb * len);
		poperror();
		CMD(STOP_TRANSMISSION, 0);
		poperror();
		b += nb;
	}else{
		for(b = bno; b < bno + nb; b++){
			io->iosetup(unit->subno, write, buf, len, 1);
			CMD(write? WRITE_BLOCK : READ_SINGLE_BLOCK,
				card->ocr & Ccs? b: b * len);
			io->io(unit->subno, write, buf, len);
			buf += len;
		}
	}
	return (b - bno) * len;
}

static int
mmcrio(SDreq*)
{
	return -1;
}

SDifc sdmmcifc = {
	.name	= "mmc",
	.pnp	= mmcpnp,
	.enable	= mmcenable,
	.verify	= mmcverify,
	.online	= mmconline,
	.rctl	= mmcrctl,
	.bio	= mmcbio,
	.rio	= mmcrio,
};

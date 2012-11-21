/*
 * mmc / sd memory card
 *
 * Copyright © 2012 Richard Miller <r.miller@acm.org>
 *
 * Assumes only one card on the bus
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

typedef struct Ctlr Ctlr;

enum {
	Inittimeout	= 15,
	Multiblock	= 1,

	/* Commands */
	GO_IDLE_STATE	= 0,
	ALL_SEND_CID	= 2,
	SEND_RELATIVE_ADDR= 3,
	SELECT_CARD	= 7,
	SD_SEND_IF_COND	= 8,
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

	/* OCR (operating conditions register) */
	Powerup	= 1<<31,
};

struct Ctlr {
	SDev	*dev;
	SDio	*io;
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

static void
identify(SDunit *unit, u32int *csd)
{
	uint csize, mult;

	unit->secsize = 1 << CSD(83, 80);
	switch(CSD(127, 126)){
	case 0:				/* CSD version 1 */
		csize = CSD(73, 62);
		mult = CSD(49, 47);
		unit->sectors = (csize+1) * (1<<(mult+2));
		break;
	case 1:				/* CSD version 2 */
		csize = CSD(69, 48);
		unit->sectors = (csize+1) * 512LL*KiB / unit->secsize;
		break;
	}
	if(unit->secsize == 1024){
		unit->sectors <<= 1;
		unit->secsize = 512;
	}
}

static SDev*
mmcpnp(void)
{
	SDev *sdev;
	Ctlr *ctl;

	if(sdio.init() < 0)
		return nil;
	sdev = malloc(sizeof(SDev));
	if(sdev == nil)
		return nil;
	ctl = malloc(sizeof(Ctlr));
	if(ctl == nil){
		free(sdev);
		return nil;
	}
	sdev->idno = 'M';
	sdev->ifc = &sdmmcifc;
	sdev->nunit = 1;
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

	ctl = unit->dev->ctlr;
	n = ctl->io->inquiry((char*)&unit->inquiry[8], sizeof(unit->inquiry)-8);
	if(n < 0)
		return 0;
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
	int hcs, i;
	u32int r[4];
	Ctlr *ctl;
	SDio *io;

	ctl = unit->dev->ctlr;
	io = ctl->io;
	assert(unit->subno == 0);

	if(waserror()){
		unit->sectors = 0;
		return 0;
	}
	if(unit->sectors != 0){
		io->cmd(SEND_STATUS, ctl->rca<<Rcashift, r);
		poperror();
		return 1;
	}
	io->cmd(GO_IDLE_STATE, 0, r);
	hcs = 0;
	if(!waserror()){
		io->cmd(SD_SEND_IF_COND, Voltage|Checkpattern, r);
		if(r[0] == (Voltage|Checkpattern))	/* SD 2.0 or above */
			hcs = Hcs;
		poperror();
	}
	for(i = 0; i < Inittimeout; i++){
		delay(100);
		io->cmd(APP_CMD, 0, r);
		io->cmd(SD_SEND_OP_COND, hcs|V3_3, r);
		if(r[0] & Powerup)
			break;
	}
	if(i == Inittimeout){
		print("sdmmc: card won't power up\n");
		poperror();
		return 2;
	}
	ctl->ocr = r[0];
	io->cmd(ALL_SEND_CID, 0, r);
	memmove(ctl->cid, r, sizeof ctl->cid);
	io->cmd(SEND_RELATIVE_ADDR, 0, r);
	ctl->rca = r[0]>>16;
	io->cmd(SEND_CSD, ctl->rca<<Rcashift, r);
	memmove(ctl->csd, r, sizeof ctl->csd);
	identify(unit, ctl->csd);
	io->cmd(SELECT_CARD, ctl->rca<<Rcashift, r);
	io->cmd(SET_BLOCKLEN, unit->secsize, r);
	io->cmd(APP_CMD, ctl->rca<<Rcashift, r);
	io->cmd(SET_BUS_WIDTH, Width4, r);
	poperror();
	return 1;
}

static int
mmcrctl(SDunit *unit, char *p, int l)
{
	Ctlr *ctl;
	int i, n;

	ctl = unit->dev->ctlr;
	assert(unit->subno == 0);
	if(unit->sectors == 0){
		mmconline(unit);
		if(unit->sectors == 0)
			return 0;
	}
	n = snprint(p, l, "rca %4.4ux ocr %8.8ux\ncid ", ctl->rca, ctl->ocr);
	for(i = nelem(ctl->cid)-1; i >= 0; i--)
		n += snprint(p+n, l-n, "%8.8ux", ctl->cid[i]);
	n += snprint(p+n, l-n, " csd ");
	for(i = nelem(ctl->csd)-1; i >= 0; i--)
		n += snprint(p+n, l-n, "%8.8ux", ctl->csd[i]);
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
	SDio *io;

	USED(lun);
	ctl = unit->dev->ctlr;
	io = ctl->io;
	assert(unit->subno == 0);
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
		io->iosetup(write, buf, len, nb);
		if(waserror()){
			io->cmd(STOP_TRANSMISSION, 0, r);
			nexterror();
		}
		io->cmd(write? WRITE_MULTIPLE_BLOCK: READ_MULTIPLE_BLOCK,
			ctl->ocr & Ccs? b: b * len, r);
		io->io(write, buf, nb * len);
		poperror();
		io->cmd(STOP_TRANSMISSION, 0, r);
		poperror();
		b += nb;
	}else{
		for(b = bno; b < bno + nb; b++){
			io->iosetup(write, buf, len, 1);
			io->cmd(write? WRITE_BLOCK : READ_SINGLE_BLOCK,
				ctl->ocr & Ccs? b: b * len, r);
			io->io(write, buf, len);
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

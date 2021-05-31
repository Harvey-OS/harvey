#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "usb.h"
#include "serial.h"

enum {
	ReadVersion	= 0x5F,
	ReadReg		= 0x95,
	WriteReg	= 0x9A,
	SerialInit	= 0xA1,
	ModemCtrl	= 0xA4,

	LcrEnableRx	= 0x80,
	LcrEnableTx	= 0x40,
	LcrMarkSpace	= 0x20,
	LcrParEven	= 0x10,
	LcrEnablePar	= 0x08,
	LcrStopBits2	= 0x04,

	LcrCS8		= 0x03,
	LcrCS7		= 0x02,
	LcrCS6		= 0x01,
	LcrCS5		= 0x00,
};

static Cinfo chinfo[] = {
	{ 0x4348, 0x5523, },
	{ 0x1a86, 0x7523, },
	{ 0x1a86, 0x5523, },
	{ 0,	0, },
};

static Serialops chops;

int
chprobe(Serial *ser)
{
	Usbdev *ud = ser->dev->usb;

	if(matchid(chinfo, ud->vid, ud->did) == nil)
		return -1;
	ser->Serialops = chops;
	return 0;
}

static int
chin(Serialport *p, uint req, uint val, uint index, void *buf, int len)
{
	Serial *ser;

	ser = p->s;
	return usbcmd(ser->dev, Rd2h | Rvendor | Rdev, req, val, index, buf, len);
}

static int
chout(Serialport *p, uint req, uint val, uint index)
{
	Serial *ser;

	ser = p->s;
	return usbcmd(ser->dev, Rh2d | Rvendor | Rdev, req, val, index, nil, 0);
}


static int
chinit(Serialport *p)
{
	Serial *ser;
	uchar ver[2];

	ser = p->s;
	dsprint(2, "chinit\n");

	if(chin(p, ReadVersion, 0, 0, ver, sizeof(ver)) < 0)
		return -1;

	dsprint(2, "ch340: chip version: %ux.%ux\n", ver[0], ver[1]);

	if(chout(p, SerialInit, 0, 0) < 0)
		return -1;

	p->baud = 115200;
	p->stop = 1;
	p->bits = 8;

	chops.setparam(p);

	/* p gets freed by closedev, the process has a reference */
	incref(ser->dev);
	return 0;
}

static int
chsetbaud(Serialport *p)
{
	uint factor, divisor, a;

	factor = 1532620800 / p->baud;
	divisor = 3;

	while((factor > 0xfff0) && divisor) {
		factor /= 8;
		divisor--;
	}

	if(factor > 0xfff0)
		return -1;

	factor = 0x10000 - factor;
	a = (factor & 0xFF00) | divisor;
	a |= 1<<7;
	return chout(p, WriteReg, 0x1312, a);
}

static int
chsetparam(Serialport *p)
{
	uint lcr;

	if(p->baud > 0)
		chsetbaud(p);

	lcr = LcrEnableRx | LcrEnableTx;

	switch(p->bits){
	case 5:
		lcr |= LcrCS5;
		break;
	case 6:
		lcr |= LcrCS6;
		break;
	case 7:
		lcr |= LcrCS7;
		break;
	case 8:
	default:
		lcr |= LcrCS8;
		break;
	}

	switch(p->parity){
	case 0:
	default:
		break;
	case 1:
		lcr |= LcrEnablePar;
		break;
	case 2:
		lcr |= LcrEnablePar|LcrParEven;
		break;
	case 3:
		lcr |= LcrEnablePar|LcrMarkSpace;
		break;
	case 4:
		lcr |= LcrEnablePar|LcrParEven|LcrMarkSpace;
		break;
	};

	if(p->stop == 2)
		lcr |= LcrStopBits2;

	return chout(p, WriteReg, 0x2518, lcr);
}

static Serialops chops = {
	.init		= chinit,
	.setparam	= chsetparam,
};

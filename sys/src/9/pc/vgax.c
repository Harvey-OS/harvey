#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

static Lock vgaxlock;			/* access to index registers */

int
vgaxi(long port, uchar index)
{
	uchar data;

	ilock(&vgaxlock);
	switch(port){

	case Seqx:
	case Crtx:
	case Grx:
		outb(port, index);
		data = inb(port+1);
		break;

	case Attrx:
		/*
		 * Allow processor access to the colour
		 * palette registers. Writes to Attrx must
		 * be preceded by a read from Status1 to
		 * initialise the register to point to the
		 * index register and not the data register.
		 * Processor access is allowed by turning
		 * off bit 0x20.
		 */
		inb(Status1);
		if(index < 0x10){
			outb(Attrx, index);
			data = inb(Attrx+1);
			inb(Status1);
			outb(Attrx, 0x20|index);
		}
		else{
			outb(Attrx, 0x20|index);
			data = inb(Attrx+1);
		}
		break;

	default:
		iunlock(&vgaxlock);
		return -1;
	}
	iunlock(&vgaxlock);

	return data & 0xFF;
}

int
vgaxo(long port, uchar index, uchar data)
{
	ilock(&vgaxlock);
	switch(port){

	case Seqx:
	case Crtx:
	case Grx:
		/*
		 * We could use an outport here, but some chips
		 * (e.g. 86C928) have trouble with that for some
		 * registers.
		 */
		outb(port, index);
		outb(port+1, data);
		break;

	case Attrx:
		inb(Status1);
		if(index < 0x10){
			outb(Attrx, index);
			outb(Attrx, data);
			inb(Status1);
			outb(Attrx, 0x20|index);
		}
		else{
			outb(Attrx, 0x20|index);
			outb(Attrx, data);
		}
		break;

	default:
		iunlock(&vgaxlock);
		return -1;
	}
	iunlock(&vgaxlock);

	return 0;
}

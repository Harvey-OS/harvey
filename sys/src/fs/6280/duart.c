#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"mem.h"
#include	"io.h"

#define	PAD	3	/* registers are well-spaced */

typedef struct Duart Duart;

/*
 * Register set for half the duart.  There are really two sets.
 */
struct
Duart
{
	uchar	mr1_2,		/* Mode Register Channels 1 & 2 */
		pad0[PAD];
	uchar	sr_csr,		/* Status Register/Clock Select Register */
		pad1[PAD];
	uchar	cmnd,		/* Command Register */
		pad2[PAD];
	uchar	data,		/* RX Holding / TX Holding Register */
		pad3[PAD];
	uchar	ipc_acr,	/* Input Port Change/Aux. Control Register */
		pad4[PAD];
	uchar	is_imr,		/* Interrupt Status/Interrupt Mask Register */
		pad5[PAD];
	uchar	ctur,		/* Counter/Timer Upper Register */
		pad6[PAD];
	uchar	ctlr,		/* Counter/Timer Lower Register */
		pad7[PAD];
};
#define	ppcr	is_imr		/* in the second register set */

#define DBD75		0
#define DBD110		1
#define DBD38400	2
#define DBD150		3
#define DBD300		4
#define DBD600		5
#define DBD1200		6
#define DBD2000		7
#define DBD2400		8
#define DBD4800		9
#define DBD1800		10
#define DBD9600		11
#define DBD19200	12

enum
{
	CHAR_ERR	=0x00,	/* MR1x - Mode Register 1 */
	EVEN_PAR	=0x00,
	ODD_PAR		=0x04,
	NO_PAR		=0x10,
	CBITS8		=0x03,
	CBITS7		=0x02,
	CBITS6		=0x01,
	CBITS5		=0x00,
	NORM_OP		=0x00,	/* MR2x - Mode Register 2 */
	TWOSTOPB	=0x0F,
	ONESTOPB	=0x07,
	ENB_RX		=0x01,	/* CRx - Command Register */
	DIS_RX		=0x02,
	ENB_TX		=0x04,
	DIS_TX		=0x08,
	RESET_MR 	=0x10,
	RESET_RCV  	=0x20,
	RESET_TRANS  	=0x30,
	RESET_ERR  	=0x40,
	RESET_BCH	=0x50,
	STRT_BRK	=0x60,
	STOP_BRK	=0x70,
	RCV_RDY		=0x01,	/* SRx - Channel Status Register */
	FIFOFULL	=0x02,
	XMT_RDY		=0x04,
	XMT_EMT		=0x08,
	OVR_ERR		=0x10,
	PAR_ERR		=0x20,
	FRM_ERR		=0x40,
	RCVD_BRK	=0x80,
	IM_IPC		=0x80,	/* IMRx/ISRx - Interrupt Mask/Interrupt Status */
	IM_DBB		=0x40,
	IM_RRDYB	=0x20,
	IM_XRDYB	=0x10,
	IM_CRDY		=0x08,
	IM_DBA		=0x04,
	IM_RRDYA	=0x02,
	IM_XRDYA	=0x01,
};

void
duartinit(void)
{
	Duart *duart;

	duart = DUARTREG;

	duart->cmnd = RESET_RCV|DIS_TX|DIS_RX;
	duart->cmnd = RESET_TRANS;
	duart->cmnd = RESET_ERR;
	duart->cmnd = STOP_BRK;

	duart->ipc_acr = 0x80;		/* baud-rate set 2 */
	duart->cmnd = RESET_MR;
	duart->mr1_2 = NO_PAR|CBITS8;
	duart->mr1_2 = ONESTOPB;
	duart->sr_csr = (DBD9600<<4)|DBD9600;
	duart->is_imr = IM_RRDYA|IM_XRDYA;
	duart->cmnd = ENB_TX|ENB_RX;

	setioavector(DUARTIOAVEC, DUARTIE);

	intrclr(DUARTIE);
	SBCREG->intmask |= DUARTIE;
	*IOAERRREG = (1<<15);
}

void
duartreset(void)
{
	DUARTREG->cmnd = ENB_TX|ENB_RX;
}

void
duartintr(void)
{
	int cause, status, c;
	Duart *duart;

	duart = DUARTREG;
	cause = duart->is_imr;
	/*
	 * I can guess your interrupt.
	 * Is it 1?
	 */
	if(cause & IM_RRDYA) {
		/* keyboard input interrupt */
		status = duart->sr_csr;
		c = duart->data;
		if(status & (FRM_ERR|OVR_ERR|PAR_ERR))
			duart->cmnd = RESET_ERR;
		kbdchar(c & 0x7f);
	}
	/*
	 * Is it 2?
	 */
	if(cause & IM_XRDYA) {
		c = conschar();
		if(c == -1)
			duart->cmnd = DIS_TX;
		else
			duartxmit(c);
	}
	*IOAERRREG = (1<<15);
}

void
duartxmit(int c)
{
	Duart *duart;
	int i;

	duart = DUARTREG;
	duart->cmnd = ENB_TX|ENB_RX;
	for(i=0; i<100; i++) {
		if(duart->sr_csr & XMT_RDY)
			break;
		delay(1);
	}
	duart->data = c;
}

int
duartrecv(void)
{
	Duart *duart;

	duart = DUARTREG;
	duart->cmnd = ENB_TX|ENB_RX;
	if(duart->sr_csr & RCV_RDY)
		return duart->data & 0xFF;
	return 0;
}

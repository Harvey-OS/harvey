enum {
	IRQtimer0	= 0,
	IRQtimer1	= 1,
	IRQtimer2	= 2,
	IRQtimer3	= 3,
	IRQclock	= IRQtimer3,
	IRQusb		= 9,
	IRQdma0		= 16,
#define IRQDMA(chan)	(IRQdma0+(chan))
	IRQaux		= 29,
	IRQmmc		= 62,

	IRQbasic	= 64,
	IRQtimerArm	= IRQbasic + 0,

	IRQfiq		= IRQusb,	/* only one source can be FIQ */

	DmaD2M		= 0,		/* device to memory */
	DmaM2D		= 1,		/* memory to device */
	DmaM2M		= 2,		/* memory to memory */

	DmaChanEmmc	= 4,		/* can only use 2-5, maybe 0 */
	DmaDevEmmc	= 11,

	PowerSd		= 0,
	PowerUart0,
	PowerUart1,
	PowerUsb,
	PowerI2c0,
	PowerI2c1,
	PowerI2c2,
	PowerSpi,
	PowerCcp2tx,

	ClkEmmc		= 1,
	ClkUart,
	ClkArm,
	ClkCore,
	ClkV3d,
	ClkH264,
	ClkIsp,
	ClkSdram,
	ClkPixel,
	ClkPwm,
};

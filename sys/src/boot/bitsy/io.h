/*
 *  Definitions for IO devices.  Used only in C.
 */

enum
{
	/* hardware counter frequency */
	ClockFreq=	3686400,
};

/*
 *  IRQ's defined by SA1100
 */
enum
{
	IRQgpio0=	0,
	IRQgpio1=	1,
	IRQgpio2=	2,
	IRQgpio3=	3,
	IRQgpio4=	4,
	IRQgpio5=	5,
	IRQgpio6=	6,
	IRQgpio7=	7,
	IRQgpio8=	8,
	IRQgpio9=	9,
	IRQgpio10=	10,
	IRQgpiohi=	11,
	IRQlcd=		12,
	IRQudc=		13,
	IRQuart1b=	15,
	IRQuart2=	16,
	IRQuart3=	17,
	IRQmcp=		18,
	IRQssp=		19,
	IRQdma0=	20,
	IRQdma1=	21,
	IRQdma2=	22,
	IRQdma3=	23,
	IRQdma4=	24,
	IRQdma5=	25,
	IRQtimer0=	26,
	IRQtimer1=	27,
	IRQtimer2=	28,
	IRQtimer3=	29,
	IRQsecond=	30,
	IRQrtc=		31,
};

/*
 *  GPIO lines (signal names from compaq document).  _i indicates input
 *  and _o output.
 */
enum
{
	GPIO_PWR_ON_i=		1<<0,	/* power button */
	GPIO_UP_IRQ_i=		1<<1,	/* microcontroller interrupts */
	GPIO_LDD8_o=		1<<2,	/* LCD data 8-15 */
	GPIO_LDD9_o=		1<<3,
	GPIO_LDD10_o=		1<<4,
	GPIO_LDD11_o=		1<<5,
	GPIO_LDD12_o=		1<<6,
	GPIO_LDD13_o=		1<<7,
	GPIO_LDD14_o=		1<<8,
	GPIO_LDD15_o=		1<<9,
	GPIO_CARD_IND1_i=	1<<10,	/* card inserted in PCMCIA socket 1 */
	GPIO_CARD_IRQ1_i=	1<<11,	/* PCMCIA socket 1 interrupt */
	GPIO_CLK_SET0_o=	1<<12,	/* clock selects for audio codec */
	GPIO_CLK_SET1_o=	1<<13,
	GPIO_L3_SDA_io=		1<<14,	/* UDA1341 interface */
	GPIO_L3_MODE_o=		1<<15,
	GPIO_L3_SCLK_o=		1<<16,
	GPIO_CARD_IND0_i=	1<<17,	/* card inserted in PCMCIA socket 0 */
	GPIO_KEY_ACT_i=		1<<18,	/* hot key from cradle */
	GPIO_SYS_CLK_i=		1<<19,	/* clock from codec */
	GPIO_BAT_FAULT_i=	1<<20,	/* battery fault */
	GPIO_CARD_IRQ0_i=	1<<21,	/* PCMCIA socket 0 interrupt */
	GPIO_LOCK_i=		1<<22,	/* expansion pack lock/unlock */
	GPIO_COM_DCD_i=		1<<23,	/* DCD from UART3 */
	GPIO_OPT_IRQ_i=		1<<24,	/* expansion pack IRQ */
	GPIO_COM_CTS_i=		1<<25,	/* CTS from UART3 */
	GPIO_COM_RTS_o=		1<<26,	/* RTS to UART3 */
	GPIO_OPT_IND_i=		1<<27,	/* expansion pack inserted */

/* Peripheral Unit GPIO pin assignments: alternate functions */
	GPIO_SSP_TXD_o=		1<<10,	/* SSP Transmit Data */
	GPIO_SSP_RXD_i=		1<<11,	/* SSP Receive Data */
	GPIO_SSP_SCLK_o=	1<<12,	/* SSP Sample CLocK */
	GPIO_SSP_SFRM_o=	1<<13,	/* SSP Sample FRaMe */
	/* ser. port 1: */
	GPIO_UART_TXD_o=	1<<14,	/* UART Transmit Data */
	GPIO_UART_RXD_i=	1<<15,	/* UART Receive Data */
	GPIO_SDLC_SCLK_io=	1<<16,	/* SDLC Sample CLocK (I/O) */
	GPIO_SDLC_AAF_o=	1<<17,	/* SDLC Abort After Frame */
	GPIO_UART_SCLK1_i=	1<<18,	/* UART Sample CLocK 1 */
	/* ser. port 4: */
	GPIO_SSP_CLK_i=		1<<19,	/* SSP external CLocK */
	/* ser. port 3: */
	GPIO_UART_SCLK3_i=	1<<20,	/* UART Sample CLocK 3 */
	/* ser. port 4: */
	GPIO_MCP_CLK_i=		1<<21,	/* MCP CLocK */
	/* test controller: */
	GPIO_TIC_ACK_o=		1<<21,	/* TIC ACKnowledge */
	GPIO_MBGNT_o=		1<<21,	/* Memory Bus GraNT */
	GPIO_TREQA_i=		1<<22,	/* TIC REQuest A */
	GPIO_MBREQ_i=		1<<22,	/* Memory Bus REQuest */
	GPIO_TREQB_i=		1<<23,	/* TIC REQuest B */
	GPIO_1Hz_o=			1<<25,	/* 1 Hz clock */
	GPIO_RCLK_o=		1<<26,	/* internal (R) CLocK (O, fcpu/2) */
	GPIO_32_768kHz_o=	1<<27,	/* 32.768 kHz clock (O, RTC) */
};

/*
 *  types of interrupts
 */
enum
{
	GPIOrising,
	GPIOfalling,
	GPIOboth,
	IRQ,
};

/* hardware registers */
typedef struct Uartregs Uartregs;
struct Uartregs
{
	ulong	ctl[4];
	ulong	dummya;
	ulong	data;
	ulong	dummyb;
	ulong	status[2];
};
Uartregs *uart3regs;

/* general purpose I/O lines control registers */
typedef struct GPIOregs GPIOregs;
struct GPIOregs
{
	ulong	level;		/* 1 == high */
	ulong	direction;	/* 1 == output */
	ulong	set;		/* a 1 sets the bit, 0 leaves it alone */
	ulong	clear;		/* a 1 clears the bit, 0 leaves it alone */
	ulong	rising;		/* rising edge detect enable */
	ulong	falling;	/* falling edge detect enable */
	ulong	edgestatus;	/* writing a 1 bit clears */
	ulong	altfunc;	/* turn on alternate function for any set bits */
};

extern GPIOregs *gpioregs;

/* extra general purpose I/O bits, output only */
enum
{
	EGPIO_prog_flash=	1<<0,
	EGPIO_pcmcia_reset=	1<<1,
	EGPIO_exppack_reset=	1<<2,
	EGPIO_codec_reset=	1<<3,
	EGPIO_exp_nvram_power=	1<<4,
	EGPIO_exp_full_power=	1<<5,
	EGPIO_lcd_3v=		1<<6,
	EGPIO_rs232_power=	1<<7,
	EGPIO_lcd_ic_power=	1<<8,
	EGPIO_ir_power=		1<<9,
	EGPIO_audio_power=	1<<10,
	EGPIO_audio_ic_power=	1<<11,
	EGPIO_audio_mute=	1<<12,
	EGPIO_fir=		1<<13,	/* not set is sir */
	EGPIO_lcd_5v=		1<<14,
	EGPIO_lcd_9v=		1<<15,
};
extern ulong *egpioreg;

/* Peripheral pin controller registers */
typedef struct PPCregs PPCregs;
struct PPCregs {
	ulong	direction;
	ulong	state;
	ulong	assignment;
	ulong	sleepdir;
	ulong	flags;
};
extern PPCregs *ppcregs;

/* Synchronous Serial Port controller registers */
typedef struct SSPregs SSPregs;
struct SSPregs {
	ulong	control0;
	ulong	control1;
	ulong	dummy0;
	ulong	data;
	ulong	dummy1;
	ulong	status;
};
extern SSPregs *sspregs;

/* Multimedia Communications Port controller registers */
typedef struct MCPregs MCPregs;
struct MCPregs {
	ulong	control0;
	ulong	reserved0;
	ulong	data0;
	ulong	data1;
	ulong	data2;
	ulong	reserved1;
	ulong	status;
	ulong	reserved[11];
	ulong	control1;
};
extern MCPregs *mcpregs;

/*
 *  memory configuration
 */
enum
{
	/* bit shifts for pcmcia access time counters */
	MECR_io0=	0,
	MECR_attr0=	5,
	MECR_mem0=	10,
	MECR_fast0=	11,
	MECR_io1=	MECR_io0+16,
	MECR_attr1=	MECR_attr0+16,
	MECR_mem1=	MECR_mem0+16,
	MECR_fast1=	MECR_fast0+16,
};

typedef struct MemConfRegs MemConfRegs;
struct MemConfRegs
{
	ulong	mdcnfg;		/* dram */
	ulong	mdcas00;	/* dram banks 0/1 */
	ulong	mdcas01;
	ulong	mdcas02;
	ulong	msc0;		/* static */
	ulong	msc1;
	ulong	mecr;		/* pcmcia */
	ulong	mdrefr;		/* dram refresh */
	ulong	mdcas20;	/* dram banks 2/3 */
	ulong	mdcas21;
	ulong	mdcas22;
	ulong	msc2;		/* static */
	ulong	smcnfg;		/* SMROM config */
};
extern MemConfRegs *memconfregs;

/*
 *  power management
 */
typedef struct PowerRegs PowerRegs;
struct PowerRegs
{
	ulong	pmcr;	/* Power manager control register */
	ulong	pssr;	/* Power manager sleep status register */
	ulong	pspr;	/* Power manager scratch pad register */
	ulong	pwer;	/* Power manager wakeup enable register */
	ulong	pcfr;	/* Power manager general configuration register */
	ulong	ppcr;	/* Power manager PPL configuration register */
	ulong	pgsr;	/* Power manager GPIO sleep state register */
	ulong	posr;	/* Power manager oscillator status register */
};
extern PowerRegs *powerregs;

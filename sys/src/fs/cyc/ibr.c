/*
 * boot data structures - Link this so the ibr is at 0xF0C3ff00
 */

#define	Romentry	0xf0c00048		/* Start of cyclone rom area */

void
main(void)
{
}

#define BURST_ENABLE		0x1
#define BURST_DISABLE		0x0
#define READY_ENABLE		0x2
#define READY_DISABLE		0x0
#define PIPELINE_ENABLE		0x4
#define PIPELINE_DISABLE	0x0

#define BUS_WIDTH_8		0x0
#define BUS_WIDTH_16		(0x1<<19)
#define BUS_WIDTH_32		(0x2<<19)

#define BIG_ENDIAN		(0x1<<22)
#define LITTLE_ENDIAN		0x0

#define NRAD(WS)		(WS<<3)   /* WS can be 0-31   */
#define NRDD(WS)		(WS<<8)   /* WS can be 0-3    */
#define NXDA(WS)		(WS<<10)  /* WS can be 0-3    */
#define NWAD(WS)		(WS<<12)  /* WS can be 0-31   */
#define NWDD(WS)		(WS<<17)  /* WS can be 0-3    */

/*
 * Region values
 */
#define DRAM		(BURST_ENABLE | READY_ENABLE | PIPELINE_DISABLE | \
				NRAD(0) | NRDD(0) | NXDA(1) | NWAD(0) | \
				NWDD(0) | BUS_WIDTH_32 | LITTLE_ENDIAN)
#define SRAM		(BURST_ENABLE | READY_DISABLE | PIPELINE_ENABLE | \
				NRAD(1) | NRDD(1) | NXDA(0) | NWAD(2) | \
				NWDD(2) | BUS_WIDTH_32 | LITTLE_ENDIAN)
#define VME_D32		(BURST_DISABLE | READY_ENABLE | PIPELINE_DISABLE | \
				NRAD(0) | NRDD(0) | NXDA(0) | NWAD(0) | \
				NWDD(0) | BUS_WIDTH_32 | LITTLE_ENDIAN)
#define VME_D16		(BURST_DISABLE | READY_ENABLE | PIPELINE_DISABLE | \
				NRAD(0) | NRDD(0) | NXDA(0) | NWAD(0) | \
				NWDD(0) | BUS_WIDTH_16 | LITTLE_ENDIAN)
#define SCC		(BURST_DISABLE | READY_ENABLE | PIPELINE_DISABLE | \
				NRAD(0) | NRDD(0) | NXDA(2) | NWAD(0) | \
				NWDD(0) | BUS_WIDTH_8 | LITTLE_ENDIAN)
#define EPROM		(BURST_DISABLE | READY_DISABLE | PIPELINE_DISABLE | \
				NRAD(10) | NRDD(0) | NXDA(2) | NWAD(10) | \
				NWDD(0) | BUS_WIDTH_8 | LITTLE_ENDIAN)

#define BYTE0(data)	(data & 0x000000FF)
#define BYTE1(data)	((data & 0x0000FF00) >> 8)
#define BYTE2(data)	((data & 0x00FF0000) >> 16)
#define BYTE3(data)	((data & 0xFF000000) >> 24)

/* data structures required to boot processor */

unsigned long boot[] = 
{
	BYTE0(EPROM),		/* 0xffffff00 */
	BYTE1(EPROM),
	BYTE2(EPROM),
	BYTE3(EPROM),
	0xf0c00048,		/* Jump to start of rom */
	0xffffff30,		/* Link to prcb */
	-2,
	0,
	0,
	0,
	0,
	0xf400088,

	/* prcb 		   0xffffff30 */
	0,			/* adr of fault table (ram)	*/
	0xffffff60,		/* adr of control_table ctltab	*/
	0x00001000,		/* AC reg mask overflow fault	*/
	0x40000001,		/* Flt - Mask Unaligned fault	*/
	0,			/* Interrupt Table Address	*/
	0,			/* System Procedure Table	*/
	0,			/* Reserved			*/
	0,			/* Interrupt Stack Pointer	*/
	0x00000000,		/* Inst. Cache - enable cache	*/
	15,			/* Reg. Cache: 15 sets cached	*/
	0,			/* pad */
	0,			/* pad */

	/* ctltab 		   0xffffff60 */
	0,			/* IPB0 IP Breakpoint Reg 0 */
	0,			/* IPB1 IP Breakpoint Reg 1 */
	0,			/* DAB0 Data Adr Bkpt Reg 0 */
	0,			/* DAB1 Data Adr Bkpt Reg 1 */

	/* Interrupt Map Registers */
	0,			/* IMAP0 Interrupt Map Reg 0 */
	0,			/* IMAP1 Interrupt Map Reg 1 */
	0,			/* IMAP2 Interrupt Map Reg 2 */
	0x400,			/* ICON Reg (initially disabled) */

	/* Bus Configuration Registers */
	DRAM,			/* Region  0 */
	VME_D32,		/* Region  1 - VME, 32 data */
	VME_D32,		/* Region  2 */
	VME_D32,		/* Region  3 */

	VME_D32,		/* Region  4 */
	VME_D32,		/* Region  5 */
	VME_D32,		/* Region  6 */
	VME_D32,		/* Region  7 */

	VME_D32,		/* Region  8 */
	VME_D32,		/* Region  9 */
	VME_D32,		/* Region 10 */
	VME_D16,		/* Region 11 */

	0x00100000,		/* Region 12 SQUALL_1 */
	0x00104820,		/* Region 13 SQUALL_2 */
	SCC,			/* Region 14 */
	EPROM,			/* Region 15 */

	0,			/* Reserved			*/
	0,			/* BPCON (BreakPoint CONtrol)	*/
	0,			/* TC (Trace Control)		*/
	0x00000001		/* BCON (Bus CONtrol)		*/
};

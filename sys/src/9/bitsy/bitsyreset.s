#include "mem.h"

	// Bitsy development board uses two banks: KM416S4030C,
	// 12 row address bits, 8 col address bits
	// Bitsy uses two banks KM416S8030C, 12 row address bits,
	// 9 col address bits
    // Have to set DRAC0 to 14 row bits or else you only get 8 col bits
	// from the formfactor unit configuration registers:	 0xF3536257
mdcnfg:     // DRAM Configuration Register 10.2.1
	WORD	1<<0 | 1<<2 | 0<<3 | 0x5<<4 | 0x3<<8 | 3<<12 | 3<<14
mdrefr0:		// DRAM Refresh Control Register 10.2.2
	WORD	1<<0 | 0x200<<4 | 1<<21 | 1<<22 | 1 <<31
mdrefr1:		// DRAM Refresh Control Register 10.2.2
	WORD	1<<0 | 0x200<<4 | 1<<21 | 1<<22
mdrefr2:		// DRAM Refresh Control Register 10.2.2
	WORD	1<<0 | 0x200<<4 | 1<<20 | 1<<21 | 1<<22

	/* MDCAS settings from [1] Table 10-3 (page 10-18) */
waveform0:
	WORD	0xAAAAAAA7
waveform1:
	WORD	0xAAAAAAAA
waveform2:
	WORD	0xAAAAAAAA

delay:		// delay without using memory
	mov	$100, r1	// 200MHz: 100 × (2 instructions @ 5 ns) == 1 ms
l1:
	sub $1, r1
	bgt l1
	sub $1, r0
	bgt delay
	ret
	
reset:
	mov	$INTREGS+4, r0		// turn off interrupts
	mov	$0, (r0)

	// Is this necessary on wakeup?
	mov	$POWERREGS+14, r0	// set clock speed to 191.7MHz
	mov	$0xb, (r0)

	// This is necessary on hard reset, but not on sleep reset
	mov	$0x80, r0			// wait ±128 µs
	bl	delay

	/* check to see if we're operating out of DRAM */
	bic	$0x000000ff, pc, r4
	bic	$0x0000ff00, r4
	bic	$0x00ff0000, r4
	cmp	r4, $PHYSDRAM0
	beq	dram

dramwakeup:

	mov	$POWERREGS+0x4, r1	// Clear DH in Power Manager Sleep Status Register
	bic $(1<<3), (r1)		// DH == DRAM Hold
	// This releases nCAS/DQM and nRAS/nSDCS pins to make DRAM exit selfrefresh

	/* Set up the DRAM in banks 0 and 1 [1] 10.3 */
	mov	$MEMCONFREGS, r1

	mov	mdrefr0, r2		// Turn on K1RUN
	mov	r2, 0x1c(r1)

	mov	mdrefr1, r2		// Turn off SLFRSH
	mov	r2, 0x1c(r1)

	mov	mdrefr2, r2		// Turn on E1PIN
	mov	r2, 0x1c(r1)

    mov	waveform0, r2
	mov	r2, 0x4(r1)

	mov	waveform1, r2
	mov	r2, 0x8(r1)

	mov	waveform2, r2
	mov	r2, 0xc(r1)

	mov	$PHYSDRAM0, r0	
	mov 0x00(r0), r2	// Eight non-burst read cycles
	mov 0x20(r0), r2
	mov 0x40(r0), r2
	mov 0x60(r0), r2
	mov 0x80(r0), r2
	mov 0xa0(r0), r2
	mov 0xc0(r0), r2
	mov 0xe0(r0), r2

	mov	mdcnfg, r2		// Enable memory banks
	mov	r2, 0x0(r1)

	// Is there any use in turning on EAPD and KAPD in the MDREFR register?

	ret




dram:



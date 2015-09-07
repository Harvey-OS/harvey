/****************************************************************************
 * YABEL BIOS Emulator
 *
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Copyright (c) 2008 Pattrick Hueper <phueper@hueper.net>
 ****************************************************************************/

/* PMM Structure see PMM Spec Version 1.01 Chapter 3.1.1
 * (search web for specspmm101.pdf)
 */
typedef struct {
	uint8_t signature[4];
	uint8_t struct_rev;
	uint8_t length;
	uint8_t checksum;
	uint32_t entry_point_offset;
	uint8_t reserved[5];
	/* Code is not part of the specced PMM struct, however, since I cannot
	 * put the handling of PMM in the virtual memory (I don't want to hack
	 * it together in x86 assembly ;-)) this code array is pointed to by
	 * entry_point_offset, in code there is only a INT call and a RETF,
	 * thus every PMM call will issue a PMM INT (only defined in YABEL,
	 * see interrupt.c) and the INT Handler will do the actual PMM work.
	 */
	uint8_t code[3];
} __attribute__ ((__packed__)) pmm_information_t;

/* This function is used to setup the PMM struct in virtual memory
 * at a certain offset */
uint8_t pmm_setup(uint16_t segment, uint16_t offset);

/* This is the INT Handler mentioned above, called by my special PMM INT. */
void pmm_handleInt(void);

void pmm_test(void);

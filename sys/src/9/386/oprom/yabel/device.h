/******************************************************************************
 * Copyright (c) 2004, 2008 IBM Corporation
 * Copyright (c) 2008, 2009 Pattrick Hueper <phueper@hueper.net>
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#ifndef DEVICE_LIB_H
#define DEVICE_LIB_H

#include <types.h>
#include <endian.h>
#include "compat/of.h"
#include "debug.h"


// a Expansion Header Struct as defined in Plug and Play BIOS Spec 1.0a Chapter 3.2
typedef struct {
	char signature[4];	// signature
	uint8_t structure_revision;
	uint8_t length;		// in 16 byte blocks
	uint16_t next_header_offset;	// offset to next Expansion Header as 16bit little-endian value, as offset from the start of the Expansion ROM
	uint8_t reserved;
	uint8_t checksum;	// the sum of all bytes of the Expansion Header must be 0
	uint32_t device_id;	// PnP Device ID as 32bit little-endian value
	uint16_t p_manufacturer_string;	//16bit little-endian offset from start of Expansion ROM
	uint16_t p_product_string;	//16bit little-endian offset from start of Expansion ROM
	uint8_t device_base_type;
	uint8_t device_sub_type;
	uint8_t device_if_type;
	uint8_t device_indicators;
	// the following vectors are all 16bit little-endian offsets from start of Expansion ROM
	uint16_t bcv;		// Boot Connection Vector
	uint16_t dv;		// Disconnect Vector
	uint16_t bev;		// Bootstrap Entry Vector
	uint16_t reserved_2;
	uint16_t sriv;		// Static Resource Information Vector
} __attribute__ ((__packed__)) exp_header_struct_t;

// a PCI Data Struct as defined in PCI 2.3 Spec Chapter 6.3.1.2
typedef struct {
	uint8_t signature[4];	// signature, the String "PCIR"
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t reserved;
	uint16_t pci_ds_length;	// PCI Data Structure Length, 16bit little-endian value
	uint8_t pci_ds_revision;
	uint8_t class_code[3];
	uint16_t img_length;	// length of the Exp.ROM Image, 16bit little-endian value in 512 bytes
	uint16_t img_revision;
	uint8_t code_type;
	uint8_t indicator;
	uint16_t reserved_2;
} __attribute__ ((__packed__)) pci_data_struct_t;

typedef struct {
	uint8_t bus;
	uint8_t devfn;
#if CONFIG_PCI_OPTION_ROM_RUN_YABEL
	struct device* dev;
#else
	u64 puid;
	phandle_t phandle;
	ihandle_t ihandle;
#endif
	// store the address of the BAR that is used to simulate
	// legacy VGA memory accesses
	u64 vmem_addr;
	u64 vmem_size;
	// used to buffer I/O Accesses, that do not access the I/O Range of the device...
	// 64k might be overkill, but we can buffer all I/O accesses...
	uint8_t io_buffer[64 * 1024];
	uint16_t pci_vendor_id;
	uint16_t pci_device_id;
	// translated address of the "PC-Compatible" Expansion ROM Image for this device
	unsigned long img_addr;
	uint32_t img_size;	// size of the Expansion ROM Image (read from the PCI Data Structure)
} biosemu_device_t;

typedef struct {
#if CONFIG_PCI_OPTION_ROM_RUN_YABEL
	unsigned long info;
#else
	uint8_t info;
#endif
	uint8_t bus;
	uint8_t devfn;
	uint8_t cfg_space_offset;
	u64 address;
	u64 address_offset;
	u64 size;
} __attribute__ ((__packed__)) translate_address_t;

// array to store address translations for this
// device. Needed for faster address translation, so
// not every I/O or Memory Access needs to call translate_address_dev
// and access the device tree
// 6 BARs, 1 Exp. ROM, 1 Cfg.Space, and 3 Legacy, plus 2 "special"
// translations are supported... this should be enough for
// most devices... for VGA it is enough anyways...
extern translate_address_t translate_address_array[13];

// index of last translate_address_array entry
// set by get_dev_addr_info function
extern uint8_t taa_last_entry;

// add 1:1 mapped memory regions to translation table
void biosemu_add_special_memory(uint32_t start, uint32_t size);

/* the device we are working with... */
extern biosemu_device_t bios_device;

uint8_t biosemu_dev_init(struct device * device);
// NOTE: for dev_check_exprom to work, biosemu_dev_init MUST be called first!
uint8_t biosemu_dev_check_exprom(unsigned long rom_base_addr);

uint8_t biosemu_dev_translate_address(int type, unsigned long * addr);

/* endianness swap functions for 16 and 32 bit words
 * copied from axon_pciconfig.c
 */
static inline void
out32le(void *addr, uint32_t val)
{
#if CONFIG_ARCH_X86 || CONFIG_ARCH_ARM
	*((uint32_t*) addr) = cpu_to_le32(val);
#else
	asm volatile ("stwbrx  %0, 0, %1"::"r" (val), "r"(addr));
#endif
}

static inline uint32_t
in32le(void *addr)
{
	uint32_t val;
#if CONFIG_ARCH_X86 || CONFIG_ARCH_ARM
	val = cpu_to_le32(*((uint32_t *) addr));
#else
	asm volatile ("lwbrx  %0, 0, %1":"=r" (val):"r"(addr));
#endif
	return val;
}

static inline void
out16le(void *addr, uint16_t val)
{
#if CONFIG_ARCH_X86 || CONFIG_ARCH_ARM
	*((uint16_t*) addr) = cpu_to_le16(val);
#else
	asm volatile ("sthbrx  %0, 0, %1"::"r" (val), "r"(addr));
#endif
}

static inline uint16_t
in16le(void *addr)
{
	uint16_t val;
#if CONFIG_ARCH_X86 || CONFIG_ARCH_ARM
	val = cpu_to_le16(*((uint16_t*) addr));
#else
	asm volatile ("lhbrx %0, 0, %1":"=r" (val):"r"(addr));
#endif
	return val;
}

/* debug function, dumps HID1 and HID4 to detect whether caches are on/off */
static inline void
dumpHID(void)
{
	u64 hid;
	//HID1 = 1009
	__asm__ __volatile__("mfspr %0, 1009":"=r"(hid));
	printf("HID1: %016llx\n", hid);
	//HID4 = 1012
	__asm__ __volatile__("mfspr %0, 1012":"=r"(hid));
	printf("HID4: %016llx\n", hid);
}

#endif

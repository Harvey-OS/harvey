/******************************************************************************
 * Copyright (c) 2004, 2008 IBM Corporation
 * Copyright (c) 2009 Pattrick Hueper <phueper@hueper.net>
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#include <types.h>
#include "compat/rtas.h"
#include "compat/time.h"
#include "device.h"
#include "debug.h"
#include <x86emu/x86emu.h>
#include <device/oprom/include/io.h>
#include "io.h"

#include <device/pci.h>
#include <device/pci_ops.h>
#include <device/resource.h>

#include <arch/io.h>

#if CONFIG_YABEL_DIRECTHW
uint8_t my_inb(X86EMU_pioAddr addr)
{
	uint8_t val;

	val = inb(addr);
	DEBUG_PRINTF_IO("inb(0x%04x) = 0x%02x\n", addr, val);

	return val;
}

uint16_t my_inw(X86EMU_pioAddr addr)
{
	uint16_t val;

	val = inw(addr);
	DEBUG_PRINTF_IO("inw(0x%04x) = 0x%04x\n", addr, val);

	return val;
}

uint32_t my_inl(X86EMU_pioAddr addr)
{
	uint32_t val;

	val = inl(addr);
	DEBUG_PRINTF_IO("inl(0x%04x) = 0x%08x\n", addr, val);

	return val;
}

void my_outb(X86EMU_pioAddr addr, uint8_t val)
{
	DEBUG_PRINTF_IO("outb(0x%02x, 0x%04x)\n", val, addr);
	outb(val, addr);
}

void my_outw(X86EMU_pioAddr addr, uint16_t val)
{
	DEBUG_PRINTF_IO("outw(0x%04x, 0x%04x)\n", val, addr);
	outw(val, addr);
}

void my_outl(X86EMU_pioAddr addr, uint32_t val)
{
	DEBUG_PRINTF_IO("outl(0x%08x, 0x%04x)\n", val, addr);
	outl(val, addr);
}

#else

static unsigned int
read_io(void *addr, size_t sz)
{
        unsigned int ret;
	/* since we are using inb instructions, we need the port number as 16bit value */
	uint16_t port = (uint16_t)(uint32_t) addr;

        switch (sz) {
        case 1:
		ret = inb(port);
                break;
        case 2:
		ret = inw(port);
                break;
        case 4:
		ret = inl(port);
                break;
        default:
                ret = 0;
        }

        return ret;
}

static int
write_io(void *addr, unsigned int value, size_t sz)
{
	uint16_t port = (uint16_t)(uint32_t) addr;
        switch (sz) {
	/* since we are using inb instructions, we need the port number as 16bit value */
        case 1:
		outb(value, port);
                break;
        case 2:
		outw(value, port);
                break;
        case 4:
		outl(value, port);
                break;
        default:
                return -1;
        }

        return 0;
}

uint32_t pci_cfg_read(X86EMU_pioAddr addr, uint8_t size);
void pci_cfg_write(X86EMU_pioAddr addr, uint32_t val, uint8_t size);
uint8_t handle_port_61h(void);

uint8_t
my_inb(X86EMU_pioAddr addr)
{
	uint8_t rval = 0xFF;
	unsigned long translated_addr = addr;
	uint8_t translated = biosemu_dev_translate_address(IORESOURCE_IO, &translated_addr);
	if (translated != 0) {
		//translation successful, access Device I/O (BAR or Legacy...)
		DEBUG_PRINTF_IO("%s(%x): access to Device I/O\n", __func__,
				addr);
		//DEBUG_PRINTF_IO("%s(%04x): translated_addr: %llx\n", __func__, addr, translated_addr);
		rval = read_io((void *)translated_addr, 1);
		DEBUG_PRINTF_IO("%s(%04x) Device I/O --> %02x\n", __func__,
				addr, rval);
		return rval;
	} else {
		switch (addr) {
		case 0x61:
			//8254 KB Controller / Timer Port
			// rval = handle_port_61h();
			rval = inb(0x61);
			//DEBUG_PRINTF_IO("%s(%04x) KB / Timer Port B --> %02x\n", __func__, addr, rval);
			return rval;
			break;
		case 0xCFC:
		case 0xCFD:
		case 0xCFE:
		case 0xCFF:
			// PCI Config Mechanism 1 Ports
			return (uint8_t) pci_cfg_read(addr, 1);
			break;
		case 0x0a:
			CHECK_DBG(DEBUG_INTR) {
				X86EMU_trace_on();
			}
			M.x86.debug &= ~DEBUG_DECODE_NOPRINT_F;
			//HALT_SYS();
			// no break, intentional fall-through to default!!
		default:
			DEBUG_PRINTF_IO
			    ("%s(%04x) reading from bios_device.io_buffer\n",
			     __func__, addr);
			rval = *((uint8_t *) (bios_device.io_buffer + addr));
			DEBUG_PRINTF_IO("%s(%04x) I/O Buffer --> %02x\n",
					__func__, addr, rval);
			return rval;
			break;
		}
	}
}

uint16_t
my_inw(X86EMU_pioAddr addr)
{
	unsigned long translated_addr = addr;
	uint8_t translated = biosemu_dev_translate_address(IORESOURCE_IO, &translated_addr);
	if (translated != 0) {
		//translation successful, access Device I/O (BAR or Legacy...)
		DEBUG_PRINTF_IO("%s(%x): access to Device I/O\n", __func__,
				addr);
		//DEBUG_PRINTF_IO("%s(%04x): translated_addr: %llx\n", __func__, addr, translated_addr);
		uint16_t rval;
		if ((translated_addr & (u64) 0x1) == 0) {
			// 16 bit aligned access...
			uint16_t tempval = read_io((void *)translated_addr, 2);
			//little endian conversion
			rval = in16le((void *) &tempval);
		} else {
			// unaligned access, read single bytes, little-endian
			rval = (read_io((void *)translated_addr, 1) << 8)
				| (read_io((void *)(translated_addr + 1), 1));
		}
		DEBUG_PRINTF_IO("%s(%04x) Device I/O --> %04x\n", __func__,
				addr, rval);
		return rval;
	} else {
		switch (addr) {
		case 0xCFC:
		case 0xCFE:
			//PCI Config Mechanism 1
			return (uint16_t) pci_cfg_read(addr, 2);
			break;
		default:
			DEBUG_PRINTF_IO
			    ("%s(%04x) reading from bios_device.io_buffer\n",
			     __func__, addr);
			uint16_t rval =
			    in16le((void *) bios_device.io_buffer + addr);
			DEBUG_PRINTF_IO("%s(%04x) I/O Buffer --> %04x\n",
					__func__, addr, rval);
			return rval;
			break;
		}
	}
}

uint32_t
my_inl(X86EMU_pioAddr addr)
{
	unsigned long translated_addr = addr;
	uint8_t translated = biosemu_dev_translate_address(IORESOURCE_IO, &translated_addr);
	if (translated != 0) {
		//translation successful, access Device I/O (BAR or Legacy...)
		DEBUG_PRINTF_IO("%s(%x): access to Device I/O\n", __func__,
				addr);
		//DEBUG_PRINTF_IO("%s(%04x): translated_addr: %llx\n", __func__, addr, translated_addr);
		uint32_t rval;
		if ((translated_addr & (u64) 0x3) == 0) {
			// 32 bit aligned access...
			uint32_t tempval = read_io((void *) translated_addr, 4);
			//little endian conversion
			rval = in32le((void *) &tempval);
		} else {
			// unaligned access, read single bytes, little-endian
			rval = (read_io((void *)(translated_addr), 1) << 24)
				| (read_io((void *)(translated_addr + 1), 1) << 16)
				| (read_io((void *)(translated_addr + 2), 1) << 8)
				| (read_io((void *)(translated_addr + 3), 1));
		}
		DEBUG_PRINTF_IO("%s(%04x) Device I/O --> %08x\n", __func__,
				addr, rval);
		return rval;
	} else {
		switch (addr) {
		case 0xCFC:
			//PCI Config Mechanism 1
			return pci_cfg_read(addr, 4);
			break;
		default:
			DEBUG_PRINTF_IO
			    ("%s(%04x) reading from bios_device.io_buffer\n",
			     __func__, addr);
			uint32_t rval =
			    in32le((void *) bios_device.io_buffer + addr);
			DEBUG_PRINTF_IO("%s(%04x) I/O Buffer --> %08x\n",
					__func__, addr, rval);
			return rval;
			break;
		}
	}
}

void
my_outb(X86EMU_pioAddr addr, uint8_t val)
{
	unsigned long translated_addr = addr;
	uint8_t translated = biosemu_dev_translate_address(IORESOURCE_IO, &translated_addr);
	if (translated != 0) {
		//translation successful, access Device I/O (BAR or Legacy...)
		DEBUG_PRINTF_IO("%s(%x, %x): access to Device I/O\n",
				__func__, addr, val);
		//DEBUG_PRINTF_IO("%s(%04x): translated_addr: %llx\n", __func__, addr, translated_addr);
		write_io((void *) translated_addr, val, 1);
		DEBUG_PRINTF_IO("%s(%04x) Device I/O <-- %02x\n", __func__,
				addr, val);
	} else {
		switch (addr) {
		case 0xCFC:
		case 0xCFD:
		case 0xCFE:
		case 0xCFF:
			// PCI Config Mechanism 1 Ports
			pci_cfg_write(addr, val, 1);
			break;
		default:
			DEBUG_PRINTF_IO
			    ("%s(%04x,%02x) writing to bios_device.io_buffer\n",
			     __func__, addr, val);
			*((uint8_t *) (bios_device.io_buffer + addr)) = val;
			break;
		}
	}
}

void
my_outw(X86EMU_pioAddr addr, uint16_t val)
{
	unsigned long translated_addr = addr;
	uint8_t translated = biosemu_dev_translate_address(IORESOURCE_IO, &translated_addr);
	if (translated != 0) {
		//translation successful, access Device I/O (BAR or Legacy...)
		DEBUG_PRINTF_IO("%s(%x, %x): access to Device I/O\n",
				__func__, addr, val);
		//DEBUG_PRINTF_IO("%s(%04x): translated_addr: %llx\n", __func__, addr, translated_addr);
		if ((translated_addr & (u64) 0x1) == 0) {
			// little-endian conversion
			uint16_t tempval = in16le((void *) &val);
			// 16 bit aligned access...
			write_io((void *) translated_addr, tempval, 2);
		} else {
			// unaligned access, write single bytes, little-endian
			write_io(((void *) (translated_addr + 1)),
				(uint8_t) ((val & 0xFF00) >> 8), 1);
			write_io(((void *) translated_addr),
				(uint8_t) (val & 0x00FF), 1);
		}
		DEBUG_PRINTF_IO("%s(%04x) Device I/O <-- %04x\n", __func__,
				addr, val);
	} else {
		switch (addr) {
		case 0xCFC:
		case 0xCFE:
			// PCI Config Mechanism 1 Ports
			pci_cfg_write(addr, val, 2);
			break;
		default:
			DEBUG_PRINTF_IO
			    ("%s(%04x,%04x) writing to bios_device.io_buffer\n",
			     __func__, addr, val);
			out16le((void *) bios_device.io_buffer + addr, val);
			break;
		}
	}
}

void
my_outl(X86EMU_pioAddr addr, uint32_t val)
{
	unsigned long translated_addr = addr;
	uint8_t translated = biosemu_dev_translate_address(IORESOURCE_IO, &translated_addr);
	if (translated != 0) {
		//translation successful, access Device I/O (BAR or Legacy...)
		DEBUG_PRINTF_IO("%s(%x, %x): access to Device I/O\n",
				__func__, addr, val);
		//DEBUG_PRINTF_IO("%s(%04x): translated_addr: %llx\n", __func__, addr, translated_addr);
		if ((translated_addr & (u64) 0x3) == 0) {
			// little-endian conversion
			uint32_t tempval = in32le((void *) &val);
			// 32 bit aligned access...
			write_io((void *) translated_addr,  tempval, 4);
		} else {
			// unaligned access, write single bytes, little-endian
			write_io(((void *) translated_addr + 3),
			    (uint8_t) ((val & 0xFF000000) >> 24), 1);
			write_io(((void *) translated_addr + 2),
			    (uint8_t) ((val & 0x00FF0000) >> 16), 1);
			write_io(((void *) translated_addr + 1),
			    (uint8_t) ((val & 0x0000FF00) >> 8), 1);
			write_io(((void *) translated_addr),
			    (uint8_t) (val & 0x000000FF), 1);
		}
		DEBUG_PRINTF_IO("%s(%04x) Device I/O <-- %08x\n", __func__,
				addr, val);
	} else {
		switch (addr) {
		case 0xCFC:
			// PCI Config Mechanism 1 Ports
			pci_cfg_write(addr, val, 4);
			break;
		default:
			DEBUG_PRINTF_IO
			    ("%s(%04x,%08x) writing to bios_device.io_buffer\n",
			     __func__, addr, val);
			out32le((void *) bios_device.io_buffer + addr, val);
			break;
		}
	}
}

uint32_t
pci_cfg_read(X86EMU_pioAddr addr, uint8_t size)
{
	uint32_t rval = 0xFFFFFFFF;
	struct device * dev;
	if ((addr >= 0xCFC) && ((addr + size) <= 0xD00)) {
		// PCI Configuration Mechanism 1 step 1
		// write to 0xCF8, sets bus, device, function and Config Space offset
		// later read from 0xCFC-0xCFF returns the value...
		uint8_t bus, devfn, offs;
		uint32_t port_cf8_val = my_inl(0xCF8);
		if ((port_cf8_val & 0x80000000) != 0) {
			//highest bit enables config space mapping
			bus = (port_cf8_val & 0x00FF0000) >> 16;
			devfn = (port_cf8_val & 0x0000FF00) >> 8;
			offs = (port_cf8_val & 0x000000FF);
			offs += (addr - 0xCFC);	// if addr is not 0xcfc, the offset is moved accordingly
			DEBUG_PRINTF_INTR("%s(): PCI Config Read from device: bus: %02x, devfn: %02x, offset: %02x\n",
				__func__, bus, devfn, offs);
#if CONFIG_YABEL_PCI_ACCESS_OTHER_DEVICES
			dev = dev_find_slot(bus, devfn);
			DEBUG_PRINTF_INTR("%s(): dev_find_slot() returned: %s\n",
				__func__, dev_path(dev));
			if (dev == 0) {
				// fail accesses to non-existent devices...
#else
			dev = bios_device.dev;
			if ((bus != bios_device.bus)
			     || (devfn != bios_device.devfn)) {
				// fail accesses to any device but ours...
#endif
				printf
				    ("%s(): Config read access invalid device! bus: %02x (%02x), devfn: %02x (%02x), offs: %02x\n",
				     __func__, bus, bios_device.bus, devfn,
				     bios_device.devfn, offs);
				SET_FLAG(F_CF);
				HALT_SYS();
				return 0;
			} else {
#if CONFIG_PCI_OPTION_ROM_RUN_YABEL
				switch (size) {
					case 1:
						rval = pci_read_config8(dev, offs);
						break;
					case 2:
						rval = pci_read_config16(dev, offs);
						break;
					case 4:
						rval = pci_read_config32(dev, offs);
						break;
				}
#else
				rval =
				    (uint32_t) rtas_pci_config_read(bios_device.
								    puid, size,
								    bus, devfn,
								    offs);
#endif
				DEBUG_PRINTF_IO
				    ("%s(%04x) PCI Config Read @%02x, size: %d --> 0x%08x\n",
				     __func__, addr, offs, size, rval);
			}
		}
	}
	return rval;
}

void
pci_cfg_write(X86EMU_pioAddr addr, uint32_t val, uint8_t size)
{
	if ((addr >= 0xCFC) && ((addr + size) <= 0xD00)) {
		// PCI Configuration Mechanism 1 step 1
		// write to 0xCF8, sets bus, device, function and Config Space offset
		// later write to 0xCFC-0xCFF sets the value...
		uint8_t bus, devfn, offs;
		uint32_t port_cf8_val = my_inl(0xCF8);
		if ((port_cf8_val & 0x80000000) != 0) {
			//highest bit enables config space mapping
			bus = (port_cf8_val & 0x00FF0000) >> 16;
			devfn = (port_cf8_val & 0x0000FF00) >> 8;
			offs = (port_cf8_val & 0x000000FF);
			offs += (addr - 0xCFC);	// if addr is not 0xcfc, the offset is moved accordingly
			if ((bus != bios_device.bus)
			    || (devfn != bios_device.devfn)) {
				// fail accesses to any device but ours...
				printf
				    ("Config write access invalid! PCI device %x:%x.%x, offs: %x\n",
				     bus, devfn >> 3, devfn & 7, offs);
#if !CONFIG_YABEL_PCI_FAKE_WRITING_OTHER_DEVICES_CONFIG
				HALT_SYS();
#endif
			} else {
#if CONFIG_PCI_OPTION_ROM_RUN_YABEL
				switch (size) {
					case 1:
						pci_write_config8(bios_device.dev, offs, val);
						break;
					case 2:
						pci_write_config16(bios_device.dev, offs, val);
						break;
					case 4:
						pci_write_config32(bios_device.dev, offs, val);
						break;
				}
#else
				rtas_pci_config_write(bios_device.puid,
						      size, bus, devfn, offs,
						      val);
#endif
				DEBUG_PRINTF_IO
				    ("%s(%04x) PCI Config Write @%02x, size: %d <-- 0x%08x\n",
				     __func__, addr, offs, size, val);
			}
		}
	}
}

uint8_t
handle_port_61h(void)
{
	static u64 last_time = 0;
	u64 curr_time = get_time();
	u64 time_diff;	// time since last call
	uint32_t period_ticks;	// length of a period in ticks
	uint32_t nr_periods;	//number of periods passed since last call
	// bit 4 should toggle with every (DRAM) refresh cycle... (66kHz??)
	time_diff = curr_time - last_time;
	// at 66kHz a period is ~ 15 ns long, converted to ticks: (tb_freq is ticks/second)
	// TODO: as long as the frequency does not change, we should not calculate this every time
	period_ticks = (15 * tb_freq) / 1000000;
	nr_periods = time_diff / period_ticks;
	// if the number if ticks passed since last call is odd, we toggle bit 4
	if ((nr_periods % 2) != 0) {
		*((uint8_t *) (bios_device.io_buffer + 0x61)) ^= 0x10;
	}
	//finally read the value from the io_buffer
	return *((uint8_t *) (bios_device.io_buffer + 0x61));
}
#endif

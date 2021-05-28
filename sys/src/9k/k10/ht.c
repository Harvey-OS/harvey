#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "io.h"

static void
htcapabilities(Pcidev* pci, int cp)
{
	u64int idr;
	u32int command, r;

	/*
	 * Top 5 bits of command give type:
	 *	000xx	slave or primary interface
	 *	001xx	host or secondary interface
	 *	10000	interrupt discovery and configuration
	 * Other values don't concern this interface.
	 */
	r = pcicfgr32(pci, cp);
	command = (r>>16) & 0xFFFF;
	if((command & 0xE000) == 0x0000){
		DBG("HT: slave or primary interface\n");
	}
	else if((command & 0xE000) == 0x2000){
		DBG("HT: host or secondary interface\n");
	}
	else if((command & 0xF800) == 0x8000){
		/*
		 * The Interrupt and Discovery block uses
		 * an index and data scheme to access the
		 * registers. Index is a byte at +2, data is
		 * 32 bits at +4.
		 * The only interesting information is the 64-bit
		 * Interrupt Definition Register at offset 0x10.
		 */
		pcicfgw8(pci, cp+0x02, 0x11);
		idr = pcicfgr32(pci, cp+0x04);
		idr <<= 32;
		pcicfgw8(pci, cp+0x02, 0x10);
		idr |= (u32int)pcicfgr32(pci, cp+0x04);
		DBG("HT: Interrupt and discovery block: idr %#16.16llux\n", idr);
	}
	else{
		DBG("HT: capability code %#ux\n", command>>11);
	}
}

void
htlink(void)
{
	int cp;
	char *p;
	Pcidev *pci;
	u32int r, *rp;

	pci = nil;
	while(pci = pcimatch(pci, 0, 0)){
		/*
		 * AMD-8111 Hypertransport I/O Hub
		 */
		if(pci->vid == 0x1022 && pci->did == 0x1100){
			DBG("HT: AMD-8111: tc %#8.8ux ic %#8.8ux\n",
				pcicfgr32(pci, 0x68), pcicfgr32(pci, 0x6C));
		}

		/*
		 * AMD-8111 PCI Bridge
		 */
		if(pci->vid == 0x1022 && pci->did == 0x7460){
			pcicfgw32(pci, 0xF0, 1);
			DBG("HT: AMD-8111: 0xF4: %#8.8ux\n",
				pcicfgr32(pci, 0xF4));
			pcicfgw32(pci, 0xF0, 0x10);
			DBG("HT: AMD-8111: 0x10: %#8.8ux\n",
				pcicfgr32(pci, 0xF4));
			pcicfgw32(pci, 0xF0, 0x11);
			DBG("HT: AMD-8111: 0x11: %#8.8ux\n",
				pcicfgr32(pci, 0xF4));
		}

		/*
		 * AMD-8111 LPC Bridge
		 */
		if(pci->vid == 0x1022 && pci->did == 0x7468){
			r = pcicfgr32(pci, 0xA0);
			DBG("HT: HPET @ %#ux\n", r);
			if((rp = vmap(r & ~0x0F, 0x200)) != nil){
				DBG("HT: HPET00: %#8.8ux%8.8ux\n",
					rp[4/4], rp[0/4]);
				DBG("HT: HPET10: %#8.8ux%8.8ux\n",
					rp[0x10/4], rp[0x10/4]);
				DBG("HT: HPET20: %#8.8ux%8.8ux\n",
					rp[0x24/4], rp[0x20/4]);
				DBG("HT: HPETF0: %#8.8ux%8.8ux\n",
					rp[0xF4/4], rp[0xF0/4]);
				DBG("HT: HPET100: %#8.8ux%8.8ux\n",
					rp[0x104/4], rp[0x100/4]);
				DBG("HT: HPET120: %#8.8ux%8.8ux\n",
					rp[0x124/4], rp[0x120/4]);
				DBG("HT: HPET140: %#8.8ux%8.8ux\n",
					rp[0x144/4], rp[0x140/4]);
				vunmap(rp, 0x200);
			}
		}

		/*
		 * Check if there are extended capabilities implemented,
		 * (bit 4 in the status register).
		 * Find the capabilities pointer based on PCI header type.
		 *
		 * Make this more general (e.g. pcigetcap(pcidev, id, cp))
		 * and merge back into PCI code.
		 */
		if(!(pcicfgr16(pci, PciPSR) & 0x0010))
			continue;

		switch(pcicfgr8(pci, PciHDT)){
		default:
			continue;
		case 0:				/* all other */
		case 1:				/* PCI to PCI bridge */
			cp = PciCP;
			break;
		}

		for(cp = pcicfgr8(pci, cp); cp != 0; cp = pcicfgr8(pci, cp+1)){
			/*
			 * Check for validity.
			 * Can't be in standard header and must be double
			 * word aligned.
			 */
			if(cp < 0x40 || (cp & ~0xFC))
				break;
			r = pcicfgr32(pci, cp);
			switch(r & 0xFF){
			default:
				DBG("HT: %#4.4ux/%#4.4ux: unknown ID %d\n",
					pci->vid, pci->did, r & 0xFF);
				continue;
			case 0x01:
				p = "PMI";
				break;
			case 0x02:
				p = "AGP";
				break;
			case 0x03:
				p = "VPD";
				break;
			case 0x04:
				p = "Slot Identification";
				break;
			case 0x05:
				p = "MSI";
				break;
			case 0x06:
				p = "CPCI Hot Swap";
				break;
			case 0x07:
				p = "PCI-X";
				break;
			case 0x08:
				DBG("HT: %#4.4ux/%#4.4ux: HT\n",
					pci->vid, pci->did);
				htcapabilities(pci, cp);
				continue;
			case 0x09:
				p = "Vendor Specific";
				break;
			case 0x0A:
				p = "Debug Port";
				break;
			case 0x0B:
				p = "CPCI Central Resource Control";
				break;
			case 0x0C:
				p = "PCI Hot-Plug";
				break;
			case 0x0E:
				p = "AGP 8x";
				break;
			case 0x0F:
				p = "Secure Device";
				break;
			case 0x10:
				p = "PCIe";
				break;
			case 0x11:
				p = "MSI-X";
				break;
			case 0x12:
				p = "SATA HBA";
				break;
			}
			DBG("HT: %#4.4ux/%#4.4ux: %s\n", pci->vid, pci->did, p);
		}
	}
}

/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// devvcon.c ('#C'): a virtual console (virtio-serial-pci) driver.

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"virtio_ring.h"

#include	"virtio_config.h"
#include	"virtio_console.h"
#include	"virtio_pci.h"

#include	"virtio_lib.h"

// Array of defined virtconsoles and their number

static uint32_t nvcon;

static Vqctl **vcons;

static void
vconinit(void)
{
	uint32_t wantfeat(uint32_t f) {
		return VIRTIO_CONSOLE_F_SIZE;	// We want only console size, but not multiport for simplicity
	}
	print("virtio-serial-pci initializing\n");
	uint32_t nvdev = getvdevnum();
	vcons = mallocz(nvdev * sizeof(Vqctl *), 1);
	if(vcons == nil) {
		print("no memory to allocate virtual consoles\n");
		return;
	}
	nvcon = getvdevsbypciid(PCI_DEVICE_ID_VIRTIO_CONSOLE, vcons, nvdev);
	print("virtio consoles found: %d\n", nvcon);
	for(int i = 0; i < nvcon; i++) {
		print("initializing virtual console %d\n", i);
		uint32_t feat = vdevfeat(vcons[i], wantfeat);
		print("features: 0x%08x\n", feat);
		struct virtio_console_config vcfg;
		int rc = readvdevcfg(vcons[i], &vcfg, sizeof(vcfg), 0);
		print("config area size %d\n", rc);
		print("cols=%d rows=%d ports=%d\n", vcfg.cols, vcfg.rows, vcfg.max_nr_ports);
		finalinitvdev(vcons[i]);
	}
}

Dev vcondevtab = {
	.dc = 'C',
	.name = "vcon",

	.reset = devreset,
	.init = vconinit,
	.shutdown = devshutdown,
//	.attach = vconattach,
//	.walk = vconwalk,
//	.stat = vconstat,
//	.open = vconopen,
	.create = devcreate,
//	.close = vconclose,
//	.read = vconread,
	.bread = devbread,
//	.write = vconwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};

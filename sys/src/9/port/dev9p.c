/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// dev9p.c ('#9'): a virtio9p protocol translation driver to use QEMU's built-in 9p.


#include        "u.h"
#include        "../port/lib.h"
#include        "mem.h"
#include        "dat.h"
#include        "fns.h"
#include        "io.h"
#include        "../port/error.h"

#include        "virtio_ring.h"

#include        "virtio_config.h"
#include        "virtio_pci.h"

#include        "virtio_lib.h"

Dev v9pdevtab = {
        .dc = '9',
        .name = "9p",

        .reset = devreset,
        .init = v9pinit,
        .shutdown = devshutdown,
        .attach = v9pattach,
        .walk = v9pwalk,
        .stat = v9pstat,
        .open = v9popen,
        .create = devcreate,
        .close = v9pclose,
        .read = v9pread,
        .bread = devbread,
        .write = v9pwrite,
        .bwrite = devbwrite,
        .remove = devremove,
        .wstat = devwstat,
};

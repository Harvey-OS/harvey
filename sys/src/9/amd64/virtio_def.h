/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// Common definitions for virtio in Harvey. Some definitions are borrowed from
// other sources, where importing a whole file did not seem feasible.

// Virtio-pci device type constants, from 
// http://git.qemu.org/?p=qemu.git;a=blob;f=include/hw/pci/pci.h

#define PCI_VENDOR_ID_REDHAT_QUMRANET    0x1af4
#define PCI_SUBVENDOR_ID_REDHAT_QUMRANET 0x1af4
#define PCI_SUBDEVICE_ID_QEMU            0x1100

#define PCI_DEVICE_ID_VIRTIO_NET         0x1000
#define PCI_DEVICE_ID_VIRTIO_BLOCK       0x1001
#define PCI_DEVICE_ID_VIRTIO_BALLOON     0x1002
#define PCI_DEVICE_ID_VIRTIO_CONSOLE     0x1003
#define PCI_DEVICE_ID_VIRTIO_SCSI        0x1004
#define PCI_DEVICE_ID_VIRTIO_RNG         0x1005
#define PCI_DEVICE_ID_VIRTIO_9P          0x1009

// Specific definitions for virtio-9p devices, from
// https://git.kernel.org/cgit/linux/kernel/git/stable/linux-stable.git/tree/include/uapi/linux/virtio_9p.h
// Replace __u16 and __u8 with equivalent types known to the compiler.

/* The feature bitmap for virtio 9P */

/* The mount point is specified in a config variable */
#define VIRTIO_9P_MOUNT_TAG 0

struct virtio_9p_config {
	/* length of the tag name */
	uint16_t tag_len;
	/* non-NULL terminated tag name */
	uint8_t tag[0];
} __attribute__((packed));


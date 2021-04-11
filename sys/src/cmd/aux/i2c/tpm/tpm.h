/*
 * Copyright (C) 2011 Infineon Technologies
 *
 * Authors:
 * Peter Huewe <huewe.external@infineon.com>
 *
 * Version: 2.1.1
 *
 * Description:
 * Device driver for TCG/TCPA TPM (trusted platform module).
 * Specifications at www.trustedcomputinggroup.org
 *
 * It is based on the Linux kernel driver tpm.c from Leendert van
 * Dorn, Dave Safford, Reiner Sailer, and Kyleen Hall.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __DRIVERS_TPM_SLB9635_I2C_TPM_H__
#define __DRIVERS_TPM_SLB9635_I2C_TPM_H__

#include <stdint.h>

enum tpm_timeout {
	TPM_TIMEOUT = 1,	/* msecs */
};

/* Size of external transmit buffer (used for stack buffer in tpm_sendrecv) */
#define TPM_BUFSIZE 1260

/* Index of fields in TPM command buffer */
#define TPM_CMD_SIZE_BYTE 2
#define TPM_CMD_ORDINAL_BYTE 6

/* Index of Count field in TPM response buffer */
#define TPM_RSP_SIZE_BYTE 2
#define TPM_RSP_RC_BYTE 6

struct tpm_chip;

struct tpm_vendor_specific {
	const u8 req_complete_mask;
	const u8 req_complete_val;
	const u8 req_canceled;
	int irq;
	int (*recv)(struct tpm_chip *, u8 *, usize);
	int (*send)(struct tpm_chip *, u8 *, usize);
	void (*cancel)(struct tpm_chip *);
	u8(*status)(struct tpm_chip *);
	int locality;
};

struct tpm_chip {
	int is_open;
	struct tpm_vendor_specific vendor;
};

struct tpm_input_header {
	u16 tag;
	u32 length;
	u32 ordinal;
} __attribute__ ((packed));

struct tpm_output_header {
	u16 tag;
	u32 length;
	u32 return_code;
} __attribute__ ((packed));

struct timeout_t {
	u32 a;
	u32 b;
	u32 c;
	u32 d;
} __attribute__ ((packed));

struct duration_t {
	u32 tpm_short;
	u32 tpm_medium;
	u32 tpm_long;
} __attribute__ ((packed));

typedef union {
	struct timeout_t timeout;
	struct duration_t duration;
} cap_t;

struct tpm_getcap_params_in {
	u32 cap;
	u32 subcap_size;
	u32 subcap;
} __attribute__ ((packed));

struct tpm_getcap_params_out {
	u32 cap_size;
	cap_t cap;
} __attribute__ ((packed));

typedef union {
	struct tpm_input_header in;
	struct tpm_output_header out;
} tpm_cmd_header;

typedef union {
	struct tpm_getcap_params_out getcap_out;
	struct tpm_getcap_params_in getcap_in;
} tpm_cmd_params;

struct tpm_cmd_t {
	tpm_cmd_header header;
	tpm_cmd_params params;
} __attribute__ ((packed));

/* ---------- Interface for TPM vendor ------------ */

int tpm_vendor_init(unsigned bus, u32 dev_addr);

void tpm_vendor_cleanup(struct tpm_chip *chip);

#endif /* __DRIVERS_TPM_SLB9635_I2C_TPM_H__ */

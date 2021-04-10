/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 The ChromiumOS Authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define LOG_REGBYTES 3
#define REGBYTES (1 << LOG_REGBYTES)
#define STORE sd
#define HLS_SIZE 64
#define MENTRY_FRAME_SIZE HLS_SIZE

#define TOHOST_CMD(dev, cmd, payload) \
	(((u64)(dev) << 56) | ((u64)(cmd) << 48) | (u64)(payload))

#define FROMHOST_DEV(fromhost_value) ((u64)(fromhost_value) >> 56)
#define FROMHOST_CMD(fromhost_value) ((u64)(fromhost_value) << 8 >> 56)
#define FROMHOST_DATA(fromhost_value) ((u64)(fromhost_value) << 16 >> 16)

typedef struct {
	unsigned long base;
	unsigned long size;
	unsigned long node_id;
} memory_block_info;

typedef struct {
	unsigned long dev;
	unsigned long cmd;
	unsigned long data;
	unsigned long sbi_private_data;
} sbi_device_message;

typedef struct {
	sbi_device_message *device_request_queue_head;
	unsigned long device_request_queue_size;
	sbi_device_message *device_response_queue_head;
	sbi_device_message *device_response_queue_tail;

	int hart_id;
	int ipi_pending;
} hls_t;

#define MACHINE_STACK_TOP() ({ \
  register uintptr sp __asm__ ("sp"); \
  (void*)((sp + RISCV_PGSIZE) & -RISCV_PGSIZE); })

// hart-local storage, at top of stack
#define HLS() ((hls_t *)(MACHINE_STACK_TOP() - HLS_SIZE))
#define OTHER_HLS(id) ((hls_t *)((void *)HLS() + RISCV_PGSIZE * ((id)-HLS()->hart_id)))

#define MACHINE_STACK_SIZE RISCV_PGSIZE

uintptr translate_address(uintptr vAddr);
uintptr mcall_query_memory(uintptr id, memory_block_info *p);
uintptr mcall_hart_id(void);
uintptr htif_interrupt(uintptr mcause, uintptr *regs);
uintptr mcall_console_putchar(u8 ch);
void putchar(void);
uintptr mcall_dev_req(sbi_device_message *m);
uintptr mcall_dev_resp(void);
uintptr mcall_set_timer(unsigned long long when);
uintptr mcall_clear_ipi(void);
uintptr mcall_send_ipi(uintptr recipient);
uintptr mcall_shutdown(void);
void hls_init(u32 hart_id);	// need to call this before launching linux

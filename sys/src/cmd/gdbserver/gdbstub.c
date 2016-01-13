/*
 * Kernel Debug Core
 *
 * Maintainer: Jason Wessel <jason.wessel@windriver.com>
 *
 * Copyright (C) 2000-2001 VERITAS Software Corporation.
 * Copyright (C) 2002-2004 Timesys Corporation
 * Copyright (C) 2003-2004 Amit S. Kale <amitkale@linsyssoft.com>
 * Copyright (C) 2004 Pavel Machek <pavel@ucw.cz>
 * Copyright (C) 2004-2006 Tom Rini <trini@kernel.crashing.org>
 * Copyright (C) 2004-2006 LinSysSoft Technologies Pvt. Ltd.
 * Copyright (C) 2005-2009 Wind River Systems, Inc.
 * Copyright (C) 2007 MontaVista Software, Inc.
 * Copyright (C) 2008 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *
 * Contributors at various stages not listed above:
 *  Jason Wessel ( jason.wessel@windriver.com )
 *  George Anzinger <george@mvista.com>
 *  Anurekh Saxena (anurekh.saxena@timesys.com)
 *  Lake Stevens Instrument Division (Glenn Engel)
 *  Jim Kingdon, Cygnus Support.
 *
 * Original KGDB stub: David Grothe <dave@gcom.com>,
 * Tigran Aivazian <tigran@sco.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <u.h>
#include <libc.h>
#include "debug_core.h"

#define KGDB_MAX_THREAD_QUERY 17
#define BUFMAX 1024
#define BUF_THREAD_ID_SIZE	8
#define NUMREGBYTES 512

#define NO_POLL_CHAR '$'

// Everything always does EINVAL.
#define EINVAL 22

/* Our I/O buffers. */
static char			remcom_in_buffer[BUFMAX];
static char			remcom_out_buffer[BUFMAX];
static int			gdbstub_use_prev_in_buf;
static int			gdbstub_prev_in_buf_pos;
static int kgdb_connected;

/* Storage for the registers, in GDB format. */
static unsigned long		gdb_regs[(NUMREGBYTES +
					sizeof(unsigned long) - 1) /
					sizeof(unsigned long)];

/* support crap */
/*
 * GDB remote protocol parser:
 */

static char *hex_asc = "0123456789abcdef";

static uint8_t hex_asc_lo(int x)
{
	return hex_asc[x&0xf];
}

static uint8_t hex_asc_hi(int x)
{
	return hex_asc_lo(x>>4);
}

static void *hex_byte_pack(char *dest, int val)
{
	*dest++ = hex_asc_hi(val);
	*dest++ = hex_asc_lo(val);
	return dest;
}

static int hex_to_bin(int c)
{
	c &= 0xf;
	if ((c >= '0') && (c <= '9'))
		return c - '0';
	if ((c >= 'a') && (c <= 'f'))
		return c + 10 - 'a';
	if ((c >= 'A') && (c <= 'F'))
		return c + 10 - 'A';
	return 0;
}

char *p8(char *dest, int c)
{
	*dest++ = hex_to_bin(c);
	*dest++ = hex_to_bin(c>>4);
	return dest;
}

char *p16(char *dest, int c)
{
	dest = p8(dest, c);
	dest = p8(dest, c>>4);
	return dest;
}

char *p32(char *dest, uint32_t c)
{
	dest = p16(dest, c);
	dest = p16(dest, c>>16);
	return dest;
}

char *p64(char *dest, uint64_t c)
{
	dest = p32(dest, c);
	dest = p32(dest, c>>32);
	return dest;
}
	
static void write_char(uint8_t c)
{
	write(1, &c, 1);
}

static int read_char()
{
	uint8_t c;
	read(0, &c, 1);
	return c;
}

static int gdbstub_read_wait(void)
{
	int ret = read_char();
	while (ret == NO_POLL_CHAR)
		ret = read_char();
	return ret;
}

/* scan for the sequence $<data>#<checksum> */
static void get_packet(char *buffer)
{
	unsigned char checksum;
	unsigned char xmitcsum;
	int count;
	char ch;

	do {
		/*
		 * Spin and wait around for the start character, ignore all
		 * other characters:
		 */
		while ((ch = (gdbstub_read_wait())) != '$')
			/* nothing */;

		checksum = 0;
		xmitcsum = -1;

		count = 0;

		/*
		 * now, read until a # or end of buffer is found:
		 */
		while (count < (BUFMAX - 1)) {
			ch = gdbstub_read_wait();
			if (ch == '#')
				break;
			checksum = checksum + ch;
			buffer[count] = ch;
			count = count + 1;
		}

		if (ch == '#') {
			xmitcsum = hex_to_bin(gdbstub_read_wait()) << 4;
			xmitcsum += hex_to_bin(gdbstub_read_wait());

			if (checksum != xmitcsum)
				/* failed checksum */
				write_char('-');
			else
				/* successful transfer */
				write_char('+');
		}
		buffer[count] = 0;
	} while (checksum != xmitcsum);
}

/*
 * Send the packet in buffer.
 * Check for gdb connection if asked for.
 */
static void put_packet(char *buffer)
{
	unsigned char checksum;
	int count;
	char ch;

	/*
	 * $<packet info>#<checksum>.
	 */
	while (1) {
		write_char('$');
		checksum = 0;
		count = 0;

		while ((ch = buffer[count])) {
			write_char(ch);
			checksum += ch;
			count++;
		}

		write_char('#');
		write_char(hex_asc_hi(checksum));
		write_char(hex_asc_lo(checksum));

		/* Now see what we get in reply. */
		ch = gdbstub_read_wait();

		if (ch == 3)
			ch = gdbstub_read_wait();

		/* If we get an ACK, we are done. */
		if (ch == '+')
			return;

		/*
		 * If we get the start of another packet, this means
		 * that GDB is attempting to reconnect.  We will NAK
		 * the packet being sent, and stop trying to send this
		 * packet.
		 */
		if (ch == '$') {
			write_char('-');
			return;
		}
	}
}

static char gdbmsgbuf[BUFMAX + 1];

void gdbstub_msg_write(const char *s, int len)
{
	char *bufptr;
	int wcount;
	int i;

	if (len == 0)
		len = strlen(s);

	/* 'O'utput */
	gdbmsgbuf[0] = 'O';

	/* Fill and send buffers... */
	while (len > 0) {
		bufptr = gdbmsgbuf + 1;

		/* Calculate how many this time */
		if ((len << 1) > (BUFMAX - 2))
			wcount = (BUFMAX - 2) >> 1;
		else
			wcount = len;

		/* Pack in hex chars */
		for (i = 0; i < wcount; i++)
			bufptr = hex_byte_pack(bufptr, s[i]);
		*bufptr = '\0';

		/* Move up */
		s += wcount;
		len -= wcount;

		/* Write packet */
		put_packet(gdbmsgbuf);
	}
}

/*
 * Convert the memory pointed to by mem into hex, placing result in
 * buf.  Return a pointer to the last char put in buf (null). May
 * return an error.
 */
char *kgdb_mem2hex(char *mem, char *buf, int count)
{
	char *tmp;
	int err;

	/*
	 * We use the upper half of buf as an intermediate buffer for the
	 * raw memory copy.  Hex conversion will work against this one.
	 */
	tmp = buf + count;

//	err = proc_mem_read(tmp, pid, mem, count);
	if (err)
		return nil;
	while (count > 0) {
		buf = hex_byte_pack(buf, *tmp);
		tmp++;
		count--;
	}
	*buf = 0;

	return buf;
}

/*
 * Convert the hex array pointed to by buf into binary to be placed in
 * mem.  Return a pointer to the character AFTER the last byte
 * written.  May return an error.
 */
int kgdb_hex2mem(char *buf, char *mem, int count)
{
	char *tmp_raw;
	char *tmp_hex;

	/*
	 * We use the upper half of buf as an intermediate buffer for the
	 * raw memory that is converted from hex.
	 */
	tmp_raw = buf + count * 2;

	tmp_hex = tmp_raw - 1;
	while (tmp_hex >= buf) {
		tmp_raw--;
		*tmp_raw = hex_to_bin(*tmp_hex--);
		*tmp_raw |= hex_to_bin(*tmp_hex--) << 4;
	}

	return 0; //proc_mem_write(mem, pid, tmp_raw, count);
}

/*
 * While we find nice hex chars, build a long_val.
 * Return number of chars processed.
 */
int kgdb_hex2long(char **ptr, unsigned long *long_val)
{
	int hex_val;
	int num = 0;
	int negate = 0;

	*long_val = 0;

	if (**ptr == '-') {
		negate = 1;
		(*ptr)++;
	}
	while (**ptr) {
		hex_val = hex_to_bin(**ptr);
		if (hex_val < 0)
			break;

		*long_val = (*long_val << 4) | hex_val;
		num++;
		(*ptr)++;
	}

	if (negate)
		*long_val = -*long_val;

	return num;
}

/*
 * Copy the binary array pointed to by buf into mem.  Fix $, #, and
 * 0x7d escaped with 0x7d. Return -EFAULT on failure or 0 on success.
 * The input buf is overwitten with the result to write to mem.
 */
static int kgdb_ebin2mem(char *buf, char *mem, int count)
{
	int size = 0;
	char *c = buf;

	while (count-- > 0) {
		c[size] = *buf++;
		if (c[size] == 0x7d)
			c[size] = *buf++ ^ 0x20;
		size++;
	}

	return 0; //proc_mem_write(mem, c, size);
}

#if DBG_MAX_REG_NUM > 0
void pt_regs_to_gdb_regs(unsigned long *gdb_regs, struct pt_regs *regs)
{
	int i;
	int idx = 0;
	char *ptr = (char *)gdb_regs;

	for (i = 0; i < DBG_MAX_REG_NUM; i++) {
		dbg_get_reg(i, ptr + idx, regs);
		idx += dbg_reg_def[i].size;
	}
}

void gdb_regs_to_pt_regs(unsigned long *gdb_regs, struct pt_regs *regs)
{
	int i;
	int idx = 0;
	char *ptr = (char *)gdb_regs;

	for (i = 0; i < DBG_MAX_REG_NUM; i++) {
		dbg_set_reg(i, ptr + idx, regs);
		idx += dbg_reg_def[i].size;
	}
}
#endif /* DBG_MAX_REG_NUM > 0 */

/* Write memory due to an 'M' or 'X' packet. */
static int write_mem_msg(int binary)
{
	char *ptr = &remcom_in_buffer[1];
	unsigned long addr;
	unsigned long length;
	int err;

	if (kgdb_hex2long(&ptr, &addr) > 0 && *(ptr++) == ',' &&
	    kgdb_hex2long(&ptr, &length) > 0 && *(ptr++) == ':') {
		if (binary)
			err = kgdb_ebin2mem(ptr, (char *)addr, length);
		else
			err = kgdb_hex2mem(ptr, (char *)addr, length);
		if (err)
			return err;
		return 0;
	}

	return -1; // -EINVAL;
}

static void error_packet(char *pkt, int error)
{
	error = -error;
	pkt[0] = 'E';
	pkt[1] = hex_asc[(error / 10)];
	pkt[2] = hex_asc[(error % 10)];
	pkt[3] = '\0';
}

/*
 * Thread ID accessors. We represent a flat TID space to GDB, where
 * the per CPU idle threads (which under Linux all have PID 0) are
 * remapped to negative TIDs.
 */


static char *pack_threadid(char *pkt, unsigned char *id)
{
	unsigned char *limit;
	int lzero = 1;

	limit = id + (BUF_THREAD_ID_SIZE / 2);
	while (id < limit) {
		if (!lzero || *id != 0) {
			pkt = hex_byte_pack(pkt, *id);
			lzero = 0;
		}
		id++;
	}

	if (lzero)
		pkt = hex_byte_pack(pkt, 0);

	return pkt;
}

static void int_to_threadref(unsigned char *id, int value)
{
	print("%s: panic\n", __func__); exits("fuck");
//	put_unaligned_be32(value, id);
}

/*
 * All the functions that start with gdb_cmd are the various
 * operations to implement the handlers for the gdbserial protocol
 * where KGDB is communicating with an external debugger
 */

/* Handle the '?' status packets */
static void gdb_cmd_status(struct kgdb_state *ks)
{
	/*
	 * We know that this packet is only sent
	 * during initial connect.  So to be safe,
	 * we clear out our breakpoints now in case
	 * GDB is reconnecting.
	 */
	dbg_remove_all_break();

	remcom_out_buffer[0] = 'S';
	hex_byte_pack(&remcom_out_buffer[1], ks->signo);
}

static void gdb_get_regs_helper(struct kgdb_state *ks)
{

	/* Plan 9 registers can be read whether the proc is stopped or not. */
	// opnen /proc/s->pid/regs
	// read them. 
	// convert them.
}

/* Handle the 'g' get registers request */
static void gdb_cmd_getregs(struct kgdb_state *ks)
{
	gdb_get_regs_helper(ks);
	kgdb_mem2hex((char *)gdb_regs, remcom_out_buffer, NUMREGBYTES);
}

/* Handle the 'G' set registers request */
static void gdb_cmd_setregs(struct kgdb_state *ks)
{
	kgdb_hex2mem(&remcom_in_buffer[1], (char *)gdb_regs, NUMREGBYTES);

	error_packet(remcom_out_buffer, -EINVAL);

	//gdb_regs_to_pt_regs(gdb_regs, ks->linux_regs);
//		strcpy(remcom_out_buffer, "OK");
}

/* Handle the 'm' memory read bytes */
static void gdb_cmd_memread(struct kgdb_state *ks)
{
	char *ptr = &remcom_in_buffer[1];
	unsigned long length;
	unsigned long addr;
	char *err;

	if (kgdb_hex2long(&ptr, &addr) > 0 && *ptr++ == ',' &&
					kgdb_hex2long(&ptr, &length) > 0) {
		err = kgdb_mem2hex((char *)addr, remcom_out_buffer, length);
		if (!err)
			error_packet(remcom_out_buffer, -EINVAL);
	} else {
		error_packet(remcom_out_buffer, -EINVAL);
	}
}

/* Handle the 'M' memory write bytes */
static void gdb_cmd_memwrite(struct kgdb_state *ks)
{
	int err = write_mem_msg(0);

	if (err)
		error_packet(remcom_out_buffer, err);
	else
		strcpy(remcom_out_buffer, "OK");
}

#if DBG_MAX_REG_NUM > 0
static char *gdb_hex_reg_helper(int regnum, char *out)
{
	int i;
	int offset = 0;

	for (i = 0; i < regnum; i++)
		offset += dbg_reg_def[i].size;
	return kgdb_mem2hex((char *)gdb_regs + offset, out,
			    dbg_reg_def[i].size);
}

/* Handle the 'p' individual regster get */
static void gdb_cmd_reg_get(struct kgdb_state *ks)
{
	unsigned long regnum;
	char *ptr = &remcom_in_buffer[1];

	kgdb_hex2long(&ptr, &regnum);
	if (regnum >= DBG_MAX_REG_NUM) {
		error_packet(remcom_out_buffer, -EINVAL);
		return;
	}
	gdb_get_regs_helper(ks);
	gdb_hex_reg_helper(regnum, remcom_out_buffer);
}

/* Handle the 'P' individual regster set */
static void gdb_cmd_reg_set(struct kgdb_state *ks)
{
	unsigned long regnum;
	char *ptr = &remcom_in_buffer[1];
	int i = 0;

	kgdb_hex2long(&ptr, &regnum);
	if (*ptr++ != '=' ||
	    !(!kgdb_usethread || kgdb_usethread == current) ||
	    !dbg_get_reg(regnum, gdb_regs, ks->linux_regs)) {
		error_packet(remcom_out_buffer, -EINVAL);
		return;
	}
	memset(gdb_regs, 0, sizeof(gdb_regs));
	while (i < sizeof(gdb_regs) * 2)
		if (hex_to_bin(ptr[i]) >= 0)
			i++;
		else
			break;
	i = i / 2;
	kgdb_hex2mem(ptr, (char *)gdb_regs, i);
	dbg_set_reg(regnum, gdb_regs, ks->linux_regs);
	strcpy(remcom_out_buffer, "OK");
}
#endif /* DBG_MAX_REG_NUM > 0 */

/* Handle the 'X' memory binary write bytes */
static void gdb_cmd_binwrite(struct kgdb_state *ks)
{
	int err = write_mem_msg(1);

	if (err)
		error_packet(remcom_out_buffer, err);
	else
		strcpy(remcom_out_buffer, "OK");
}

/* Handle the 'D' or 'k', detach or kill packets */
static void gdb_cmd_detachkill(struct kgdb_state *ks)
{
	int error;

	/* The detach case */
	if (remcom_in_buffer[0] == 'D') {
		error = dbg_remove_all_break();
		if (error < 0) {
			error_packet(remcom_out_buffer, error);
		} else {
			strcpy(remcom_out_buffer, "OK");
			//kgdb_connected = 0;
		}
		put_packet(remcom_out_buffer);
	} else {
		/*
		 * Assume the kill case, with no exit code checking,
		 * trying to force detach the debugger:
		 */
		dbg_remove_all_break();
		kgdb_connected = 0;
	}
}

/* Handle the 'R' reboot packets */
static int gdb_cmd_reboot(struct kgdb_state *ks)
{
	/* For now, only honor R0 */
	if (strcmp(remcom_in_buffer, "R0") == 0) {
		print(" NOT Executing emergency reboot\n");
		strcpy(remcom_out_buffer, "OK");
		put_packet(remcom_out_buffer);

		/*
		 * Execution should not return from
		 * machine_emergency_restart()
		 */
		kgdb_connected = 0;

		return 1;
	}
	return 0;
}

/* Handle the 'q' query packets */
static void gdb_cmd_query(struct kgdb_state *ks)
{
	unsigned char thref[BUF_THREAD_ID_SIZE];
	char *ptr;
//	int i;
	//int cpu;
	//int finished = 0;

	switch (remcom_in_buffer[1]) {
	case 's':
	case 'f':
		if (memcmp(remcom_in_buffer + 2, "ThreadInfo", 10))
			break;


		remcom_out_buffer[0] = 'm';
		ptr = remcom_out_buffer + 1;
		if (remcom_in_buffer[1] == 'f') {
			/* Each cpu is a shadow thread */
			// iterate over /proc and return the IDS.
#if 0
			for_each_online_cpu(cpu) {
				ks->thr_query = 0;
				int_to_threadref(thref, -cpu - 2);
				ptr = pack_threadid(ptr, thref);
				*(ptr++) = ',';
				i++;
			}
#endif
		}
#if 0
		do_each_thread(g, p) {
			if (i >= ks->thr_query && !finished) {
				int_to_threadref(thref, p->pid);
				ptr = pack_threadid(ptr, thref);
				*(ptr++) = ',';
				ks->thr_query++;
				if (ks->thr_query % KGDB_MAX_THREAD_QUERY == 0)
					finished = 1;
			}
			i++;
		} while_each_thread(g, p);
#endif
		*(--ptr) = '\0';
		break;

	case 'C':
		/* Current thread id */
		strcpy(remcom_out_buffer, "QC");
		pack_threadid(remcom_out_buffer + 2, (uint8_t*)&ks->threadid);
		break;
	case 'T':
		if (memcmp(remcom_in_buffer + 1, "ThreadExtraInfo,", 16))
			break;

		ks->threadid = 0;
		ptr = remcom_in_buffer + 17;
		kgdb_hex2long(&ptr, &ks->threadid);
		//if (!getthread(ks->linux_regs, ks->threadid)) {
		error_packet(remcom_out_buffer, -EINVAL);
		//break;
		//}
#if 0
		if ((int)ks->threadid > 0) {
			kgdb_mem2hex(getthread(ks->linux_regs,
					ks->threadid)->comm,
					remcom_out_buffer, 16);
		} else {
			static char tmpstr[23 + BUF_THREAD_ID_SIZE];

			sprintf(tmpstr, "shadowCPU%d",
					(int)(-ks->threadid - 2));
			kgdb_mem2hex(tmpstr, remcom_out_buffer, strlen(tmpstr));
		}
#endif
		break;
	}
}

/* Handle the 'H' task query packets */
static void gdb_cmd_task(struct kgdb_state *ks)
{
	char *ptr;

	switch (remcom_in_buffer[1]) {
	case 'g':
		ptr = &remcom_in_buffer[2];
		kgdb_hex2long(&ptr, &ks->threadid);
		//thread = getthread(ks->linux_regs, ks->threadid);
		//if (!thread && ks->threadid > 0) {
			error_packet(remcom_out_buffer, -EINVAL);
//			break;
//		}
//		kgdb_usethread = thread;
//		ks->kgdb_usethreadid = ks->threadid;
//		strcpy(remcom_out_buffer, "OK");
		break;
	case 'c':
		ptr = &remcom_in_buffer[2];
		kgdb_hex2long(&ptr, &ks->threadid);
		strcpy(remcom_out_buffer, "OK");
		break;
	}
}

/* Handle the 'T' thread query packets */
static void gdb_cmd_thread(struct kgdb_state *ks)
{
	char *ptr = &remcom_in_buffer[1];
	int ok;
	
	kgdb_hex2long(&ptr, &ks->threadid);
	
	ok = gpr(ks->gpr, ks->threadid);
	if (ok)
		strcpy(remcom_out_buffer, "OK");
	else
		error_packet(remcom_out_buffer, -EINVAL);
}

/* Handle the 'z' or 'Z' breakpoint remove or set packets */
static void gdb_cmd_break(struct kgdb_state *ks)
{
	/*
	 * Since GDB-5.3, it's been drafted that '0' is a software
	 * breakpoint, '1' is a hardware breakpoint, so let's do that.
	 */
	char *bpt_type = &remcom_in_buffer[1];
	char *ptr = &remcom_in_buffer[2];
	unsigned long addr;
	unsigned long length;
	int error = 0;

	if (*bpt_type >= '0') {
		return;
	}

	if (*(ptr++) != ',') {
		error_packet(remcom_out_buffer, -EINVAL);
		return;
	}
	if (!kgdb_hex2long(&ptr, &addr)) {
		error_packet(remcom_out_buffer, -EINVAL);
		return;
	}
	if (*(ptr++) != ',' ||
		!kgdb_hex2long(&ptr, &length)) {
		error_packet(remcom_out_buffer, -EINVAL);
		return;
	}

	if (remcom_in_buffer[0] == 'Z' && *bpt_type == '0')
		error = dbg_set_sw_break(addr);
	else if (remcom_in_buffer[0] == 'z' && *bpt_type == '0')
		error = dbg_remove_sw_break(addr);

	if (error == 0)
		strcpy(remcom_out_buffer, "OK");
	else
		error_packet(remcom_out_buffer, error);
}

/* Handle the 'C' signal / exception passing packets */
static int gdb_cmd_exception_pass(struct kgdb_state *ks)
{
	/* C09 == pass exception
	 * C15 == detach kgdb, pass exception
	 */
	if (remcom_in_buffer[1] == '0' && remcom_in_buffer[2] == '9') {

		ks->pass_exception = 1;
		remcom_in_buffer[0] = 'c';

	} else if (remcom_in_buffer[1] == '1' && remcom_in_buffer[2] == '5') {

		ks->pass_exception = 1;
		remcom_in_buffer[0] = 'D';
		dbg_remove_all_break();
		kgdb_connected = 0;
		return 1;

	} else {
		gdbstub_msg_write("KGDB only knows signal 9 (pass)"
			" and 15 (pass and disconnect)\n"
			"Executing a continue without signal passing\n", 0);
		remcom_in_buffer[0] = 'c';
	}

	/* Indicate fall through */
	return -1;
}

/*
 * This function performs all gdbserial command procesing
 */
int gdb_serial_stub(struct kgdb_state *ks)
{
	int error = 0;
	int tmp;

	/* Initialize comm buffer and globals. */
	memset(remcom_out_buffer, 0, sizeof(remcom_out_buffer));

	if (kgdb_connected) {
		unsigned char thref[BUF_THREAD_ID_SIZE];
		char *ptr;

		/* Reply to host that an exception has occurred */
		ptr = remcom_out_buffer;
		*ptr++ = 'T';
		ptr = hex_byte_pack(ptr, ks->signo);
		ptr += strlen(strcpy(ptr, "thread:"));
		int_to_threadref(thref, 1);
		ptr = pack_threadid(ptr, thref);
		*ptr++ = ';';
		put_packet(remcom_out_buffer);
	}

	while (1) {
		error = 0;

		/* Clear the out buffer. */
		memset(remcom_out_buffer, 0, sizeof(remcom_out_buffer));

		get_packet(remcom_in_buffer);

		switch (remcom_in_buffer[0]) {
		case '?': /* gdbserial status */
			gdb_cmd_status(ks);
			break;
		case 'g': /* return the value of the CPU registers */
			gdb_cmd_getregs(ks);
			break;
		case 'G': /* set the value of the CPU registers - return OK */
			gdb_cmd_setregs(ks);
			break;
		case 'm': /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
			gdb_cmd_memread(ks);
			break;
		case 'M': /* MAA..AA,LLLL: Write LLLL bytes at address AA..AA */
			gdb_cmd_memwrite(ks);
			break;
#if DBG_MAX_REG_NUM > 0
		case 'p': /* pXX Return gdb register XX (in hex) */
			gdb_cmd_reg_get(ks);
			break;
		case 'P': /* PXX=aaaa Set gdb register XX to aaaa (in hex) */
			gdb_cmd_reg_set(ks);
			break;
#endif /* DBG_MAX_REG_NUM > 0 */
		case 'X': /* XAA..AA,LLLL: Write LLLL bytes at address AA..AA */
			gdb_cmd_binwrite(ks);
			break;
			/* kill or detach. KGDB should treat this like a
			 * continue.
			 */
		case 'D': /* Debugger detach */
		case 'k': /* Debugger detach via kill */
			gdb_cmd_detachkill(ks);
			goto default_handle;
		case 'R': /* Reboot */
			if (gdb_cmd_reboot(ks))
				goto default_handle;
			break;
		case 'q': /* query command */
			gdb_cmd_query(ks);
			break;
		case 'H': /* task related */
			gdb_cmd_task(ks);
			break;
		case 'T': /* Query thread status */
			gdb_cmd_thread(ks);
			break;
		case 'z': /* Break point remove */
		case 'Z': /* Break point set */
			gdb_cmd_break(ks);
			break;
#ifdef CONFIG_KGDB_KDB
		case '3': /* Escape into back into kdb */
			if (remcom_in_buffer[1] == '\0') {
				gdb_cmd_detachkill(ks);
				return DBG_PASS_EVENT;
			}
#endif
		case 'C': /* Exception passing */
			tmp = gdb_cmd_exception_pass(ks);
			if (tmp > 0)
				goto default_handle;
			if (tmp == 0)
				break;
			/* Fall through on tmp < 0 */
		case 'c': /* Continue packet */
		case 's': /* Single step packet */
			dbg_activate_sw_breakpoints();
			/* Fall through to default processing */
		default:
default_handle:
			/*
			 * I have no idea what to do.
			 * Leave cmd processing on error, detach,
			 * kill, continue, or single step.
			 */
			if (error >= 0 || remcom_in_buffer[0] == 'D' ||
			    remcom_in_buffer[0] == 'k') {
				error = 0;
				goto kgdb_exit;
			}

		}

		/* reply to the request */
		put_packet(remcom_out_buffer);
	}

kgdb_exit:
	if (ks->pass_exception)
		error = 1;
	return error;
}

int gdbstub_state(struct kgdb_state *ks, char *cmd)
{
	int error;

	switch (cmd[0]) {
	case 'e':
#if 0
		error = kgdb_arch_handle_exception(ks->ex_vector,
						   ks->signo,
						   ks->err_code,
						   remcom_in_buffer,
						   remcom_out_buffer,
						   ks->linux_regs);
#endif
		return error;
	case 's':
	case 'c':
		strcpy(remcom_in_buffer, cmd);
		return 0;
	case '$':
		strcpy(remcom_in_buffer, cmd);
		gdbstub_use_prev_in_buf = strlen(remcom_in_buffer);
		gdbstub_prev_in_buf_pos = 0;
		return 0;
	}
	write_char('+');
	put_packet(remcom_out_buffer);
	return 0;
}

/**
 * gdbstub_exit - Send an exit message to GDB
 * @status: The exit code to report.
 */
void gdbstub_exit(int status)
{
	unsigned char checksum, ch, buffer[3];
	int loop;

	if (!kgdb_connected)
		return;
	kgdb_connected = 0;

	buffer[0] = 'W';
	buffer[1] = hex_asc_hi(status);
	buffer[2] = hex_asc_lo(status);

	write_char('$');
	checksum = 0;

	for (loop = 0; loop < 3; loop++) {
		ch = buffer[loop];
		checksum += ch;
		write_char(ch);
	}

	write_char('#');
	write_char(hex_asc_hi(checksum));
	write_char(hex_asc_lo(checksum));
}

int gpr(uint64_t *regs, int pid)
{
	int i;
	char aregs[176]; /* 176? 8 * 16 is 128. It's 64 more. Hmm. */
	char *cp = aregs;
	char *regname = smprint("/proc/%d/regs", pid);
	int fd = open(regname, 0);

	if (pread(fd, aregs, sizeof(aregs), 0) < sizeof(aregs)) {
		return -1;
	}
	close(fd);
	for(i = 0; i < 16; i++) {
		cp = p8((void *)&regs[i], *cp);
	}
	return 0;
	
}

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
#include <ureg.h>
#include <ctype.h>

#include "debug_core.h"
#include "gdb.h"

#define MAX_THREAD_QUERY 17
#define BUFMAX 1024
#define BUF_THREAD_ID_SIZE	8

// Everything always does EINVAL.
#define EINVAL 22

/* Our I/O buffers. */
char remcom_in_buffer[BUFMAX];
char remcom_out_buffer[BUFMAX];
static int gdbstub_use_prev_in_buf;
static int gdbstub_prev_in_buf_pos;
int bpsize;
int remotefd;
int debug = 0;

/* support crap */
/*
 * GDB remote protocol parser:
 */

static char *hex_asc = "0123456789abcdef";

// not reentrant, bla bla bla
char *
errstring(char *prefix)
{
	static char error[ERRMAX];
	snprint(error, sizeof(error), "%s%r", prefix);
	return error;
}

static uint8_t
hex_asc_lo(int x)
{
	return hex_asc[x & 0xf];
}

static uint8_t
hex_asc_hi(int x)
{
	return hex_asc_lo(x >> 4);
}

static void *
hex_byte_pack(char *dest, int val)
{
	*dest++ = hex_asc_hi(val);
	*dest++ = hex_asc_lo(val);
	return dest;
}

static int
hex_to_bin(int c)
{
	if ((c >= '0') && (c <= '9'))
		return c - '0';
	if ((c >= 'a') && (c <= 'f'))
		return c + 10 - 'a';
	if ((c >= 'A') && (c <= 'F'))
		return c + 10 - 'A';
	return -1;
}

char *
p8(char *dest, int c)
{
	*dest++ = hex_to_bin(c);
	*dest++ = hex_to_bin(c >> 4);
	return dest;
}

char *
p16(char *dest, int c)
{
	dest = p8(dest, c);
	dest = p8(dest, c >> 4);
	return dest;
}

char *
p32(char *dest, uint32_t c)
{
	dest = p16(dest, c);
	dest = p16(dest, c >> 16);
	return dest;
}

char *
p64(char *dest, uint64_t c)
{
	dest = p32(dest, c);
	dest = p32(dest, c >> 32);
	return dest;
}

void
sendctl(int pid, char* message)
{
    char buf[100];
    int ctlfd;

    sprint(buf, "/proc/%d/ctl", pid);
    print("%s\n", buf);
    ctlfd = open(buf, OWRITE);
    if (ctlfd >= 0) {
        write(ctlfd, message, strlen(message));
        close(ctlfd);
    }
}

static void
write_char(uint8_t c)
{
	write(remotefd, &c, 1);
}

static int
read_char()
{
	uint8_t c;
	if (read(remotefd, &c, 1) < 0)
		return -1;
	return c;
}

static int
gdbstub_read_wait(void)
{
	int ret;
	ret = read_char();
	if (ret < 0) {
		write(1, "DIE", 3);
		exits("die");
	}
	return ret;
}
#define I_AM_HERE syslog(0, "gdbserver", "%s %d\n", __FILE__, __LINE__);
/* scan for the sequence $<data>#<checksum> */
static void
get_packet(char *buffer)
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
			/* nothing */ ;

I_AM_HERE
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

I_AM_HERE
		if (ch == '#') {
			xmitcsum = hex_to_bin(gdbstub_read_wait()) << 4;
			xmitcsum += hex_to_bin(gdbstub_read_wait());
if (checksum != xmitcsum) syslog(0, "gdbserver", "BAD CSUM Computed 0x%x want 0x%x\n", xmitcsum, checksum);
			if (checksum != xmitcsum)
				/* failed checksum */
				write_char('-');
			else
				/* successful transfer */
				write_char('+');
		}
		buffer[count] = 0;
	} while (checksum != xmitcsum);
	if (debug)
	    print("<-%s\n", buffer);
}

/*
 * Send the packet in buffer.
 * Check for gdb connection if asked for.
 */
static void
put_packet(char *buffer)
{
	unsigned char checksum;
	int count;
	char ch;

	/*
	 * $<packet info>#<checksum>.
	 */
	if (debug)
	    print("->%s\n", buffer);
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

void
gdbstub_msg_write(const char *s, int len)
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
char *
mem2hex(unsigned char *mem, char *buf, int count)
{
	char *tmp;

	for(tmp = buf; count; tmp += 2, count--, mem++){
		syslog(0, "gdbserver", "%02x", *mem);
		sprint(tmp, "%02x", *mem);
	}

	*tmp = 0;
	return tmp;
}

/*
 * Convert the hex array pointed to by buf into binary to be placed in
 * mem.  Return a pointer to the character AFTER the last byte
 * written.  May return an error.
 */
char *
hex2mem(char *buf, unsigned char *mem, int count)
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
	return nil; // Check for valid stuff some day?
}

/*
 * While we find nice hex chars, build a long_val.
 * Return number of chars processed.
 */
int
hex2long(char **ptr, unsigned long *long_val)
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
static  void
ebin2mem(unsigned char *buf, unsigned char *mem, int count)
{
	int size = 0;
	unsigned char *c = buf;

	while (count-- > 0) {
		c[size] = *buf++;
		if (c[size] == 0x7d)
			c[size] = *buf++ ^ 0x20;
		size++;
	}

}

/* Write memory due to an 'M' or 'X' packet. */
static char *
write_mem_msg(struct state *ks, int binary)
{
	char *cp = (char *)&remcom_in_buffer[1];

	unsigned long addr;
	unsigned long length;
	char *err;

	if (hex2long(&cp, &addr) > 0 && *(cp++) == ',' &&
		hex2long(&cp, &length) > 0 && *(cp++) == ':') {
		unsigned char *data = malloc(length);
		if (binary)
			ebin2mem((unsigned char *)cp, data, length);
		else
			hex2mem(cp, data, length);
		err = wmem(addr, ks->threadid, data, length);
		free(data);
		return err;
	}

	return Einval;
}

// Yep, the protocol really is this shitty: errors are numbers. Assholes.
void
error_packet(char *pkt, char *error)
{
	pkt[0] = 'E';
	pkt[1] = error[0];
	pkt[2] = error[1];
	pkt[3] = '\0';
}

static char *
pack_threadid(char *pkt, unsigned char *id)
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

static void
int_to_threadref(unsigned char *id, int value)
{
	print("%s: panic\n", __func__);
	exits("fuck");
//  put_unaligned_be32(value, id);
}

/*
 * All the functions that start with gdb_cmd are the various
 * operations to implement the handlers for the gdbserial protocol
 * where KGDB is communicating with an external debugger
 */

/* Handle the '?' status packets */
static void
gdb_cmd_status(struct state *ks)
{
	/*
	 * We know that this packet is only sent
	 * during initial connect.  So to be safe,
	 * we clear out our breakpoints now in case
	 * GDB is reconnecting.
	 */
	dbg_remove_all_break(ks);

	remcom_out_buffer[0] = 'S';
	hex_byte_pack((char *)&remcom_out_buffer[1], ks->signo);
}

/* Handle the 'g' get registers request */
static void
gdb_cmd_getregs(struct state *ks)
{
I_AM_HERE;
	if (ks->threadid <= 0) {
		syslog(0, "gdbserver", "%s: id <= 0, fuck it, make it 1\n", __func__);
	}
	gpr(ks, ks->threadid);
	mem2hex(ks->gdbregs, (char *)remcom_out_buffer, NUMREGBYTES);
}

/* Handle the 'G' set registers request */
static void
gdb_cmd_setregs(struct state *ks)
{
	hex2mem((char *)&remcom_in_buffer[1], ks->gdbregs, NUMREGBYTES);

	error_packet(remcom_out_buffer, Einval);

	//gdb_regs_to_pt_regs(gdb_regs, ks->linux_regs);
//      strcpy(remcom_out_buffer, "OK");
}

/* Handle the 'm' memory read bytes */
static void
gdb_cmd_memread(struct state *ks)
{
	char *ptr = (char *)&remcom_in_buffer[1];
	unsigned long length;
	unsigned long addr;
	char *err;


	if (hex2long(&ptr, &addr) > 0 && *ptr++ == ',' &&
		hex2long(&ptr, &length) > 0) {
		char *data = malloc(length);
		if (err = rmem(data, ks->threadid, addr, length)) {
		    if(debug)
		        print("%s: %r", __func__);
			syslog(0, "gdbserver", "%s: %r", __func__);
			error_packet(remcom_out_buffer, err);
			free(data);
			return;
		}
		mem2hex((unsigned char *)data, (char *)remcom_out_buffer, length);
		free(data);
	} else {
		syslog(0, "gdbserver", "%s failed: %s : no good", __func__, remcom_in_buffer);
		error_packet(remcom_out_buffer, Einval);
	}
}

/* Handle the 'M' memory write bytes */
static void
gdb_cmd_memwrite(struct state *ks)
{
	char *err = write_mem_msg(ks, 0);

	if (err)
		error_packet(remcom_out_buffer, err);
	else
		strcpy((char *)remcom_out_buffer, "OK");
}

/* Handle the 'X' memory binary write bytes */
static void
gdb_cmd_binwrite(struct state *ks)
{
	char *err = write_mem_msg(ks, 1);

	if (err)
		error_packet(remcom_out_buffer, err);
	else
		strcpy((char *)remcom_out_buffer, "OK");
}

/* Handle the 'D' or 'k', detach or kill packets */
static void
gdb_cmd_detachkill(struct state *ks)
{
	char *error;

	/* The detach case */
	if (remcom_in_buffer[0] == 'D') {
		error = dbg_remove_all_break(ks);
		if (error < 0) {
			error_packet(remcom_out_buffer, error);
		} else {
			strcpy((char *)remcom_out_buffer, "OK");
			//connected = 0;
		}
		put_packet(remcom_out_buffer);
	} else {
		/*
		 * Assume the kill case, with no exit code checking,
		 * trying to force detach the debugger:
		 */
		dbg_remove_all_break(ks);
		connected = 0;
	}
}

/* Handle the 'R' reboot packets */
static int
gdb_cmd_reboot(struct state *ks)
{
	/* For now, only honor R0 */
	if (strcmp((char *)remcom_in_buffer, "R0") == 0) {
		print(" NOT Executing emergency reboot\n");
		strcpy((char *)remcom_out_buffer, "OK");
		put_packet(remcom_out_buffer);

		/*
		 * Execution should not return from
		 * machine_emergency_restart()
		 */
		connected = 0;

		return 1;
	}
	return 0;
}

/* Handle the 'q' query packets */
static void
gdb_cmd_query(struct state *ks)
{
	Dir *db;
	char *ptr;
	int fd, n;

	switch (remcom_in_buffer[1]) {
		case 'S':
			if (memcmp(remcom_in_buffer + 2, "upported", 8))
				break;
			// this code sucks.
			// We don't really have procs and threads for now. Just use the
			// thread id as the pid. i.e. no multiprocess.
			strcpy((char *)remcom_out_buffer, ""); // "multiprocess+");
			break;
		case 's':
		case 'f':
			if (memcmp(remcom_in_buffer + 2, "ThreadInfo", 10))
				break;

			fd = open("/proc", 0);
			if(fd == -1){
				error_packet(remcom_out_buffer, Einval);
				return;
			}
			n = dirreadall(fd, &db);
			if(n < 0) {
				error_packet(remcom_out_buffer, Eio);
				return;
			}

			close(fd);

			remcom_out_buffer[0] = 'm';
			ptr = (char *)remcom_out_buffer + 1;
			if (remcom_in_buffer[1] == 'f') {

				for(int i = 0; i < n; i++) {
					if (! isdigit(db[i].name[0]))
						continue;
					ptr = pack_threadid(ptr, (unsigned char *)db[i].name);
					*(ptr++) = ',';
					ks->thr_query++;
					if (ks->thr_query % MAX_THREAD_QUERY == 0)
						break;
				}
			}
			*(--ptr) = '\0';
			break;

		case 'C':
			/* Current thread id */
			strcpy((char *)remcom_out_buffer, "QC");
			pack_threadid((char *)remcom_out_buffer + 2, (uint8_t *) & ks->threadid);
			break;
		case 'T':
		    print("%s\n", remcom_in_buffer );
		    if (memcmp(remcom_in_buffer, "Status", 6) == 0) {
		        // TODO: proper status
		        strcpy(remcom_out_buffer, "Tnotrun:0");
		        break;
		    }
			error_packet(remcom_out_buffer, Einval);
			break;
	}
}

/* Handle the 'H' task query packets */
static void
gdb_cmd_task(struct state *ks)
{
	char *ptr;
	char *err;
	unsigned long id;

	switch (remcom_in_buffer[1]) {
		case 'g':
			ptr = (char *)&remcom_in_buffer[2];
			//hex2long(&ptr, &ks->threadid);
			if (ks->threadid <=  0) {
				syslog(0, "gdbserver", "Warning: using 1 instead of 0\n");
			}
			err = gpr(ks, ks->threadid);
			syslog(0, "gdbserver", "Try to use thread %d: %s\n", ks->threadid, err);
			if (err){
				error_packet(remcom_out_buffer, err);
			} else {
				strcpy((char *)remcom_out_buffer, "OK");
			}
			break;
		case 'c':
			ptr = (char *)&remcom_in_buffer[2];
			hex2long(&ptr, &id);
			strcpy((char *)remcom_out_buffer, "OK");
			break;
	}
}

/* Handle the 'T' thread query packets */
static void
gdb_cmd_thread(struct state *ks)
{
	//char *ptr = (char *)&remcom_in_buffer[1];
	char *err;

	//hex2long(&ptr, &ks->threadid);

	if (ks->threadid <= 0) {
		syslog(0, "gdbserver", "%s: id <= 0, fuck it, make it 1\n", __func__);
	}
	err = gpr(ks, ks->threadid);
	if (!err)
		strcpy((char *)remcom_out_buffer, "OK");
	else
		error_packet(remcom_out_buffer, Einval);
}

/* Handle the 'z' or 'Z' breakpoint remove or set packets */
static void
gdb_cmd_break(struct state *ks)
{
	/*
	 * Since GDB-5.3, it's been drafted that '0' is a software
	 * breakpoint, '1' is a hardware breakpoint, so let's do that.
	 */
	char *bpt_type = (char *)&remcom_in_buffer[1];
	char *ptr = (char *)&remcom_in_buffer[2];
	unsigned long addr;
	unsigned long length;
	char *error = nil;

	if (*bpt_type >= '0') {
		return;
	}

	if (*(ptr++) != ',') {
		error_packet(remcom_out_buffer, Einval);
		return;
	}
	if (!hex2long(&ptr, &addr)) {
		error_packet(remcom_out_buffer, Einval);
		return;
	}
	if (*(ptr++) != ',' || !hex2long(&ptr, &length)) {
		error_packet(remcom_out_buffer, Einval);
		return;
	}

	if (remcom_in_buffer[0] == 'Z' && *bpt_type == '0')
		error = dbg_set_sw_break(ks, addr);
	else if (remcom_in_buffer[0] == 'z' && *bpt_type == '0')
		error = dbg_remove_sw_break(ks, addr);

	if (error == 0)
		strcpy((char *)remcom_out_buffer, "OK");
	else
		error_packet(remcom_out_buffer, error);
}

/* Handle the 'C' signal / exception passing packets */
static int
gdb_cmd_exception_pass(struct state *ks)
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
		dbg_remove_all_break(ks);
		connected = 0;
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
int
gdb_serial_stub(struct state *ks, int port)
{
	int error = 0;
	int tmp;
	char adir[40], ldir[40], buff[256];
	int acfd, lcfd;

    sprint(buff, "tcp!*!%d", port);
	acfd = announce(buff, adir);
	if (acfd < 0) {
	    fprint(2, "Unable to connect %r\n");
	    exits("announce");
	}
	lcfd = listen(adir, ldir);
	if (lcfd < 0) {
	    fprint(2, "listen failed %r\n");
	    exits("listen");
	}
    print("Waiting for connection on %d...\n", port);
	remotefd = accept(lcfd, ldir);
	if (remotefd < 0) {
	    fprint(2, "Accept failed %r\n");
	    exits("accept");
	}
	print("Connected\n");
	sendctl(ks->threadid , "stop");

	/* Initialize comm buffer and globals. */
	memset(remcom_out_buffer, 0, sizeof(remcom_out_buffer));

	if (connected) {
		unsigned char thref[BUF_THREAD_ID_SIZE];
		char *ptr;

		/* Reply to host that an exception has occurred */
		ptr = (char *)remcom_out_buffer;
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
        syslog(0, "gdbserver", "packet :%s:\n", remcom_in_buffer);

		switch (remcom_in_buffer[0]) {
			case '?':	/* gdbserial status */
				gdb_cmd_status(ks);
				break;
			case 'g':	/* return the value of the CPU registers */
				gdb_cmd_getregs(ks);
				break;
			case 'G':	/* set the value of the CPU registers - return OK */
				gdb_cmd_setregs(ks);
				break;
			case 'm':	/* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
				gdb_cmd_memread(ks);
				break;
			case 'M':	/* MAA..AA,LLLL: Write LLLL bytes at address AA..AA */
				gdb_cmd_memwrite(ks);
				break;
			case 'p':	/* pXX Return gdb register XX (in hex) */
				gdb_cmd_reg_get(ks);
				break;
			case 'P':	/* PXX=aaaa Set gdb register XX to aaaa (in hex) */
				gdb_cmd_reg_set(ks);
				break;
			case 'X':	/* XAA..AA,LLLL: Write LLLL bytes at address AA..AA */
				gdb_cmd_binwrite(ks);
				break;
				/* kill or detach. KGDB should treat this like a
				 * continue.
				 */
			case 'D':	/* Debugger detach */
			case 'k':	/* Debugger detach via kill */
				gdb_cmd_detachkill(ks);
				goto default_handle;
			case 'R':	/* Reboot */
				if (gdb_cmd_reboot(ks))
					goto default_handle;
				break;
			case 'q':	/* query command */
				gdb_cmd_query(ks);
				break;
			case 'H':	/* task related */
				gdb_cmd_task(ks);
				break;
			case 'T':	/* Query thread status */
				gdb_cmd_thread(ks);
				break;
			case 'z':	/* Break point remove */
			case 'Z':	/* Break point set */
				gdb_cmd_break(ks);
				break;
#ifdef CONFIG_KDB
			case '3':	/* Escape into back into kdb */
				if (remcom_in_buffer[1] == '\0') {
					gdb_cmd_detachkill(ks);
					return DBG_PASS_EVENT;
				}
#endif
			case 'C':	/* Exception passing */
				tmp = gdb_cmd_exception_pass(ks);
				if (tmp > 0)
					goto default_handle;
				if (tmp == 0)
					break;
				/* Fall through on tmp < 0 */
			case 'c':	/* Continue packet */
			case 's':	/* Single step packet */
				dbg_activate_sw_breakpoints(ks);
				/* Fall through to default processing */
			default:
default_handle:
				if (remcom_in_buffer[0] == 'v') {
					// Specifically indicate we don't handle
					// v packets.
					strcpy((char *)remcom_out_buffer, "");
				}
				else if (error >= 0 || remcom_in_buffer[0] == 'D' ||
					remcom_in_buffer[0] == 'k') {
					/*
					* I have no idea what to do.
					* Leave cmd processing on error, detach,
					* kill, continue, or single step.
					*/
					error = 0;
					goto exit;
				}
		}
syslog(0, "gdbserver", "RETURN :%s:\n", remcom_out_buffer);
		/* reply to the request */
		put_packet(remcom_out_buffer);
	}

exit:
	if (ks->pass_exception)
		error = 1;
	return error;
}

char *
gdbstub_state(struct state *ks, char *cmd)
{
	char *error;

	switch (cmd[0]) {
		case 'e':
			error = Einval;
#if 0
			error = arch_handle_exception(ks->ex_vector,
										  ks->signo,
										  ks->err_code,
										  remcom_in_buffer,
										  remcom_out_buffer, ks->linux_regs);
#endif
			return error;
		case 's':
		case 'c':
			strcpy(remcom_in_buffer, cmd);
			return nil;
		case '$':
			strcpy(remcom_in_buffer, cmd);
			gdbstub_use_prev_in_buf = strlen(remcom_in_buffer);
			gdbstub_prev_in_buf_pos = 0;
			return nil;
	}
	write_char('+');
	put_packet(remcom_out_buffer);
	return nil;
}

/**
 * gdbstub_exit - Send an exit message to GDB
 * @status: The exit code to report.
 */
void
gdbstub_exit(int status)
{
	unsigned char checksum, ch, buffer[3];
	int loop;

	if (!connected)
		return;
	connected = 0;

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

char *
rmem(void *dest, int pid, uint64_t addr, int size)
{
	char *memname = smprint("/proc/%d/mem", pid);
	int fd = open(memname, 0);
	if (fd < 0) {
		syslog(0, "gdbserver", "open(%s, 0): %r\n", memname);
		return errstring(Enoent);
	}

	if (pread(fd, dest, size, addr) < size) {
		syslog(0, "gdbserver", "rmem(%p, %d, %p, %d): %r\n", dest, pid, addr, size);
		close(fd);
		return errstring(Eio);
	}
	close(fd);
	syslog(0, "gdbserver", "%s: read 0x%x for %d bytes\n", __func__, addr, size);
	return nil;
}

char *
wmem(uint64_t dest, int pid, void *addr, int size)
{
	char *memname = smprint("/proc/%d/mem", pid);
	int fd = open(memname, 1);
	if (fd < 0) {
		syslog(0, "gdbserver", "open(%s, 1): %r\n", memname);
		return errstring(Enoent);
	}

	if (pwrite(fd, addr, size, dest) < size) {
	    if (debug)
	        print("wmem(%p, %d, %p, %d): %r\n", dest, pid, addr, size);
		syslog(0, "gdbserver", "wmem(%p, %d, %p, %d): %r\n", dest, pid, addr, size);
		close(fd);
		return errstring(Eio);
	}
	close(fd);
	return nil;
}

void
gdbinit(void)
{
	bpsize = ebreakpoint - breakpoint;
}
static struct state ks;

void
main(int argc, char **argv)
{
    char* pid = nil;
    char* port = "1666";
    ARGBEGIN {
    case 'l':
        port = ARGF();
        if (port == nil) {
            fprint(2, "Please specify a listening port\n");
            exits("listen");
        }
        break;
    case 'p':
        pid = ARGF();
        if (pid == nil) {
            fprint(2, "Please specify a pid\n");
            exits("pid");
        }
        break;
    case 'd':
        debug = 1;
        break;
    default:
        fprint(2, " badflag('%c')", ARGC());
    } ARGEND

        if (pid == nil) {
            fprint(2, "Please specify a pid\n");
            exits("pid");
        }
		
    ks.threadid = atoi(pid);
	gdbinit();
	gdb_serial_stub(&ks, atoi(port));
}

/* Copyright (C) 1989, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: zfileio.c,v 1.18 2005/08/30 23:19:01 ray Exp $ */
/* File I/O operators */
#include "memory_.h"
#include "ghost.h"
#include "gp.h"
#include "oper.h"
#include "stream.h"
#include "files.h"
#include "store.h"
#include "strimpl.h"		/* for ifilter.h */
#include "ifilter.h"		/* for procedure streams */
#include "interp.h"		/* for gs_errorinfo_put_string */
#include "gsmatrix.h"		/* for gxdevice.h */
#include "gxdevice.h"
#include "gxdevmem.h"
#include "estack.h"

/* Forward references */
private int write_string(ref *, stream *);
private int handle_read_status(i_ctx_t *, int, const ref *, const uint *,
			       op_proc_t);
private int handle_write_status(i_ctx_t *, int, const ref *, const uint *,
				op_proc_t);

/* ------ Operators ------ */

/* <file> closefile - */
int
zclosefile(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;

    check_type(*op, t_file);
    if (file_is_valid(s, op)) {	/* closing a closed file is a no-op */
	int status = sclose(s);

	if (status != 0 && status != EOFC) {
	    if (s_is_writing(s))
		return handle_write_status(i_ctx_p, status, op, NULL,
					   zclosefile);
	    else
		return handle_read_status(i_ctx_p, status, op, NULL,
					  zclosefile);
	}
    }
    pop(1);
    return 0;
}

/* <file> read <int> -true- */
/* <file> read -false- */
private int
zread(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;
    int ch;

    check_read_file(s, op);
    /* We 'push' first in case of ostack block overflow and the */
    /* usual case is we will need to push anyway. If we get EOF */
    /* we will need to 'pop' and decrement the 'op' pointer.    */
    /* This is required since the 'push' macro might return with*/
    /* stackoverflow which will result in another stack block   */
    /* added on, then the operator being retried. We can't read */
    /* (sgetc) prior to having a place on the ostack to return  */
    /* the character.						*/
    push(1);
    ch = sgetc(s);
    if (ch >= 0) {
	make_int(op - 1, ch);
	make_bool(op, 1);
    } else {
	pop(1);		/* Adjust ostack back from preparatory 'pop' */
	op--;
	if (ch == EOFC) 
	make_bool(op, 0);
    else
	return handle_read_status(i_ctx_p, ch, op, NULL, zread);
    }
    return 0;
}

/* <file> <int> write - */
int
zwrite(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;
    byte ch;
    int status;

    check_write_file(s, op - 1);
    check_type(*op, t_integer);
    ch = (byte) op->value.intval;
    status = sputc(s, (byte) ch);
    if (status >= 0) {
	pop(2);
	return 0;
    }
    return handle_write_status(i_ctx_p, status, op - 1, NULL, zwrite);
}

/* <file> <string> readhexstring <substring> <filled_bool> */
private int zreadhexstring_continue(i_ctx_t *);

/* We keep track of the odd digit in the next byte of the string */
/* beyond the bytes already used.  (This is just for convenience; */
/* we could do the same thing by passing 2 state parameters to the */
/* continuation procedure instead of 1.) */
private int
zreadhexstring_at(i_ctx_t *i_ctx_p, os_ptr op, uint start)
{
    stream *s;
    uint len, nread;
    byte *str;
    int odd;
    stream_cursor_write cw;
    int status;

    check_read_file(s, op - 1);
    /*check_write_type(*op, t_string); *//* done by caller */
    str = op->value.bytes;
    len = r_size(op);
    if (start < len) {
	odd = str[start];
	if (odd > 0xf)
	    odd = -1;
    } else
	odd = -1;
    cw.ptr = str + start - 1;
    cw.limit = str + len - 1;
    for (;;) {
	status = s_hex_process(&s->cursor.r, &cw, &odd,
			       hex_ignore_garbage);
	if (status == 1) {	/* filled the string */
	    ref_assign_inline(op - 1, op);
	    make_true(op);
	    return 0;
	} else if (status != 0)	/* error or EOF */
	    break;
	/* Didn't fill, keep going. */
	status = spgetc(s);
	if (status < 0)
	    break;
	sputback(s);
    }
    nread = cw.ptr + 1 - str;
    if (status != EOFC) {	/* Error */
	if (nread < len)
	    str[nread] = (odd < 0 ? 0x10 : odd);
	return handle_read_status(i_ctx_p, status, op - 1, &nread,
				  zreadhexstring_continue);
    }
    /* Reached end-of-file before filling the string. */
    /* Return an appropriate substring. */
    ref_assign_inline(op - 1, op);
    r_set_size(op - 1, nread);
    make_false(op);
    return 0;
}
private int
zreadhexstring(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_write_type(*op, t_string);
    if (r_size(op) > 0)
	*op->value.bytes = 0x10;
    return zreadhexstring_at(i_ctx_p, op, 0);
}
/* Continue a readhexstring operation after a callout. */
/* *op is the index within the string. */
private int
zreadhexstring_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_type(*op, t_integer);
    if (op->value.intval < 0 || op->value.intval > r_size(op - 1))
	return_error(e_rangecheck);
    check_write_type(op[-1], t_string);
    code = zreadhexstring_at(i_ctx_p, op - 1, (uint) op->value.intval);
    if (code >= 0)
	pop(1);
    return code;
}

/* <file> <string> writehexstring - */
private int zwritehexstring_continue(i_ctx_t *);
private int
zwritehexstring_at(i_ctx_t *i_ctx_p, os_ptr op, uint odd)
{
    register stream *s;
    register byte ch;
    register const byte *p;
    register const char *const hex_digits = "0123456789abcdef";
    register uint len;
    int status;

#define MAX_HEX 128
    byte buf[MAX_HEX];

    check_write_file(s, op - 1);
    check_read_type(*op, t_string);
    p = op->value.bytes;
    len = r_size(op);
    while (len) {
	uint len1 = min(len, MAX_HEX / 2);
	register byte *q = buf;
	uint count = len1;
	ref rbuf;

	do {
	    ch = *p++;
	    *q++ = hex_digits[ch >> 4];
	    *q++ = hex_digits[ch & 0xf];
	}
	while (--count);
	r_set_size(&rbuf, (len1 << 1) - odd);
	rbuf.value.bytes = buf + odd;
	status = write_string(&rbuf, s);
	switch (status) {
	    default:
		return_error(e_ioerror);
	    case 0:
		len -= len1;
		odd = 0;
		continue;
	    case INTC:
	    case CALLC:
		count = rbuf.value.bytes - buf;
		op->value.bytes += count >> 1;
		r_set_size(op, len - (count >> 1));
		count &= 1;
		return handle_write_status(i_ctx_p, status, op - 1, &count,
					   zwritehexstring_continue);
	}
    }
    pop(2);
    return 0;
#undef MAX_HEX
}
private int
zwritehexstring(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    return zwritehexstring_at(i_ctx_p, op, 0);
}
/* Continue a writehexstring operation after a callout. */
/* *op is the odd/even hex digit flag for the first byte. */
private int
zwritehexstring_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_type(*op, t_integer);
    if ((op->value.intval & ~1) != 0)
	return_error(e_rangecheck);
    code = zwritehexstring_at(i_ctx_p, op - 1, (uint) op->value.intval);
    if (code >= 0)
	pop(1);
    return code;
}

/* <file> <string> readstring <substring> <filled_bool> */
private int zreadstring_continue(i_ctx_t *);
private int
zreadstring_at(i_ctx_t *i_ctx_p, os_ptr op, uint start)
{
    stream *s;
    uint len, rlen;
    int status;

    check_read_file(s, op - 1);
    check_write_type(*op, t_string);
    len = r_size(op);
    status = sgets(s, op->value.bytes + start, len - start, &rlen);
    rlen += start;
    switch (status) {
	case EOFC:
	case 0:
	    break;
	default:
	    return handle_read_status(i_ctx_p, status, op - 1, &rlen,
				      zreadstring_continue);
    }
    /*
     * The most recent Adobe specification says that readstring
     * must signal a rangecheck if the string length is zero.
     * I can't imagine the motivation for this, but we emulate it.
     * It's safe to check it here, rather than earlier, because if
     * len is zero, sgets will return 0 immediately with rlen = 0.
     */
    if (len == 0)
	return_error(e_rangecheck);
    r_set_size(op, rlen);
    op[-1] = *op;
    make_bool(op, (rlen == len ? 1 : 0));
    return 0;
}
private int
zreadstring(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    return zreadstring_at(i_ctx_p, op, 0);
}
/* Continue a readstring operation after a callout. */
/* *op is the index within the string. */
private int
zreadstring_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_type(*op, t_integer);
    if (op->value.intval < 0 || op->value.intval > r_size(op - 1))
	return_error(e_rangecheck);
    code = zreadstring_at(i_ctx_p, op - 1, (uint) op->value.intval);
    if (code >= 0)
	pop(1);
    return code;
}

/* <file> <string> writestring - */
private int
zwritestring(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;
    int status;

    check_write_file(s, op - 1);
    check_read_type(*op, t_string);
    status = write_string(op, s);
    if (status >= 0) {
	pop(2);
	return 0;
    }
    return handle_write_status(i_ctx_p, status, op - 1, NULL, zwritestring);
}

/* <file> <string> readline <substring> <bool> */
private int zreadline(i_ctx_t *);
private int zreadline_continue(i_ctx_t *);

/*
 * We could handle readline the same way as readstring,
 * except for the anomalous situation where we get interrupted
 * between the CR and the LF of an end-of-line marker.
 * We hack around this in the following way: if we get interrupted
 * before we've read any characters, we just restart the readline;
 * if we get interrupted at any other time, we use readline_continue;
 * we use start=0 (which we have just ruled out as a possible start value
 * for readline_continue) to indicate interruption after the CR.
 */
private int
zreadline_at(i_ctx_t *i_ctx_p, os_ptr op, uint count, bool in_eol)
{
    stream *s;
    int status;
    gs_string str;

    check_read_file(s, op - 1);
    check_write_type(*op, t_string);
    str.data = op->value.bytes;
    str.size = r_size(op);
    status = zreadline_from(s, &str, NULL, &count, &in_eol);
    switch (status) {
	case 0:
	case EOFC:
	    break;
	case 1:
	    return_error(e_rangecheck);
	default:
	    if (count == 0 && !in_eol)
		return handle_read_status(i_ctx_p, status, op - 1, NULL,
					  zreadline);
	    else {
		if (in_eol) {
		    r_set_size(op, count);
		    count = 0;
		}
		return handle_read_status(i_ctx_p, status, op - 1, &count,
					  zreadline_continue);
	    }
    }
    r_set_size(op, count);
    op[-1] = *op;
    make_bool(op, status == 0);
    return 0;
}
private int
zreadline(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    return zreadline_at(i_ctx_p, op, 0, false);
}
/* Continue a readline operation after a callout. */
/* *op is the index within the string, or 0 for an interrupt after a CR. */
private int
zreadline_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint size = r_size(op - 1);
    uint start;
    int code;

    check_type(*op, t_integer);
    if (op->value.intval < 0 || op->value.intval > size)
	return_error(e_rangecheck);
    start = (uint) op->value.intval;
    code = (start == 0 ? zreadline_at(i_ctx_p, op - 1, size, true) :
	    zreadline_at(i_ctx_p, op - 1, start, false));
    if (code >= 0)
	pop(1);
    return code;
}

/* Internal readline routine. */
/* Returns a stream status value, or 1 if we overflowed the string. */
/* This is exported for %lineedit. */
int
zreadline_from(stream *s, gs_string *buf, gs_memory_t *bufmem,
	       uint *pcount, bool *pin_eol)
{
    sreadline_proc((*readline));

    if (zis_stdin(s))
	readline = gp_readline;
    else
	readline = sreadline;
    return readline(s, NULL, NULL /*WRONG*/, NULL, buf, bufmem,
		    pcount, pin_eol, zis_stdin);
}

/* <file> bytesavailable <int> */
private int
zbytesavailable(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;
    long avail;

    check_read_file(s, op);
    switch (savailable(s, &avail)) {
	default:
	    return_error(e_ioerror);
	case EOFC:
	    avail = -1;
	case 0:
	    ;
    }
    make_int(op, avail);
    return 0;
}

/* - flush - */
int
zflush(i_ctx_t *i_ctx_p)
{
    stream *s;
    int status;
    ref rstdout;
    int code = zget_stdout(i_ctx_p, &s);

    if (code < 0)
	return code;

    make_stream_file(&rstdout, s, "w");
    status = sflush(s);
    if (status == 0 || status == EOFC) {
	return 0;
    }
    return
	(s_is_writing(s) ?
	 handle_write_status(i_ctx_p, status, &rstdout, NULL, zflush) :
	 handle_read_status(i_ctx_p, status, &rstdout, NULL, zflush));
}

/* <file> flushfile - */
private int
zflushfile(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;
    int status;

    check_type(*op, t_file);
    /*
     * We think flushfile is a no-op on closed input files, but causes an
     * error on closed output files.
     */
    if (file_is_invalid(s, op)) {
	if (r_has_attr(op, a_write))
	    return_error(e_invalidaccess);
	pop(1);
	return 0;
    }
    status = sflush(s);
    if (status == 0 || status == EOFC) {
	pop(1);
	return 0;
    }
    return
	(s_is_writing(s) ?
	 handle_write_status(i_ctx_p, status, op, NULL, zflushfile) :
	 handle_read_status(i_ctx_p, status, op, NULL, zflushfile));
}

/* <file> resetfile - */
private int
zresetfile(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;

    /* According to Adobe, resetfile is a no-op on closed files. */
    check_type(*op, t_file);
    if (file_is_valid(s, op))
	sreset(s);
    pop(1);
    return 0;
}

/* <string> print - */
private int
zprint(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;
    int status;
    ref rstdout;
    int code;

    check_read_type(*op, t_string);
    code = zget_stdout(i_ctx_p, &s);
    if (code < 0)
	return code;
    status = write_string(op, s);
    if (status >= 0) {
	pop(1);
	return 0;
    }
    /* Convert print to writestring on the fly. */
    make_stream_file(&rstdout, s, "w");
    code = handle_write_status(i_ctx_p, status, &rstdout, NULL,
			       zwritestring);
    if (code != o_push_estack)
	return code;
    push(1);
    *op = op[-1];
    op[-1] = rstdout;
    return code;
}

/* <bool> echo - */
private int
zecho(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_type(*op, t_boolean);
    /****** NOT IMPLEMENTED YET ******/
    pop(1);
    return 0;
}

/* ------ Level 2 extensions ------ */

/* <file> fileposition <int> */
private int
zfileposition(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;

    check_file(s, op);
    /*
     * The PLRM says fileposition must give an error for non-seekable
     * streams.
     */
    if (!s_can_seek(s))
	return_error(e_ioerror);
    make_int(op, stell(s));
    return 0;
}
/* <file> .fileposition <int> */
private int
zxfileposition(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;

    check_file(s, op);
    /*
     * This version of fileposition doesn't give the error, so we can
     * use it to get the position of string or procedure streams.
     */
    make_int(op, stell(s));
    return 0;
}

/* <file> <int> setfileposition - */
private int
zsetfileposition(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;

    check_file(s, op - 1);
    check_type(*op, t_integer);
    if (sseek(s, op->value.intval) < 0)
	return_error(e_ioerror);
    pop(2);
    return 0;
}

/* ------ Non-standard extensions ------ */

/* <file> .filename <string> true */
/* <file> .filename false */
private int
zfilename(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;
    gs_const_string fname;
    byte *str;

    check_file(s, op);
    if (sfilename(s, &fname) < 0) {
	make_false(op);
	return 0;
    }
    check_ostack(1);
    str = ialloc_string(fname.size, "filename");
    if (str == 0)
	return_error(e_VMerror);
    memcpy(str, fname.data, fname.size);
    push(1);			/* can't fail */
    make_const_string( op - 1 , 
		      a_all | imemory_space((const struct gs_ref_memory_s*) imemory), 
		      fname.size, 
		      str);
    make_true(op);
    return 0;
}

/* <file> .isprocfilter <bool> */
private int
zisprocfilter(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;

    check_file(s, op);
    while (s->strm != 0)
	s = s->strm;
    make_bool(op, s_is_proc(s));
    return 0;
}

/* <file> <string> .peekstring <substring> <filled_bool> */
private int
zpeekstring(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;
    uint len, rlen;

    check_read_file(s, op - 1);
    check_write_type(*op, t_string);
    len = r_size(op);
    while ((rlen = sbufavailable(s)) < len) {
	int status = s->end_status;

	switch (status) {
	case EOFC:
	    break;
	case 0:
	    /*
	     * The following is a HACK.  It should reallocate the buffer to hold
	     * at least len bytes.  However, this raises messy problems about
	     * which allocator to use and how it should interact with restore.
	     */
	    if (len >= s->bsize)
		return_error(e_rangecheck);
	    s_process_read_buf(s);
	    continue;
	default:
	    return handle_read_status(i_ctx_p, status, op - 1, NULL,
				      zpeekstring);
	}
	break;
    }
    if (rlen > len)
	rlen = len;
    /* Don't remove the data from the buffer. */
    memcpy(op->value.bytes, sbufptr(s), rlen);
    r_set_size(op, rlen);
    op[-1] = *op;
    make_bool(op, (rlen == len ? 1 : 0));
    return 0;
}

/* <file> <int> .unread - */
private int
zunread(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream *s;
    ulong ch;

    check_read_file(s, op - 1);
    check_type(*op, t_integer);
    ch = op->value.intval;
    if (ch > 0xff)
	return_error(e_rangecheck);
    if (sungetc(s, (byte) ch) < 0)
	return_error(e_ioerror);
    pop(2);
    return 0;
}

/* <file> <obj> <==flag> .writecvp - */
private int zwritecvp_continue(i_ctx_t *);
private int
zwritecvp_at(i_ctx_t *i_ctx_p, os_ptr op, uint start, bool first)
{
    stream *s;
    byte str[100];		/* arbitrary */
    ref rstr;
    const byte *data = str;
    uint len;
    int code, status;

    check_write_file(s, op - 2);
    check_type(*op, t_integer);
    code = obj_cvp(op - 1, str, sizeof(str), &len, (int)op->value.intval,
		   start, imemory);
    if (code == e_rangecheck) {
        code = obj_string_data(imemory, op - 1, &data, &len);
	if (len < start)
	    return_error(e_rangecheck);
	data += start;
	len -= start;
    }
    if (code < 0)
	return code;
    r_set_size(&rstr, len);
    rstr.value.const_bytes = data;
    status = write_string(&rstr, s);
    switch (status) {
	default:
	    return_error(e_ioerror);
	case 0:
	    break;
	case INTC:
	case CALLC:
	    len = start + len - r_size(&rstr);
	    if (!first)
		--osp;		/* pop(1) without affecting op */
	    return handle_write_status(i_ctx_p, status, op - 2, &len,
				       zwritecvp_continue);
    }
    if (code == 1) {
	if (first)
	    check_ostack(1);
	push_op_estack(zwritecvp_continue);
	if (first)
	    push(1);
	make_int(osp, start + len);
	return o_push_estack;
    }
    if (first)			/* zwritecvp */
	pop(3);
    else			/* zwritecvp_continue */
	pop(4);
    return 0;
}
private int
zwritecvp(i_ctx_t *i_ctx_p)
{
    return zwritecvp_at(i_ctx_p, osp, 0, true);
}
/* Continue a .writecvp after a callout. */
/* *op is the index within the string. */
private int
zwritecvp_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_type(*op, t_integer);
    if (op->value.intval != (uint) op->value.intval)
	return_error(e_rangecheck);
    return zwritecvp_at(i_ctx_p, op - 1, (uint) op->value.intval, false);
}

/* Callout for stdin */
/* - .needstdin - */
int
zneedstdin(i_ctx_t *i_ctx_p)
{
    return e_NeedStdin;		/* Interpreter will exit to caller. */
}

/* Callout for stdout */
/* - .needstdout - */
int
zneedstdout(i_ctx_t *i_ctx_p)
{
    return e_NeedStdout;	/* Interpreter will exit to caller. */
}

/* Callout for stderr */
/* - .needstderr - */
int
zneedstderr(i_ctx_t *i_ctx_p)
{
    return e_NeedStderr;	/* Interpreter will exit to caller. */
}



/* ------ Initialization procedure ------ */

/* We need to split the table because of the 16-element limit. */
const op_def zfileio1_op_defs[] = {
    {"1bytesavailable", zbytesavailable},
    {"1closefile", zclosefile},
		/* currentfile is in zcontrol.c */
    {"1echo", zecho},
    {"1.filename", zfilename},
    {"1.fileposition", zxfileposition},
    {"1fileposition", zfileposition},
    {"0flush", zflush},
    {"1flushfile", zflushfile},
    {"1.isprocfilter", zisprocfilter},
    {"2.peekstring", zpeekstring},
    {"1print", zprint},
    {"1read", zread},
    {"2readhexstring", zreadhexstring},
    {"2readline", zreadline},
    {"2readstring", zreadstring},
    op_def_end(0)
};
const op_def zfileio2_op_defs[] = {
    {"1resetfile", zresetfile},
    {"2setfileposition", zsetfileposition},
    {"2.unread", zunread},
    {"2write", zwrite},
    {"3.writecvp", zwritecvp},
    {"2writehexstring", zwritehexstring},
    {"2writestring", zwritestring},
		/* Internal operators */
    {"3%zreadhexstring_continue", zreadhexstring_continue},
    {"3%zreadline_continue", zreadline_continue},
    {"3%zreadstring_continue", zreadstring_continue},
    {"4%zwritecvp_continue", zwritecvp_continue},
    {"3%zwritehexstring_continue", zwritehexstring_continue},
    {"0.needstdin", zneedstdin},
    {"0.needstdout", zneedstdout},
    {"0.needstderr", zneedstderr},
    op_def_end(0)
};

/* ------ Non-operator routines ------ */

/* Switch a file open for read/write access but currently in write mode */
/* to read mode. */
int
file_switch_to_read(const ref * op)
{
    stream *s = fptr(op);

    if (s->write_id != r_size(op) || s->file == 0)	/* not valid */
	return_error(e_invalidaccess);
    if (sswitch(s, false) < 0)
	return_error(e_ioerror);
    s->read_id = s->write_id;	/* enable reading */
    s->write_id = 0;		/* disable writing */
    return 0;
}

/* Switch a file open for read/write access but currently in read mode */
/* to write mode. */
int
file_switch_to_write(const ref * op)
{
    stream *s = fptr(op);

    if (s->read_id != r_size(op) || s->file == 0)	/* not valid */
	return_error(e_invalidaccess);
    if (sswitch(s, true) < 0)
	return_error(e_ioerror);
    s->write_id = s->read_id;	/* enable writing */
    s->read_id = 0;		/* disable reading */
    return 0;
}

/* ------ Internal routines ------ */

/* Write a string on a file.  The file and string have been validated. */
/* If the status is INTC or CALLC, updates the string on the o-stack. */
private int
write_string(ref * op, stream * s)
{
    const byte *data = op->value.const_bytes;
    uint len = r_size(op);
    uint wlen;
    int status = sputs(s, data, len, &wlen);

    switch (status) {
	case INTC:
	case CALLC:
	    op->value.const_bytes = data + wlen;
	    r_set_size(op, len - wlen);
	    /* falls through */
	default:		/* 0, EOFC, ERRC */
	    return status;
    }
}

/*
 * Look for a stream error message that needs to be copied to
 * $error.errorinfo, if any.
 */
private int
copy_error_string(i_ctx_t *i_ctx_p, const ref *fop)
{
    stream *s;

    for (s = fptr(fop); s->strm != 0 && s->state->error_string[0] == 0;)
	s = s->strm;
    if (s->state->error_string[0]) {
	int code = gs_errorinfo_put_string(i_ctx_p, s->state->error_string);

	if (code < 0)
	    return code;
	s->state->error_string[0] = 0; /* just do it once */
    }
    return_error(e_ioerror);
}

/* Handle an exceptional status return from a read stream. */
/* fop points to the ref for the stream. */
/* ch may be any stream exceptional value. */
/* Return 0, 1 (EOF), o_push_estack, or an error. */
private int
handle_read_status(i_ctx_t *i_ctx_p, int ch, const ref * fop,
		   const uint * pindex, op_proc_t cont)
{
    switch (ch) {
	default:		/* error */
	    return copy_error_string(i_ctx_p, fop);
	case EOFC:
	    return 1;
	case INTC:
	case CALLC:
	    if (pindex) {
		ref index;

		make_int(&index, *pindex);
		return s_handle_read_exception(i_ctx_p, ch, fop, &index, 1,
					       cont);
	    } else
		return s_handle_read_exception(i_ctx_p, ch, fop, NULL, 0,
					       cont);
    }
}

/* Handle an exceptional status return from a write stream. */
/* fop points to the ref for the stream. */
/* ch may be any stream exceptional value. */
/* Return 0, 1 (EOF), o_push_estack, or an error. */
private int
handle_write_status(i_ctx_t *i_ctx_p, int ch, const ref * fop,
		    const uint * pindex, op_proc_t cont)
{
    switch (ch) {
	default:		/* error */
	    return copy_error_string(i_ctx_p, fop);
	case EOFC:
	    return 1;
	case INTC:
	case CALLC:
	    if (pindex) {
		ref index;

		make_int(&index, *pindex);
		return s_handle_write_exception(i_ctx_p, ch, fop, &index, 1,
						cont);
	    } else
		return s_handle_write_exception(i_ctx_p, ch, fop, NULL, 0,
						cont);
    }
}

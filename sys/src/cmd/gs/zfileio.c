/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* zfileio.c */
/* File I/O operators */
#include "ghost.h"
#include "gp.h"
#include "errors.h"
#include "oper.h"
#include "stream.h"
#include "files.h"
#include "store.h"
#include "strimpl.h"			/* for ifilter.h */
#include "ifilter.h"			/* for procedure streams */
#include "gsmatrix.h"			/* for gxdevice.h */
#include "gxdevice.h"
#include "gxdevmem.h"

/* Imported from gsfile.c */
extern int gs_writeppmfile(P2(gx_device_memory *, FILE *));

/* Forward references */
private int write_string(P2(ref *, stream *));
private int handle_read_status(P4(int, const ref *, const uint *,
  int (*)(P1(os_ptr))));
private int handle_write_status(P4(int, const ref *, const uint *,
  int (*)(P1(os_ptr))));

/* ------ Operators ------ */

/* <file> closefile - */
int
zclosefile(register os_ptr op)
{	stream *s;
	check_type(*op, t_file);
	if ( file_is_valid(s, op) )	/* closing a closed file is a no-op */
	{	int status = sclose(s);
		if ( status != 0 )
		  { if ( s_is_writing(s) )
		      return handle_write_status(status, op, NULL,
						 zclosefile);
		    else
		      return handle_read_status(status, op, NULL,
						zclosefile);
		  }
	}
	pop(1);
	return 0;
}

/* <file> read <int> -true- */
/* <file> read -false- */
int
zread(register os_ptr op)
{	stream *s;
	int ch;
	check_read_file(s, op);
	ch = sgetc(s);
	if ( ch >= 0 )
	{	push(1);
		make_int(op - 1, ch);
		make_bool(op, 1);
	}
	else if ( ch == EOFC )
		make_bool(op, 0);
	else
		return handle_read_status(ch, op, NULL, zread);
	return 0;
}

/* <file> <int> write - */
int
zwrite(register os_ptr op)
{	stream *s;
	byte ch;
	int status;
	check_write_file(s, op - 1);
	check_type(*op, t_integer);
	ch = (byte)op->value.intval;
	status = sputc(s, (byte)ch);
	if ( status >= 0 )
	{	pop(2);
		return 0;
	}
	return handle_write_status(status, op - 1, NULL, zwrite);
}

/* <file> <string> readhexstring <substring> <filled_bool> */
private int zreadhexstring_continue(P1(os_ptr));
/* We keep track of the odd digit in the next byte of the string */
/* beyond the bytes already used.  (This is just for convenience; */
/* we could do the same thing by passing 2 state parameters to the */
/* continuation procedure instead of 1.) */
private int
zreadhexstring_at(register os_ptr op, uint start)
{	stream *s;
	uint len, nread;
	byte *str;
	int odd;
	stream_cursor_write cw;
	int status;
	check_read_file(s, op - 1);
	/*check_write_type(*op, t_string);*/	/* done by caller */
	str = op->value.bytes;
	len = r_size(op);
	if ( start < len )
	  {	odd = str[start];
		if ( odd > 0xf ) odd = -1;
	  }
	else
	  odd = -1;
	cw.ptr = str + start - 1;
	cw.limit = str + len - 1;
	for ( ; ; )
	{	status = s_hex_process(&s->cursor.r, &cw, &odd,
				       hex_ignore_garbage);
		if ( status == 1 )	/* filled the string */
		  {	ref_assign_inline(op - 1, op);
			make_true(op);
			return 0;
		  }
		else if ( status != 0 )		/* error or EOF */
		  break;
		/* Didn't fill, keep going. */
		status = spgetc(s);
		if ( status < 0 )
		  break;
		sputback(s);
	}
	nread = cw.ptr + 1 - str;
	if ( status != EOFC )
	  {	/* Error */
		if ( nread < len )
		  str[nread] = (odd < 0 ? 0x10 : odd);
		return handle_read_status(status, op - 1, &nread,
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
zreadhexstring(os_ptr op)
{	check_write_type(*op, t_string);
	if ( r_size(op) > 0 )
	  *op->value.bytes = 0x10;
	return zreadhexstring_at(op, 0);
}
/* Continue a readhexstring operation after a callout. */
/* *op is the index within the string. */
private int
zreadhexstring_continue(register os_ptr op)
{	int code;
	check_type(*op, t_integer);
	if ( op->value.intval < 0 || op->value.intval > r_size(op - 1) )
	  return_error(e_rangecheck);
	check_write_type(op[-1], t_string);
	code = zreadhexstring_at(op - 1, (uint)op->value.intval);
	if ( code >= 0 )
	  pop(1);
	return code;
}

/* <file> <string> writehexstring - */
private int zwritehexstring_continue(P1(os_ptr));
private int
zwritehexstring_at(register os_ptr op, uint odd)
{	register stream *s;
	register byte ch;
	register const byte *p;
	register const char _ds *hex_digits = "0123456789abcdef";
	register uint len;
	int status;
#define max_hex 128
	byte buf[max_hex];
	check_write_file(s, op - 1);
	check_read_type(*op, t_string);
	p = op->value.bytes;
	len = r_size(op);
	while ( len )
	{	uint len1 = min(len, max_hex / 2);
		register byte *q = buf;
		uint count = len1;
		ref rbuf;
		do
		{	ch = *p++;
			*q++ = hex_digits[ch >> 4];
			*q++ = hex_digits[ch & 0xf];
		}
		while ( --count );
		r_set_size(&rbuf, (len1 << 1) - odd);
		rbuf.value.bytes = buf + odd;
		status = write_string(&rbuf, s);
		switch ( status )
		{
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
			return handle_write_status(status, op - 1, &count,
						   zwritehexstring_continue);
		}
	}
	pop(2);
	return 0;
#undef max_hex
}
private int
zwritehexstring(os_ptr op)
{	return zwritehexstring_at(op, 0);
}
/* Continue a writehexstring operation after a callout. */
/* *op is the odd/even hex digit flag for the first byte. */
private int
zwritehexstring_continue(register os_ptr op)
{	int code;
	check_type(*op, t_integer);
	if ( (op->value.intval & ~1) != 0 )
	  return_error(e_rangecheck);
	code = zwritehexstring_at(op - 1, (uint)op->value.intval);
	if ( code >= 0 )
	  pop(1);
	return code;
}

/* <file> <string> readstring <substring> <filled_bool> */
private int zreadstring_continue(P1(os_ptr));
private int
zreadstring_at(register os_ptr op, uint start)
{	stream *s;
	uint len, rlen;
	int status;
	check_read_file(s, op - 1);
	check_write_type(*op, t_string);
	len = r_size(op);
	status = sgets(s, op->value.bytes + start, len - start, &rlen);
	rlen += start;
	switch ( status )
	{
	case EOFC:
	case 0:
		break;
	default:
		return handle_read_status(status, op - 1, &rlen,
					  zreadstring_continue);
	}
	r_set_size(op, rlen);
	op[-1] = *op;
	make_bool(op, (rlen == len ? 1 : 0));
	return 0;
}
private int
zreadstring(os_ptr op)
{	return zreadstring_at(op, 0);
}
/* Continue a readstring operation after a callout. */
/* *op is the index within the string. */
private int
zreadstring_continue(register os_ptr op)
{	int code;
	check_type(*op, t_integer);
	if ( op->value.intval < 0 || op->value.intval > r_size(op - 1) )
	  return_error(e_rangecheck);
	code = zreadstring_at(op - 1, (uint)op->value.intval);
	if ( code >= 0 )
	  pop(1);
	return code;
}

/* <file> <string> writestring - */
int
zwritestring(register os_ptr op)
{	stream *s;
	int status;
	check_write_file(s, op - 1);
	check_read_type(*op, t_string);
	status = write_string(op, s);
	if ( status >= 0 )
	{	pop(2);
		return 0;
	}
	return handle_write_status(status, op - 1, NULL, zwritestring);
}

/* <file> <string> readline <substring> <bool> */
private int zreadline(P1(os_ptr));
private int zreadline_continue(P1(os_ptr));
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
zreadline_at(register os_ptr op, uint count, bool in_eol)
{	stream *s;
	byte *ptr;
	uint len;
	int status;
	check_read_file(s, op - 1);
	check_write_type(*op, t_string);
	ptr = op->value.bytes;
	len = r_size(op);
	status = zreadline_from(s, ptr, len, &count, &in_eol);
	switch ( status )
	{
	case 0:
	case EOFC:
		break;
	case 1:
		return_error(e_rangecheck);
	default:
		if ( count == 0 && !in_eol )
		  return handle_read_status(status, op - 1, NULL,
					    zreadline);
		else
		  { if ( in_eol )
		      { r_set_size(op, count);
			count = 0;
		      }
		    return handle_read_status(status, op - 1, &count,
					      zreadline_continue);
		}
	}
	r_set_size(op, count);
	op[-1] = *op;
	make_bool(op, status == 0);
	return 0;
}
private int
zreadline(register os_ptr op)
{	return zreadline_at(op, 0, false);
}
/* Continue a readline operation after a callout. */
/* *op is the index within the string, or 0 for an interrupt after a CR. */
private int
zreadline_continue(register os_ptr op)
{	uint start;
	int code;
	check_type(*op, t_integer);
	if ( op->value.intval < 0 || op->value.intval > r_size(op - 1) )
	  return_error(e_rangecheck);
	start = (uint)op->value.intval;
	code = (start == 0 ? zreadline_at(op - 1, 0, true) :
		zreadline_at(op - 1, start, false));
	if ( code >= 0 )
	  pop(1);
	return code;
}

/* Internal readline routine. */
/* Returns a stream status value, or 1 if we overflowed the string. */
/* This is exported for %lineedit. */
int
zreadline_from(stream *s, byte *ptr, uint size, uint *pcount, bool *pin_eol)
{	uint count = *pcount;

	/* Most systems define \n as 0xa and \r as 0xd; however, */
	/* OS-9 has \n == \r == 0xd and \l == 0xa.  The following */
	/* code works properly regardless of environment. */
#if '\n' == '\r'
#  define LF 0xa
#else
#  define LF '\n'
#endif

top:	if ( *pin_eol )
	{	/*
		 * We're in the middle of checking for a two-character
		 * end-of-line sequence.  If we get an EOF here, stop, but
		 * don't signal EOF now; wait till the next read.
		 */
		int ch;
		*pin_eol = false;
		while ( sendrp(s) )
		  {	int status = s->end_status;
			if ( status == EOFC )
			  return 0;
			else if ( status < 0 )
			  return status;
			s_process_read_buf(s);
		  }
		ch = sgetc(s);	/* can't fail */
		if ( ch != LF )
		  sputback(s);
		return 0;
	}
	for ( ; ; )
	{	int ch = sgetc(s);
		if ( ch < 0 )		/* EOF or exception */
		{	*pcount = count;
			return ch;
		}

		switch ( ch )
		{
		case '\r':
#if '\n' == '\r'			/* OS-9 or similar */
			if ( s != gs_stream_stdin )
#endif
			{	*pcount = count;
				*pin_eol = true;
				goto top;
			}
			/* falls through */
		case LF:
#undef LF
			*pcount = count;
			return 0;
		}
		if ( count >= size )	/* filled the string */
		{	sputback(s);
			return 1;
		}
		ptr[count++] = ch;
	}
	/*return 0;*/		/* not reached */
}

/* <file> bytesavailable <int> */
int
zbytesavailable(register os_ptr op)
{	stream *s;
	long avail;
	check_read_file(s, op);
	switch ( savailable(s, &avail) )
	  {
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
zflush(register os_ptr op)
{	sflush(gs_stream_stdout);
	return 0;
}

/* <file> flushfile - */
int
zflushfile(register os_ptr op)
{	stream *s;
	int status;
	check_file(s, op);
	status = sflush(s);
	if ( status == 0 )
	{	pop(1);
		return 0;
	}
	return handle_write_status(status, op, NULL, zflushfile);
}

/* <file> resetfile - */
int
zresetfile(register os_ptr op)
{	stream *s;
	check_file(s, op);
	sreset(s);
	pop(1);
	return 0;
}

/* <string> print - */
int
zprint(register os_ptr op)
{	int status;
	ref rstdout;
	int code;
	check_read_type(*op, t_string);
	status = write_string(op, gs_stream_stdout);
	if ( status >= 0 )
	{	pop(1);
		return 0;
	}
	/* Convert print to writestring on the fly. */
	make_stream_file(&rstdout, gs_stream_stdout, "w");
	code = handle_write_status(status, &rstdout, NULL, zwritestring);
	if ( code != o_push_estack )
	  return code;
	push(1);
	*op = op[-1];
	op[-1] = rstdout;
	return code;
}

/* <bool> echo - */
int
zecho(register os_ptr op)
{	check_type(*op, t_boolean);
	/****** NOT IMPLEMENTED YET ******/
	pop(1);
	return 0;
}

/* ------ Level 2 extensions ------ */

/* <file> fileposition <int> */
int
zfileposition(register os_ptr op)
{	stream *s;
	check_file(s, op);
	make_int(op, stell(s));
	return 0;
}

/* <file> <int> setfileposition - */
int
zsetfileposition(register os_ptr op)
{	stream *s;
	check_file(s, op - 1);
	check_type(*op, t_integer);
	if ( sseek(s, op->value.intval) < 0 )
		return_error(e_ioerror);
	pop(2);
	return 0;
}

/* ------ Non-standard extensions ------ */

/* <file> <int> unread - */
int
zunread(register os_ptr op)
{	stream *s;
	ulong ch;
	check_read_file(s, op - 1);
	check_type(*op, t_integer);
	ch = op->value.intval;
	if ( ch > 0xff )
		return_error(e_rangecheck);
	if ( sungetc(s, (byte)ch) < 0 )
		return_error(e_ioerror);
	pop(2);
	return 0;
}

/* <file> <object> .writecvs - */
private int zwritecvs_continue(P1(os_ptr));
private int
zwritecvs_at(register os_ptr op, uint start)
{	stream *s;
#define max_cvs 128
	byte str[max_cvs];
	ref rstr;
	byte *pchars = str;
	uint len;
	int code, status;
	check_write_file(s, op - 1);
	code = obj_cvs(op, str, max_cvs, &len, &pchars);
	if ( code < 0 )
	{	if ( pchars == str )
			return code;
	}
	if ( start > len )
		return_error(e_rangecheck);
	r_set_size(&rstr, len - start);
	rstr.value.bytes = pchars + start;
	status = write_string(&rstr, s);
	switch ( status )
	{
	default:
		return_error(e_ioerror);
	case 0:
		break;
	case INTC:
	case CALLC:
		len -= r_size(&rstr);
		return handle_write_status(status, op - 1, &len,
					   zwritecvs_continue);
	}
	pop(2);
	return 0;
#undef max_cvs
}
private int
zwritecvs(os_ptr op)
{	return zwritecvs_at(op, 0);
}
/* Continue a .writecvs after a callout. */
/* *op is the index within the string. */
private int
zwritecvs_continue(os_ptr op)
{	int code;
	check_type(*op, t_integer);
	if ( op->value.intval != (uint)op->value.intval )
	  return_error(e_rangecheck);
	code = zwritecvs_at(op - 1, (uint)op->value.intval);
	if ( code >= 0 )
	  pop(1);
	return code;
}

/* <file> <device> writeppmfile - */
int
zwriteppmfile(register os_ptr op)
{	stream *s;
	int code;
	check_type(*op, t_device);
	check_write_file(s, op - 1);
	if ( !gs_device_is_memory(op->value.pdevice) )
	  return_error(e_rangecheck);
	sflush(s);
	code = gs_writeppmfile((gx_device_memory *)(op->value.pdevice), s->file);
	if ( code >= 0 )
	  pop(2);
	return code;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zfileio_op_defs) {
	{"1bytesavailable", zbytesavailable},
	{"1closefile", zclosefile},
		/* currentfile is in zcontrol.c */
	{"1echo", zecho},
	{"1fileposition", zfileposition},
	{"0flush", zflush},
	{"1flushfile", zflushfile},
	{"1print", zprint},
	{"1read", zread},
	{"2readhexstring", zreadhexstring},
	{"2readline", zreadline},
	{"2readstring", zreadstring},
	{"1resetfile", zresetfile},
	{"2setfileposition", zsetfileposition},
	{"2unread", zunread},
	{"2write", zwrite},
	{"2.writecvs", zwritecvs},
	{"2writehexstring", zwritehexstring},
	{"2writeppmfile", zwriteppmfile},
	{"2writestring", zwritestring},
END_OP_DEFS(0) }

/* ------ Non-operator routines ------ */

/* Switch a file open for read/write access but currently in write mode */
/* to read mode. */
int
file_switch_to_read(ref *op)
{	stream *s = fptr(op);
	uint modes = s->file_modes;
	long pos;
	if ( s->file == 0 || !(modes & s_mode_read) )	/* not valid */
	  return_error(e_invalidaccess);
	pos = stell(s);
	if ( sflush(s) < 0 )
	  return_error(e_ioerror);
	s->read_id = s->write_id;		/* enable reading */
	s->write_id = 0;			/* disable writing */
	sread_file(s, s->file, s->cbuf, s->cbsize);
	fseek(s->file, 0L, SEEK_CUR);		/* pacify C library */
	s->modes |= modes & s_mode_append;	/* don't lose append info */
	s->file_modes = modes;
	s->position = pos;
	return 0;
}

/* Switch a file open for read/write access but currently in read mode */
/* to write mode. */
int
file_switch_to_write(ref *op)
{	stream *s = fptr(op);
	uint modes = s->file_modes;
	long pos;
	if ( s->file == 0 || !(modes & s_mode_write) )	/* not valid */
	  return_error(e_invalidaccess);
	pos = stell(s);
	s->write_id = s->read_id;		/* enable writing */
	s->read_id = 0;				/* disable reading */
	fseek(s->file, pos, SEEK_SET);		/* pacify C library */
	if ( modes & s_mode_append )
	  {	sappend_file(s, s->file, s->cbuf, s->cbsize);	/* sets position */
	  }
	else
	  {	swrite_file(s, s->file, s->cbuf, s->cbsize);
		s->position = pos;
	  }
	s->file_modes = modes;
	return 0;
}

/* ------ Internal routines ------ */

/* Write a string on a file.  The file and string have been validated. */
/* If the status is INTC or CALLC, updates the string on the o-stack. */
private int
write_string(ref *op, stream *s)
{	const byte *data = op->value.const_bytes;
	uint len = r_size(op);
	uint wlen;
	int status = sputs(s, data, len, &wlen);
	switch ( status )
	{
	case INTC:
	case CALLC:
		op->value.const_bytes = data + wlen;
		r_set_size(op, len - wlen);
		/* falls through */
	default:		/* 0, EOFC, ERRC */
		return status;
	}
}

/* Handle an exceptional status return from a read stream. */
/* fop points to the ref for the stream. */
/* ch may be any stream exceptional value. */
/* Return 0, 1 (EOF), o_push_estack, or an error. */
private int
handle_read_status(int ch, const ref *fop, const uint *pindex,
  int (*cont)(P1(os_ptr)))
{	switch ( ch )
	{
	default:		/* error */
		return_error(e_ioerror);
	case EOFC:
		return 1;
	case INTC:
	case CALLC:
	{	ref index;
		const ref *pstate;
		if ( pindex )
		  {	make_int(&index, *pindex);
			pstate = &index;
		  }
		else
			pstate = NULL;
		return s_handle_read_exception(ch, fop, pstate, cont);
	}
	}
}

/* Handle an exceptional status return from a write stream. */
/* fop points to the ref for the stream. */
/* ch may be any stream exceptional value. */
/* Return 0, o_push_estack, or an error. */
private int
handle_write_status(int ch, const ref *fop, const uint *pindex,
  int (*cont)(P1(os_ptr)))
{	switch ( ch )
	{
	default:		/* error */
		return_error(e_ioerror);
	case INTC:
	case CALLC:
	{	ref index;
		const ref *pstate;
		if ( pindex )
		  {	make_int(&index, *pindex);
			pstate = &index;
		  }
		else
			pstate = NULL;
		return s_handle_write_exception(ch, fop, pstate, cont);
	}
	}
}

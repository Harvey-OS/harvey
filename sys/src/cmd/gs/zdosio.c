/* Copyright (C) 1992 Aladdin Enterprises.  All rights reserved.
  
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

/* zdosio.c */
/* MS-DOS direct I/O operators. */
/* These should NEVER be included in a released configuration! */
#include "dos_.h"
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "store.h"

/* <port> .inport <word> */
private int
zinport(register os_ptr op)
{	check_type(*op, t_integer);
	make_int(op, inport((int)op->value.intval));
	return 0;
}

/* <port> .inportb <byte> */
private int
zinportb(register os_ptr op)
{	check_type(*op, t_integer);
	make_int(op, inportb((int)op->value.intval));
	return 0;
}

/* <port> <word> .outport - */
private int
zoutport(register os_ptr op)
{	check_type(*op, t_integer);
	check_type(op[-1], t_integer);
	outport((int)op[-1].value.intval, (int)op->value.intval);
	pop(1);
	return 0;
}

/* <port> <byte> .outportb - */
private int
zoutportb(register os_ptr op)
{	check_type(*op, t_integer);
	check_int_leu(op[-1], 0xff);
	outportb((int)op[-1].value.intval, (byte)op->value.intval);
	pop(1);
	return 0;
}

/* <loc> .peek <byte> */
private int
zpeek(register os_ptr op)
{	check_type(*op, t_integer);
	make_int(op, *(byte *)(op->value.intval));
	return 0;
}

/* <loc> <byte> .poke - */
private int
zpoke(register os_ptr op)
{	check_type(*op, t_integer);
	check_int_leu(op[-1], 0xff);
	*(byte *)(op[-1].value.intval) = (byte)op->value.intval;
	pop(1);
	return 0;
}

/* ------ Operator initialization ------ */

BEGIN_OP_DEFS(zdosio_op_defs) {
	{"1.inport", zinport},
	{"1.inportb", zinportb},
	{"2.outport", zoutport},
	{"2.outportb", zoutportb},
	{"1.peek", zpeek},
	{"2.poke", zpoke},
END_OP_DEFS(0) }

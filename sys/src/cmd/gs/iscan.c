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

/* iscan.c */
/* Token scanner for Ghostscript interpreter */
#include "ghost.h"
#include "memory_.h"
#include "stream.h"
#include "errors.h"
#include "files.h"			/* for fptr */
#include "ialloc.h"
#include "idict.h"			/* for //name lookup */
#include "dstack.h"			/* ditto */
#include "ilevel.h"
#include "iname.h"
#include "ipacked.h"
#include "iparray.h"
#include "strimpl.h"			/* for string decoding */
#include "sfilter.h"			/* ditto */
#include "ostack.h"			/* for accumulating proc bodies; */
					/* must precede iscan.h */
#include "iscan.h"			/* defines interface */
#include "iscannum.h"
#include "istream.h"
#include "istruct.h"		/* for gs_reloc_refs */
#include "iutil.h"
#include "ivmspace.h"
#include "store.h"
#include "scanchar.h"

/* Array packing flag */
ref ref_array_packing;			/* t_boolean */
/* Binary object format flag. This will never be set non-zero */
/* unless the binary token feature is enabled. */
ref ref_binary_object_format;		/* t_integer */
#define recognize_btokens()\
  (ref_binary_object_format.value.intval != 0 && level2_enabled)

/* Procedure for binary tokens.  Set at initialization if Level 2 */
/* features are included; only called if recognize_btokens() is true. */
/* Returns 0 or scan_BOS on success, <0 on failure. */
int (*scan_btoken_proc)(P3(stream *, ref *, scanner_state *)) = NULL;

/* Stream template for scanning ASCII85 literals. */
/* Set at initialization if Level 2 features are included. */
const stream_template _ds *scan_ascii85_template = NULL;

/* Procedure for handling DSC comments if desired. */
/* Set at initialization if a DSC handling module is included. */
int (*scan_dsc_proc)(P2(const byte *, uint)) = NULL;

/*
 * Level 2 includes some changes in the scanner:
 *	- \ is always recognized in strings, regardless of the data source;
 *	- << and >> are legal tokens;
 *	- <~ introduces an ASCII85 encoded string (terminated by ~>);
 *	- Character codes above 127 introduce binary objects.
 * We explicitly enable or disable these changes here.
 */
#define scan_enable_level2 level2_enabled	/* from ilevel.h */

/* ------ Dynamic strings ------ */

/* Begin collecting a dynamically allocated string. */
#define dynamic_init(pda, mem)\
	((pda)->is_dynamic = false,\
	 (pda)->limit = (pda)->buf + da_buf_size,\
	 (pda)->next = (pda)->base = (pda)->buf,\
	 (pda)->memory = (mem))

/* Free a dynamic string. */
private void
dynamic_free(da_ptr pda)
{	if ( pda->is_dynamic )
	  gs_free_string(pda->memory, pda->base, da_size(pda), "scanner");
}

/* Resize a dynamic string. */
/* If the allocation fails, return e_VMerror; otherwise, return 0. */
private int
dynamic_resize(register da_ptr pda, uint new_size)
{	uint old_size = da_size(pda);
	uint pos = pda->next - pda->base;
	gs_memory_t *mem = pda->memory;
	byte *base;
	if ( pda->is_dynamic )
	{	base = gs_resize_string(mem, pda->base, old_size,
					new_size, "scanner");
		if ( base == 0 )
			return_error(e_VMerror);
	}
	else		/* switching from static to dynamic */
	{	base = gs_alloc_string(mem, new_size, "scanner");
		if ( base == 0 )
		  return_error(e_VMerror);
		memcpy(base, pda->base, min(old_size, new_size));
		pda->is_dynamic = true;
	}
	pda->base = base;
	pda->next = base + pos;
	pda->limit = base + new_size;
	return 0;
}

/* Grow a dynamic string. */
/* Return 0 if the allocation failed, the new 'next' ptr if OK. */
private byte *
dynamic_grow(register da_ptr pda, byte *next)
{	uint old_size = da_size(pda);
	uint new_size = (old_size < 10 ? 20 :
			 old_size >= (max_uint >> 1) ? max_uint :
			 old_size << 1);
	int code;
	pda->next = next;
	while ( (code = dynamic_resize(pda, new_size)) < 0 &&
		new_size > old_size
	      )
	{	/* Try trimming down the requested new size. */
		new_size -= (new_size - old_size + 1) >> 1;
	}
	if ( code < 0 )
	  return NULL;
	return pda->next;
}

/* Ensure that a dynamic string is either on the heap or in the */
/* private buffer. */
private void
dynamic_save(da_ptr pda)
{	if ( !pda->is_dynamic && pda->base != pda->buf )
	  {	memcpy(pda->buf, pda->base, da_size(pda));
		pda->next = pda->buf + da_size(pda);
		pda->base = pda->buf;
	  }
}

/* Finish collecting a dynamic string. */
private int
dynamic_make_string(ref *pref, da_ptr pda, byte *next)
{	uint size = (pda->next = next) - pda->base;
	int code = dynamic_resize(pda, size);
	if ( code < 0 )
	  return code;
	make_tasv_new(pref, t_string,
		      a_all | imemory_space((gs_ref_memory_t *)pda->memory),
		      size, bytes, pda->base);
	return 0;
}

/* ------ Main scanner ------ */

/* GC procedures */
#define ssptr ((scanner_state *)vptr)
#define ssarray ssptr->s_ss.binary.bin_array
private CLEAR_MARKS_PROC(scanner_clear_marks) {
	r_clear_attrs(&ssarray, l_mark);
}
private ENUM_PTRS_BEGIN(scanner_enum_ptrs) return 0;
	case 0:
		*pep = 0;
		if ( ssptr->s_scan_type == scanning_none ||
		     !ssptr->s_da.is_dynamic
		   )
		  break;
		ssptr->s_da.str.data = ssptr->s_da.base;
		ssptr->s_da.str.size = da_size(&ssptr->s_da);
		ENUM_RETURN_STRING_PTR(scanner_state, s_da.str);
	case 1:
		if ( ssptr->s_scan_type != scanning_binary )
		  return 0;
		*pep = &ssarray;
		return ptr_ref_type;
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(scanner_reloc_ptrs) {
	if ( ssptr->s_scan_type != scanning_none && ssptr->s_da.is_dynamic )
	  {	RELOC_STRING_PTR(scanner_state, s_da.str);
		ssptr->s_da.limit = ssptr->s_da.str.data + ssptr->s_da.str.size;
		ssptr->s_da.next = ssptr->s_da.str.data + (ssptr->s_da.next - ssptr->s_da.base);
		ssptr->s_da.base = ssptr->s_da.str.data;
	  }
	if ( ssptr->s_scan_type == scanning_binary )
	  {	gs_reloc_refs((ref_packed *)&ssarray,
			      (ref_packed *)(&ssarray + 1),
			      gcst);
		r_clear_attrs(&ssarray, l_mark);
	  }
} RELOC_PTRS_END
#undef ssptr
/* Structure type */
public_st_scanner_state();

/* Initialize the scanner. */
void
scan_init(void)
{	make_false(&ref_array_packing);
	make_int(&ref_binary_object_format, 0);
}

/* Handle a scan_Refill return from scan_token. */
/* This may return o_push_estack, 0 (meaning just call scan_token again), */
/* or an error code. */
int
scan_handle_refill(const ref *fop, scanner_state *sstate, bool save,
  int (*cont)(P1(os_ptr)))
{	stream *s = fptr(fop);
	uint avail = sbufavailable(s);
	int status;
	scanner_state *pstate;
	ref rstate;
	if ( s->end_status == EOFC )
	  {	/* More data needed, but none available, */
		/* so this is a syntax error. */
		return_error(e_syntaxerror);
	  }
	status = s_process_read_buf(s);
	if ( sbufavailable(s) > avail )
		return 0;
	if ( status == 0 )
		status = s->end_status;
	switch ( status )
	  {
	  case EOFC:
		/* We just discovered that we're at EOF. */
		/* Let the caller find this out. */
		return 0;
	  case ERRC:
		return_error(e_ioerror);
	  case INTC:
	  case CALLC:
		if ( save )
		  {	pstate =
			  ialloc_struct(scanner_state, &st_scanner_state,
					"scan_handle_refill");
			if ( pstate == 0 )
			  return_error(e_VMerror);
			*pstate = *sstate;
		  }
		else
			pstate = sstate;
		make_istruct(&rstate, 0, pstate);
		return s_handle_read_exception(status, fop, &rstate, cont);
	  }
	/* No more data available, but no exception.  How can this be? */
	lprintf("Can't refill scanner input buffer!");
	return_error(e_Fatal);
}

/*
 * Read a token from a stream.
 * Return 0 if an ordinary token was read,
 * scan_BOS for a binary object sequence, scan_EOF for end-of-stream,
 * scan_Refill if more data needed, or a (negative) error code.
 * If the token required a terminating character (i.e., was a name or
 * number) and the next character was whitespace, read and discard
 * that character.  Note that the state is relevant for e_VMerror
 * as well as for scan_Refill.
 */
int
scan_token(register stream *s, ref *pref, scanner_state *pstate)
{	ref *myref = pref;
	int retcode = 0;
	register int c;
	s_declare_inline(s, sptr, endptr);
#define scan_begin_inline() s_begin_inline(s, sptr, endptr)
#define scan_getc() sgetc_inline(s, sptr, endptr)
#define scan_putback() sputback_inline(s, sptr, endptr)
#define scan_end_inline() s_end_inline(s, sptr, endptr)
	const byte *newptr;
	byte *daptr;
#define sreturn(code)\
  { retcode = gs_note_error(code); goto sret; }
#define sreturn_no_error(code)\
  { scan_end_inline(); return(code); }
#define if_not_spush1()\
  if ( osp < ostop ) osp++;\
  else if ( (retcode = ref_stack_push(&o_stack, 1)) >= 0 )\
    ;\
  else
#define spop1()\
  if ( osp >= osbot ) osp--;\
  else ref_stack_pop(&o_stack, 1)
	int max_name_ctype =
		(recognize_btokens() ? ctype_name : ctype_btoken);
#define scan_sign(sign, ptr)\
  switch ( *ptr ) {\
    case '-': sign = -1; ptr++; break;\
    case '+': sign = 1; ptr++; break;\
    default: sign = 0;\
  }
#define ensure2_back(styp,nback)\
  if ( sptr >= endptr ) { sptr -= nback; scan_type = styp; goto pause; }
#define ensure2(styp) ensure2_back(styp, 1)
	byte s1[2];
	register const byte _ds *decoder = scan_char_decoder;
	int status;
	scanner_state sstate;
#define pstack sstate.s_pstack
#define pdepth sstate.s_pdepth
#define scan_type sstate.s_scan_type
#define da sstate.s_da
#define name_type sstate.s_ss.s_name.s_name_type
#define try_number sstate.s_ss.s_name.s_try_number
	if ( pstate->s_pstack != 0 )
	  {	if_not_spush1()
		  return retcode;
		myref = osp;
	  }
	/* Check whether we are resuming after an interruption. */
	if ( pstate->s_scan_type != scanning_none )
	  {	sstate = *pstate;
		if ( !da.is_dynamic && da.base != da.buf )
		  {	/* The da contains some self-referencing pointers. */
			/* Fix them up now. */
			uint size = da.next - da.base;
			da.base = da.buf;
			da.next = da.buf + size;
			da.limit = da.buf + da_buf_size;
		  }
		daptr = da.next;
		switch ( scan_type )
		{
		case scanning_binary:
			retcode = (*sstate.s_ss.binary.cont)(s, myref, &sstate);
			scan_begin_inline();
			if ( retcode == scan_Refill )
			  goto pause;
			goto sret;
		case scanning_comment:
			scan_begin_inline();
			goto cont_comment;
		case scanning_name:
			goto cont_name;
		case scanning_string:
			goto cont_string;
		default:
			return_error(e_Fatal);
		}
	  }
	/* Fetch any state variables that are relevant even if */
	/* scan_type == scanning_none. */
	pstack = pstate->s_pstack;
	pdepth = pstate->s_pdepth;
	scan_begin_inline();
	/*
	 * Loop invariants:
	 *	If pstack != 0, myref = osp, and *osp is a valid slot.
	 */
top:	c = scan_getc();
	if_debug1('S', (c >= 32 && c <= 126 ? "`%c'" : c >= 0 ? "`\\%03o'" : "`%d'"), c);
	switch ( c )
	{
	case ' ': case '\f': case '\t':
	case char_CR: case char_EOL:
	case char_NULL: case char_VT: case char_DOS_EOF:
		goto top;
	case '[':
	case ']':
		s1[0] = (byte)c;
		retcode = name_ref(s1, 1, myref, 1);	/* can't fail */
		r_set_attrs(myref, a_executable);
		break;
	case '<':
		if ( scan_enable_level2 )
		{	ensure2(scanning_none);
			c = scan_getc();
			switch ( c )
			{
			case '<':
				scan_putback();
				name_type = 0;
				try_number = false;
				goto try_funny_name;
			case '~':
				s_A85D_init_inline(&sstate.s_ss.a85d);
				sstate.s_ss.st.template =
				  scan_ascii85_template;
				goto str;
			}
			scan_putback();
		}
		s_AXD_init_inline(&sstate.s_ss.axd);
		sstate.s_ss.st.template = &s_AXD_template;
str:		scan_end_inline();
		dynamic_init(&da, imemory);
cont_string:	for ( ; ; )
		  {	stream_cursor_write w;
			w.ptr = da.next - 1;
			w.limit = da.limit - 1;
			status = (*sstate.s_ss.st.template->process)
			  (&sstate.s_ss.st, &s->cursor.r, &w,
			   s->end_status == EOFC);
			da.next = w.ptr + 1;
			switch ( status )
			{
			case 0:
				status = s->end_status;
				if ( status < 0 )
				  {	if ( status == EOFC )
					  sreturn(e_syntaxerror);
					break;
				  }
				s_process_read_buf(s);
				continue;
			case 1:
				if ( !dynamic_grow(&da, da.next) )
				  {	retcode = e_VMerror;
					scan_type = scanning_string;
					goto suspend;
				  }
				continue;
			}
			break;
		  }
		scan_begin_inline();
		switch ( status )
		{
		default:
		/*case ERRC:*/
			sreturn(e_syntaxerror);
		case INTC:
		case CALLC:
			scan_type = scanning_string;
			goto pause;
		case EOFC:
			;
		}
		retcode = dynamic_make_string(myref, &da, da.next);
		if ( retcode < 0 )	/* VMerror */
		  {	sputback(s);	/* rescan ) */
			scan_type = scanning_string;
			goto suspend;
		  }
		break;
	case '(':
		sstate.s_ss.pssd.from_string =
		  pstate->s_from_string && scan_enable_level2;
		s_PSSD_init_inline(&sstate.s_ss.pssd);
		sstate.s_ss.st.template = &s_PSSD_template;
		goto str;
	case '{':
		if ( pstack == 0 )	/* outermost procedure */
		  {	if_not_spush1()
			  {	scan_putback();
				scan_type = scanning_none;
				goto pause_ret;
			  }
			pdepth = ref_stack_count_inline(&o_stack);
		  }
		make_int(osp, pstack);
		pstack = ref_stack_count_inline(&o_stack);
		if_debug3('S', "[S{]d=%d, s=%d->%d\n",
			  pdepth, (int)osp->value.intval, pstack);
		goto snext;
	case '>':
		if ( scan_enable_level2 )
		{	ensure2(scanning_none);
			name_type = 0;
			try_number = false;
			goto try_funny_name;
		}
		/* falls through */
	case ')':
		sreturn(e_syntaxerror);
	case '}':
		if ( pstack == 0 )
		  sreturn(e_syntaxerror);
		osp--;
		{	uint size = ref_stack_count_inline(&o_stack) - pstack;
			ref arr;
			if_debug4('S', "[S}]d=%d, s=%d->%ld, c=%d\n",
				  pdepth, pstack,
				  (pstack == pdepth ? 0 :
				   ref_stack_index(&o_stack, size)->value.intval),
				  size + pstack);
			myref = (pstack == pdepth ? pref : &arr);
			if ( ref_array_packing.value.boolval )
			  {	retcode = make_packed_array(myref, &o_stack,
						size, "scanner(packed)");
				if ( retcode < 0 )	/* must be VMerror */
				  {	osp++;
					scan_putback();
					scan_type = scanning_none;
					goto pause_ret;
				  }
				r_set_attrs(myref, a_executable);
			  }
			else
			  {	retcode = ialloc_ref_array(myref,
						a_executable + a_all, size,
						"scanner(proc)");
				if ( retcode < 0 )	/* must be VMerror */
				  {	osp++;
					scan_putback();
					scan_type = scanning_none;
					goto pause_ret;
				  }
				retcode = ref_stack_store(&o_stack, myref,
						size, 0, 1, false, "scanner");
				if ( retcode < 0 )
				  {	ifree_ref_array(myref, "scanner(proc)");
					sreturn(retcode);
				  }
				ref_stack_pop(&o_stack, size);
			  }
			if ( pstack == pdepth )
			  {	/* This was the top-level procedure. */
				spop1();
				pstack = 0;
			  }
			else
			  {	if ( osp < osbot )
				  ref_stack_pop_block(&o_stack);
				pstack = osp->value.intval;
				*osp = arr;
				goto snext;
			  }
		}
		break;
	case '/':
		ensure2(scanning_none);
		c = scan_getc();
		if ( c == '/' )
		{	name_type = 2;
			c = scan_getc();
		}
		else
			name_type = 1;
		try_number = false;
		switch ( decoder[c] )
		{
		case ctype_name:
		default:
			goto do_name;
		case ctype_btoken:
			if ( !recognize_btokens() ) goto do_name;
			/* otherwise, an empty name */
		case ctype_exception:
		case ctype_space:
non:			da.base = daptr = 0;
			goto nx;
		case ctype_other:
			switch ( c )
			{
			case '[':	/* only special as first character */
			case ']':	/* ditto */
				s1[0] = (byte)c;
				name_ref(s1, 1, myref, 1);	/* can't fail */
				goto have_name;
			case '<':	/* legal in Level 2 */
			case '>':
				if ( scan_enable_level2 )
				  {	ensure2_back(scanning_none, 2);
					goto try_funny_name;
				  }
			default:
				goto non;
			}
		}
	case '%':
	{	/* Scan as much as possible within the buffer. */
		const byte *base = sptr;
		const byte *end;
		while ( ++sptr < endptr )	/* stop 1 char early */
		  switch ( *sptr )
		    {
		    case char_CR:
		      end = sptr;
		      if ( sptr[1] == char_EOL )
			sptr++;
cend:		      /* Check for a DSC comment. */
		      if ( base[1] == '%' && end - base >= 3 &&
			   scan_dsc_proc != NULL
			 )
			{	retcode = (*scan_dsc_proc)(base, end - base);
				if ( retcode < 0 )
				  goto sret;
			}
		      goto top;
		    case char_EOL:
		    case '\f':
		      end = sptr;
		      goto cend;
		    }
		/*
		 * We got to the end of the buffer while still inside
		 * the comment.  If we're collecting a DSC comment,
		 * or if we had the bad luck to hit the end-of-buffer
		 * immediately after the initial % (so we can't tell
		 * whether or not this is a DSC comment), move whatever
		 * we have collected so far into a private buffer now.
		 * We always collect the rest of the comment in the buffer;
		 * this costs a little for non-DSC comments that straddle
		 * a buffer boundary, but we don't expect this to be common.
		 */
#define dsc_line da.buf
		--sptr;
		dsc_line[1] = 0;
		if ( (sptr == base || base[2] == '%') &&
		     scan_dsc_proc != NULL
		   )
		  {	/* Could be a DSC comment. */
			uint len = sptr + 1 - base;
			memcpy(dsc_line, base, len);
			daptr = dsc_line + len;
		  }
		else
		  {	/* Not a DSC comment. */
			daptr = dsc_line + (max_dsc_line + 1);
		  }
		da.base = dsc_line;
		da.is_dynamic = false;
		/* Enter here to continue scanning a comment. */
		/* daptr must be set. */
cont_comment:	for ( ; ; )
		  { switch ( (c = scan_getc()) )
		      {
		      default:
			if ( c < 0 )
			  switch ( c )
			    {
			    case INTC:
			    case CALLC:
			      da.next = daptr;
			      scan_type = scanning_comment;
			      goto pause;
			    case EOFC:
			      /* You would think that an EOF in a comment */
			      /* should be a syntax error, but there are */
			      /* quite a number of files that end that way. */
			      goto end_comment;
			    default:
			      sreturn(e_syntaxerror);
			    }
			if ( daptr < dsc_line + max_dsc_line )
			  *daptr++ = c;
			continue;
		      case char_CR:
		      case char_EOL:
		      case '\f':
end_comment:		if ( dsc_line[1] == '%' && scan_dsc_proc != NULL)
			  { retcode  = (*scan_dsc_proc)(dsc_line, daptr - dsc_line);
			    if ( retcode < 0 )
			      goto sret;
			  }
			goto top;
		      }
		    break;
		  }
#undef dsc_line
		/*NOTREACHED*/
	}
	case EOFC:
		if ( pstack != 0 )
		  sreturn(e_syntaxerror)
		retcode = scan_EOF;
		break;
	case ERRC:
		sreturn(e_ioerror);

	/* Check for a Level 2 funny name (<< or >>). */
	/* c is '<' or '>'.  We already did an ensure2. */
try_funny_name:
	{	int c1 = scan_getc();
		if ( c1 == c )
		{	s1[0] = s1[1] = c;
			name_ref(s1, 2, myref, 1);	/* can't fail */
			goto have_name;
		}
		scan_putback();
	}	sreturn(e_syntaxerror);

	/* Handle separately the names that might be a number. */
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	case '.':
		/* We must stop the scan 1 character early, to be sure that */
		/* we can test for CR+LF within the buffer.  That's why */
		/* we pass endptr rather than endptr + 1. */
		retcode = scan_number(sptr, endptr, 0, myref, &newptr);
nr:		if ( retcode == 1 && decoder[newptr[-1]] == ctype_space )
		{	sptr = newptr - 1;
			if ( *sptr == char_CR && sptr[1] == char_EOL )
				sptr++;
			retcode = 0;
			break;
		}
		name_type = 0;
		try_number = true;
		goto do_name;
	case '+':
		retcode = scan_number(sptr + 1, endptr, 1, myref, &newptr);
		goto nr;
	case '-':
		retcode = scan_number(sptr + 1, endptr, -1, myref, &newptr);
		goto nr;

	/* Check for a binary object */
#define case4(c) case c: case c+1: case c+2: case c+3
	case4(128): case4(132): case4(136): case4(140):
	case4(144): case4(148): case4(152): case4(156):
#undef case4
		if ( recognize_btokens() )
		{	scan_end_inline();
			retcode = (*scan_btoken_proc)(s, myref, &sstate);
			scan_begin_inline();
			if ( retcode == scan_Refill )
			  goto pause;
			break;
		}
	/* Not a binary object, fall through. */

	/* The default is a name. */
	default:
		if ( c < 0 )
		  {	dynamic_init(&da, name_memory());	/* da state must be clean */
			scan_type = scanning_none;
			goto pause;
		  }
	/* Populate the switch with enough cases to force */
	/* simple compilers to use a dispatch rather than tests. */
	case '!': case '"': case '#': case '$': case '&': case '\'':
	case '*': case ',': case '=': case ':': case ';': case '?': case '@':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
	case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
	case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
	case '\\': case '^': case '_': case '`':
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
	case 'n': case 'o': case 'p': case 'q': case 'r': case 's':
	case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
	case '|': case '~':
		/* Common code for scanning a name. */
		/* try_number and name_type are already set. */
		/* We know c has ctype_name (or maybe ctype_btoken) */
		/* or is a digit. */
		name_type = 0;
		try_number = false;
do_name:
	{	/* Try to scan entirely within the stream buffer. */
		/* We stop 1 character early, so we don't switch buffers */
		/* looking ahead if the name is terminated by \r\n. */
		const byte *endp1 = endptr - 1;
		da.base = (byte *)sptr;
		da.is_dynamic = false;
		do
		{	if ( sptr >= endp1 )	/* stop 1 early! */
				goto dyn_name;
		}
		while ( decoder[*++sptr] <= max_name_ctype );	/* digit or name */
		/* Name ended within the buffer. */
		daptr = (byte *)sptr;
		c = *sptr;
		goto nx;
dyn_name:	/* Name extended past end of buffer. */
		scan_end_inline();
		/* Initialize the dynamic area. */
		/* We have to do this before the next */
		/* sgetc, which will overwrite the buffer. */
		da.limit = (byte *)++sptr;
		da.memory = name_memory();
		daptr = dynamic_grow(&da, da.limit);
		if ( !daptr )
		  {	dynamic_save(&da);
			scan_type = scanning_name;
			retcode = e_VMerror;
			goto pause_ret;
		  }
		/* Enter here to continue scanning a name. */
		/* daptr must be set. */
cont_name:	scan_begin_inline();
		while ( decoder[c = scan_getc()] <= max_name_ctype )
		{	if ( daptr == da.limit )
			{	daptr = dynamic_grow(&da, daptr);
				if ( !daptr )
				  {	dynamic_save(&da);
					scan_putback();
					scan_type = scanning_name;
					retcode = e_VMerror;
					goto pause_ret;
				  }
			}
			*daptr++ = c;
		}
nx:		switch ( decoder[c] )
		  {
		  case ctype_btoken:
		  case ctype_other:
			scan_putback();
			break;
		  case ctype_space:
			/* Check for \r\n */
			if ( c == char_CR )
			  {	if ( sptr >= endptr ) /* ensure2 */
				  {	/* We have to check specially for */
					/* the case where the very last */
					/* character of a file is a CR. */
					if ( s->end_status != EOFC )
					  {	sptr--;
						goto pause_name;
					  }
				  }
				else if ( sptr[1] == char_EOL )
					sptr++;
			  }
			break;
		  case ctype_exception:
			switch ( c )
			{
			case INTC:
			case CALLC:
				goto pause_name;
			case ERRC:
				sreturn(e_ioerror);
			case EOFC:
				break;
			}
		  }
		/* Check for a number */
		if ( try_number )
		{	int sign;
			const byte *base = da.base;
			scan_sign(sign, base);
			retcode = scan_number(base, daptr, sign, myref, &newptr);
			if ( retcode == 1 )
				retcode = 0;
			else if ( retcode != e_syntaxerror )
			{	dynamic_free(&da);
				if ( name_type == 2 )
					sreturn(e_syntaxerror);
				break;	/* might be e_limitcheck */
			}
		}
		if ( da.is_dynamic )
		{	/* We've already allocated the string on the heap. */
			uint size = daptr - da.base;
			retcode = name_ref(da.base, size, myref, -1);
			if ( retcode >= 0 )
			{	dynamic_free(&da);
			}
			else
			{	retcode = dynamic_resize(&da, size);
				if ( retcode < 0 )	/* VMerror */
				  {	if ( c != EOFC )
					  scan_putback();
					scan_type = scanning_name;
					goto pause_ret;
				  }
				retcode = name_ref(da.base, size, myref, 2);
			}
		}
		else
		{	retcode = name_ref(da.base, (uint)(daptr - da.base),
					   myref, 1);
		}
	}
		/* Done scanning.  Check for preceding /'s. */
		if ( retcode < 0 )
		  {	if ( retcode != e_VMerror )
			  sreturn(retcode);
			if ( !da.is_dynamic )
			  {	da.next = daptr;
				dynamic_save(&da);
			  }
			if ( c != EOFC )
			  scan_putback();
			scan_type = scanning_name;
			goto pause_ret;
		  }
have_name:	switch ( name_type )
		{
		case 0:			/* ordinary executable name */
			if ( r_has_type(myref, t_name) )	/* i.e., not a number */
			  r_set_attrs(myref, a_executable);
		case 1:			/* quoted name */
			break;
		case 2:			/* immediate lookup */
		{	ref *pvalue;
			if ( !r_has_type(myref, t_name) )
				sreturn(e_undefined);
			if ( (pvalue = dict_find_name(myref)) == 0 )
				sreturn(e_undefined);
			if ( pstack != 0 &&
			     r_space(pvalue) > ialloc_space(idmemory)
			   )
				sreturn(e_invalidaccess);
			ref_assign_new(myref, pvalue);
		}
		}
	}
sret:	if ( retcode < 0 )
	  {	scan_end_inline();
		if ( pstack != 0 )
		  ref_stack_pop(&o_stack,
				ref_stack_count(&o_stack) - (pdepth - 1));
		return retcode;
	  }
	/* If we are at the top level, return the object, */
	/* otherwise keep going. */
	if ( pstack == 0 )
	  {	scan_end_inline();
		return retcode;
	  }
snext:	if_not_spush1()
	  {	scan_end_inline();
		scan_type = scanning_none;
		goto save;
	  }
	myref = osp;
	goto top;

	/* Pause for an interrupt or callout. */
pause_name:
	/* If we're still scanning within the stream buffer, */
	/* move the characters to the private buffer (da.buf) now. */
	da.next = daptr;
	dynamic_save(&da);
	scan_type = scanning_name;
pause:
	retcode = scan_Refill;
pause_ret:
	scan_end_inline();
suspend:
	if ( pstack != 0 )
	  osp--;		/* myref */
save:
	sstate.s_from_string = pstate->s_from_string;
	*pstate = sstate;
	return retcode;
}

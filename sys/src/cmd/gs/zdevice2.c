/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zdevice2.c */
/* Level 2 device operators */
#include "math_.h"
#include "memory_.h"
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "dstack.h"			/* for dict_find_name */
#include "estack.h"
#include "idict.h"
#include "idparam.h"
#include "igstate.h"
#include "iname.h"
#include "store.h"
#include "gxdevice.h"
#include "gsstate.h"

extern int zreadonly(P1(os_ptr));

/* - .currentpagedevice <dict> */
private int
zcurrentpagedevice(register os_ptr op)
{	int code = 0;
	gx_device *dev = gs_currentdevice(igs);
	push(1);
	if ( (*dev_proc(dev, get_page_device))(dev) != 0 )
		*op = istate->pagedevice;
	else
	{	code = dict_create(0, op);
		if ( code < 0 )
			pop(1);
	}
	return code;
}

/* <pagedict> <attrdict> <keys> .matchmedia <key> true */
/* <pagedict> <attrdict> <keys> .matchmedia false */
/* <pagedict> null <keys> .matchmedia null true */
private int near attr_eq(P3(const ref *, const ref *, const ref *));
private int
zmatchmedia(register os_ptr op)
{	os_ptr preq = op - 2;
	os_ptr pattr = op - 1;
#define pkeys op
	ref mmkey, nmkey;
	uint matched_priority;
	ref no_priority;
	ref *ppriority;
	int code;
	int ai;
	ref aelt[2];
#define mkey aelt[0]
#define mdict aelt[1]
	if ( r_has_type(op - 1, t_null) )
	  {	check_op(3);
		make_null(op - 2);
		make_true(op - 1);
		pop(1);
		return 0;
	  }
	check_dict_read(*preq);
	check_dict_read(*pattr);
	check_array(*op);
	check_read(*op);
	if ( dict_find_string(pattr, "Priority", &ppriority) > 0 )
	{	check_array_only(*ppriority);
		check_read(*ppriority);
	}
	else
	{	make_empty_array(&no_priority, a_readonly);
		ppriority = &no_priority;
	}
	matched_priority = r_size(ppriority);
	make_null(&mmkey);
	make_null(&nmkey);
	for ( ai = dict_first(pattr); (ai = dict_next(pattr, ai, aelt)) >= 0; )
	{	if ( r_has_type(&mdict, t_dictionary) &&
		     r_has_attr(dict_access_ref(&mdict), a_read) &&
		     r_has_type(&mkey, t_integer)
		   )
		{	bool match_all;
			uint ki, pi;
			code = dict_bool_param(&mdict, "MatchAll", false,
					       &match_all);
			if ( code < 0 )
				return code;
			for ( ki = 0; ki < r_size(pkeys); ki++ )
			{	ref key;
				ref *prvalue;
				ref *pmvalue;
				array_get(pkeys, ki, &key);
				if ( dict_find(&mdict, &key, &pmvalue) <= 0 )
					continue;
				if ( dict_find(preq, &key, &prvalue) <= 0 ||
				     r_has_type(prvalue, t_null)
				   )
				{	if ( match_all )
						goto no;
					else
						continue;
				}
				if ( !attr_eq(&key, prvalue, pmvalue) )
					goto no;
			}
			/* We have a match.  See if it has priority. */
			for ( pi = matched_priority; pi > 0; )
			{	ref pri;
				pi--;
				array_get(ppriority, pi, &pri);
				if ( obj_eq(&mkey, &pri) )
				{	/* Yes, higher priority. */
					mmkey = mkey;
					matched_priority = pi;
					break;
				}
			}
			/* Save the match in case no match has priority. */
			nmkey = mkey;
no:			;
		}
	}
#undef mkey
#undef mdict
#undef pkeys
	if ( r_has_type(&nmkey, t_null) )
	{	make_false(op - 2);
		pop(2);
	}
	else
	{	if ( r_has_type(&mmkey, t_null) )
			op[-2] = nmkey;
		else
			op[-2] = mmkey;
		make_true(op - 1);
		pop(1);
	}
	return 0;
}
/* Compare two attribute values for equality. */
/* PageSize is special. */
private int near
attr_eq(const ref *pkey, const ref *pv1, const ref *pv2)
{	ref kstr;
	if ( r_has_type(pkey, t_name) &&
	     (name_string_ref(pkey, &kstr),
	      r_size(&kstr) == 8 &&
	      !memcmp(kstr.value.bytes, "PageSize", 8))
	   )
	{	/* Compare PageSize specially. */
		if ( r_is_array(pv1) && r_size(pv1) == 2 &&
		     r_is_array(pv2) && r_size(pv2) == 2
		   )
		{	ref rv[4];
			float v[4];
			array_get(pv1, 0, &rv[0]);
			array_get(pv1, 1, &rv[1]);
			array_get(pv2, 0, &rv[2]);
			array_get(pv2, 1, &rv[3]);
			if ( num_params(rv + 3, 4, v) >= 0 )
			{	float t;
#define swap(x, y) t = x, x = y, y = t
				if ( v[0] > v[1] )
					swap(v[0], v[1]);
				if ( v[2] > v[3] )
					swap(v[2], v[3]);
				return (fabs(v[2] - v[0]) <= 5 &&
					fabs(v[3] - v[1]) <= 5);
#undef swap
			}
		}
	}
	return obj_eq(pv1, pv2);
}

/* <dict> .setpagedevice - */
private int
zsetpagedevice(register os_ptr op)
{	int code;
/******
	if ( igs->in_cachedevice )
		return_error(e_undefined);
 ******/
	check_dict_read(*op);
	/* Make the dictionary read-only. */
	code = zreadonly(op);
	if ( code < 0 )
		return code;
	istate->pagedevice = *op;
	pop(1);
	return 0;
}

/* - .beginpage - */
private int
z2beginpage(register os_ptr op)
{	gx_device *dev = gs_currentdevice(igs);
	ref *pproc;
	es_ptr ep = esp;
	if ( !((*dev_proc(dev, get_page_device))(dev) != 0 &&
	       dict_find_string(&istate->pagedevice, "BeginPage", &pproc) > 0)
	   )
	  return 0;
	check_estack(1);
	push(1);
	esp = ++ep;
	make_int(op, dev->ShowpageCount);
	*ep = *pproc;
	return o_push_estack;
}

/* <reason_int> .endpage <flush_bool> */
private int
z2endpage(register os_ptr op)
{	gx_device *dev = gs_currentdevice(igs);
	ref *pproc;
	es_ptr ep = esp;
	check_type(*op, t_integer);
	if ( !((*dev_proc(dev, get_page_device))(dev) != 0 &&
	       dict_find_string(&istate->pagedevice, "EndPage", &pproc) > 0)
	   )
	{	make_bool(op, op->value.intval != 2);
		return 0;
	}
	check_estack(1);
	push(1);
	esp = ++ep;
	*op = op[-1];
	make_int(op - 1, dev->ShowpageCount);
	*ep = *pproc;
	return o_push_estack;
}

/* Wrap setdevice so we clear the current pagedevice. */
/* (This is not correct, but we aren't sure yet what is.) */
extern int zsetdevice(P1(os_ptr));
private int
z2setdevice(os_ptr op)
{	int code = zsetdevice(op);
	gx_device *dev;
	if ( code < 0 )
		return code;
	dev = gs_currentdevice(igs);
	if ( (*dev_proc(dev, get_page_device))(dev) != 0 )
		code = dict_create(0, &istate->pagedevice);
	return code;
}

/* Default Install/BeginPage/EndPage procedures */
/* that just call the procedure in the device. */

/* - .callinstall - */
private int
zcallinstall(os_ptr op)
{	gx_device *dev = gs_currentdevice(igs);
	if ( (dev = (*dev_proc(dev, get_page_device))(dev)) != 0 )
	  {	int code = (*dev->page_procs.install)(dev, igs);
		if ( code < 0 )
		  return code;
	  }
	return 0;
}

/* <showpage_count> .callbeginpage - */
private int
zcallbeginpage(os_ptr op)
{	gx_device *dev = gs_currentdevice(igs);
	check_type(*op, t_integer);
	if ( (dev = (*dev_proc(dev, get_page_device))(dev)) != 0 )
	  {	int code = (*dev->page_procs.begin_page)(dev, igs);
		if ( code < 0 )
		  return code;
	  }
	pop(1);
	return 0;
}

/* <showpage_count> <reason_int> .callendpage <flush_bool> */
private int
zcallendpage(os_ptr op)
{	gx_device *dev = gs_currentdevice(igs);
	int code;
	check_type(op[-1], t_integer);
	check_type(*op, t_integer);
	if ( (dev = (*dev_proc(dev, get_page_device))(dev)) != 0 )
	  {	code = (*dev->page_procs.end_page)(dev, (int)op->value.intval, igs);
		if ( code < 0 )
		  return code;
		if ( code > 1 )
		  return_error(e_rangecheck);
	  }
	else
	  {	code = (op->value.intval == 2 ? 0 : 1);
	  }
	make_bool(op - 1, code);
	pop(1);
	return 0;
}

/* ------ Replacements for operators that reset the graphics state. ------ */

/* Forward references */
private int push_end_page(P3(const ref *, int, int (*)(P1(os_ptr))));
private int push_begin_page(P2(const ref *, int (*)(P1(os_ptr))));
private int finish_page(P1(os_ptr));

#define switch_page_device(dev_old, dev_new, dev_t1, dev_t2)\
  (dev_old != dev_new &&\
   (dev_t1 = (*dev_proc(dev_old, get_page_device))(dev_old)) != 0 &&\
   (dev_t2 = (*dev_proc(dev_new, get_page_device))(dev_new)) != 0 &&\
   dev_t1 != dev_t2)

/* - grestore - */
extern int zgrestore(P1(os_ptr));
private int grestore_continue(P1(os_ptr));
private int no_continue(P1(os_ptr));
private int
z2grestore(os_ptr op)
{	gx_device *dev_old = gs_currentdevice(igs);
	gx_device *dev_new = gs_currentdevice(gs_state_saved(igs));
	gx_device *dev_t1;
	gx_device *dev_t2;
	if ( !switch_page_device(dev_old, dev_new, dev_t1, dev_t2) )
		return zgrestore(op);
	return push_end_page(NULL, 2, grestore_continue);
}
private int
grestore_continue(os_ptr op)
{	/* Force the grestore to succeed. */
	int code;
	gx_device_no_output(igs);
	code = gs_grestore(igs);
	if ( code < 0 )
		return code;
	return push_begin_page(NULL, no_continue);
}
private int
no_continue(os_ptr op)
{	return 0;
}

/* - grestoreall - */
extern int zgrestoreall(P1(os_ptr));
private int grestoreall_continue(P1(os_ptr));
private int
z2grestoreall(os_ptr op)
{	for ( ; ; )
	{	gx_device *dev_old = gs_currentdevice(igs);
		gx_device *dev_new = gs_currentdevice(gs_state_saved(igs));
		gx_device *dev_t1;
		gx_device *dev_t2;
		if ( !switch_page_device(dev_old, dev_new, dev_t1, dev_t2) )
		{	zgrestore(op);
			if ( !gs_state_saved(gs_state_saved(igs)) )
				break;
		}
		else
			return push_end_page(NULL, 2, grestoreall_continue);
	}
	return 0;
}
private int
grestoreall_continue(os_ptr op)
{	/* Force the grestore to succeed. */
	int code;
	gx_device_no_output(igs);
	code = gs_grestore(igs);
	if ( code < 0 )
		return code;
	return push_begin_page(NULL, z2grestoreall);
}

/* <save> restore - */
extern int zrestore(P1(os_ptr));
private int restore_continue(P1(os_ptr));
private int
z2restore(os_ptr op)
{	for ( ; ; )
	{	gx_device *dev_old = gs_currentdevice(igs);
		gx_device *dev_new = gs_currentdevice(gs_state_saved(igs));
		gx_device *dev_t1;
		gx_device *dev_t2;
		if ( !switch_page_device(dev_old, dev_new, dev_t1, dev_t2) )
		{	zgrestore(op);
			if ( !gs_state_saved(gs_state_saved(igs)) )
				break;
		}
		else
		{	pop(1);  op = osp;
			return push_end_page(op + 1, 2, restore_continue);
		}
	}
	return zrestore(op);
}
private int
restore_continue(os_ptr op)
{	ref *param = op;
	/* Force the grestore to succeed. */
	int code;
	gx_device_no_output(igs);
	code = gs_grestore(igs);
	if ( code < 0 )
		return code;
	pop(1);  op = osp;
	return push_begin_page(param, z2restore);
}

/* <gstate> setgstate - */
extern int zsetgstate(P1(os_ptr));
private int setgstate_continue(P1(os_ptr));
private int
z2setgstate(os_ptr op)
{	gx_device *dev_old = gs_currentdevice(igs);
	gx_device *dev_new;
	gx_device *dev_t1;
	gx_device *dev_t2;
	check_stype(*op, st_igstate_obj);
	dev_new = gs_currentdevice(igstate_ptr(op));
	if ( !switch_page_device(dev_old, dev_new, dev_t1, dev_t2) )
		return zsetgstate(op);
	pop(1);  op = osp;
	return push_end_page(op + 1, 2, setgstate_continue);
}
private int
setgstate_continue(os_ptr op)
{	/* Force the grestore to succeed. */
	int code;
	gx_device_no_output(igs);
	code = gs_setgstate(igs, igstate_ptr(op));
	if ( code < 0 )
		return code;
	pop(1);  op = osp;
	return push_begin_page(NULL, no_continue);
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zdevice2_l2_op_defs) {
		op_def_begin_level2(),
	{"0.currentpagedevice", zcurrentpagedevice},
	{"3.matchmedia", zmatchmedia},
	{"1.setpagedevice", zsetpagedevice},
		/* Note that the following replace prior definitions */
		/* in the indicated files: */
	{"0.beginpage", z2beginpage},		/* gs_init.ps */
	{"1.endpage", z2endpage},		/* gs_init.ps */
	{"0grestore", z2grestore},		/* zgstate.c */
	{"0grestoreall", z2grestoreall},	/* zgstate.c */
	{"1restore", z2restore},		/* zvmem.c */
	{"1.setdevice", z2setdevice},		/* zdevice.c */
	{"1setgstate", z2setgstate},		/* zdps1.c */
		/* Default Install/BeginPage/EndPage procedures */
		/* that just call the procedure in the device. */
	{"0.callinstall", zcallinstall},
	{"1.callbeginpage", zcallbeginpage},
	{"2.callendpage", zcallendpage},
		/* Internal operators */
	{"0%no_continue", no_continue},
	{"0%grestore_continue", grestore_continue},
	{"0%grestoreall_continue", grestoreall_continue},
	{"0%restore_continue", restore_continue},
	{"0%setgstate_continue", setgstate_continue},
	{"1%finish_page", finish_page},
END_OP_DEFS(0) }

/* ------ Internal routines ------ */

/* Schedule the call on the EndPage procedure when resetting the gstate. */
private int
push_end_page(const ref *param, int reason, int (*contin)(P1(os_ptr)))
{	int epush = (param ? 5 : 4);
	es_ptr ep = esp;
	check_estack(epush);
	make_op_estack(ep + 1, contin);
	esp = ep += epush;
	if ( param )
		ep[-3] = *param;
	make_op_estack(ep - 2, finish_page);
	make_op_estack(ep - 1, z2endpage);
	make_int(ep, reason);
	return o_push_estack;
}

/* Schedule the call on the BeginPage procedure when resetting the gstate. */
private int
push_begin_page(const ref *param, int (*contin)(P1(os_ptr)))
{	int epush = (param ? 3 : 2);
	es_ptr ep = esp;
	check_estack(epush);
	make_op_estack(ep + 1, contin);
	esp = ep += epush;
	if ( param )
		ep[-1] = *param;
	make_op_estack(ep, z2beginpage);
	return o_push_estack;
}

/* Finish the page.  The top of the ostack contains the Boolean value */
/* returned by the EndPage procedure. */
private int
finish_page(register os_ptr op)
{	check_type(*op, t_boolean);
	if ( op->value.boolval )
	  {	int code;
		ref *pcopies;
		long ncopies;
		const ref *pagedict = &gs_int_gstate(igs)->pagedevice;
		if ( dict_find_string(pagedict, "NumCopies", &pcopies) <= 0 ||
		     !r_has_type(pcopies, t_integer) ||
		     (ncopies = pcopies->value.intval) < 0
		   )
		  {	ref name_ncopies;
			if ( name_enter_string("#copies", &name_ncopies) != 0 ||
			     (pcopies = dict_find_name(&name_ncopies)) == 0 ||
			     !r_has_type(pcopies, t_integer) ||
			     (ncopies = pcopies->value.intval) < 0
			   )
			  ncopies = 1;
		  }
		code = gs_output_page(igs, (int)ncopies, false);
		if ( code < 0 )
		  return code;
	  }
	pop(1);
	return 0;
}

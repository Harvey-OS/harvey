/* Copyright (C) 2002 Artifex Software Inc.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: zccube.c,v 1.1.2.1 2002/01/17 06:57:55 dancoby Exp $ */
/* Create Color Cube routines for tint transform replacement. */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gxcspace.h"
#include "estack.h"
#include "ialloc.h"
#include "ifunc.h"
#include "ostack.h"
#include "store.h"
#include "gsfunc0.h"

typedef ushort color_cube_value;

/* Forward references */

private int color_cube_enum_init(i_ctx_t *i_ctx_p, int num_in, int num_out,
	const ref * pproc, int (*finish_proc)(P1(i_ctx_t *)),
	gs_memory_t * mem);
private int color_cube_finish(i_ctx_t *i_ctx_p);

/*
 * Build a color cube to replace the tint transform procedure
 * <num_inputs> <num_outputs> <tint_proc> .buildcolorcube <cubeproc>
 */
private int
zbuildcolorcube(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const ref * pfunc = op;
    int num_inputs, num_outputs;
    gs_function_t *pfn;
    int code = 0;

    /* Check tint transform procedure. */
    check_proc(*pfunc);

    /* Verify that we have a valid num_inputs value */
    check_type(op[-2], t_integer);
    num_inputs = op[-2].value.intval;
    if (num_inputs <= 0 || num_inputs > GS_CLIENT_COLOR_MAX_COMPONENTS)
	return_error(e_limitcheck);

    /* Verify that we have a valid num_outputs value */
    check_type(op[-1], t_integer);
    num_outputs = op[-1].value.intval;
    if (num_outputs <= 0 || num_outputs > GS_CLIENT_COLOR_MAX_COMPONENTS)
	return_error(e_limitcheck);

    pop(2);

    /* Now setup to collect the color cube data */
    return color_cube_enum_init(i_ctx_p, num_inputs, num_outputs,
			pfunc, color_cube_finish, imemory);
}
    
/* ------ Internal procedures ------ */

/* -------- Build color cube ------- */

/*
 * This structure is used to hold data required while collecting the
 * color cube information for an "alternate tint transform" that cannot
 * be converted into a type 4 function.
 */
struct gs_color_cube_enum_s {
    int indexes[GS_CLIENT_COLOR_MAX_COMPONENTS];
    gs_function_t * pfn;
};

typedef struct gs_color_cube_enum_s gs_color_cube_enum;


gs_private_st_ptrs1(st_gs_color_cube_enum, gs_color_cube_enum, "gs_color_cube_enum",
		    gs_color_cube_enum_enum_ptrs, gs_color_cube_enum_reloc_ptrs, pfn);

/*
 * Determine how big of a cube that we want to use.  The space requirements
 * is cube_size ** num_inputs.  Thus we must use fewer points if we have
 * many input components. 
 *
 * We will return a e_rangecheck error if the cube will not fit into the maximum
 * string size.
 */
private int
determine_color_cube_size(int num_inputs, int num_outputs)
{
    static const int size_list[] = {512, 50, 20, 10, 7, 5, 4, 3};
    int size;

    /* Start with initial guess at cube size */
    if (num_inputs > 0 && num_inputs <= 8)
	size = size_list[num_inputs - 1];
    else
	size = 2;

    /*
     * Verify that the cube will fit into 64k.  We are using a string
     * to hold the data for the cube.  Max string size is 64k.
     */
#define MAX_CUBE_SIZE 0x10000
    while (true) {
        int i, total_size = num_outputs * sizeof(color_cube_value);

        for (i = 0; i < num_inputs; i++) {
	    total_size *= size;
	    if (total_size > MAX_CUBE_SIZE)
	        break;
	}
	if (total_size <= MAX_CUBE_SIZE)
	    return size;		/* return value if size fits */
	if (size == 2)
	    return_error(e_rangecheck); /* Cannot have less than 2 points per side */
	size--;
    }
#undef MAX_CUBE_SIZE
}


/* Allocate a color cube enumerator. */
private gs_color_cube_enum *
gs_color_cube_enum_alloc(gs_memory_t * mem, client_name_t cname)
{
    return gs_alloc_struct(mem, gs_color_cube_enum, &st_gs_color_cube_enum, cname);
}

/*
 * This routine will determine the location of a block of data
 * in the color cube.  Basically this does an index calculation
 * for an n dimensional cube.
 */
private color_cube_value *
cube_ptr_from_index(gs_function_Sd_params_t * params, int indexes[])
{
    int i, sum = indexes[params->m - 1];

    for (i = params->m - 2; i >= 0; i--) {
	sum *= params->Size[i];
	sum += indexes[i];
    }
    return (color_cube_value *)(params->DataSource.data.str.data) + sum * params->n;
}

/*
 * This routine will increment the index values.  This is
 * used for collecting the data.  If we have incremented the
 * last index beyond its last value then we return a 1, else 0;
 */
private int
increment_cube_indexes(gs_function_Sd_params_t * params, int indexes[])
{
    int i = 0;

    while (1) {
	indexes[i]++;
	if (indexes[i] < params->Size[i])
	    return 0;
	indexes[i] = 0;
	i++;
	if (i == params->m)
	    return 1;
    }
}

/*
 * Fill in the data for a function type 0 parameer object to be used while
 * we collect the data for the color cube.  At the end of the process, we
 * will create a function type 0 object to be used to calculate values
 * for the alternate tint transform.
 */
int
cube_build_func0(int num_inputs, int num_outputs,
		    gs_function_Sd_params_t * params, gs_memory_t *mem)
{
    color_cube_value * bytes = 0;
    int code, i, cube_size;
    int total_size;
    int * size;
    float * domain, * range;

    params->m = num_inputs;
    params->n = num_outputs;
    params->Order = 3;			/* Use cubic interpolation */
    params->BitsPerSample = 8 * sizeof(color_cube_value); /* Use 16 bits per sample */
    params->Encode = 0;
    params->Decode = 0;
    params->Size = 0;

    /* Allocate  storage for pieces */

    if (!(domain = (float *) gs_alloc_byte_array(mem, 2 * params->m,
    	    			sizeof(float), "cube_build_func0(Domain)" )) ||
    	!(range = (float *) gs_alloc_byte_array(mem, 2 * params->n,
    	    			sizeof(float), "cube_build_func0(Range)" )) ||
    	!(size = (int *) gs_alloc_byte_array(mem, params->m,
    	    			sizeof(int), "cube_build_func0(Size)"))) {
	code = gs_note_error(e_VMerror);
	goto fail;
    }
    params->Domain = domain;
    params->Range = range;
    params->Size = size;
    if (!domain || !range  || !size) {
	code = gs_note_error(e_VMerror);
	goto fail;
    }

    /* Determine space required for the color cube storage */

    code = cube_size = determine_color_cube_size(params->m, params->n);
    if (code < 0)
        goto fail;

    total_size = params->n;
    for (i = 0; i < params->m; i++)
	total_size *= cube_size;

    /* Allocate space for the color cube itself */

    bytes = (color_cube_value *) gs_alloc_byte_array(mem, total_size,
			sizeof(color_cube_value), "cube_build_func0(bytes)");
    if (!bytes) {
	code = gs_note_error(e_VMerror);
	goto fail;
    }
    data_source_init_bytes(&params->DataSource, (const unsigned char *)bytes,
    			total_size * sizeof(color_cube_value));

    /* Initialize Domain, Range, and Size arrays */

    for (i = 0; i < params->m; i++) {
        domain[2 * i] = 0.0;
        domain[2 * i + 1] = 1.0;
	size[i] = cube_size;
    }
    for (i = 0; i < params->n; i++) {
        range[2 * i] = 0.0;
        range[2 * i + 1] = 1.0;
    }

    return 0;

fail:
    gs_function_Sd_free_params(params, mem);
    return (code < 0 ? code : gs_note_error(e_rangecheck));
}

private int color_cube_sample(i_ctx_t *i_ctx_p);
private int color_cube_continue(i_ctx_t *i_ctx_p);

/*
 * Layout of stuff pushed on estack while collecting color cube data.
 * The data is saved there since it is safe from attack by the tint
 * transform function and is convient.
 *
 *      finishing procedure (or 0)
 *      alternate tint transform procedure
 *      enumeration structure (as bytes)
 */
#define estack_storage 3
#define esp_finish_proc (*real_opproc(esp - 2))
#define tint_proc esp[-1]
#define senum r_ptr(esp, gs_color_cube_enum)

/*
 * Set up to collect the data for the color cube.  This is used for
 * those alternate tine transforms that cannot be converted into a
 * type 4 function.
 */
private int
color_cube_enum_init(i_ctx_t *i_ctx_p, int num_in, int num_out,
	const ref * pproc, int (*finish_proc)(P1(i_ctx_t *)), gs_memory_t * mem)
{
    gs_color_cube_enum *penum;
    int i, code;
    gs_function_t *pfn;
    gs_function_Sd_params_t params;

    check_estack(estack_storage + 1);		/* Verify space on estack */
    check_ostack(num_in);			/* and the operand stack */
    check_ostack(num_out);

    penum = gs_color_cube_enum_alloc(imemory, "color_cube_enum_init");
    if (penum == 0)
	return_error(e_VMerror);

    code = cube_build_func0(num_in, num_out, &params, mem);
    if (code < 0) {
	gs_free_object(mem, penum, "color_cube_enum_init(penum)");
	return code;
    }

    /*
     * This is temporary.  We will call gs_function_Sd_init again after
     * we have collected the cube data.  We are doing it now because we need
     * a function structure created (along with its GC enumeration stuff)
     * that we can use while collecting the cube data.  We will call
     * the routine again after the cube data is collected to correctly
     * initialize the function.
     */
    code = gs_function_Sd_init(&pfn, &params, mem);
    if (code < 0) {
	gs_free_object(mem, penum, "color_cube_enum_init(penum)");
	return code;
    }

    /* Initialize data in the enumeration structure */

    penum->pfn = pfn;
    for(i=0; i< num_in; i++)
        penum->indexes[i] = 0;

    /* Push everything on the estack */

    esp += estack_storage;
    make_op_estack(esp - 2, finish_proc);	/* Finish proc onto estack */
    tint_proc = *pproc;				/* Alternate tint xform onto estack */
    make_istruct(esp, 0, penum);		/* Color cube enumeration structure */
    push_op_estack(color_cube_sample);		/* Start sampling data */
    return o_push_estack;
}

/*
 * Set up to collect the next color cube value.
 */
private int
color_cube_sample(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_color_cube_enum *penum = senum;
    gs_point pt;
    ref proc;
    gs_function_Sd_params_t * params = (gs_function_Sd_params_t *)&penum->pfn->params;
    int i;
    int num_inputs = params->m;
    color_cube_value * data_ptr;
    double cube_step = 1.0 / (params->Size[0] - 1);	/* All sizes are the same */

    /*
     * Put set of color values onto the stack.
     */
    push(num_inputs);
    for (i = 0; i < num_inputs; i++)
	make_real(op - num_inputs + i + 1, penum->indexes[i] * cube_step);

    proc = tint_proc;			    /* Get tint xfrom from storage */
    push_op_estack(color_cube_continue);    /* Put 'save' routine on estack, after tint xform */
    *++esp = proc;			    /* Put tint xform to be executed */
    return o_push_estack;
}

/* 
 * Continuation procedure for processing sampled pixels.
 */
private int
color_cube_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_color_cube_enum *penum = senum;
    gs_function_Sd_params_t * params = (gs_function_Sd_params_t *)&penum->pfn->params;
    int i, done, num_out = params->n;
    int code;
    byte * data_ptr;
    int color_cube_value_max = (1 << (sizeof(color_cube_value) * 8)) - 1;

    /* Save data from tint transform function */

    check_op(num_out);
    data_ptr = (byte *)cube_ptr_from_index(params, penum->indexes);
    for (i=0; i < num_out; i++) {
	color_cube_value cv;
        double value;

        code = real_param(op + i - num_out + 1, &value);
        if (code < 0)
	    return code;
	if (value < 0.0)
	    value = 0.0;
	else if (value > 1.0)
	    value = 1.0;
	cv = (color_cube_value) (value * color_cube_value_max + 0.5);
	data_ptr[2 * i] = cv >> 8;	/* MSB first */
	data_ptr[2 * i + 1] = cv;	/* LSB second */
    }
    pop(num_out);		    /* Move op to base of result values */
    
    /* Check if we are done collecting data. */

    done = increment_cube_indexes(params, penum->indexes);
    if (done) {

	/* Execute the closing procedure, if given */
	code = 0;
	if (esp_finish_proc != 0)
	    code = esp_finish_proc(i_ctx_p);

	return code;
    }

    /* Now get the data for the next location */

    return color_cube_sample(i_ctx_p);
}

/*
 * Finish collecting color cube.
 */
private int
color_cube_finish(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_color_cube_enum *penum = senum;
    /* Build a type 0 function using the given parameters */
    gs_function_Sd_params_t * params = (gs_function_Sd_params_t *)&penum->pfn->params;
    gs_function_t * pfn;
    ref cref;			/* closure */
    int code = gs_function_Sd_init(&pfn, params, imemory);

    if (code < 0)
	return code;

    code = ialloc_ref_array(&cref, a_executable | a_execute, 2,
			    "color_cube_finish(cref)");
    if (code < 0)
	return code;

    make_istruct_new(cref.value.refs, a_executable | a_execute, pfn);
    make_oper_new(cref.value.refs + 1, 0, zexecfunction);
    ref_assign(op, &cref);

    esp -= estack_storage;
    ifree_object(penum->pfn, "color_cube_finish(pfn)");
    ifree_object(penum, "color_cube_finish(enum)");
    return o_pop_estack;
}


/* ------ Initialization procedure ------ */

const op_def zccube_op_defs[] =
{
    op_def_begin_level2(),
    {"3.buildcolorcube", zbuildcolorcube},
    op_def_end(0)
};


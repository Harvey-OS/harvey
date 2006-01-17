/* Copyright (C) 2002 Artifex Software Inc.  All rights reserved.
  
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

/* $Id: zfsample.c,v 1.9 2003/04/26 18:40:16 dan Exp $ */
/* Sample data to create a type 0 function */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gxcspace.h"
#include "estack.h"
#include "ialloc.h"
#include "idict.h"
#include "idparam.h"
#include "ifunc.h"
#include "ostack.h"
#include "store.h"
#include "gsfunc0.h"

/*
 * We store the data in a string.  Since the max size for a string is 64k,
 * we use that as our max data size.
 */
#define MAX_DATA_SIZE 0x10000
/*
 * We cannot handle more than  16 inputs.  Otherwise the the data will not
 * fit within MAX_DATA_SIZE.
 */
#define MAX_NUM_INPUTS 16
/*
 * This value is rather arbitrary.
 */
#define MAX_NUM_OUTPUTS 128

/* --- Build sampled data function --- */

/*
 * This structure is used to hold data required while collecting samples
 * for a type 0 function (sampled data).
 */
struct gs_sampled_data_enum_s {
    int indexes[MAX_NUM_INPUTS];
    int o_stack_depth;		/* used to verify stack while sampling */
    gs_function_t * pfn;
};

typedef struct gs_sampled_data_enum_s gs_sampled_data_enum;


gs_private_st_ptrs1(st_gs_sampled_data_enum, gs_sampled_data_enum,
		"gs_sampled_data_enum", gs_sampled_data_enum_enum_ptrs,
		gs_sampled_data_enum_reloc_ptrs, pfn);


/* Forward references */

private int cube_build_func0(const ref * pdict,
	gs_function_Sd_params_t * params, gs_memory_t *mem);
private int sampled_data_setup(i_ctx_t *i_ctx_p, gs_function_t *pfn,
	const ref * pproc, int (*finish_proc)(i_ctx_t *),
	gs_memory_t * mem);
private int sampled_data_sample(i_ctx_t *i_ctx_p);
private int sampled_data_continue(i_ctx_t *i_ctx_p);
private int sampled_data_finish(i_ctx_t *i_ctx_p);

private gs_sampled_data_enum * gs_sampled_data_enum_alloc
	(gs_memory_t * mem, client_name_t cname);

/*
 * Collect data for a type 0 (sampled data) function
 * <dict> .buildsampledfunction <function_struct>
 *
 * The following keys are used from the dictionary:
 *    Function (required)
 *    Domain (required)
 *    Range (required)
 *    Size (optional)  If Size is not specified then a default value is determined
 *        based upon the number of inputs and outputs.
 *    BitsPerSample (required) Only 8, 16, 24, and 32 accepted,
 * The remaining keys are ignored.
 */
private int
zbuildsampledfunction(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const ref * pdict = op;
    ref * pfunc;
    int code = 0;
    gs_function_t *pfn;
    gs_function_Sd_params_t params = {0};

    check_type(*pdict, t_dictionary);
    /* 
     * Check procedure to be sampled.
     */
    if (dict_find_string(pdict, "Function", &pfunc) <= 0)
	return_error(e_rangecheck);
    check_proc(*pfunc);
    /*
     * Set up the hyper cube function data structure.
     */
    code = cube_build_func0(pdict, &params, imemory);
    if (code < 0)
	return code;
    /*
     * This is temporary.  We will call gs_function_Sd_init again after
     * we have collected the cube data.  We are doing it now because we need
     * a function structure created (along with its GC enumeration stuff)
     * that we can use while collecting the cube data.  We will call
     * the routine again after the cube data is collected to correctly
     * initialize the function.
     */
    code = gs_function_Sd_init(&pfn, &params, imemory);
    if (code < 0)
	return code;
    /*
     * Now setup to collect the sample data.
     */
    return sampled_data_setup(i_ctx_p, pfn, pfunc, sampled_data_finish, imemory);
}
    
/* ------- Internal procedures ------- */


#define bits2bytes(x) ((x) >> 3)	/* Convert bit count to byte count */

/*
 * This routine will verify that the requested data hypercube parameters will require
 * a data storage size less than or equal to the MAX_DATA_SIZE.
 */
private bool
valid_cube_size(int num_inputs, int num_outputs, int sample_size, const int Size[])
{
    int i, total_size = num_outputs * sample_size;

    for (i = 0; i < num_inputs; i++) {
	if (Size[i] <= 0 || Size[i] > MAX_DATA_SIZE / total_size)
	    return false;
	total_size *= Size[i];
    }
    return true;
}

/*
 * This routine is used to determine a default value for the sampled data size.
 * As a default, we will build a hyper cube with each side having the same
 * size.  The space requirements for a hypercube grow exponentially with the
 * number of dimensions.  Thus we must use fewer points if our functions has
 * many inputs.  The values returned were chosen simply to given a reasonable
 * tradeoff between keeping storage requirements low but still having enough
 * points per side to minimize loss of information.
 *
 * We do check to see if the data will fit using our initial guess.  If not
 * then we decrement the size of each edge until it fits.  We will return a
 * e_rangecheck error if the cube can not fit into the maximum  size.
 * On exit the Size array contains the cube size (if a valid size was found).
 */
private int
determine_sampled_data_size(int num_inputs, int num_outputs,
				int sample_size, int Size[])
{
    static const int size_list[] = {512, 50, 20, 10, 7, 5, 4, 3};
    int i, size;

    /* Start with initial guess at cube size */
    if (num_inputs > 0 && num_inputs <= 8)
	size = size_list[num_inputs - 1];
    else
	size = 2;
    /*
     * Verify that the cube will fit into MAX_DATA_SIZE.  If not then
     * decrement the cube size until it will fit.
     */
    while (true) {
        /* Fill Size array with value. */
        for (i = 0; i < num_inputs; i++)
            Size[i] = size;

	if (valid_cube_size(num_inputs, num_outputs, sample_size, Size))
	    return 0;		/* We have a valid size */

	if (size == 2)		/* Cannot have less than 2 points per side */
	    return_error(e_rangecheck);
	size--;
    }
}


/*
 * Allocate the enumerator used while collecting sampled data.  This enumerator
 * is used to hold the various state data required while sampling.
 */
private gs_sampled_data_enum *
gs_sampled_data_enum_alloc(gs_memory_t * mem, client_name_t cname)
{
    return gs_alloc_struct(mem, gs_sampled_data_enum,
    				&st_gs_sampled_data_enum, cname);
}

/*
 * This routine will determine the location of a block of data
 * in the hyper cube.  Basically this does an index calculation
 * for an n dimensional cube.
 */
private byte *
cube_ptr_from_index(gs_function_Sd_params_t * params, int indexes[])
{
    int i, sum = indexes[params->m - 1];

    for (i = params->m - 2; i >= 0; i--) {
	sum *= params->Size[i];
	sum += indexes[i];
    }
    return (byte *)(params->DataSource.data.str.data) + 
	sum * params->n * bits2bytes(params->BitsPerSample);
}

/*
 * This routine will increment the index values for the hypercube.  This
 * is used for collecting the data.  If we have incremented the
 * last index beyond its last value then we return a true, else false;
 */
private bool
increment_cube_indexes(gs_function_Sd_params_t * params, int indexes[])
{
    int i = 0;

    while (true) {
	/*
	 * Increment an index value for an edge and test if we have
	 * gone past the final value for the edge.
	 */
	indexes[i]++;
	if (indexes[i] < params->Size[i])
	    /*
	     * We have not reached the end of the edge.  Exit but
	     * indicate that we are not done with the hypercube.
	     */
	    return false;
	/*
	 * We have reached the end of one edge of the hypercube and we
	 * need to increment the next index.
	 */
	indexes[i] = 0;
	i++;
	if (i == params->m)
	    /*
	     * We have finished the last edge of the hyper cube.
	     * We are done.
	     */
	    return true;
    }
}

/*
 * Fill in the data for a function type 0 parameter object to be used while
 * we collect the data for the data cube.  At the end of the process, we
 * will create a function type 0 object to be used to calculate values
 * as a replacement for the original function.
 */
private int
cube_build_func0(const ref * pdict, gs_function_Sd_params_t * params,
							gs_memory_t *mem)
{
    byte * bytes = 0;
    int code, i;
    int total_size;

    if ((code = dict_int_param(pdict, "Order", 1, 3, 1, &params->Order)) < 0 ||
	(code = dict_int_param(pdict, "BitsPerSample", 1, 32, 0,
			       &params->BitsPerSample)) < 0 ||
	((code = params->m =
	    fn_build_float_array(pdict, "Domain", false, true,
		    			&params->Domain, mem)) < 0 ) ||
	((code = params->n =
	    fn_build_float_array(pdict, "Range", false, true,
		    			&params->Range, mem)) < 0) 
	) {
	goto fail;
    }
    /*
     * The previous logic set the size of m and n to the size of the Domain
     * and Range arrays.  This is twice the actual size.  Correct this and
     * check for valid values.
     */
    params->m >>= 1;
    params->n >>= 1;
    if (params->m == 0 || params->n == 0 ||
        params->m > MAX_NUM_INPUTS || params->n > MAX_NUM_OUTPUTS) {
	code = gs_note_error(e_rangecheck);
        goto fail;
    }
    /*
     * The Size array may or not be specified.  If it is not specified then
     * we need to determine a set of default values for the Size array.
     */
    {
	int *ptr = (int *)
	    gs_alloc_byte_array(mem, params->m, sizeof(int), "Size");

	if (ptr == NULL) {
	    code = gs_note_error(e_VMerror);
	    goto fail;
	}
	params->Size = ptr;
	code = dict_ints_param(pdict, "Size", params->m, ptr);
        if (code < 0)
	    goto fail;
        if (code == 0) {
	    /*
	     * The Size array has not been specified.  Determine a default
	     * set of values.
	     */
            code = determine_sampled_data_size(params->m, params->n,
	     			params->BitsPerSample, (int *)params->Size);
            if (code < 0)
	        goto fail;
        }
	else {			/* Size array specified - verify valid */
	    if (code != params->m || !valid_cube_size(params->m, params->n,
	    				params->BitsPerSample, params->Size))
	        code = gs_note_error(e_rangecheck);
	        goto fail;
	}
    }
    /*
     * Determine space required for the sample data storage.
     */
    total_size = params->n * bits2bytes(params->BitsPerSample);
    for (i = 0; i < params->m; i++)
	total_size *= params->Size[i];
    /*
     * Allocate space for the data cube itself.
     */
    bytes = gs_alloc_byte_array(mem, total_size, 1, "cube_build_func0(bytes)");
    if (!bytes) {
	code = gs_note_error(e_VMerror);
	goto fail;
    }
    data_source_init_bytes(&params->DataSource,
    				(const unsigned char *)bytes, total_size);

    return 0;

fail:
    gs_function_Sd_free_params(params, mem);
    return (code < 0 ? code : gs_note_error(e_rangecheck));
}

/*
 * Layout of stuff pushed on estack while collecting the sampled data.
 * The data is saved there since it is safe from attack by the procedure
 * being sampled and is convient.
 *
 *      finishing procedure (or 0)
 *      procedure being sampled
 *      enumeration structure (as bytes)
 */
#define estack_storage 3
#define esp_finish_proc (*real_opproc(esp - 2))
#define sample_proc esp[-1]
#define senum r_ptr(esp, gs_sampled_data_enum)
/*
 * Sone invalid tint transform functions pop more items off of the stack
 * then they are supposed to use.  This is a violation of the PLRM however
 * this is done by Adobe and we have to handle the situation.  This is
 * a kludge but we set aside some unused stack space below the input
 * variables.  The tint transform can trash this without causing any
 * real problems.
 */
#define O_STACK_PAD 3

/*
 * Set up to collect the data for the sampled function.  This is used for
 * those alternate tint transforms that cannot be converted into a
 * type 4 function.
 */
private int
sampled_data_setup(i_ctx_t *i_ctx_p, gs_function_t *pfn,
	const ref * pproc, int (*finish_proc)(i_ctx_t *), gs_memory_t * mem)
{
    os_ptr op = osp;
    gs_sampled_data_enum *penum;
    int i;
    gs_function_Sd_params_t * params = (gs_function_Sd_params_t *)&pfn->params;

    check_estack(estack_storage + 1);		/* Verify space on estack */
    check_ostack(params->m + O_STACK_PAD);	/* and the operand stack */
    check_ostack(params->n + O_STACK_PAD);

    /*
     * Allocate space for the enumerator data structure.
     */
    penum = gs_sampled_data_enum_alloc(imemory, "zbuildsampledfuntion(params)");
    if (penum == NULL)
	return_error(e_VMerror);

    /* Initialize data in the enumeration structure */

    penum->pfn = pfn;
    for(i=0; i< params->m; i++)
        penum->indexes[i] = 0;
    /*
     * Save stack depth for checking the correct number of values on stack
     * after the function, which is being sampled, is called.
     */
    penum->o_stack_depth = ref_stack_count(&o_stack);
    /*
     * Note:  As previously mentioned, we are putting some spare (unused) stack
     * space under the input values in case the function unbalances the stack.
     * It is possible for the function to pop or change values on the stack
     * outside of the input values.  (This has been found to happen with some
     * proc sets from Adobe.)
     */
    push(O_STACK_PAD);
    for (i = 0; i < O_STACK_PAD; i++) 		/* Set space = null */
	make_null(op - i);

    /* Push everything on the estack */

    esp += estack_storage;
    make_op_estack(esp - 2, finish_proc);	/* Finish proc onto estack */
    sample_proc = *pproc;			/* Save function to be sampled */
    make_istruct(esp, 0, penum);		/* Color cube enumeration structure */
    push_op_estack(sampled_data_sample);	/* Start sampling data */
    return o_push_estack;
}

/*
 * Set up to collect the next sampled data value.
 */
private int
sampled_data_sample(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_sampled_data_enum *penum = senum;
    ref proc;
    gs_function_Sd_params_t * params =
    			(gs_function_Sd_params_t *)&penum->pfn->params;
    int num_inputs = params->m;
    int i;

    /* Put set of input values onto the stack. */
    push(num_inputs);
    for (i = 0; i < num_inputs; i++) {
	double dmin = params->Domain[2 * i];
	double dmax = params->Domain[2 * i + 1];

	make_real(op - num_inputs + i + 1, (float) (
	    penum->indexes[i] * (dmax - dmin)/(params->Size[i] - 1) + dmin));
    }

    proc = sample_proc;			    /* Get procedure from storage */
    push_op_estack(sampled_data_continue);  /* Put 'save' routine on estack, after sample proc */
    *++esp = proc;			    /* Put procedure to be executed */
    return o_push_estack;
}

/* 
 * Continuation procedure for processing sampled values.
 */
private int
sampled_data_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_sampled_data_enum *penum = senum;
    gs_function_Sd_params_t * params =
	    (gs_function_Sd_params_t *)&penum->pfn->params;
    int i, j, num_out = params->n;
    int code = 0;
    byte * data_ptr;
    double sampled_data_value_max = (double)((1 << params->BitsPerSample) - 1);
    int bps = bits2bytes(params->BitsPerSample);

    /*
     * Check to make sure that the procedure produced the correct number of
     * values.  If not, move the stack back to where it belongs and abort
     */
    if (num_out + O_STACK_PAD + penum->o_stack_depth != ref_stack_count(&o_stack)) {
	int stack_depth_adjust = ref_stack_count(&o_stack) - penum->o_stack_depth;
	
	if (stack_depth_adjust >= 0)
	    pop(stack_depth_adjust);
	else {
	    /*
	     * If we get to here then there were major problems.  The function
	     * removed too many items off of the stack.  We had placed extra
	     * (unused) stack stack space to allow for this but the function
	     * exceeded even that.  Data on the stack may have been lost.
	     * The only thing that we can do is move the stack pointer back and
	     * hope.  (We have not seen real Postscript files that have this
	     * problem.)
	     */
	    push(-stack_depth_adjust);
	}
	ifree_object(penum->pfn, "sampled_data_continue(pfn)");
	ifree_object(penum, "sampled_data_continue((enum)");
	return_error(e_undefinedresult);
    }
    
    /* Save data from the given function */
    data_ptr = cube_ptr_from_index(params, penum->indexes);
    for (i=0; i < num_out; i++) {
	ulong cv;
        double value;
	double rmin = params->Range[2 * i];
	double rmax = params->Range[2 * i + 1];

        code = real_param(op + i - num_out + 1, &value);
        if (code < 0)
	    return code;
	if (value < rmin)
	    value = rmin;
	else if (value > rmax)
	    value = rmax;
	value = (value - rmin) / (rmax - rmin);		/* Convert to 0 to 1.0 */
	cv = (int) (value * sampled_data_value_max + 0.5);
	for (j = 0; j < bps; j++)
	    data_ptr[bps * i + j] = (byte)(cv >> ((bps - 1 - j) * 8));	/* MSB first */
    }
    pop(num_out);		    /* Move op to base of result values */
    
    /* Check if we are done collecting data. */

    if (increment_cube_indexes(params, penum->indexes)) {
	pop(O_STACK_PAD);	    /* Remove spare stack space */
	/* Execute the closing procedure, if given */
	code = 0;
	if (esp_finish_proc != 0)
	    code = esp_finish_proc(i_ctx_p);

	return code;
    }

    /* Now get the data for the next location */

    return sampled_data_sample(i_ctx_p);
}

/*
 * We have collected all of the sample data.  Create a type 0 function stucture.
 */
private int
sampled_data_finish(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_sampled_data_enum *penum = senum;
    /* Build a type 0 function using the given parameters */
    gs_function_Sd_params_t * params =
	(gs_function_Sd_params_t *)&penum->pfn->params;
    gs_function_t * pfn;
    ref cref;			/* closure */
    int code = gs_function_Sd_init(&pfn, params, imemory);

    if (code < 0)
	return code;

    code = ialloc_ref_array(&cref, a_executable | a_execute, 2,
			    "sampled_data_finish(cref)");
    if (code < 0)
	return code;

    make_istruct_new(cref.value.refs, a_executable | a_execute, pfn);
    make_oper_new(cref.value.refs + 1, 0, zexecfunction);
    ref_assign(op, &cref);

    esp -= estack_storage;
    ifree_object(penum->pfn, "sampled_data_finish(pfn)");
    ifree_object(penum, "sampled_data_finish(enum)");
    return o_pop_estack;
}


/* ------ Initialization procedure ------ */

const op_def zfsample_op_defs[] =
{
    op_def_begin_level2(),
    {"1.buildsampledfunction", zbuildsampledfunction},
    op_def_end(0)
};

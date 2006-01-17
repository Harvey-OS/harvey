/* Copyright (C) 2000 Artifex Software Inc.   All rights reserved.
  
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

/* $Id: zdscpars.c,v 1.17 2004/11/17 19:48:01 ray Exp $ */
/* C language interface routines to DSC parser */

/*
 * The DSC parser consists of three pieces.  The first piece is a DSC parser
 * which was coded by Russell Lang (dscparse.c and dscparse.h).  The second
 * piece is this module.  These two are sufficient to parse DSC comments
 * and make them available to a client written in PostScript.  The third
 * piece is a PostScript language module (gs_dscp.ps) that uses certain
 * comments to affect the interpretation of the file.
 *
 * The .initialize_dsc_parser operator defined in this file creates an
 * instance of Russell's parser, and puts it in a client-supplied dictionary
 * under a known name (/DSC_struct).
 *
 * When the PostScript scanner sees a possible DSC comment (first characters
 * in a line are %%), it calls the procedure that is the value of the user
 * parameter ProcessDSCComments.  This procedure should loads the dictionary
 * that was passed to .initialize_dsc_parser, and then call the
 * .parse_dsc_comments operator defined in this file.
 *
 * These two operators comprise the interface between PostScript and C code.
 *
 * There is a "feature" named usedsc that loads a PostScript file
 * (gs_dscp.ps), which installs a simple framework for processing DSC
 * comments and having them affect interpretation of the file (e.g., by
 * setting page device parameters).  See gs_dscp.ps for more information.
 *
 * .parse_dsc_comments pulls the comment string off of the stack and passes
 * it to Russell's parser.  That parser parses the comment and puts any
 * parameter values into a DSC structure.  That parser also returns a code
 * which indicates which type of comment was found.  .parse_dsc_comments
 * looks at the return code and transfers any interesting parameters from
 * the DSC structure into key value pairs in the dsc_dict dictionary.  It
 * also translates the comment type code into a key name (comment name).
 * The key name is placed on the operand stack.  Control then returns to
 * PostScript code, which can pull the key name from the operand stack and
 * use it to determine what further processing needs to be done at the PS
 * language level.
 *
 * To add support for new DSC comments:
 *
 * 1. Verify that Russell's parser supports the comment.  If not, then add
 *    the required support.
 *
 * 2. Add an entry into DSCcmdlist.  This table contains three values for
 *    each command that we support.  The first is Russell's return code for
 *    the command. The second is the key name that we pass back on the
 *    operand stack.  (Thus this table translates Russell's codes into key
 *    names for the PostScript client.)  The third entry is a pointer to a
 *    local function for transferring values from Russell's DSC structure
 *    into key/value pairs in dsc_dict.
 *
 * 3. Create the local function described at the end of the last item.
 *    There are some support routines like dsc_put_integer() and
 *    dsc_put_string() to help implement these functions.
 *
 * 4. If the usedsc feature should recognize and process the new comments,
 *    add a processing routine into the dictionary DSCparserprocs in
 *    gs_dscp.ps.  The keys in this dictionary are the key names described
 *    in item 2 above.
 */

#include "ghost.h"
#include "string_.h"
#include "memory_.h"
#include "gsstruct.h"
#include "ialloc.h"
#include "iname.h"
#include "istack.h"		/* for iparam.h */
#include "iparam.h"
#include "ivmspace.h"
#include "oper.h"
#include "estack.h"
#include "store.h"
#include "idict.h"
#include "iddict.h"
#include "dscparse.h"

/*
 * Declare the structure we use to represent an instance of the parser
 * as a t_struct.  Currently it just saves a pointer to Russell's
 * data structure.
 */
typedef struct dsc_data_s {
    CDSC *dsc_data_ptr;
} dsc_data_t;

/* Structure descriptors */
private void dsc_finalize(void *vptr);
gs_private_st_simple_final(st_dsc_data_t, dsc_data_t, "dsc_data_struct", dsc_finalize);

/* Define the key name for storing the instance pointer in a dictionary. */
private const char * const dsc_dict_name = "DSC_struct";

/* ---------------- Initialization / finalization ---------------- */

/*
 * If we return CDSC_OK then Russell's parser will make it best guess when
 * it encounters unexpected comment situations.
 */
private int
dsc_error_handler(void *caller_data, CDSC *dsc, unsigned int explanation,
		  const char *line, unsigned int line_len)
{
    return CDSC_OK;
}

/*
 * This operator creates a new, initialized instance of the DSC parser.
 */
/* <dict> .initialize_dsc_parser - */
private int
zinitialize_dsc_parser(i_ctx_t *i_ctx_p)
{
    ref local_ref;
    int code;
    os_ptr const op = osp;
    dict * const pdict = op->value.pdict;
    gs_memory_t * const mem = (gs_memory_t *)dict_memory(pdict);
    dsc_data_t * const data =
	gs_alloc_struct(mem, dsc_data_t, &st_dsc_data_t,
			"DSC parser init");

    data->dsc_data_ptr = dsc_init((void *) "Ghostscript DSC parsing");
    if (!data->dsc_data_ptr)
    	return_error(e_VMerror);
    dsc_set_error_function(data->dsc_data_ptr, dsc_error_handler);
    make_astruct(&local_ref, a_readonly | r_space(op), (byte *) data);
    code = idict_put_string(op, dsc_dict_name, &local_ref);
    if (code >= 0)
	pop(1);
    return code;
}

/*
 * This routine will free the memory associated with Russell's parser.
 */
private void
dsc_finalize(void *vptr)
{
    dsc_data_t * const st = vptr;

    if (st->dsc_data_ptr)
	dsc_free(st->dsc_data_ptr);
    st->dsc_data_ptr = NULL;
}


/* ---------------- Parsing ---------------- */

/* ------ Utilities for returning values ------ */

/* Return an integer value. */
private int
dsc_put_int(gs_param_list *plist, const char *keyname, int value)
{
    return param_write_int(plist, keyname, &value);
}

/* Return a string value. */
private int
dsc_put_string(gs_param_list *plist, const char *keyname,
	       const char *string)
{
    gs_param_string str;

    param_string_from_transient_string(str, string);
    return param_write_string(plist, keyname, &str);
}

/* Return a BoundingBox value. */
private int
dsc_put_bounding_box(gs_param_list *plist, const char *keyname,
		     const CDSCBBOX *pbbox)
{
    /* pbbox is NULL iff the bounding box values was "(atend)". */
    int values[4];
    gs_param_int_array va;

    if (!pbbox)
	return 0;
    values[0] = pbbox->llx;
    values[1] = pbbox->lly;
    values[2] = pbbox->urx;
    values[3] = pbbox->ury;
    va.data = values;
    va.size = 4;
    va.persistent = false;
    return param_write_int_array(plist, keyname, &va);
}

/* ------ Return values for individual comment types ------ */

/*
 * These routines transfer data from the C structure into Postscript
 * key/value pairs in a dictionary.
 */
private int
dsc_adobe_header(gs_param_list *plist, const CDSC *pData)
{
    return dsc_put_int(plist, "EPSF", (int)(pData->epsf? 1: 0));
}

private int
dsc_creator(gs_param_list *plist, const CDSC *pData)
{
    return dsc_put_string(plist, "Creator", pData->dsc_creator );
}

private int
dsc_creation_date(gs_param_list *plist, const CDSC *pData)
{
    return dsc_put_string(plist, "CreationDate", pData->dsc_date );
}

private int
dsc_title(gs_param_list *plist, const CDSC *pData)
{
    return dsc_put_string(plist, "Title", pData->dsc_title );
}

private int
dsc_for(gs_param_list *plist, const CDSC *pData)
{
    return dsc_put_string(plist, "For", pData->dsc_for);
}

private int
dsc_bounding_box(gs_param_list *plist, const CDSC *pData)
{
    return dsc_put_bounding_box(plist, "BoundingBox", pData->bbox);
}

private int
dsc_page(gs_param_list *plist, const CDSC *pData)
{
    int page_num = pData->page_count;

    if (page_num)		/* If we have page information */
        return dsc_put_int(plist, "PageNum",
		       pData->page[page_num - 1].ordinal );
    else			/* No page info - so return page=0 */
        return dsc_put_int(plist, "PageNum", 0 );
}

private int
dsc_pages(gs_param_list *plist, const CDSC *pData)
{
    return dsc_put_int(plist, "NumPages", pData->page_pages);
}

private int
dsc_page_bounding_box(gs_param_list *plist, const CDSC *pData)
{
    return dsc_put_bounding_box(plist, "PageBoundingBox", pData->page_bbox);
}

/*
 * Translate Russell's defintions of orientation into Postscript's.
 */
private int
convert_orient(CDSC_ORIENTATION_ENUM orient)
{
    switch (orient) {
    case CDSC_PORTRAIT: return 0;
    case CDSC_LANDSCAPE: return 1;
    case CDSC_UPSIDEDOWN: return 2;
    case CDSC_SEASCAPE: return 3;
    default: return -1;
    }
}

private int
dsc_page_orientation(gs_param_list *plist, const CDSC *pData)
{
    int page_num = pData->page_count;

    /*
     * The PageOrientation comment might be either in the 'defaults'
     * section or in a page section.  If in the defaults then fhe value
     * will be in page_orientation.
     */
    if (page_num && pData->page[page_num - 1].orientation != CDSC_ORIENT_UNKNOWN)
	return dsc_put_int(plist, "PageOrientation",
			convert_orient(pData->page[page_num - 1].orientation));
    else
        return dsc_put_int(plist, "Orientation",
			   convert_orient(pData->page_orientation));
}

private int
dsc_orientation(gs_param_list *plist, const CDSC *pData)
{
    return dsc_put_int(plist, "Orientation", 
			   convert_orient(pData->page_orientation));
}

private int
dsc_viewing_orientation(gs_param_list *plist, const CDSC *pData)
{
    int page_num = pData->page_count;
    const char *key;
    const CDSCCTM *pctm;
    float values[4];
    gs_param_float_array va;

    /*
     * As for PageOrientation, ViewingOrientation may be either in the
     * 'defaults' section or in a page section.
     */
    if (page_num && pData->page[page_num - 1].viewing_orientation != NULL) {
	key = "PageViewingOrientation";
	pctm = pData->page[page_num - 1].viewing_orientation;
    } else {
        key = "ViewingOrientation";
	pctm = pData->viewing_orientation;
    }
    values[0] = pctm->xx;
    values[1] = pctm->xy;
    values[2] = pctm->yx;
    values[3] = pctm->yy;
    va.data = values;
    va.size = 4;
    va.persistent = false;
    return param_write_float_array(plist, key, &va);
}

/*
 * This list is used to translate the commment code returned
 * from Russell's DSC parser, define a name, and a parameter procedure.
 */
typedef struct cmd_list_s {
    int code;			/* Russell's DSC parser code (see dsc.h) */
    const char *comment_name;	/* A name to be returned to postscript caller */
    int (*dsc_proc) (gs_param_list *, const CDSC *);
				/* A routine for transferring parameter values
				   from C data structure to postscript dictionary
				   key/value pairs. */
} cmdlist_t;

private const cmdlist_t DSCcmdlist[] = { 
    { CDSC_PSADOBE,	    "Header",		dsc_adobe_header },
    { CDSC_CREATOR,	    "Creator",		dsc_creator },
    { CDSC_CREATIONDATE,    "CreationDate",	dsc_creation_date },
    { CDSC_TITLE,	    "Title",		dsc_title },
    { CDSC_FOR,		    "For",		dsc_for },
    { CDSC_BOUNDINGBOX,     "BoundingBox",	dsc_bounding_box },
    { CDSC_ORIENTATION,	    "Orientation",	dsc_orientation },
    { CDSC_BEGINDEFAULTS,   "BeginDefaults",	NULL },
    { CDSC_ENDDEFAULTS,     "EndDefaults",	NULL },
    { CDSC_PAGE,	    "Page",		dsc_page },
    { CDSC_PAGES,	    "Pages",		dsc_pages },
    { CDSC_PAGEORIENTATION, "PageOrientation",  dsc_page_orientation },
    { CDSC_PAGEBOUNDINGBOX, "PageBoundingBox",	dsc_page_bounding_box },
    { CDSC_VIEWINGORIENTATION, "ViewingOrientation", dsc_viewing_orientation },
    { CDSC_EOF,		    "EOF",		NULL },
    { 0,		    "NOP",		NULL }  /* Table terminator */
};

/* ------ Parser interface ------ */

/*
 * There are a few comments that we do not want to send to Russell's
 * DSC parser.  If we send the data block type comments, Russell's
 * parser will want to skip the specified block of data.  This is not
 * appropriate for our situation.  So we use this list to check for this
 * type of comment and do not send it to Russell's parser if found.
 */
private const char * const BadCmdlist[] = {
    "%%BeginData:",
    "%%EndData",
    "%%BeginBinary:",
    "%%EndBinary",
    NULL			    /* List terminator */
};

/* See comments at start of module for description. */
/* <dict> <string> .parse_dsc_comments <dict> <dsc code> */
private int
zparse_dsc_comments(i_ctx_t *i_ctx_p)
{
#define MAX_DSC_MSG_SIZE (DSC_LINE_LENGTH + 4)	/* Allow for %% and CR/LF */
    os_ptr const opString = osp;
    os_ptr const opDict = opString - 1;
    uint ssize;
    int comment_code, code;
    char dsc_buffer[MAX_DSC_MSG_SIZE + 2];
    const cmdlist_t *pCmdList = DSCcmdlist;
    const char * const *pBadList = BadCmdlist;
    ref * pvalue;
    CDSC * dsc_data = NULL;
    dict_param_list list;

    /*
     * Verify operand types and length of DSC comment string.  If a comment
     * is too long then we simply truncate it.  Russell's parser gets to
     * handle any errors that may result.  (Crude handling but the comment
     * is bad, so ...).
     */
    check_type(*opString, t_string);
    check_dict_write(*opDict);
    ssize = r_size(opString);
    if (ssize > MAX_DSC_MSG_SIZE)   /* need room for EOL + \0 */
        ssize = MAX_DSC_MSG_SIZE;
    /*
     * Pick up the comment string to be parsed.
     */
    memcpy(dsc_buffer, opString->value.bytes, ssize);
    dsc_buffer[ssize] = 0x0d;	    /* Russell wants a 'line end' */
    dsc_buffer[ssize + 1] = 0;	    /* Terminate string */
    /*
     * Skip data block comments (see comments in front of BadCmdList).
     */
    while (*pBadList && strncmp(*pBadList, dsc_buffer, strlen(*pBadList)))
        pBadList++;
    if (*pBadList) {		    /* If found in list, then skip comment */	
        comment_code = 0;	    /* Force NOP */
    }
    else {
        /*
         * Parse comments - use Russell Lang's DSC parser.  We need to get
         * data area for Russell Lang's parser.  Note: We have saved the
         * location of the data area for the parser in our DSC dict.
         */
        code = dict_find_string(opDict, dsc_dict_name, &pvalue);
	dsc_data = r_ptr(pvalue, dsc_data_t)->dsc_data_ptr;
        if (code < 0)
            return code;
        comment_code = dsc_scan_data(dsc_data, dsc_buffer, ssize + 1);
        if_debug1('%', "[%%].parse_dsc_comments: code = %d\n", comment_code);
	/*
	 * We ignore any errors from Russell's parser.  The only value that
	 * it will return for an error is -1 so there is very little information.
	 * We also do not want bad DSC comments to abort processing of an
	 * otherwise valid PS file.
	 */
        if (comment_code < 0)
	    comment_code = 0;
    }
    /*
     * Transfer data from DSC structure to postscript variables.
     * Look up proper handler in the local cmd decode list.
     */
    while (pCmdList->code && pCmdList->code != comment_code )
	pCmdList++;
    if (pCmdList->dsc_proc) {
	code = dict_param_list_write(&list, opDict, NULL, iimemory);
	if (code < 0)
	    return code;
	code = (pCmdList->dsc_proc)((gs_param_list *)&list, dsc_data);
	iparam_list_release(&list);
	if (code < 0)
	    return code;
    }

    /* Put DSC comment name onto operand stack (replace string). */

    return name_enter_string(imemory, pCmdList->comment_name, opString);
}

/* ------ Initialization procedure ------ */

const op_def zdscpars_op_defs[] = {
    {"1.initialize_dsc_parser", zinitialize_dsc_parser},
    {"2.parse_dsc_comments", zparse_dsc_comments},
    op_def_end(0)
};

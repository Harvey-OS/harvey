/* Copyright (C) 1989, 2000 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zfile.c,v 1.2 2000/03/10 04:35:08 lpd Exp $ */
/* Non-I/O file operators */
#include "memory_.h"
#include "string_.h"
#include "ghost.h"
#include "gscdefs.h"		/* for gx_io_device_table */
#include "gp.h"
#include "gsfname.h"
#include "gsstruct.h"		/* for registering root */
#include "gxalloc.h"		/* for streams */
#include "oper.h"
#include "estack.h"		/* for filenameforall, .execfile */
#include "ialloc.h"
#include "ilevel.h"		/* %names only work in Level 2 */
#include "interp.h"		/* gs_errorinfo_put_string prototype */
#include "isave.h"		/* for restore */
#include "iutil.h"
#include "stream.h"
#include "strimpl.h"
#include "sfilter.h"
#include "gxiodev.h"		/* must come after stream.h */
				/* and before files.h */
#include "files.h"
#include "main.h"		/* for gs_lib_paths */
#include "store.h"

/* Import the IODevice table. */
extern_gx_io_device_table();

/* Import the dtype of the stdio IODevices. */
extern const char iodev_dtype_stdio[];

/* Forward references: file name parsing. */
private int parse_file_name(P2(const ref * op, gs_parsed_file_name_t * pfn));
private int parse_real_file_name(P4(const ref * op,
				    gs_parsed_file_name_t * pfn,
				    gs_memory_t *mem, client_name_t cname));

/* Forward references: other. */
private int zopen_file(P4(const gs_parsed_file_name_t *pfn,
			  const char *file_access, stream **ps,
			  gs_memory_t *mem));
private iodev_proc_open_file(iodev_os_open_file);
private int execfile_finish(P1(i_ctx_t *));
private int execfile_cleanup(P1(i_ctx_t *));

/*
 * Since there can be many file objects referring to the same file/stream,
 * we can't simply free a stream when we close it.  On the other hand,
 * we don't want freed streams to clutter up memory needlessly.
 * Our solution is to retain the freed streams, and reuse them.
 * To prevent an old file object from being able to access a reused stream,
 * we keep a serial number in each stream, and check it against a serial
 * number stored in the file object (as the "size"); when we close a file,
 * we increment its serial number.  If the serial number ever overflows,
 * we leave it at zero, and do not reuse the stream.
 * (This will never happen.)
 *
 * Storage management for this scheme is a little tricky.  We maintain an
 * invariant that says that a stream opened at a given save level always
 * uses a stream structure allocated at that level.  By doing this, we don't
 * need to keep track separately of streams open at a level vs. streams
 * allocated at a level.  To make this interact properly with save and
 * restore, we maintain a list of all streams allocated at this level, both
 * open and closed.  We store this list in the allocator: this is a hack,
 * but it simplifies bookkeeping (in particular, it guarantees the list is
 * restored properly by a restore).
 *
 * We want to close streams freed by restore and by garbage collection.  We
 * use the finalization procedure for this.  For restore, we don't have to
 * do anything special to make this happen.  For garbage collection, we do
 * something more drastic: we simply clear the list of known streams (at all
 * save levels).  Any streams open at the time of garbage collection will no
 * longer participate in the list of known streams, but this does no harm;
 * it simply means that they won't get reused, and can only be reclaimed by
 * a future garbage collection or restore.
 */

/* 
 * Define the default stream buffer sizes.  For file streams,
 * this is arbitrary, since the C library or operating system
 * does its own buffering in addition.
 * However, the buffer size for eexec decoding is NOT arbitrary:
 * it must be at most 512.
 */
#define DEFAULT_BUFFER_SIZE 512
const uint file_default_buffer_size = DEFAULT_BUFFER_SIZE;

/* An invalid file object */
private stream invalid_file_stream;
stream *const invalid_file_entry = &invalid_file_stream;

/* Initialize the file table */
private int
zfile_init(i_ctx_t *i_ctx_p)
{
    /* Create and initialize an invalid (closed) stream. */
    /* Initialize the stream for the sake of the GC, */
    /* and so it can act as an empty input stream. */

    stream *const s = &invalid_file_stream;

    s_init(s, NULL);
    sread_string(s, NULL, 0);
    s->next = s->prev = 0;
    s_init_no_id(s);
    return 0;
}

/* Make an invalid file object. */
void
make_invalid_file(ref * fp)
{
    make_file(fp, avm_invalid_file_entry, ~0, invalid_file_entry);
}

/* <name_string> <access_string> file <file> */
private int
zfile(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    char file_access[3];
    gs_parsed_file_name_t pname;
    const byte *astr;
    int code;
    stream *s;

    check_read_type(*op, t_string);
    astr = op->value.const_bytes;
    switch (r_size(op)) {
	case 2:
	    if (astr[1] != '+')
		return_error(e_invalidfileaccess);
	    file_access[1] = '+';
	    file_access[2] = 0;
	    break;
	case 1:
	    file_access[1] = 0;
	    break;
	default:
	    return_error(e_invalidfileaccess);
    }
    switch (astr[0]) {
	case 'r':
	case 'w':
	case 'a':
	    break;
	default:
	    return_error(e_invalidfileaccess);
    }
    file_access[0] = astr[0];
    code = parse_file_name(op - 1, &pname);
    if (code < 0)
	return code;
	/*
	 * HACK: temporarily patch the current context pointer into the
	 * state pointer for stdio-related devices.  See ziodev.c for
	 * more information.
	 */
    if (pname.iodev && pname.iodev->dtype == iodev_dtype_stdio) {
	if (pname.fname)
	    return_error(e_invalidfileaccess);
	pname.iodev->state = i_ctx_p;
	code = (*pname.iodev->procs.open_device)(pname.iodev,
						 file_access, &s, imemory);
	pname.iodev->state = NULL;
    } else {
	if (pname.iodev == NULL)
	    pname.iodev = iodev_default;
	code = zopen_file(&pname, file_access, &s, imemory);
    }
    if (code < 0)
	return code;
    make_stream_file(op - 1, s, file_access);
    pop(1);
    return code;
}

/* ------ Level 2 extensions ------ */

/* <string> deletefile - */
private int
zdeletefile(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_parsed_file_name_t pname;
    int code = parse_real_file_name(op, &pname, imemory, "deletefile");

    if (code < 0)
	return code;
    code = (*pname.iodev->procs.delete_file)(pname.iodev, pname.fname);
    gs_free_file_name(&pname, "deletefile");
    if (code < 0)
	return code;
    pop(1);
    return 0;
}

/* <template> <proc> <scratch> filenameforall - */
/****** NOT CONVERTED FOR IODEVICES YET ******/
private int file_continue(P1(i_ctx_t *));
private int file_cleanup(P1(i_ctx_t *));
private int
zfilenameforall(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    file_enum *pfen;
    int code;

    check_write_type(*op, t_string);
    check_proc(op[-1]);
    check_read_type(op[-2], t_string);
    /* Push a mark, the pattern, the scratch string, the enumerator, */
    /* and the procedure, and invoke the continuation. */
    check_estack(7);
    pfen = gp_enumerate_files_init((char *)op[-2].value.bytes, r_size(op - 2), imemory);
    if (pfen == 0)
	return_error(e_VMerror);
    push_mark_estack(es_for, file_cleanup);
    *++esp = op[-2];
    *++esp = *op;
    ++esp;
    make_istruct(esp, 0, pfen);
    *++esp = op[-1];
    pop(3);
    code = file_continue(i_ctx_p);
    return (code == o_pop_estack ? o_push_estack : code);
}
/* Continuation operator for enumerating files */
private int
file_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    es_ptr pscratch = esp - 2;
    file_enum *pfen = r_ptr(esp - 1, file_enum);
    uint len = r_size(pscratch);
    uint code =
    gp_enumerate_files_next(pfen, (char *)pscratch->value.bytes, len);

    if (code == ~(uint) 0) {	/* all done */
	esp -= 4;		/* pop proc, pfen, scratch, mark */
	return o_pop_estack;
    } else if (code > len)	/* overran string */
	return_error(e_rangecheck);
    else {
	push(1);
	ref_assign(op, pscratch);
	r_set_size(op, code);
	push_op_estack(file_continue);	/* come again */
	*++esp = pscratch[2];	/* proc */
	return o_push_estack;
    }
}
/* Cleanup procedure for enumerating files */
private int
file_cleanup(i_ctx_t *i_ctx_p)
{
    gp_enumerate_files_close(r_ptr(esp + 4, file_enum));
    return 0;
}

/* <string1> <string2> renamefile - */
private int
zrenamefile(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_parsed_file_name_t pname1, pname2;
    int code = parse_real_file_name(op - 1, &pname1, imemory,
				    "renamefile(from)");

    if (code < 0)
	return code;
    pname2.fname = 0;
    code = parse_real_file_name(op, &pname2, imemory, "renamefile(to)");
    if (code < 0 || pname1.iodev != pname2.iodev ||
	(code = (*pname1.iodev->procs.rename_file)(pname1.iodev,
					    pname1.fname, pname2.fname)) < 0
	) {
	if (code >= 0)
	    code = gs_note_error(e_invalidfileaccess);
    }
    gs_free_file_name(&pname2, "renamefile(to)");
    gs_free_file_name(&pname1, "renamefile(from)");
    if (code < 0)
	return code;
    pop(2);
    return 0;
}

/* <file> status <open_bool> */
/* <string> status <pages> <bytes> <ref_time> <creation_time> true */
/* <string> status false */
private int
zstatus(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    switch (r_type(op)) {
	case t_file:
	    {
		stream *s;

		make_bool(op, (file_is_valid(s, op) ? 1 : 0));
	    }
	    return 0;
	case t_string:
	    {
		gs_parsed_file_name_t pname;
		struct stat fstat;
		int code = parse_file_name(op, &pname);

		if (code < 0)
		    return code;
		code = gs_terminate_file_name(&pname, imemory, "status");
		if (code < 0)
		    return code;
		code = (*pname.iodev->procs.file_status)(pname.iodev,
						       pname.fname, &fstat);
		switch (code) {
		    case 0:
			check_ostack(4);
			/*
			 * Check to make sure that the file size fits into
			 * a PostScript integer.  (On some systems, long is
			 * 32 bits, but file sizes are 64 bits.)
			 */
			push(4);
			make_int(op - 4, stat_blocks(&fstat));
			make_int(op - 3, fstat.st_size);
			/*
			 * We can't check the value simply by using ==,
			 * because signed/unsigned == does the wrong thing.
			 * Instead, since integer assignment only keeps the
			 * bottom bits, we convert the values to double
			 * and then test for equality.  This handles all
			 * cases of signed/unsigned or width mismatch.
			 */
			if ((double)op[-4].value.intval !=
			      (double)stat_blocks(&fstat) ||
			    (double)op[-3].value.intval !=
			      (double)fstat.st_size
			    )
			    return_error(e_limitcheck);
			make_int(op - 2, fstat.st_mtime);
			make_int(op - 1, fstat.st_ctime);
			make_bool(op, 1);
			break;
		    case e_undefinedfilename:
			make_bool(op, 0);
			code = 0;
		}
		gs_free_file_name(&pname, "status");
		return code;
	    }
	default:
	    return_op_typecheck(op);
    }
}

/* ------ Non-standard extensions ------ */

/* <executable_file> .execfile - */
private int
zexecfile(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_type_access(*op, t_file, a_executable | a_read | a_execute);
    check_estack(4);		/* cleanup, file, finish, file */
    push_mark_estack(es_other, execfile_cleanup);
    *++esp = *op;
    push_op_estack(execfile_finish);
    return zexec(i_ctx_p);
}
/* Finish normally. */
private int
execfile_finish(i_ctx_t *i_ctx_p)
{
    check_ostack(1);
    esp -= 2;
    execfile_cleanup(i_ctx_p);
    return o_pop_estack;
}
/* Clean up by closing the file. */
private int
execfile_cleanup(i_ctx_t *i_ctx_p)
{
    check_ostack(1);
    *++osp = esp[2];
    return zclosefile(i_ctx_p);
}

/* <dir> <name> .filenamedirseparator <string> */
private int
zfilenamedirseparator(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const char *sepr;

    check_read_type(*op, t_string);
    check_read_type(op[-1], t_string);
    sepr =
	gp_file_name_concat_string((const char *)op[-1].value.const_bytes,
				   r_size(op - 1),
				   (const char *)op->value.const_bytes,
				   r_size(op));
    make_const_string(op - 1, avm_foreign | a_readonly,
		      strlen(sepr), (const byte *)sepr);
    pop(1);
    return 0;
}

/* - .filenamelistseparator <string> */
private int
zfilenamelistseparator(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    make_const_string(op, avm_foreign | a_readonly, 1,
		      (const byte *)&gp_file_name_list_separator);
    return 0;
}

/* <name> .filenamesplit <dir> <base> <extension> */
private int
zfilenamesplit(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_read_type(*op, t_string);
/****** NOT IMPLEMENTED YET ******/
    return_error(e_undefined);
}

/* <string> findlibfile <found_string> <file> true */
/* <string> findlibfile <string> false */
private int
zfindlibfile(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;
#define MAX_CNAME 200
    byte cname[MAX_CNAME];
    uint clen;
    gs_parsed_file_name_t pname;
    stream *s;

    check_ostack(2);
    code = parse_file_name(op, &pname);
    if (code < 0)
	return code;
    if (pname.iodev == NULL)
	pname.iodev = iodev_default;
    if (pname.iodev != iodev_default) {		/* Non-OS devices don't have search paths (yet). */
	code = zopen_file(&pname, "r", &s, imemory);
	if (code < 0) {
	    push(1);
	    make_false(op);
	    return 0;
	}
	make_stream_file(op + 1, s, "r");
    } else {
	byte *cstr;

	code = lib_file_open(pname.fname, pname.len, cname, MAX_CNAME,
			     &clen, op + 1, imemory);
	if (code == e_VMerror)
	    return code;
	if (code < 0) {
	    push(1);
	    make_false(op);
	    return 0;
	}
	cstr = ialloc_string(clen, "findlibfile");
	if (cstr == 0)
	    return_error(e_VMerror);
	memcpy(cstr, cname, clen);
	make_string(op, a_all | icurrent_space, clen, cstr);
    }
    push(2);
    make_true(op);
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zfile_op_defs[] =
{
    {"1deletefile", zdeletefile},
    {"1.execfile", zexecfile},
    {"2file", zfile},
    {"3filenameforall", zfilenameforall},
    {"2.filenamedirseparator", zfilenamedirseparator},
    {"0.filenamelistseparator", zfilenamelistseparator},
    {"1.filenamesplit", zfilenamesplit},
    {"1findlibfile", zfindlibfile},
    {"2renamefile", zrenamefile},
    {"1status", zstatus},
		/* Internal operators */
    {"0%file_continue", file_continue},
    {"0%execfile_finish", execfile_finish},
    op_def_end(zfile_init)
};

/* ------ File name parsing ------ */

/* Parse a file name into device and individual name. */
/* See gsfname.c for details. */
private int
parse_file_name(const ref * op, gs_parsed_file_name_t * pfn)
{
    check_read_type(*op, t_string);
    return gs_parse_file_name(pfn, (const char *)op->value.const_bytes,
			      r_size(op));
}

/* Parse a real (non-device) file name and convert to a C string. */
/* See gsfname.c for details. */
private int
parse_real_file_name(const ref *op, gs_parsed_file_name_t *pfn,
		     gs_memory_t *mem, client_name_t cname)
{
    check_read_type(*op, t_string);
    return gs_parse_real_file_name(pfn, (const char *)op->value.const_bytes,
				   r_size(op), mem, cname);
}

/* ------ Stream opening ------ */

/*
 * Open a file specified by a parsed file name (which may be only a
 * device).
 */
private int
zopen_file(const gs_parsed_file_name_t *pfn, const char *file_access,
	   stream **ps, gs_memory_t *mem)
{
    gx_io_device *const iodev = pfn->iodev;

    if (pfn->fname == NULL)	/* just a device */
	return iodev->procs.open_device(iodev, file_access, ps, mem);
    else {			/* file */
	iodev_proc_open_file((*open_file)) = iodev->procs.open_file;

	if (open_file == 0)
	    open_file = iodev_os_open_file;
	return open_file(iodev, pfn->fname, pfn->len, file_access, ps, mem);
    }
}

/*
 * Define the file_open procedure for the %os% IODevice (also used, as the
 * default, for %pipe% and possibly others).
 */
private int
iodev_os_open_file(gx_io_device * iodev, const char *fname, uint len,
		   const char *file_access, stream ** ps, gs_memory_t * mem)
{
    return file_open_stream(fname, len, file_access,
			    file_default_buffer_size, ps,
			    iodev->procs.fopen, mem);
}

/* Make a t_file reference to a stream. */
void
make_stream_file(ref * pfile, stream * s, const char *access)
{
    uint attrs =
	(access[1] == '+' ? a_write + a_read + a_execute : 0) |
	imemory_space((gs_ref_memory_t *) s->memory);

    if (access[0] == 'r') {
	make_file(pfile, attrs | (a_read | a_execute), s->read_id, s);
	s->write_id = 0;
    } else {
	make_file(pfile, attrs | a_write, s->write_id, s);
	s->read_id = 0;
    }
}

/* Open an OS-level file (like fopen), using the search paths if necessary. */
/* Note that it does not automatically look in the current */
/* directory first (or at all): this is like Unix, and unlike MS-DOS. */
private int
lib_file_fopen(gx_io_device * iodev, const char *bname,
	       const char *ignore_access, FILE ** pfile,
	       char *rfname, uint rnamelen)
{
    char fmode[3];		/* r, [b], null */
    int len = strlen(bname);
    const gs_file_path *pfpath = &gs_lib_path;
    uint pi;

    strcpy(fmode, "r");
    strcat(fmode, gp_fmode_binary_suffix);
    if (gp_file_name_is_absolute(bname, len))
	return (*iodev->procs.fopen)(iodev, bname, fmode, pfile,
				     rfname, rnamelen);
    /* Go through the list of search paths */
    for (pi = 0; pi < r_size(&pfpath->list); ++pi) {
	const ref *prdir = pfpath->list.value.refs + pi;
	const char *pstr = (const char *)prdir->value.const_bytes;
	uint plen = r_size(prdir);
	const char *cstr =
	gp_file_name_concat_string(pstr, plen, bname, len);
	int up, i;
	int code;

	/* Concatenate the prefix, combiner, and file name. */
	/* Do this carefully in case rfname is the same */
	/* as fname.  (We don't worry about the case */
	/* where rfname only overlaps fname.) */
	up = plen + strlen(cstr);
	if (up + len + 1 > rnamelen)
	    return_error(e_limitcheck);
	for (i = len + 1; --i >= 0;)
	    rfname[i + up] = bname[i];
	memcpy(rfname, pstr, plen);
	memcpy(rfname + plen, cstr, strlen(cstr));
	code = (*iodev->procs.fopen)(iodev, rfname, fmode,
				     pfile, rfname, rnamelen);
	if (code >= 0)
	    return code;
	/* strcpy isn't guaranteed to work for overlapping */
	/* source and destination, so: */
	if (rfname == bname)
	    for (i = 0; (rfname[i] = rfname[i + up]) != 0; i++);
    }
    return_error(e_undefinedfilename);
}
/* The startup code calls this to open @-files. */
FILE *
lib_fopen(const char *bname)
{
    FILE *file = NULL;
    /* We need a buffer to hold the expanded file name. */
    char buffer[gp_file_name_sizeof];
    /* We can't count on the IODevice table to have been initialized yet. */
    /* Allocate a copy of the default IODevice. */
    gx_io_device iodev_default_copy;
    int code;

    iodev_default_copy = *gx_io_device_table[0];
    code = lib_file_fopen(&iodev_default_copy, bname, "r", &file,
			  buffer, gp_file_name_sizeof);
    return (code < 0 ? NULL : file);
}

/* Open a file stream on an OS file and create a file object, */
/* using the search paths. */
/* The startup code calls this to open the initialization file gs_init.ps. */
int
lib_file_open(const char *fname, uint len, byte * cname, uint max_clen,
	      uint * pclen, ref * pfile, gs_memory_t *mem)
{
    stream *s;
    int code = file_open_stream(fname, len, "r", file_default_buffer_size,
				&s, lib_file_fopen, mem);
    char *bname;
    uint blen;

    if (code < 0)
	return code;
    /* Get the name from the stream buffer. */
    bname = (char *)s->cbuf;
    blen = strlen(bname);
    if (blen > max_clen) {
	sclose(s);
	return_error(e_limitcheck);
    }
    memcpy(cname, bname, blen);
    *pclen = blen;
    make_stream_file(pfile, s, "r");
    return 0;
}

/* Open a file stream that reads a string. */
/* (This is currently used only by the ccinit feature.) */
/* The string must be allocated in non-garbage-collectable (foreign) space. */
int
file_read_string(const byte *str, uint len, ref *pfile, gs_ref_memory_t *imem)
{
    stream *s = file_alloc_stream((gs_memory_t *)imem, "file_read_string");

    if (s == 0)
	return_error(e_VMerror);
    sread_string(s, str, len);
    s->foreign = 1;
    s->write_id = 0;
    make_file(pfile, a_readonly | imemory_space(imem), s->read_id, s);
    s->save_close = s->procs.close;
    s->procs.close = file_close_disable;
    return 0;
}

/* Open a file stream, optionally on an OS file. */
/* Return 0 if successful, error code if not. */
/* On a successful return, the C file name is in the stream buffer. */
/* If fname==0, set up the file entry, stream, and buffer, */
/* but don't open an OS file or initialize the stream. */
int
file_open_stream(const char *fname, uint len, const char *file_access,
		 uint buffer_size, stream ** ps, iodev_proc_fopen_t fopen_proc,
		 gs_memory_t *mem)
{
    byte *buffer;
    register stream *s;

    if (buffer_size == 0)
	buffer_size = file_default_buffer_size;
    if (len >= buffer_size)    /* we copy the file name into the buffer */
	return_error(e_limitcheck);
    /* Allocate the stream first, since it persists */
    /* even after the file has been closed. */
    s = file_alloc_stream(mem, "file_open_stream");
    if (s == 0)
	return_error(e_VMerror);
    /* Allocate the buffer. */
    buffer = gs_alloc_bytes(mem, buffer_size, "file_open_stream(buffer)");
    if (buffer == 0)
	return_error(e_VMerror);
    if (fname != 0) {
	/* Copy the name (so we can terminate it with a zero byte.) */
	char *file_name = (char *)buffer;
	char fmode[4];		/* r/w/a, [+], [b], null */
	FILE *file;
	int code;

	memcpy(file_name, fname, len);
	file_name[len] = 0;	/* terminate string */
	/* Open the file, always in binary mode. */
	strcpy(fmode, file_access);
	strcat(fmode, gp_fmode_binary_suffix);
	/****** iodev_default IS QUESTIONABLE ******/
	code = (*fopen_proc)(iodev_default, file_name, fmode, &file,
			     (char *)buffer, buffer_size);
	if (code < 0) {
	    gs_free_object(mem, buffer, "file_open_stream(buffer)");
	    return code;
	}
	/* Set up the stream. */
	switch (fmode[0]) {
	    case 'a':
		sappend_file(s, file, buffer, buffer_size);
		break;
	    case 'r':
		/* Defeat buffering for terminals. */
		{
		    struct stat rstat;

		    fstat(fileno(file), &rstat);
		    sread_file(s, file, buffer,
			       (S_ISCHR(rstat.st_mode) ? 1 : buffer_size));
		}
		break;
	    case 'w':
		swrite_file(s, file, buffer, buffer_size);
	}
	if (fmode[1] == '+')
	    s->file_modes |= s_mode_read | s_mode_write;
	s->save_close = s->procs.close;
	s->procs.close = file_close_file;
    } else {			/* save the buffer and size */
	s->cbuf = buffer;
	s->bsize = s->cbsize = buffer_size;
    }
    *ps = s;
    return 0;
}

/* Report an error by storing it in the stream's error_string. */
int
filter_report_error(stream_state * st, const char *str)
{
    if_debug1('s', "[s]stream error: %s\n", str);
    strncpy(st->error_string, str, STREAM_MAX_ERROR_STRING);
    /* Ensure null termination. */
    st->error_string[STREAM_MAX_ERROR_STRING] = 0;
    return 0;
}

/* Open a file stream for a filter. */
int
filter_open(const char *file_access, uint buffer_size, ref * pfile,
	    const stream_procs * procs, const stream_template * template,
	    const stream_state * st, gs_memory_t *mem)
{
    stream *s;
    uint ssize = gs_struct_type_size(template->stype);
    stream_state *sst = 0;
    int code;

    if (template->stype != &st_stream_state) {
	sst = s_alloc_state(mem, template->stype, "filter_open(stream_state)");
	if (sst == 0)
	    return_error(e_VMerror);
    }
    code = file_open_stream((char *)0, 0, file_access,
			    buffer_size, &s, (iodev_proc_fopen_t)0, mem);
    if (code < 0) {
	gs_free_object(mem, sst, "filter_open(stream_state)");
	return code;
    }
    s_std_init(s, s->cbuf, s->bsize, procs,
	       (*file_access == 'r' ? s_mode_read : s_mode_write));
    s->procs.process = template->process;
    s->save_close = s->procs.close;
    s->procs.close = file_close_file;
    if (sst == 0) {
	/* This stream doesn't have any state of its own. */
	/* Hack: use the stream itself as the state. */
	sst = (stream_state *) s;
    } else if (st != 0)		/* might not have client parameters */
	memcpy(sst, st, ssize);
    s->state = sst;
    sst->template = template;
    sst->memory = mem;
    sst->report_error = filter_report_error;
    if (template->init != 0) {
	code = (*template->init)(sst);
	if (code < 0) {
	    gs_free_object(mem, sst, "filter_open(stream_state)");
	    gs_free_object(mem, s->cbuf, "filter_open(buffer)");
	    return code;
	}
    }
    make_stream_file(pfile, s, file_access);
    return 0;
}

/* Allocate and return a file stream. */
/* Return 0 if the allocation failed. */
/* The stream is initialized to an invalid state, so the caller need not */
/* worry about cleaning up if a later step in opening the stream fails. */
stream *
file_alloc_stream(gs_memory_t * mem, client_name_t cname)
{
    stream *s;
    gs_ref_memory_t *imem = 0;

    /*
     * HACK: Figure out whether this is a gs_ref_memory_t.
     * Avoiding this hack would require rippling a change
     * from gs_memory_t to gs_ref_memory_t into the open_file and
     * open_device procedures of gx_io_device, which in turn would
     * impact other things we don't want to change.
     */
    if (mem->procs.free_object == gs_ref_memory_procs.free_object)
	imem = (gs_ref_memory_t *) mem;

    if (imem) {
	/* Look first for a free stream allocated at this level. */
	s = imem->streams;
	while (s != 0) {
	    if (!s_is_valid(s) && s->read_id != 0 /* i.e. !overflowed */ ) {
		s->is_temp = 0;	/* not a temp stream */
		return s;
	    }
	    s = s->next;
	}
    }
    s = s_alloc(mem, cname);
    if (s == 0)
	return 0;
    s_init_ids(s);
    s->is_temp = 0;		/* not a temp stream */
    /*
     * Disable the stream now (in case we can't open the file,
     * or a filter init procedure fails) so that `restore' won't
     * crash when it tries to close open files.
     */
    s_disable(s);
    if (imem) {
	/* Add s to the list of files. */
	if (imem->streams != 0)
	    imem->streams->prev = s;
	s->next = imem->streams;
	imem->streams = s;
    } else {
	s->next = 0;
    }
    s->prev = 0;
    return s;
}

/* ------ Stream closing ------ */

/*
 * Finish closing a file stream.  This used to check whether it was
 * currentfile, but we don't have to do this any longer.  This replaces the
 * close procedure for the std* streams, which cannot actually be closed.
 *
 * This is exported for ziodev.c.  */
int
file_close_finish(stream * s)
{
    return 0;
}

/*
 * Close a file stream, but don't deallocate the buffer.  This replaces the
 * close procedure for %lineedit and %statementedit.  (This is WRONG: these
 * streams should allocate a new buffer each time they are opened, but that
 * would overstress the allocator right now.)  This is exported for ziodev.c.
 * This also replaces the close procedure for the string-reading stream
 * created for gs_run_string.
 */
int
file_close_disable(stream * s)
{
    int code = (*s->save_close)(s);

    if (code)
	return code;
    /* Increment the IDs to prevent further access. */
    s->read_id = s->write_id = (s->read_id | s->write_id) + 1;
    return file_close_finish(s);
}

/* Close a file stream.  This replaces the close procedure in the stream */
/* for normal (OS) files and for filters. */
int
file_close_file(stream * s)
{
    stream *stemp = s->strm;
    gs_memory_t *mem;
    int code = file_close_disable(s);

    if (code)
	return code;
    /*
     * Check for temporary streams created for filters.
     * There may be more than one in the case of a procedure-based filter,
     * or if we created an intermediate stream to ensure
     * a large enough buffer.  Note that these streams may have been
     * allocated by file_alloc_stream, so we mustn't free them.
     */
    while (stemp != 0 && stemp->is_temp != 0) {
	stream *snext = stemp->strm;

	mem = stemp->memory;
	if (stemp->is_temp > 1)
	    gs_free_object(mem, stemp->cbuf,
			   "file_close(temp stream buffer)");
	s_disable(stemp);
	stemp = snext;
    }
    mem = s->memory;
    gs_free_object(mem, s->cbuf, "file_close(buffer)");
    if (s->close_strm && stemp != 0)
	return sclose(stemp);
    return 0;
}

/* Close a file object. */
/* This is exported only for gsmain.c. */
int
file_close(ref * pfile)
{
    stream *s;

    if (file_is_valid(s, pfile)) {	/* closing a closed file is a no-op */
	if (sclose(s))
	    return_error(e_ioerror);
    }
    return 0;
}

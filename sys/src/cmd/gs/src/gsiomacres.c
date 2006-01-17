/* Copyright (C) 2002 artofcode LLC  All rights reserved.

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

/* $Id: gsiomacres.c,v 1.5 2003/09/03 03:22:59 giles Exp $ */
/* %macresource% IODevice implementation for loading MacOS fonts */

/*
 * MacOS often stores fonts in 'resource forks' an extended attribute
 * of the HFS filesystem that allows storing chunks of data indexed by
 * a resource 'type' and 'id'. MacOS X introduced the '.dfont' format
 * for compatibility with non-HFS filesystems; it consists of the
 * resource data for a font stored on the data fork (normal file).
 *
 * We provide here a special %macresource% IODevice so that Ghostscript
 * can open these native fonts on MacOS. It opens the requested file and
 * returns a stream containing the indicated resource data, whether from
 * the resource fork or, if that fails, from the datafork assuming a
 * .dfont file.
 *
 * Since the only use at this point is font loading, we don't implement
 * the usual complement of delete, rename and so on.
 */

#include "std.h"
#include "stdio_.h"
#include "string_.h"
#include "malloc_.h"
#include "ierrors.h"
#include "gserror.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "stream.h"
#include "gdebug.h"
#include "gxiodev.h"
#include "gp.h"

/* dfont loading code */

/* 4-byte resource types as uint32s */
#define TYPE_sfnt 0x73666e74
#define TYPE_FOND 0x464f4e44
#define TYPE_NFNT 0x4e464e54
#define TYPE_ftag 0x66746167
#define TYPE_vers 0x76657273

typedef struct {
	unsigned int data_offset;
	unsigned int map_offset;
	unsigned int data_length;
	unsigned int map_length;
} resource_header;

typedef struct {
	unsigned int type;
	unsigned int offset;
	unsigned int length;
	byte *data;
	char *name;
	unsigned short id;
	byte flags;
} resource;

typedef struct {
    resource *resources;
    int n_resources;
} resource_list;

private
int get_int32(byte *p) {
    return (p[0]&0xFF)<<24 | (p[1]&0xFF)<<16 | (p[2]&0xFF)<<8 | (p[3]&0xFF);
}

private
int get_int24(byte *p) {
    return (p[0]&0xFF)<<16 | (p[1]&0xFF)<<8 | (p[2]&0xFF);
}

private
int get_int16(byte *p) {
    return (p[0]&0xFF)<<8 | (p[1]&0xFF);
}

private
int read_int32(FILE *in, void *p)
{
	byte w[4], err;

	err = fread(w, 1, 4, in);
	if (err != 4) return -1;

	*(unsigned int*)p = get_int32(w);
	return 0;
}

private
int read_int24(FILE *in, void *p)
{
	byte w[3], err;
	
	err = fread(w, 1, 3, in);
	if (err != 3) return -1;
	
	*(unsigned int*)p = get_int24(w);
	return 0;
}

private
int read_int16(FILE *in, void *p)
{
	byte w[2], err;
	
	err = fread(w, 1, 2, in);
	if (err != 2) return -1;
	
	*(unsigned short*)p = get_int16(w);
	return 0;
}

private
int read_int8(FILE *in, void *p)
{
	byte c = fgetc(in);
	if (c < 0) return -1;
	
    *(byte*)p = (c&0xFF);
    return 0;
}

/* convert a 4-character typecode from C string to uint32 representation */
private unsigned int res_string2type(const char *type_string)
{
	unsigned int type = type_string[0] << 24 |
           				type_string[1] << 16 |
          				type_string[2] <<  8 |
           				type_string[3];
    return (type);
}
/* convert a 4-character typecode from unsigned int to C string representation */
private char * res_type2string(const unsigned int type, char *type_string)
{
	if (type_string == NULL) return NULL;
	
	type_string[0] = (type >> 24) & 0xFF;
    type_string[1] = (type >> 16) & 0xFF;
    type_string[2] = (type >> 8) & 0xFF;
    type_string[3] = (type) & 0xFF;
    type_string[4] = '\0';    

    return (type_string);
}

private
resource_header *read_resource_header(FILE *in, int offset)
{
	resource_header *header = malloc(sizeof(*header));

	fseek(in, offset, SEEK_SET);

	read_int32(in, &(header->data_offset));
	read_int32(in, &(header->map_offset));
	read_int32(in, &(header->data_length));
	read_int32(in, &(header->map_length));

        if (feof(in)) {
            free (header);
            header = NULL;
        }
        
	return header;
}

private
resource_list *read_resource_map(FILE *in, resource_header *header)
{
    resource_list *list;
    int n_resources;
	int type_offset, name_offset, this_name_offset;
    unsigned int *types;
    int *number, *ref_offsets;
    int n_types;
    char type_string[5];
    byte *buf, *p;
    int i,j,k;

    buf = malloc(sizeof(*buf)*header->map_length);
    if (buf == NULL) {
        if_debug1('s', "error: could not allocate %d bytes for resource map\n", header->map_length);
        return NULL;
    }
        
    /* read in the whole resource map */
	fseek(in, header->map_offset, SEEK_SET);
    fread(buf, 1, header->map_length, in);
    
    type_offset = get_int16(buf + 24);
    name_offset = get_int16(buf + 26);
    n_types = get_int16(buf + 28); n_types++;
    
    if (type_offset != 30)
        if_debug1('s', "[s] warning! resource type list offset is %d, not 30!\n", type_offset);
            
    /* determine the total number of resources */
    types = malloc(sizeof(*types)*n_types);
    number = malloc(sizeof(*number)*n_types);
    ref_offsets = malloc(sizeof(*ref_offsets)*n_types);
    n_resources = 0;
    p = buf + type_offset;	/* this seems to be off by two in files!? */
    p = buf + 30;
    for (i = 0; i < n_types; i++) {
        types[i] = get_int32(p);
        number[i] = get_int16(p + 4) + 1;
        ref_offsets[i] = get_int16(p + 6);
        p += 8;
        n_resources += number[i];
    }
    
    /* parse the individual resources */
    list = malloc(sizeof(resource_list));
    list->resources = malloc(sizeof(resource)*n_resources);
    list->n_resources = n_resources;
    k = 0;
    for (i = 0; i < n_types; i++) {
    	res_type2string(types[i], type_string);
    	if_debug2('s', "[s] %d resources of type '%s':\n", number[i], type_string);
        p = buf + type_offset + ref_offsets[i]; /* FIXME: also off? */
        /* p = buf + 32 + ref_offsets[i]; */
        for (j = 0; j < number[i]; j++) {
            list->resources[k].type = types[i];
            list->resources[k].id = get_int16(p);
            this_name_offset = get_int16(p + 2);
            if (this_name_offset == 0xFFFF) { /* no name field */
                list->resources[k].name = NULL;
            } else { /* read name field (a pascal string) */
                char *c = buf + name_offset + this_name_offset;
                int len = *c;
                list->resources[k].name = malloc(sizeof(char)*(len+1));
                memcpy(list->resources[k].name, c + 1, len);
                list->resources[k].name[len] = '\0';
            }
            list->resources[k].flags = *(p+4);
            list->resources[k].offset = get_int24(p+5);
            /* load the actual resource data separately */
            list->resources[k].length = 0;
            list->resources[k].data = NULL;
            
            p += 12;
            if_debug4('s', "\tid %d offset 0x%08x flags 0x%02x '%s'\n",
                list->resources[k].id, list->resources[k].offset,
                list->resources[k].flags, list->resources[k].name);
            k++;
        }
    }        
    
    free(buf);
        
	return list;
}

private
void load_resource(FILE *in, resource_header *header, resource *res)
{
    unsigned int len;
    byte *buf;
    
    fseek(in, header->data_offset + res->offset, SEEK_SET);
    read_int32(in, &len);
    
    buf = malloc(len);
    fread(buf, 1, len, in);
    res->data = buf;
    res->length = len;
    
    return;
}

private
int read_datafork_resource(byte *buf, const char *fname, const uint type, const ushort id)
{
    resource_header *header;
    resource_list *list;
    FILE *in;
    int i;


    in = fopen(fname, "rb");
    if (in == NULL) {
        if_debug1('s', "[s] couldn't open '%s'\n", fname);
        return 0;
    }

    header = read_resource_header(in, 0);
    if (header == NULL) {
        if_debug0('s', "[s] could not read resource file header.\n");
        if_debug0('s', "[s] not a serialized data fork resource file?\n");
        return 0;
    }
    
    if_debug0('s', "[s] loading resource map\n");
	list = read_resource_map(in, header);
    if (list == NULL) {
        if_debug0('s', "[s] couldn't read resource map.\n");
        return 0;
    }
    
    /* load the resource data we're interested in */
    for (i = 0; i < list->n_resources; i++) {
        if ((list->resources[i].type == type) && 
        	(list->resources[i].id == id)) {
        	if_debug2('s', "[s] loading '%s' resource id %d",
                list->resources[i].name, list->resources[i].id);
            load_resource(in, header, &(list->resources[i]));
            if_debug1('s', " (%d bytes)\n", list->resources[i].length);
            fclose(in);
            if (buf) memcpy (buf, list->resources[i].data, list->resources[i].length);
            return (list->resources[i].length);
        }
    }
    
    fclose(in);
    free(list);
    free(header);
    
    return (0);
}

/* end dfont loading code */


/* prototypes */
private iodev_proc_init(iodev_macresource_init);
private iodev_proc_open_file(iodev_macresource_open_file);
/* there is no close_file()...stream closure takes care of it */
/* ignore rest for now */

/* Define the %macresource% device */
const gx_io_device gs_iodev_macresource =
{
    "%macresource%", "FileSystem", 
    {
     iodev_macresource_init, iodev_no_open_device,
     iodev_macresource_open_file,
     iodev_no_fopen, iodev_no_fclose,
     iodev_no_delete_file, iodev_no_rename_file,
     iodev_no_file_status,
     iodev_no_enumerate_files, NULL, NULL,
     iodev_no_get_params, iodev_no_put_params
    }
};

/* init state. don't know if we need state yet--probably not */
private int
iodev_macresource_init(gx_io_device *iodev, gs_memory_t *mem)
{
    return 0;
}

/* open the requested file return (in ps) a stream containing the resource data */
private int
iodev_macresource_open_file(gx_io_device *iodev, const char *fname, uint namelen,
    const char *access, stream **ps, gs_memory_t *mem)
{
    char filename[gp_file_name_sizeof];
    char *res_type_string, *res_id_string;
    uint type;
    ushort id;
    bool datafork = 0;
    int size;
    byte *buf;
    
    /* return NULL if there's an error */
    *ps = NULL;
    
    strncpy(filename, fname, min(namelen, gp_file_name_sizeof));
    if (namelen < gp_file_name_sizeof) filename[namelen] = '\0';
    /* parse out the resource type and id. they're appended to the pathname
       in the form '#<type>+<id>' */
    res_type_string = strrchr(filename, '#');
    if (res_type_string == NULL) {
        if_debug0('s', "[s] couldn't find resource type separator\n");
        return_error(e_invalidfileaccess);
    }
    *res_type_string++ = '\0';
    res_id_string = strrchr(res_type_string, '+');
    if (res_id_string == NULL) {
        if_debug0('s', "couldn't find resource id separator\n");
        return_error(e_invalidfileaccess);
    }
    *res_id_string++ = '\0';
    type = res_string2type(res_type_string);
    id = (ushort)atoi(res_id_string);
    if_debug3('s', "[s] opening resource fork of '%s' for type '%s' id '%d'\n", filename, res_type_string, id);
    
    /* we call with a NULL buffer to get the size */
    size = gp_read_macresource(NULL, filename, type, id);
    if (size == 0) {
        /* this means that opening the resource fork failed */
        /* try to open as a .dfont from here */
        if_debug0('s', "[s] trying to open as a datafork file instead...\n");
        size = read_datafork_resource(NULL, filename, type, id);
    	if (size != 0) {
            datafork = true;
    	} else {
            if_debug0('s', "could not get resource size\n");
    	    return_error(e_invalidfileaccess);
    	}
    }
    if_debug1('s', "[s] got resource size %d bytes\n", size);
    /* allocate a buffer */    
    buf = gs_alloc_string(mem, size, "macresource buffer");
    if (buf == NULL) {
        if_debug0('s', "macresource: could not allocate buffer for resource data\n");
        return_error(e_VMerror);
    }
    /* call again to get the resource data */
    if (!datafork) {
        size = gp_read_macresource(buf, filename, type, id);
    } else {
        size = read_datafork_resource(buf, filename, type, id);
    }
	 
    /* allocate stream *ps and set it up with the buffered data */
    *ps = s_alloc(mem, "macresource");
    sread_string(*ps, buf, size);

    /* return success */
    return 0;
}

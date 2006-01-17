/* Copyright (C) 2005 artofcode LLC.  All rights reserved.

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

/* $Id: mkromfs.c,v 1.2 2005/09/16 03:59:44 ray Exp $ */
/* Generate source data for the %rom% IODevice */
 
/*
 * For reasons of convenience and footprint reduction, the various postscript
 * source files, resources and fonts required by Ghostscript can be compiled
 * into the executable.
 *
 * This file takes a set of directories, and creates a compressed filesystem
 * image that can be compiled into the executable as static data and accessed
 * through the %rom% iodevice prefix
 */

#include "stdpre.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#define ROMFS_BLOCKSIZE 4096
#define ROMFS_CBUFSIZE ((int)((ROMFS_BLOCKSIZE) * 1.001) + 12)

typedef struct romfs_inode_s {
    char *name;
    struct romfs_inode_s *next, *child;
    unsigned int blocks;
    unsigned long length;
    unsigned long offset;
    unsigned long data_size;
    unsigned char **data;
    unsigned int *data_lengths;
} romfs_inode;

static int put_int32(unsigned char *p, const unsigned int q)
{
    *p++ = (q >> 24) & 0xFF;
    *p++ = (q >> 16) & 0xFF;
    *p++ = (q >>  8) & 0xFF;
    *p++ = (q >>  0) & 0xFF;
    
    return 4;
}

/* clear the internal memory of an inode */
void inode_clear(romfs_inode* node)
{
    int i;
    
    if (node) {
        if (node->data) {
            for (i = 0; i < node->blocks; i++) {
                if (node->data[i]) free(node->data[i]);
            }
            free(node->data);
        }
        if (node->data_lengths) free(node->data_lengths);
        if (node->name) free(node->name);
    }
}

/* write out and inode and its file data */
int
inode_write(FILE *out, romfs_inode *node)
{
    int i, offset = 0;
    unsigned char buf[64];
    unsigned char *p = buf;
    
    /* 4 byte offset to next inode */
    p += put_int32(p, node->offset);
    /* 4 byte file length */
    p += put_int32(p, node->length);
    /* 4 byte path length */
    p += put_int32(p, strlen(node->name));
    
    printf("writing node '%s'...\n", node->name);
    printf(" offset %ld\n", node->offset);
    printf(" length %ld\n", node->length);
    printf(" path length %ld\n", strlen(node->name));
    
    printf(" %d compressed blocks comprising %ld bytes\n", node->blocks, node->data_size);
    
    /* write header */
    offset += fwrite(buf, 3, 4, out);
    /* write path */
    offset += fwrite(node->name, 1, strlen(node->name), out);
    /* write block sizes */
    offset += fwrite(node->data_lengths, node->blocks, sizeof(*node->data_lengths), out);
    /* write out compressed data */
    for (i = 0; i < node->blocks; i++)
        offset += fwrite(node->data[i], 1, node->data_lengths[i], out);
    
    printf(" wrote %d bytes in all\n", offset);
    return offset;
}


int
main(int argc, char *argv[])
{
    int i, ret, block;
    romfs_inode *node;
    unsigned char *ubuf, *cbuf;
    unsigned long ulen, clen;
    unsigned long offset = 0;
    FILE *in, *out;

    ubuf = malloc(ROMFS_BLOCKSIZE);
    cbuf = malloc(ROMFS_CBUFSIZE);
    
    printf("compressing with %d byte blocksize (zlib output buffer %d bytes)\n",
        ROMFS_BLOCKSIZE, ROMFS_CBUFSIZE);
    
    out = fopen("gsromfs", "wb");
    
    /* for each path on the commandline, attach an inode */
    for (i = 1; i < argc; i++) {  
        node = calloc(1, sizeof(romfs_inode));
        /* get info for this file */
        node->name = strdup(argv[i]);
        in = fopen(node->name, "rb");
        fseek(in, 0, SEEK_END);
        node->length = ftell(in);
        node->blocks = (node->length - 1) / ROMFS_BLOCKSIZE + 1;
        node->data_lengths = calloc(node->blocks, sizeof(unsigned int));
        node->data = calloc(node->blocks, sizeof(unsigned char *));
        /* compress data here */
        fclose(in);
        in = fopen(node->name, "rb");
        block = 0;
        while (!feof(in)) {
            ulen = fread(ubuf, 1, ROMFS_BLOCKSIZE, in);
            if (!ulen) break;
            clen = ROMFS_CBUFSIZE;
            ret = compress(cbuf, &clen, ubuf, ulen);
            if (ret != Z_OK) {
                printf("error compressing data block!\n");
            }
            node->data_lengths[block] = clen; 
            node->data[block] = malloc(clen);
            memcpy(node->data[block], cbuf, clen);
            block++;
            node->data_size += clen;
        }
        fclose(in);
        node->offset = 12 + 4 * node->blocks + node->data_size + strlen(node->name);
        printf("inode %d (%ld/%ld bytes) '%s'\t%ld%%\n",
            i, node->data_size, node->length, node->name, 100*node->data_size/node->length);
        /* write out data for this file */
        inode_write(out, node);
        /* clean up */
        inode_clear(node);
        free(node);
    }
    
    free(ubuf);
    
    fclose(out);
    
    return 0;
}



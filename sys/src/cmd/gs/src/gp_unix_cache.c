/* Copyright (C) 2003-2004 artofcode LLC. All rights reserved.
  
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

/* $Id: gp_unix_cache.c,v 1.3 2004/07/14 15:34:25 giles Exp $ */
/* Generic POSIX persistent-cache implementation for Ghostscript */

#include "stdio_.h"
#include "string_.h"
#include "time_.h"
#include <stdlib.h> /* should use gs_malloc() instead */
#include "gconfigd.h"
#include "gp.h"
#include "md5.h"

/* ------ Persistent data cache types ------*/

#define GP_CACHE_VERSION 0

/* cache entry type */
typedef struct gp_cache_entry_s {
    int type;
    int keylen;
    byte *key;
    md5_byte_t hash[16];
    char *filename;
    int len;
    void *buffer;
    int dirty;
    time_t last_used;
} gp_cache_entry;

/* initialize a new gp_cache_entry struct */
private void gp_cache_clear_entry(gp_cache_entry *item)
{
    item->type = -1;
    item->key = NULL;
    item->keylen = 0;
    item->filename = NULL;
    item->buffer = NULL;
    item->len = 0;
    item->dirty = 0;
    item->last_used = 0;
}

/* get the cache directory's path */
private char *gp_cache_prefix(void)
{
    char *prefix = NULL;
    int plen = 0;
    
    /* get the cache directory path */
    if (gp_getenv("GS_CACHE_DIR", (char *)NULL, &plen) < 0) {
        prefix = malloc(plen);
        gp_getenv("GS_CACHE_DIR", prefix, &plen);
        plen--;
    } else {
#ifdef GS_CACHE_DIR
        prefix = strdup(GS_CACHE_DIR);
#else
        prefix = strdup(".cache");
#endif
        plen = strlen(prefix);
    }
    
    /* substitute $HOME for '~' */
    if (plen > 1 && prefix[0] == '~') {
        char *home, *path;
        int hlen = 0;
	unsigned int pathlen = 0;
        gp_file_name_combine_result result;
        
        if (gp_getenv("HOME", (char *)NULL, &hlen) < 0) {
            home = malloc(hlen);
            if (home == NULL) return prefix;
            gp_getenv("HOME", home, &hlen);
            hlen--;
            if (plen == 1) {
                /* only "~" */
                free(prefix);
                return home;
            }
            /* substitue for the initial '~' */
            pathlen = hlen + plen + 1;
            path = malloc(pathlen);
            if (path == NULL) { free(home); return prefix; }
            result = gp_file_name_combine(home, hlen, prefix+2, plen-2, false, path, &pathlen);
            if (result == gp_combine_success) {
                free(prefix);
                prefix = path;
            } else {
                dlprintf1("file_name_combine failed with code %d\n", result);
            }
            free(home);
        }
    }
#ifdef DEBUG_CACHE    
    dlprintf1("cache dir read as '%s'\n", prefix);
#endif
    return prefix;
}

/* compute the cache index file's path */
private char *
gp_cache_indexfilename(const char *prefix)
{
    const char *fn = "gs_cache";
    char *path;
    unsigned int len;
    gp_file_name_combine_result result;
    
    len = strlen(prefix) + strlen(fn) + 2;
    path = malloc(len);
    
    result = gp_file_name_combine(prefix, strlen(prefix), fn, strlen(fn), true, path, &len);
    if (result == gp_combine_small_buffer) {
        /* handle the case when the combination requires more than one extra character */
        free(path);
        path = malloc(++len);
        result = gp_file_name_combine(prefix, strlen(prefix), fn, strlen(fn), true, path, &len);
    }
    if (result != gp_combine_success) {
        dlprintf1("pcache: file_name_combine for indexfilename failed with code %d\n", result);
        free(path);
        return NULL;
    }
    return path;
}

/* compute and set a cache key's hash */
private void gp_cache_hash(gp_cache_entry *entry)
{
    md5_state_t md5;
    
    /* we use md5 hashes of the key */
    md5_init(&md5);
    md5_append(&md5, entry->key, entry->keylen);
    md5_finish(&md5, entry->hash);
}

/* compute and set cache item's filename */
private void gp_cache_filename(const char *prefix, gp_cache_entry *item)
{
    const char hexmap[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    char *fn = malloc(gp_file_name_sizeof), *fni;
    int i;
       
    fni = fn;
    *fni++ = hexmap[item->type>>4 & 0x0F];
    *fni++ = hexmap[item->type & 0x0F];
    *fni++ = '.';
    for (i = 0; i < 16; i++) {
        *fni++ = hexmap[(item->hash[i]>>4 & 0x0F)];
        *fni++ = hexmap[(item->hash[i] & 0x0F)];
    }
    *fni = '\0';
    
    if (item->filename) free(item->filename);
    item->filename = fn;
}

/* generate an access path for a cache item */
private char *gp_cache_itempath(const char *prefix, gp_cache_entry *item)
{
    const char *fn = item->filename;
    gp_file_name_combine_result result;
    char *path;
    unsigned int len;
    
    len = strlen(prefix) + strlen(fn) + 2;
    path = malloc(len);
    result = gp_file_name_combine(prefix, strlen(prefix), 
        fn, strlen(fn), false, path, &len);

    if (result != gp_combine_success) {
        dlprintf1("pcache: file_name_combine failed on cache item filename with code %d\n", result);
    }
    
    return path;
}

private int gp_cache_saveitem(FILE *file, gp_cache_entry* item)
{
    unsigned char version = 0;
    int ret;

#ifdef DEBUG_CACHE
    dlprintf2("pcache: saving key with version %d, data length %d\n", version, item->len);
#endif
    ret = fwrite(&version, 1, 1, file);
    ret = fwrite(&(item->keylen), 1, sizeof(item->keylen), file);
    ret = fwrite(item->key, 1, item->keylen, file);
    ret = fwrite(&(item->len), 1, sizeof(item->len), file);
    ret = fwrite(item->buffer, 1, item->len, file);
    item->dirty = 0;
    return ret;
}

private int gp_cache_loaditem(FILE *file, gp_cache_entry *item, gp_cache_alloc alloc, void *userdata)
{
    unsigned char version;
    unsigned char *filekey = NULL;
    int len, keylen;
    
    fread(&version, 1, 1, file);
    if (version != GP_CACHE_VERSION) {
#ifdef DEBUG_CACHE
        dlprintf2("pcache file version mismatch (%d vs expected %d)\n", version, GP_CACHE_VERSION);
#endif
        return -1;
    }
    fread(&keylen, 1, sizeof(keylen), file);
    if (keylen != item->keylen) {
#ifdef DEBUG_CACHE
        dlprintf2("pcache file has correct hash but wrong key length (%d vs %d)\n",
            keylen, item->keylen);
#endif
        return -1;
    }
    filekey = malloc(keylen);
    if (filekey != NULL)
        fread(filekey, 1, keylen, file);
    if (memcmp(filekey, item->key, keylen)) {
#ifdef DEBUG_CACHE
        dlprintf("pcache file has correct hash but doesn't match the full key\n");
#endif
        free(filekey);
        item->buffer = NULL;
        item->len = 0;
        return -1;
    }
    free(filekey);
    
    fread(&len, 1, sizeof(len), file);
#ifdef DEBUG_CACHE
    dlprintf2("key matches file with version %d, data length %d\n", version, len);
#endif
    item->buffer = alloc(userdata, len);
    if (item->buffer == NULL) {
        dlprintf("pcache: unable to allocate buffer for file data!\n");
        return -1;
    }
    
    item->len = fread(item->buffer, 1, len, file);
    item->dirty = 1;
    item->last_used = time(NULL);
    
    return 0;
}

/* convert a two-character hex string to an integer */
private int readhexbyte(const char *s)
{
    const char hexmap[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    int i,r;
    
    for (i = 0; i < 16; i++)
        if (hexmap[i] == *s) break;
    if (i == 16) return -1;
    r = i;
    s++;
    for (i = 0; i < 16; i++)
        if (hexmap[i] == *s) break;
    if (i == 16) return -1;
    r = r<<4 | i;
    return r;
}

private int
gp_cache_read_entry(FILE *file, gp_cache_entry *item)
{
    char line[256];
    char fn[32];
    int i;
    
    if (!fgets(line, 256, file)) return -1;
    
    /* skip comments */
    if (line[0] == '#') return 1;
    
    /* otherwise, parse the line */
    sscanf(line, "%s %ld\n", fn, &item->last_used);
    /* unpack the type from the filename */
    item->type = readhexbyte(fn);
    /* unpack the md5 hash from the filename */
    for (i = 0; i < 16; i++)
        item->hash[i] = readhexbyte(fn + 3 + 2*i);
    /* remember the filename */    
    if (item->filename) free(item->filename);
    item->filename = malloc(strlen(fn) + 1);
    memcpy(item->filename, fn, strlen(fn));
    /* null other fields */
    item->key = NULL;
    item->keylen = 0;
    item->len = 0;
    item->buffer = NULL;
        
    return 0;
}

private int
gp_cache_write_entry(FILE *file, gp_cache_entry *item)
{
    fprintf(file, "%s %ld\n", item->filename, item->last_used);
    return 0;
}


/* insert a buffer under a (type, key) pair */
int gp_cache_insert(int type, byte *key, int keylen, void *buffer, int buflen)
{
    char *prefix, *path;
    char *infn,*outfn;
    FILE *file, *in, *out;
    gp_cache_entry item, item2;
    int code, hit = 0;
    
    prefix = gp_cache_prefix();
    infn = gp_cache_indexfilename(prefix);
    {
        int len = strlen(infn) + 2;
        outfn = malloc(len);
        memcpy(outfn, infn, len - 2);
        outfn[len-2] = '+';
        outfn[len-1] = '\0';
    }
    
    in = fopen(infn, "r");
    if (in == NULL) {
        dlprintf1("pcache: unable to open '%s'\n", infn);
        return -1;
    }
    out = fopen(outfn, "w");
    if (out == NULL) {
        dlprintf1("pcache: unable to open '%s'\n", outfn);
        return -1;
    }
        
    fprintf(out, "# Ghostscript persistent cache index table\n");

    /* construct our cache item */
    gp_cache_clear_entry(&item);
    item.type = type;
    item.key = key;
    item.keylen = keylen;
    item.buffer = buffer;
    item.len = buflen;
    item.dirty = 1;
    item.last_used = time(NULL);
    gp_cache_hash(&item);
    gp_cache_filename(prefix, &item);
    
    /* save it to disk */
    path = gp_cache_itempath(prefix, &item);
    file = fopen(path, "wb");
    if (file != NULL) {
        gp_cache_saveitem(file, &item);
        fclose(file);
    }
    
    /* now loop through the index to update or insert the entry */
    gp_cache_clear_entry(&item2);
    while ((code = gp_cache_read_entry(in, &item2)) >= 0) {
        if (code == 1) continue;
        if (!memcmp(item.hash, item2.hash, 16)) {
            /* replace the matching line */
            gp_cache_write_entry(out, &item);
            hit = 1;
        } else {
            /* copy out the previous line */
            gp_cache_write_entry(out, &item2);
        }
    }
    if (!hit) {
        /* there was no matching line */
        gp_cache_write_entry(out, &item);
    }
    free(item.filename);
    fclose(out);
    fclose(in);
    
    /* replace the cache index with our new version */
    unlink(infn);
    rename(outfn,infn);
    
    free(prefix);
    free(infn);
    free(outfn);
    
    return 0;
}

int gp_cache_query(int type, byte* key, int keylen, void **buffer,
    gp_cache_alloc alloc, void *userdata)
{
    char *prefix, *path;
    char *infn,*outfn;
    FILE *file, *in, *out;
    gp_cache_entry item, item2;
    int code, hit = 0;
    
    prefix = gp_cache_prefix();
    infn = gp_cache_indexfilename(prefix);
    {
        int len = strlen(infn) + 2;
        outfn = malloc(len);
        memcpy(outfn, infn, len - 2);
        outfn[len-2] = '+';
        outfn[len-1] = '\0';
    }
    
    in = fopen(infn, "r");
    if (in == NULL) {
        dlprintf1("pcache: unable to open '%s'\n", infn);
        return -1;
    }
    out = fopen(outfn, "w");
    if (out == NULL) {
        dlprintf1("pcache: unable to open '%s'\n", outfn);
        return -1;
    }
        
    fprintf(out, "# Ghostscript persistent cache index table\n");

    /* construct our query */
    gp_cache_clear_entry(&item);
    item.type = type;
    item.key = key;
    item.keylen = keylen;
    item.last_used = time(NULL);
    gp_cache_hash(&item);
    gp_cache_filename(prefix, &item);
    
    /* look for it on the disk */
    path = gp_cache_itempath(prefix, &item);
    file = fopen(path, "rb");
    if (file != NULL) {
        hit = gp_cache_loaditem(file, &item, alloc, userdata);
        fclose(file);
    } else {
        hit = -1;
    }
    
    gp_cache_clear_entry(&item2);
    while ((code = gp_cache_read_entry(in, &item2)) >= 0) {
        if (code == 1) continue;
        if (!hit && !memcmp(item.hash, item2.hash, 16)) {
            /* replace the matching line */
            gp_cache_write_entry(out, &item);
            item.dirty = 0;
        } else {
            /* copy out the previous line */
            gp_cache_write_entry(out, &item2);
        }
    }
    if (item.dirty) {
        /* there was no matching line -- shouldn't happen */
        gp_cache_write_entry(out, &item);
    }
    free(item.filename);
    fclose(out);
    fclose(in);
    
    /* replace the cache index with our new version */
    unlink(infn);
    rename(outfn,infn);
    
    free(prefix);
    free(infn);
    free(outfn);
    
    if (!hit) {
        *buffer = item.buffer;
        return item.len;
    } else {
        *buffer = NULL;
        return -1;
    }
}

/* Copyright (C) 1995, 1996, 1997, 1998, 1999, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: fapiufst.c,v 1.24 2004/02/18 14:41:50 igor Exp $ */
/* Agfa UFST plugin */

/* GS includes : */
#include "stdio_.h"
#include "memory_.h"
#include "math_.h"
#include "ierrors.h"
#include "iplugin.h"
#include "ifapi.h"
/* UFST includes : */
#include "cgconfig.h"
#include "port.h"
#include "shareinc.h"
#include "t1isfnt.h"
#include "cgmacros.h"
#include "sfntenum.h"
#define DOES_ANYONE_USE_THIS_STRUCTURE /* see TTPCLEO.H, UFST 4.2 */
#include "ttpcleo.h"
#undef  DOES_ANYONE_USE_THIS_STRUCTURE
/* GS includes : */
#include "gxfapi.h"

GLOBAL const SW16 trace_sw = 0; /* UFST 4.3 wants it. */

GLOBAL UW16  PCLswapHdr( FSP LPUB8 p, UW16 gifct ); /* UFST header doesn't define it. */

typedef struct pcleo_glyph_list_elem_s pcleo_glyph_list_elem;
struct pcleo_glyph_list_elem_s {
    UW16 chId;
    pcleo_glyph_list_elem *next;
    /* more data follows here depending on font type */
};

typedef struct fco_list_elem_s fco_list_elem;
struct fco_list_elem_s {
    int open_count;
    SW16 fcHandle;
    char *file_path;
    fco_list_elem *next;
};

typedef struct {
    SL32 font_id;
    uint tt_font_body_offset;
    UW16 is_disk_font;
    UW16 font_type;
    UW16 platformId;
    UW16 specificId;
    pcleo_glyph_list_elem *glyphs;
    char decodingID[12];
} ufst_common_font_data;

typedef struct { 
    PCLETTO_CHR_HDR h;
    UW16   add_data;
    UW16   charDataSize;
    UW16   glyphID;
} PCLETTO_CHDR;

typedef struct fapi_ufst_server_s fapi_ufst_server;
struct fapi_ufst_server_s {
    FAPI_server If;
    int bInitialized;
    FAPI_font *ff;
    i_plugin_client_memory client_mem;
    IF_STATE IFS;
    FONTCONTEXT fc;
    void *char_data;
    bool bRaster;
    double tran_xx, tran_xy, tran_yx, tran_yy;
    fco_list_elem *fco_list;
    FAPI_retcode callback_error;
    FAPI_metrics_type metrics_type;
    FracInt sb_x, aw_x; /* replaced PS metrics. */
};

/* Type casts : */

private inline fapi_ufst_server *If_to_I(FAPI_server *If)
{   return (fapi_ufst_server *)If;
}

private inline fapi_ufst_server *IFS_to_I(IF_STATE *pIFS)
{   return (fapi_ufst_server *)((char *)pIFS - offset_of(fapi_ufst_server, IFS));
}

/*------------------ FAPI_server members ------------------------------------*/

private inline void release_char_data_inline(fapi_ufst_server *r)
{   /*  The server keeps character raster between calls to get_char_raster_metrics
        and get_char_raster, as well as between calls to get_char_outline_metrics
        and get_char_outline. Meanwhile this regular
        sequence of calls may be interrupted by an error in CDefProc or setchachedevice2,
        which may be invoked between them. In this case Ghostscript
        is unable to provide a signal to FAPI that the data are not
        longer needed. This would cause memory leaks in UFST heap.
        To work around this, appropriate server's entries check whether
        raster data were left after a previous call, and ultimately release them.
        This function provides it.
    */
    if (r->char_data != NULL) {
        CHARfree(&r->IFS, (MEM_HANDLE)r->char_data);
        r->char_data = 0;
    }
}

private FAPI_retcode open_UFST(fapi_ufst_server *r)
{   IFCONFIG   config_block;
    int code;

    if ((code = CGIFinit(&r->IFS)) != 0)
	return code;
    r->IFS.mem_avail[BUFFER_POOL]  = 6000000L; /* For Asian TT fonts with vertical writing mode. */
    config_block.num_files = 10;
    config_block.bit_map_width = 1;
    if ((code = CGIFconfig(&r->IFS, &config_block)) != 0)
	return code;
    if ((code = CGIFenter(&r->IFS)) != 0)
	return code;
    return 0;
}		      

private FAPI_retcode ensure_open(FAPI_server *server)
{   fapi_ufst_server *r = If_to_I(server);

    if (r->bInitialized)
        return 0;
    r->bInitialized = 1;
    return open_UFST(r);
}

private UW16 get_font_type(FILE *f)
{   char buf[20], mark_PS[]="%!";
    int i;

    if (fread(buf, 1, sizeof(buf), f) != sizeof(buf))
        return 0;
    if (buf[0] == 0x13 || buf[0] == 0x14) /* fixme : don't know how to do correctly. */
        return FC_FCO_TYPE;
    for (i = 0; i < sizeof(buf) - sizeof(mark_PS); i++)
        if(!memcmp(buf + i, mark_PS, sizeof(mark_PS) - 1))
            return FC_PST1_TYPE;
    if (buf[0] == '\0' && buf[1] == '\1')
        return FC_TT_TYPE;
    if (buf[0] == 't' && buf[1] == 't')
        return FC_TT_TYPE;
    return 0; /* fixme : unknown type - actually an error. */
}

    
private int choose_decoding_PS(fapi_ufst_server *r, ufst_common_font_data *d, const char *cmapId)
{ strncpy(d->decodingID, "Latin1", sizeof(d->decodingID));
  /*    fixme : must depend on charset used in the font.
        Particulartly Symbol fonts need a different decoding.
  */
  return 1;
}

private int choose_decoding_TT(fapi_ufst_server *r, ufst_common_font_data *d, const char *cmapId)
{   int platId, specId, i;
    CMAP_QUERY q;
    UW16 font_access;
    bool failed;
    void *p = (d->is_disk_font ? (void *)(d + 1) : (void *)((UB8 *)d + d->tt_font_body_offset));

    if (sscanf(cmapId, "%d.%d", &platId, &specId) != 2)
        return 0;
    font_access = r->IFS.font_access;
    r->IFS.font_access = (d->is_disk_font ? DISK_ACCESS : ROM_ACCESS);
    failed = CGIFtt_cmap_query(&r->IFS, p, r->fc.ttc_index, &q);
    r->IFS.font_access = font_access;
    if(failed)
        return 0;
    for (i = 0; i < q.numCmap; i++)
        if (q.entry[i].platId == platId && q.entry[i].specId == specId) {
            d->platformId = platId;
            d->specificId = specId;
            return 1;
        }
    return 0;
}

private void scan_xlatmap(fapi_ufst_server *r, ufst_common_font_data *d, const char *xlatmap, const char *font_kind, 
                                    int (*choose_proc)(fapi_ufst_server *r, ufst_common_font_data *d, const char *cmapId))
{   const char *p = xlatmap;

    while(*p) {
        int good_kind =!strcmp(p, font_kind);
        p += strlen(p) + 2;
        while(*p) {
            const char *cmapId = p, *decodingID = p + strlen(p) + 1;
            strncpy(d->decodingID, decodingID, sizeof(d->decodingID));
            if (!decodingID[0])
                break;
            p = decodingID + strlen(decodingID) + 1;
            if (good_kind)
                if (choose_proc(r, d, cmapId))
                    return;
        }
    }
    d->decodingID[0] = 0;
}

private void choose_decoding(fapi_ufst_server *r, ufst_common_font_data *d, const char *xlatmap)
{   if (xlatmap != 0)
        switch (d->font_type) {
            case FC_IF_TYPE: /* fixme */ break;
            case FC_PST1_TYPE: scan_xlatmap(r, d, xlatmap, "PostScript", choose_decoding_PS); break;
            case FC_TT_TYPE:   scan_xlatmap(r, d, xlatmap, "TrueType", choose_decoding_TT); break;
            case FC_FCO_TYPE:  scan_xlatmap(r, d, xlatmap, "PostScript", choose_decoding_PS/* fixme */); break;
        } 
}

private inline void store_word(byte **p, ushort w)
{   *((*p)++) = w / 256;
    *((*p)++) = w % 256;
    
}

private LPUB8 get_TT_glyph(fapi_ufst_server *r, FAPI_font *ff, UW16 chId)
{   pcleo_glyph_list_elem *g;
    PCLETTO_CHDR *h;
    ufst_common_font_data *d = (ufst_common_font_data *)r->fc.font_hdr - 1;
    LPUB8 q;
    ushort glyph_length = ff->get_glyph(ff, chId, 0, 0);
    bool use_XL_format = ff->is_mtx_skipped;

    /*
     * The client must set ff->is_mtx_skipped iff
     * it requests replaced lsb for True Type.
     * If it is set, replaced width to be supplied.
     * This constraing is derived from UFST restriction :
     * the font header format must be compatible with
     * glyph header format.
     */

    if (glyph_length == (ushort)-1) {
        r->callback_error = e_invalidfont;
        return 0;
    }
    g = (pcleo_glyph_list_elem *)r->client_mem.alloc(&r->client_mem, 
	    sizeof(pcleo_glyph_list_elem) + 
	    (use_XL_format ? 12 : sizeof(PCLETTO_CHDR)) + glyph_length + 2, 
	    "PCLETTO char");
    if (g == 0) {
        r->callback_error = e_VMerror;
        return 0;
    }
    g->chId = chId;
    g->next = d->glyphs;
    d->glyphs = g;
    h = (PCLETTO_CHDR *)(g + 1);
    h->h.format = 15;
    if (use_XL_format) {
	h->h.continuation = 2; 
	q = (LPUB8)h + 2;
	store_word(&q, (ushort)(glyph_length + 10));
	store_word(&q, (ushort)(r->sb_x >> r->If.frac_shift)); /* see can_replace_metrics */
	store_word(&q, (ushort)(r->aw_x >> r->If.frac_shift));
	store_word(&q, 0);
	store_word(&q, chId);
    } else {
	h->h.continuation = 0;
	h->h.descriptorsize = 4;
	h->h.class = 15;
	h->add_data = 0;
	q = (LPUB8)&h->charDataSize;
	store_word(&q, (ushort)(glyph_length + 4));
	store_word(&q, chId);
    }
    if (ff->get_glyph(ff, chId, (LPUB8)q, glyph_length) == (ushort)-1) {
        r->callback_error = e_invalidfont;
        return 0;
    }
    q += glyph_length;
    store_word(&q, 0); /* checksum */
    return (LPUB8)h;
    /*
     * The metrics replacement here is done only for the case
     * corresponding to non-disk TT fonts with MetricsCount != 0;
     * Other cases are not supported because UFST cannot handle them.
     * Here we don't take care of cases which can_replace_metrics rejects.
     *
     * We don't care of metrics for subglyphs, because
     * it is ignored by TT interpreter.
     */
}

private LPUB8 get_T1_glyph(fapi_ufst_server *r, FAPI_font *ff, UW16 chId)
{   ushort glyph_length = ff->get_glyph(ff, chId, 0, 0);
    LPUB8 q;
    pcleo_glyph_list_elem *g = (pcleo_glyph_list_elem *)r->client_mem.alloc(&r->client_mem, sizeof(pcleo_glyph_list_elem) + sizeof(PS_CHAR_HDR) + 2 + 2 + glyph_length + 1, "PSEO char");
    PS_CHAR_HDR *h;
    ufst_common_font_data *d = (ufst_common_font_data *)r->fc.font_hdr - 1;

    if (g == 0 || glyph_length == (ushort)-1) {
        r->callback_error = e_invalidfont;
        return 0;
    }
    g->chId = chId;
    g->next = d->glyphs;
    d->glyphs = g;
    h = (PS_CHAR_HDR *)(g + 1);
    h->format = 30;           /* raster=4, DJ=5, IF=10, TT=15, PS=30 */
    h->continuation = 0;     /* always 0 */
    h->descriptorsize = 2;   /* always 2 */
    h->class = 11;           /* contour=3, compound=4, tt=10, ps=11 */
    h->len = 0;              /* # of bytes to follow (not including csum) */
    q = (byte *)h + sizeof(*h);
    q[0] = 0; /* Namelen */
    q[1] = 0; /* Namelen */
    q[2] = (glyph_length) / 256; /* Datalen */
    q[3] = (glyph_length) % 256; /* Datalen */
    /* Glyph name goes here, but we don't use it. */
    q+=4;
    glyph_length = ff->get_glyph(ff, chId, q, glyph_length);
    q += glyph_length;
    *q = 1; /* Decrypt flag */
    return (LPUB8)h;
}

private pcleo_glyph_list_elem * find_glyph(ufst_common_font_data *d, UW16 chId)
{   pcleo_glyph_list_elem *e;

    for (e = d->glyphs; e != 0; e = e->next)
        if (e->chId == chId)
            return e;
    return 0;
}

private LPUB8 gs_PCLEO_charptr(LPUB8 pfont_hdr, UW16  sym_code)
{   /* never called */
    /*  We would like to do this :
        r->callback_error = e_unregistered;
        (see gs_PCLglyphID2Ptr)
        but we can't due to the reentrancy problem of UFST.
    */
    return 0;
}

private LPUB8 gs_PCLchId2ptr(IF_STATE *pIFS, UW16 chId)
{   fapi_ufst_server *r = IFS_to_I(pIFS);
    FAPI_font *ff = r->ff;
    ufst_common_font_data *d = (ufst_common_font_data *)r->fc.font_hdr - 1;
    pcleo_glyph_list_elem *g = find_glyph(d, chId);
    LPUB8 result = 0;

    if (g != 0)
        result = (LPUB8)(g + 1);
    if ((r->fc.format & FC_FONTTYPE_MASK) == FC_PST1_TYPE)
        result = get_T1_glyph(r, ff, chId);
    if ((r->fc.format & FC_FONTTYPE_MASK) == FC_TT_TYPE)
        result = get_TT_glyph(r, ff, chId);
    return result;
}

private LPUB8 gs_PCLglyphID2Ptr(IF_STATE *pIFS, UW16 glyphID)
{   return gs_PCLchId2ptr(pIFS, glyphID);
}

private inline void pack_word(LPUB8 *p, UW16 v)
{   LPUB8 q = (LPUB8)&v;

#if (BYTEORDER == LOHI) /* defied in UFST includes */
         (*p)[1] = q[0];
         (*p)[0] = q[1];
#else
        *(UW16 *)(*p) = v;
#endif
    *p += 2;
}

private inline void pack_long(LPUB8 *p, UL32 v)
{   LPUB8 q = (LPUB8)&v;

#if (BYTEORDER == LOHI) /* defied in UFST includes */
         (*p)[3] = q[0];
         (*p)[2] = q[1];
         (*p)[1] = q[2];
         (*p)[0] = q[3];
#else
        *(UL32 *)(*p) = v;
#endif
    *p += 4;
}

private inline void pack_float(LPUB8 *p, float v)
{   sprintf((char *)(*p), "%f", v);
    *p += strlen((const char *)*p) + 1;
}

#define PACK_ZERO(p) *(p++) = 0
#define PACK_BYTE(p, c) *(p++) = c
#define PACK_WORD(p, i, var) pack_word(&p, ff->get_word(ff, var, i))
#define PACK_LONG(p, i, var) pack_long(&p, ff->get_long(ff, var, i))

private void pack_pseo_word_array(fapi_ufst_server *r, FAPI_font *ff, UB8 **p, UW16 max_count, fapi_font_feature count_id, fapi_font_feature array_id)
{   UW16 k = min(ff->get_word(ff, count_id, 0), max_count), j;

    pack_word(p, k);
    for (j = 0; j < k; j++)
        PACK_WORD(*p, j, array_id);
    for (; j < max_count; j++)
        pack_word(p, 0);
}

private void pack_pseo_fhdr(fapi_ufst_server *r, FAPI_font *ff, UB8 *p)
{   ushort j, n, skip = 0;

    while ((UL32)p & 0x03) /* align to QUADWORD */
	PACK_ZERO(p);
    pack_long(&p, 1);  /* format = 1 */
    for (j = 0; j < 6; j++)
        pack_float(&p, ff->get_float(ff, FAPI_FONT_FEATURE_FontMatrix, j));
    while ((UL32)p & 0x03) /* align to QUADWORD */
	PACK_ZERO(p);
    /* UFST has no definition for PSEO structure, so implement serialization : */
    PACK_LONG(p, 0, FAPI_FONT_FEATURE_UniqueID);
    PACK_LONG(p, 0, FAPI_FONT_FEATURE_BlueScale);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_Weight);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_ItalicAngle);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_IsFixedPitch);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_UnderLinePosition);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_UnderlineThickness);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_FontType);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_FontBBox);
    PACK_WORD(p, 1, FAPI_FONT_FEATURE_FontBBox);
    PACK_WORD(p, 2, FAPI_FONT_FEATURE_FontBBox);
    PACK_WORD(p, 3, FAPI_FONT_FEATURE_FontBBox);
    pack_pseo_word_array(r, ff, &p, 14, FAPI_FONT_FEATURE_BlueValues_count, FAPI_FONT_FEATURE_BlueValues);
    pack_pseo_word_array(r, ff, &p, 10, FAPI_FONT_FEATURE_OtherBlues_count, FAPI_FONT_FEATURE_OtherBlues);
    pack_pseo_word_array(r, ff, &p, 14, FAPI_FONT_FEATURE_FamilyBlues_count, FAPI_FONT_FEATURE_FamilyBlues);
    pack_pseo_word_array(r, ff, &p, 10, FAPI_FONT_FEATURE_FamilyOtherBlues_count, FAPI_FONT_FEATURE_FamilyOtherBlues);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_BlueShift);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_BlueFuzz);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_StdHW);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_StdVW);
    pack_pseo_word_array(r, ff, &p, 12, FAPI_FONT_FEATURE_StemSnapH_count, FAPI_FONT_FEATURE_StemSnapH);
    pack_pseo_word_array(r, ff, &p, 12, FAPI_FONT_FEATURE_StemSnapV_count, FAPI_FONT_FEATURE_StemSnapV);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_ForceBold);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_LanguageGroup);
    PACK_WORD(p, 0, FAPI_FONT_FEATURE_lenIV);
    for (j = 0; j < 12; j++)
        PACK_ZERO(p), PACK_ZERO(p);     /* Reserved2 */
    /* max data size = 107 words + 6 floats in ASCII */
    n = ff->get_word(ff, FAPI_FONT_FEATURE_Subrs_count, 0);
    pack_word(&p, n);
    for (j = 0; j < n; j++) {
        ushort subr_len = ff->get_subr(ff, j, 0, 0);
        if (subr_len != 0) {
            pack_word(&p, j);
            pack_word(&p, subr_len);
            PACK_BYTE(p, 1); /* is_decrypted */
            ff->get_subr(ff, j, p, subr_len);
            p += subr_len;
        } else
            skip = 1;
    }
    if (skip)
        pack_word(&p, 0xFFFF);
}

private void enumerate_fco(fapi_ufst_server *r, const char *font_file_path)
{   /* development perpose only */
#if 0
        UW16 i;
        for (i = 0; ; i++) {
            UW16 size;
            TTFONTINFOTYPE *pBuffer;
            UW16 code = CGIFfco_Access(&r->IFS, (LPUB8)font_file_path, i, TFATRIB_KEY, &size, NULL);
            if (code)
                break;
            pBuffer = (TTFONTINFOTYPE *)malloc(size);
            if (pBuffer == 0)
                break;
            code = CGIFfco_Access(&r->IFS, (LPUB8)font_file_path, i, TFATRIB_KEY, &size, (SB8 *)pBuffer);
            if (code)
                break;
            {   char *tfName          = (char *)pBuffer + pBuffer->tfName;
                char *pcltTypeface    = (char *)pBuffer + pBuffer->pcltTypeface;
                char *pcltFileName    = (char *)pBuffer + pBuffer->pcltFileName;
                char *familyName      = (char *)pBuffer + pBuffer->familyName;
                char *weightName      = (char *)pBuffer + pBuffer->weightName;
                char *copyrightNotice = (char *)pBuffer + pBuffer->copyrightNotice;
                pBuffer += 0; /* a place for breakpoint */
            }
            free(pBuffer);
            (void)code;
        }
#endif
}

private char *my_strdup(fapi_ufst_server *r, const char *s, const char *cname)
{   int l = strlen(s) + 1;
    char *p = (char *)r->client_mem.alloc(&r->client_mem, l, cname);

    if (p != 0)
        memcpy(p, s, l);
    return p;
}

private FAPI_retcode fco_open(fapi_ufst_server *r, const char *font_file_path, fco_list_elem **result)
{   fco_list_elem *e = r->fco_list;
    int code;

    for (; e != 0; e = e->next) {
        if (!strcmp(e->file_path, font_file_path))
            break;
    }
    if (e == 0) {
        SW16 fcHandle;
        if ((code = CGIFfco_Open(&r->IFS, (UB8 *)font_file_path, &fcHandle)) != 0)
	    return code;
        e = (fco_list_elem *)r->client_mem.alloc(&r->client_mem, sizeof(*e), "fco_list_elem");
        if (e == 0) {
            CGIFfco_Close(&r->IFS, fcHandle);
            return e_VMerror;
        }
        e->open_count = 0;
        e->fcHandle = fcHandle;
        e->file_path = my_strdup(r, font_file_path, "fco_file_path");
        if (e->file_path == 0) {
            CGIFfco_Close(&r->IFS, fcHandle);
            r->client_mem.free(&r->client_mem, e, "fco_list_elem");
            return e_VMerror;
        }
        e->next = r->fco_list;
        r->fco_list = e;
    }
    e->open_count++;
    *result = e;
    return 0;
}

private FAPI_retcode make_font_data(fapi_ufst_server *r, const char *font_file_path, int subfont, FAPI_font *ff, ufst_common_font_data **return_data)
{   ulong area_length = sizeof(ufst_common_font_data), tt_size = 0;
    LPUB8 buf;
    PCLETTO_FHDR *h;
    ufst_common_font_data *d;
    bool use_XL_format = ff->is_mtx_skipped;
    int code;

    *return_data = 0;
    r->fc.ttc_index = subfont;
    if (ff->font_file_path == NULL) {
        area_length += PCLETTOFONTHDRSIZE;
        if (ff->is_type1) {
            int subrs_count  = ff->get_word(ff, FAPI_FONT_FEATURE_Subrs_count, 0);
            int subrs_length = ff->get_long(ff, FAPI_FONT_FEATURE_Subrs_total_size, 0);
            int subrs_area_size = subrs_count * 5 + subrs_length + 2;
            area_length += 360 + subrs_area_size; /* some inprecise - see pack_pseo_fhdr */
        } else {
            tt_size  = ff->get_long(ff, FAPI_FONT_FEATURE_TT_size, 0);
            if (tt_size == 0)
                return e_invalidfont;
            area_length += tt_size + (use_XL_format ? 6 : 4) + 4 + 2;
        }
    } else
        area_length += strlen(font_file_path) + 1;
    buf = r->client_mem.alloc(&r->client_mem, area_length, "ufst font data");
    if (buf == 0)
        return e_VMerror;
    d = (ufst_common_font_data *)buf;
    d->tt_font_body_offset = 0;
    d->platformId = 0;
    d->specificId = 0;
    d->decodingID[0] = 0;
    d->glyphs = 0;
    d->font_id = 0;
    d->is_disk_font = (ff->font_file_path != NULL);
    if (d->is_disk_font) {
        FILE *f = fopen(font_file_path, "rb"); /* note: gp_fopen isn't better since UFST calls fopen. */
        if (f == NULL)
            return e_undefinedfilename;
        memcpy(d + 1, font_file_path, strlen(font_file_path) + 1);
        d->font_type = get_font_type(f);
        fclose(f);
        if (d->font_type == FC_FCO_TYPE) {
            fco_list_elem *e;
            if ((code = fco_open(r, font_file_path, &e)) != 0)
		return code;
            enumerate_fco(r, font_file_path); /* development perpose only */
            d->font_id = (e->fcHandle << 16) | subfont;
        }
    } else {
        d->font_type = (ff->is_type1 ? FC_PST1_TYPE : FC_TT_TYPE);
        h = (PCLETTO_FHDR *)(buf + sizeof(ufst_common_font_data));
        h->fontDescriptorSize = PCLETTOFONTHDRSIZE;
        h->descriptorFormat = 15;
        h->fontType = 11; /* wrong */                /*  3- 11=Unicode; 0,1,2 also possible */
        h->style_msb = 0; /* wrong */               /*  4- from PCLT table in TrueType font */
        h->reserved1 = 0;
        h->baselinePosition = 0; /* wrong */        /*  6- from head table in TT font; = 0 */
        h->cellWidth = 1024; /* wrong */               /*  8- head, use xMax - xMin */
        h->cellHeight = 1024; /* wrong */             /* 10- head, use yMax - yMin */
        h->orientation = 0;             /* 12- 0 */
        h->spacing = 1; /* wrong */                 /* 13- 1=proportional, 0-fixed pitch */
        h->characterSet = 56; /* wrong */            /* 14- same as symSetCode; =56 if unbound. */
        h->pitch = 1024; /* wrong */                   /* 16- PCLT */
        h->height = 0;                  /* 18- 0 if TrueType */
        h->xHeight = 512; /* wrong */                 /* 20- PCLT */
        h->widthType = 0; /* wrong */               /* 22- PCLT */
        h->style_lsb = 0; /* wrong */               /* 23- PCLT */
        h->strokeWeight = 0; /* wrong */            /* 24- PCLT */
        h->typeface_lsb = 0; /* wrong */            /* 25- PCLT */
        h->typeface_msb = 0; /* wrong */           /* 26- PCLT */
        h->serifStyle = 0; /* wrong */              /* 27- PCLT */
        h->quality = 0; /* wrong */                 /* 28- 2 if TrueType */
        h->placement = 0; /* wronfg */               /* 29- 0 if TrueType */
        h->underlinePosition = 0;       /* 30- 0 */
        h->underlineHeight = 0;         /* 31- 0 */
        h->textHeight = 102; /* wrong */              /* 32- from OS/2 table in TT font */
        h->textWidth = 1024; /* wrong */              /* 34- OS/2 */
        h->firstCode = 0;               /* 36- set to 0 if unbound */
        h->lastCode = 255; /* wrong */                /* 38- max number of downloadable chars if unbound */
        h->pitch_ext = 0;               /* 40- 0 if TrueType */
        h->height_ext = 0;              /* 41- 0 if TrueType */
        h->capHeight = 1024; /* wrong */               /* 42- PCLT */
        h->fontNumber = 0; /* wrong */             /* 44- PCLT */
        h->fontName[0] = 0; /* wrong */            /* 48- PCLT */
        h->scaleFactor = 1024; /* wrong */             /* 64- head:unitsPerEm */
        h->masterUnderlinePosition = 0; /* wrong */ /* 66- post table, or -20% of em */
        h->masterUnderlineHeight = 0; /* wrong */   /* 68- post table, or 5% of em */
        h->fontScalingTechnology = 1;   /* 70- 1=TrueType; 0=Intellifont */
        h->variety = 0;                 /* 71- 0 if TrueType */
        memset((LPUB8)h + PCLETTOFONTHDRSIZE, 0 ,8); /* work around bug in PCLswapHdr : it wants format 10 */
        /*  fixme : Most fields above being marked "wrong" look unused by UFST.
            Need to check for sure.
        */
        /*  fixme : This code assumes 1-byte alignment for PCLETTO_FHDR structure.
            Use PACK_* macros to improve.
        */
        PCLswapHdr(&r->IFS, (UB8 *)h, 0);
        if (ff->is_type1) {
            LPUB8 fontdata = (LPUB8)h + PCLETTOFONTHDRSIZE;
            pack_pseo_fhdr(r, ff, fontdata);
        } else {
            LPUB8 pseg = (LPUB8)h + PCLETTOFONTHDRSIZE;
            LPUB8 fontdata = pseg + (use_XL_format ? 6 : 4);
            if (tt_size > 65000)
                return e_unregistered; /* Must not happen because we skept 'glyp', 'loca' and 'cmap'. */
            pseg[0] = 'G';
            pseg[1] = 'T';
	    if (use_XL_format) {
		pseg[2] = tt_size >> 24;
		pseg[3] = (tt_size >> 16) % 256;
		pseg[4] = (tt_size >> 8) % 256;
		pseg[5] = tt_size % 256;
	    } else {
		pseg[2] = tt_size / 256;
		pseg[3] = tt_size % 256;
	    }
            d->tt_font_body_offset = (LPUB8)fontdata - (LPUB8)d;
            if (ff->serialize_tt_font(ff, fontdata, tt_size))
                return e_invalidfont;
            *(fontdata + tt_size    ) = 255;
            *(fontdata + tt_size + 1) = 255;
            *(fontdata + tt_size + 2) = 0;
            *(fontdata + tt_size + 3) = 0;
            *(fontdata + tt_size + 4) = 0;
            *(fontdata + tt_size + 5) = 0;  /* checksum */
        }
    }
    *return_data = d;
    return 0;
}

private void prepare_typeface(fapi_ufst_server *r, ufst_common_font_data *d)
{   r->fc.format = d->font_type;
    r->fc.font_id = d->font_id;
    r->fc.font_hdr = (UB8 *)(d + 1);
    if (!d->is_disk_font)
        r->fc.format |= FC_EXTERN_TYPE;
}

private FAPI_retcode get_scaled_font(FAPI_server *server, FAPI_font *ff, int subfont, 
         const FAPI_font_scale *font_scale, const char *xlatmap, bool bVertical, FAPI_descendant_code dc)
{   fapi_ufst_server *r = If_to_I(server);
    FONTCONTEXT *fc = &r->fc;
    /*  Note : UFST doesn't provide handles for opened fonts,
        but copies FONTCONTEXT to IFSTATE and caches it.
        Due to this the plugin cannot provide a handle for the font.
        This assumes that only one font context is active at a moment.
    */
    ufst_common_font_data *d = (ufst_common_font_data *)ff->server_font_data;
    const double scale = F_ONE;
    double hx, hy, sx, sy;
    FAPI_retcode code;
    bool use_XL_format = ff->is_mtx_skipped;

    if (ff->is_cid && ff->is_type1 && ff->font_file_path == NULL && 
        (dc == FAPI_TOPLEVEL_BEGIN || dc == FAPI_TOPLEVEL_COMPLETE)) {
	/* Don't need any processing for the top level font of a non-disk CIDFontType 0. 
	   See comment in FAPI_prepare_font.
	   Will do with its subfonts individually. 
	 */
	return 0; 
    }
    ff->need_decrypt = 1;
    if (d == 0) {
        if ((code = make_font_data(r, ff->font_file_path, subfont, ff, &d)) != 0)
	    return code;
        ff->server_font_data = d;
        prepare_typeface(r, d);
        if (ff->font_file_path != NULL || ff->is_type1) /* such fonts don't use RAW_GLYPH */
            choose_decoding(r, d, xlatmap);
    } else
        prepare_typeface(r, d);
    r->tran_xx = font_scale->matrix[0] / scale, r->tran_xy = font_scale->matrix[1] / scale;
    r->tran_yx = font_scale->matrix[2] / scale, r->tran_yy = font_scale->matrix[3] / scale;
    hx = hypot(r->tran_xx, r->tran_xy), hy = hypot(r->tran_yx, r->tran_yy);
    sx = r->tran_xx * r->tran_yx + r->tran_xy * r->tran_yy; 
    sy = r->tran_xx * r->tran_yy - r->tran_xy * r->tran_yx;
    fc->xspot     = F_ONE;
    fc->yspot     = F_ONE;
    fc->fc_type   = FC_MAT2_TYPE;
    /* Round towards zero for a better view of mirrored characters : */
    fc->s.m2.m[0] = (int)((double)font_scale->matrix[0] / hx + 0.5); 
    fc->s.m2.m[1] = (int)((double)font_scale->matrix[1] / hx + 0.5);
    fc->s.m2.m[2] = (int)((double)font_scale->matrix[2] / hy + 0.5);
    fc->s.m2.m[3] = (int)((double)font_scale->matrix[3] / hy + 0.5);
    fc->s.m2.matrix_scale = 16;
    fc->s.m2.xworld_res = font_scale->HWResolution[0] >> 16;
    fc->s.m2.yworld_res = font_scale->HWResolution[1] >> 16;
    fc->s.m2.world_scale = 0;
    fc->s.m2.point_size   = (int)(hy * 8 + 0.5); /* 1/8ths of pixels */
    fc->s.m2.set_size     = (int)(hx * 8 + 0.5);
    fc->numXsubpixels = font_scale->subpixels[0];
    fc->numYsubpixels = font_scale->subpixels[1];
    fc->alignment = (font_scale->align_to_pixels ? GAGG : GAPP);
    fc->ssnum = 0x8000; /* no symset mapping */
    if (ff->font_file_path == NULL && !ff->is_type1)
        fc->ssnum = RAW_GLYPH;
    else if (ff->font_file_path != NULL && ff->is_cid) {
         if (d->platformId == 3) {
            switch (d->specificId) {
                case 1 : fc->ssnum = UNICODE;   break;
                case 2 : fc->ssnum = SHIFT_JIS; break;
                case 3 : fc->ssnum = GB;        break;
                case 4 : fc->ssnum = BIG5;      break;
                case 5 : fc->ssnum = WANSUNG;   break;
                case 6 : fc->ssnum = JOHAB;     break;
            }
        } else {
            /* fixme : other platform IDs */
        }
    }
    fc->format      |= FC_NON_Z_WIND;   /* NON_ZERO Winding required for TrueType */
    fc->format      |= FC_INCHES_TYPE;  /* output in units per inch */
    fc->user_platID = d->platformId;
    fc->user_specID = d->specificId;
    fc->ExtndFlags = EF_TT_CMAPTABL;
    if (use_XL_format)
	fc->ExtndFlags |= EF_XLFONT_TYPE;
    if (bVertical)
        fc->ExtndFlags |= EF_UFSTVERT_TYPE;
    fc->dl_ssnum = (d->specificId << 4) | d->platformId;
    fc->ttc_index   = subfont;
    r->callback_error = 0;
    gx_set_UFST_Callbacks(gs_PCLEO_charptr, gs_PCLchId2ptr, gs_PCLglyphID2Ptr);
    code = CGIFfont(&r->IFS, fc);
    if (r->callback_error != 0)
	return r->callback_error;
    return code;
}

private FAPI_retcode get_decodingID(FAPI_server *server, FAPI_font *ff, const char **decodingID_result)
{   fapi_ufst_server *r = If_to_I(server);
    ufst_common_font_data *d = (ufst_common_font_data *)r->fc.font_hdr - 1;

    *decodingID_result = d->decodingID;
    return 0;
}

private FAPI_retcode get_font_bbox(FAPI_server *server, FAPI_font *ff, int BBox[4])
{   fapi_ufst_server *r = If_to_I(server);
    SW16 VLCPower = 0;
    int code;

    if ((code = CGIFbound_box(&r->IFS, BBox, &VLCPower)) < 0)
	return code;
    /*  UFST expands bbox for internal needs, and retrives the expanded bbox.
        We believe it's bug in UFST.
        Now we collapse it back to the correct size :
    */
    BBox[0] += 2;
    BBox[1] += 2;
    BBox[2] -= 2;
    BBox[3] -= 2;
    BBox[0] >>= VLCPower;
    BBox[1] >>= VLCPower;
    BBox[2] >>= VLCPower;
    BBox[3] >>= VLCPower;
    return 0;
}

private FAPI_retcode get_font_proportional_feature(FAPI_server *server, FAPI_font *ff, int subfont, bool *bProportional)
{   fapi_ufst_server *r = If_to_I(server);
    UB8 buf[74];
    UL32 length = sizeof(buf);
    *bProportional = false;

    if (ff->font_file_path == NULL || ff->is_type1)
        return 0;
    if (CGIFtt_query(&r->IFS, (UB8 *)ff->font_file_path, *(UL32 *)"OS/2", (UW16)subfont, &length, buf) != 0)
        return 0; /* No OS/2 table - no chance to get the info. Use default == false. */
    *bProportional = (buf[35] == 9);
    return 0;
}

private inline void make_asciiz_char_name(char *buf, int buf_length, FAPI_char_ref *c)
{   int len = min(buf_length - 1, c->char_name_length);

    memcpy(buf, c->char_name, len);
    buf[len] = 0;
}

#define MAX_CHAR_NAME_LENGTH 30

private FAPI_retcode can_retrieve_char_by_name(FAPI_server *server, FAPI_font *ff, FAPI_char_ref *c, int *result)
{   fapi_ufst_server *r = If_to_I(server);

    *result = 0;
    switch (r->fc.format & FC_FONTTYPE_MASK) {
        case FC_PST1_TYPE : 
            *result = 1; 
            break;
        case FC_TT_TYPE : 
#if 0 /* Doesn't work because Agfa can't retrive characters by name. 
                     It wants a char code together with the name. */
                if (ff->font_file_path != NULL) {
                    UB8 buf[2];
                    UL32 l = sizeof(buf);
                    UW16 code = CGIFtt_query(&r->IFS, r->fc.font_hdr, tag_Postscript, r->fc.ttc_index, &l, buf);
                    *result = (code == 0);
                }
#endif
            break;
    }
    return 0;
}

private FAPI_retcode can_replace_metrics(FAPI_server *server, FAPI_font *ff, FAPI_char_ref *c, int *result)
{   *result = (!ff->is_type1 && ff->font_file_path == NULL &&
	       c->metrics_scale == 0 && c->metrics_type == FAPI_METRICS_REPLACE);
    return 0;
}

private void release_glyphs(fapi_ufst_server *r, ufst_common_font_data *d)
{   while (d->glyphs != 0) {
        pcleo_glyph_list_elem *e = d->glyphs;
        d->glyphs = e->next;
        r->client_mem.free(&r->client_mem, e, "PCLEO char");
    }
}

private FAPI_retcode get_char_width(FAPI_server *server, FAPI_font *ff, FAPI_char_ref *c, FAPI_metrics *metrics)
{   fapi_ufst_server *r = If_to_I(server);
    UW16 buffer[2];
    UW16 cc = (UW16)c->char_code;
    char PSchar_name[MAX_CHAR_NAME_LENGTH];
    int code;

    make_asciiz_char_name(PSchar_name, sizeof(PSchar_name), c);
    r->ff = ff;
    CGIFchIdptr(&r->IFS, &cc, PSchar_name);
    if ((code = CGIFwidth(&r->IFS, cc, 1, 4, buffer)) != 0)
	return code;
    r->ff = 0;
    release_glyphs(r, (ufst_common_font_data *)ff->server_font_data);
    metrics->escapement = buffer[0];
    metrics->em_x = metrics->em_y = buffer[1];
    return 0;
}

private int export_outline(fapi_ufst_server *r, PIFOUTLINE pol, FAPI_path *p)
{   POUTLINE_CHAR outchar;
    SW16 num_contrs,num_segmts;
    LPSB8 segment;
    PINTRVECTOR points;
    SW16  i,j;

    if (pol == NULL)
        return 0;
    p->shift += r->If.frac_shift + pol->VLCpower;
    outchar = &pol->ol;
    num_contrs = outchar->num_loops;
    for(i=0; i<num_contrs; i++) {
     	num_segmts = outchar->loop[i].num_segmts;
        segment = (LPSB8)((LPSB8)(outchar->loop) + outchar->loop[i].segmt_offset);
        points = (PINTRVECTOR)((LPSB8)(outchar->loop) + outchar->loop[i].coord_offset);
        for(j=0; j<num_segmts; j++) {
	    int code;

            if(*segment == 0x00) {
             	if ((code = p->moveto(p, points->x, points->y)) != 0)
		    return code;
                points++;
            } else if (*segment == 0x01) {
		if ((code = p->lineto(p, points->x, points->y)) != 0)
		    return code;
                points++;
            } else if (*segment == 0x02) {
                points+=2;
                return e_invalidfont; /* This must not happen */
            } else if (*segment == 0x03) {
		if ((code = p->curveto(p, points[0].x, points[0].y,
					points[1].x, points[1].y,
					points[2].x, points[2].y)) < 0)
		    return code;
                points+=3;
            } else
                return e_invalidfont; /* This must not happen */
            segment++;
        }
    }
    return 0;
}

private inline void set_metrics(fapi_ufst_server *r, FAPI_metrics *metrics, SL32 design_bbox[4], SW16 design_escapement, int escapement, SW16 du_emx, SW16 du_emy)
{   metrics->escapement = design_escapement; 
    metrics->em_x = du_emx;
    metrics->em_y = du_emy;
    metrics->bbox_x0 = design_bbox[0];
    metrics->bbox_y0 = design_bbox[1];
    metrics->bbox_x1 = design_bbox[2];
    metrics->bbox_y1 = design_bbox[3];
}

private FAPI_retcode get_char(fapi_ufst_server *r, FAPI_font *ff, FAPI_char_ref *c, FAPI_path *p, FAPI_metrics *metrics, UW16 format)
{   UW16 code;
    UW16 cc = (UW16)c->char_code;
    SL32 design_bbox[4];
    SW16 design_escapement;
    char PSchar_name[MAX_CHAR_NAME_LENGTH];
    MEM_HANDLE result;

    memset(metrics, 0, sizeof(*metrics));
    metrics->bbox_x1 = -1;
    make_asciiz_char_name(PSchar_name, sizeof(PSchar_name), c);
    CGIFchIdptr(&r->IFS, &cc, PSchar_name);
    {   /* hack : Changing UFST internal data. Change to r->fc doesn't help, because Agfa thinks that the "outline/raster" is a property of current font. */
        r->IFS.fcCur.format &= ~FC_OUTPUT_MASK;
        r->IFS.fcCur.format |= format;
    }
    r->bRaster = false;
    r->ff = ff;
    r->callback_error = 0;
    r->sb_x = c->sb_x;
    r->aw_x = c->aw_x;
    r->metrics_type = c->metrics_type;
    code = CGIFchar_with_design_bbox(&r->IFS, cc, &result, (SW16)0, design_bbox, &design_escapement);
    if (code == ERR_find_cgnum) {
        /* There is no such char in the font, try the glyph 0 (notdef) : */
        const void *client_char_data = ff->char_data;
        UW16 c1 = 0, ssnum = r->IFS.fcCur.ssnum;
        /* hack : Changing UFST internal data - see above. */
        r->IFS.fcCur.ssnum = RAW_GLYPH;
        r->callback_error = 0;
        ff->char_data = NULL;
        CGIFchIdptr(&r->IFS, &c1, (char *)".notdef");
        code = CGIFchar_with_design_bbox(&r->IFS, c1, &result, (SW16)0, design_bbox, &design_escapement);
        r->IFS.fcCur.ssnum = ssnum;
        ff->char_data = client_char_data;
    }
    r->ff = 0;
    release_glyphs(r, (ufst_common_font_data *)ff->server_font_data);
    if (code != ERR_fixed_space && code != 0)
	return code;
    if (r->callback_error != 0)
	return r->callback_error;
    if (format == FC_BITMAP_TYPE) {
        IFBITMAP *pbm = (IFBITMAP *)result;
        set_metrics(r, metrics, design_bbox, design_escapement, pbm->escapement, pbm->du_emx, pbm->du_emy);
        r->char_data = pbm;
        r->bRaster = true;
    } else {
        IFOUTLINE *pol = (IFOUTLINE *)result;
        set_metrics(r, metrics, design_bbox, design_escapement, pol->escapement, pol->du_emx, pol->du_emy);
        r->char_data = (IFOUTLINE *)result;
    }
    if (code == ERR_fixed_space)
        release_char_data_inline(r);
    return 0;
}

private FAPI_retcode get_char_outline_metrics(FAPI_server *server, FAPI_font *ff, FAPI_char_ref *c, FAPI_metrics *metrics)
{   fapi_ufst_server *r = If_to_I(server);

    release_char_data_inline(r);
    return get_char(r, ff, c, NULL, metrics, FC_CUBIC_TYPE);
    /*	UFST cannot render enough metrics information without generating raster or outline. 
	r->char_data keeps an outline after calling this function.
    */
}

private FAPI_retcode get_char_outline(FAPI_server *server, FAPI_path *p)
{   fapi_ufst_server *r = If_to_I(server);

    return export_outline(r, (IFOUTLINE *)r->char_data, p);
}

private FAPI_retcode get_char_raster_metrics(FAPI_server *server, FAPI_font *ff, FAPI_char_ref *c, FAPI_metrics *metrics)
{   fapi_ufst_server *r = If_to_I(server);
    int code;

    release_char_data_inline(r);
    code = get_char(r, ff, c, NULL, metrics, FC_BITMAP_TYPE);
    if (code == ERR_bm_buff || code == ERR_bm_too_big) /* Too big character ? */
        return e_limitcheck;
    return code;
    /*	UFST cannot render enough metrics information without generating raster or outline. 
	r->char_data keeps a raster after calling this function.
    */
}

private FAPI_retcode get_char_raster(FAPI_server *server, FAPI_raster *rast)
{   fapi_ufst_server *r = If_to_I(server);

    if (!r->bRaster)
        return e_limitcheck;
    else if (r->char_data == NULL) {
	rast->height = rast->width = rast->line_step = 0;
	rast->p = 0;
    } else {
        IFBITMAP *pbm = (IFBITMAP *)r->char_data;
        rast->p = pbm->bm;
        rast->height = pbm->depth;
        rast->width = pbm->width << CHUNK_SHIFT;
        rast->line_step = (pbm->width + (1 << (CHUNK_SHIFT - 3)) - 1) >> (CHUNK_SHIFT - 3);
        if (rast->width != 0) {
            rast->orig_x = pbm->left_indent * 16 + pbm->xorigin;
            rast->orig_y = pbm->top_indent  * 16 + pbm->yorigin;
        } else
            rast->orig_x = rast->orig_y = 0;
    }
    return 0;
}

private FAPI_retcode release_char_data(FAPI_server *server)
{   fapi_ufst_server *r = If_to_I(server);

    release_char_data_inline(r);
    return 0;
}

private void release_fco(fapi_ufst_server *r, SW16 fcHandle)
{   fco_list_elem **e = &r->fco_list;

    for (; *e != 0; )
        if ((*e)->fcHandle == fcHandle && (--(*e)->open_count) == 0) {
            fco_list_elem *ee = *e;
            *e = ee->next;
            CGIFfco_Close(&r->IFS, ee->fcHandle);
            r->client_mem.free(&r->client_mem, ee->file_path, "fco_file_path");
            r->client_mem.free(&r->client_mem, ee, "fco_list_elem");
        } else
            e = &(*e)->next;
}

private FAPI_retcode release_typeface(FAPI_server *server, void *font_data)
{   fapi_ufst_server *r = If_to_I(server);
    ufst_common_font_data *d;
    FAPI_retcode code = 0;

    release_char_data_inline(r);
    if (font_data == 0)
        return 0;
    d = (ufst_common_font_data *)font_data;
    prepare_typeface(r, d);
    if (d->is_disk_font)
        code = CGIFhdr_font_purge(&r->IFS, &r->fc);
    else
        code = CGIFfont_purge(&r->IFS, &r->fc);
    release_glyphs(r, d);
    release_fco(r, (SW16)(d->font_id >> 16));
    r->client_mem.free(&r->client_mem, font_data, "ufst font data");
    return code;
}

/* --------------------- The plugin definition : ------------------------- */


private void gs_fapiufst_finit(i_plugin_instance *instance, i_plugin_client_memory *mem);

private const i_plugin_descriptor ufst_descriptor = {
    "FAPI",
    "AgfaUFST",
    gs_fapiufst_finit
};

private const FAPI_server If0 = {
    {   &ufst_descriptor
    },
    16, /* frac_shift */
    ensure_open,
    get_scaled_font,
    get_decodingID,
    get_font_bbox,
    get_font_proportional_feature,
    can_retrieve_char_by_name,
    can_replace_metrics,
    get_char_width,
    get_char_raster_metrics,
    get_char_raster,
    get_char_outline_metrics,
    get_char_outline,
    release_char_data,
    release_typeface
};

plugin_instantiation_proc(gs_fapiufst_instantiate);      /* check prototype */

int gs_fapiufst_instantiate(i_ctx_t *i_ctx_p, i_plugin_client_memory *client_mem, i_plugin_instance **p_instance)
{   fapi_ufst_server *r = (fapi_ufst_server *)client_mem->alloc(client_mem, sizeof(fapi_ufst_server), "fapi_ufst_server");

    if (r == 0)
        return e_Fatal;
    memset(r, 0, sizeof(*r));
    r->If = If0;
    r->client_mem = *client_mem;
    *p_instance = &r->If.ig;
    return 0;
}

private void gs_fapiufst_finit(i_plugin_instance *this, i_plugin_client_memory *mem)
{   fapi_ufst_server *r = (fapi_ufst_server *)this;

    if (r->If.ig.d != &ufst_descriptor)
        return; /* safety */
    release_char_data_inline(r);
    if (r->bInitialized) {
        CGIFexit(&r->IFS);
    }
    mem->free(mem, r, "fapi_ufst_server");
}


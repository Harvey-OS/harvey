/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpsu.c,v 1.2 2000/09/19 19:00:22 lpd Exp $ */
/* PostScript-writing utilities */
#include "math_.h"
#include "time_.h"
#include "gx.h"
#include "gscdefs.h"
#include "gxdevice.h"
#include "gdevpsu.h"
#include "spprint.h"
#include "stream.h"

/* ---------------- Low level ---------------- */

/* Write a 0-terminated array of strings as lines. */
void
psw_print_lines(FILE *f, const char *const lines[])
{
    int i;

    for (i = 0; lines[i] != 0; ++i)
	fprintf(f, "%s\n", lines[i]);
}

/* Write the ProcSet name. */
private void
psw_put_procset_name(stream *s, const gx_device *dev,
		     const gx_device_pswrite_common_t *pdpc)
{
    pprints1(s, "GS_%s", dev->dname);
    pprintd3(s, "_%d_%d_%d",
	    (int)pdpc->LanguageLevel,
	    (int)(pdpc->LanguageLevel * 10 + 0.5) % 10,
	    pdpc->ProcSet_version);
}
private void
psw_print_procset_name(FILE *f, const gx_device *dev,
		       const gx_device_pswrite_common_t *pdpc)
{
    byte buf[100];		/* arbitrary */
    stream s;

    swrite_file(&s, f, buf, sizeof(buf));
    psw_put_procset_name(&s, dev, pdpc);
    sflush(&s);
}

/* Write a bounding box. */
private void
psw_print_bbox(FILE *f, const gs_rect *pbbox)
{
    fprintf(f, "%%%%BoundingBox: %d %d %d %d\n",
	    (int)floor(pbbox->p.x), (int)floor(pbbox->p.y),
	    (int)ceil(pbbox->q.x), (int)ceil(pbbox->q.y));
    fprintf(f, "%%%%HiResBoundingBox: %f %f %f %f\n",
	    pbbox->p.x, pbbox->p.y, pbbox->q.x, pbbox->q.y);
}

/* ---------------- File level ---------------- */

private const char *const psw_ps_header[] = {
    "%!PS-Adobe-3.0",
    "%%Pages: (atend)",
    0
};

private const char *const psw_eps_header[] = {
    "%!PS-Adobe-3.0 EPSF-3.0",
    0
};

private const char *const psw_begin_prolog[] = {
    "%%EndComments",
    "%%BeginProlog",
    "% This copyright applies to everything between here and the %%EndProlog:",
				/* copyright */
				/* begin ProcSet */
    0
};

private const char *const psw_ps_procset[] = {
	/* <w> <h> <sizename> setpagesize - */
   "/setpagesize{1 index where{pop cvx exec pop pop}{pop /setpagedevice where",
    "{pop 2 array astore 1 dict dup/PageSize 4 -1 roll put setpagedevice}",
    "{pop/setpage where{pop pageparams 3{exch pop}repeat setpage}",
    "{pop pop}ifelse}ifelse}ifelse}bind def",
    0
};

private const char *const psw_end_prolog[] = {
    "end readonly def",
    "%%EndResource",		/* ProcSet */
    "/pagesave null def",	/* establish binding */
    "%%EndProlog",
    0
};

/*
 * Write the file header, up through the BeginProlog.  This must write to a
 * file, not a stream, because it may be called during finalization.
 */
void
psw_begin_file_header(FILE *f, const gx_device *dev, const gs_rect *pbbox,
		      gx_device_pswrite_common_t *pdpc, bool ascii)
{
    psw_print_lines(f, (pdpc->ProduceEPS ? psw_eps_header : psw_ps_header));
    if (pbbox) {
	psw_print_bbox(f, pbbox);
	pdpc->bbox_position = 0;
    } else if (ftell(f) < 0) {	/* File is not seekable. */
	pdpc->bbox_position = -1;
	fputs("%%BoundingBox: (atend)\n", f);
	fputs("%%HiResBoundingBox: (atend)\n", f);
    } else {		/* File is seekable, leave room to rewrite bbox. */
	pdpc->bbox_position = ftell(f);
	fputs("%...............................................................\n", f);
	fputs("%...............................................................\n", f);
    }
    fprintf(f, "%%%%Creator: %s %ld (%s)\n", gs_product, (long)gs_revision,
	    dev->dname);
    {
	time_t t;
	struct tm tms;

	time(&t);
	tms = *localtime(&t);
	fprintf(f, "%%%%CreationDate: %d/%02d/%02d %02d:%02d:%02d\n",
		tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday,
		tms.tm_hour, tms.tm_min, tms.tm_sec);
    }
    if (ascii)
	fputs("%%DocumentData: Clean7Bit\n", f);
    if (pdpc->LanguageLevel >= 2.0)
	fprintf(f, "%%%%LanguageLevel: %d\n", (int)pdpc->LanguageLevel);
    else if (pdpc->LanguageLevel == 1.5)
	fputs("%%Extensions: CMYK\n", f);
    psw_print_lines(f, psw_begin_prolog);
    fprintf(f, "%% %s\n", gs_copyright);
    fputs("%%BeginResource: procset ", f);
    psw_print_procset_name(f, dev, pdpc);
    fputs("\n/", f);
    psw_print_procset_name(f, dev, pdpc);
    fputs(" 80 dict dup begin\n", f);
    psw_print_lines(f, psw_ps_procset);
}

/*
 * End the file header.
 */
void
psw_end_file_header(FILE *f)
{
    psw_print_lines(f, psw_end_prolog);
}

/*
 * End the file.
 */
void
psw_end_file(FILE *f, const gx_device *dev,
	     const gx_device_pswrite_common_t *pdpc, const gs_rect *pbbox)
{
    fprintf(f, "%%%%Trailer\n%%%%Pages: %ld\n", dev->PageCount);
    if (dev->PageCount > 0 && pdpc->bbox_position != 0) {
	if (pdpc->bbox_position >= 0) {
	    long save_pos = ftell(f);

	    fseek(f, pdpc->bbox_position, SEEK_SET);
	    psw_print_bbox(f, pbbox);
	    fputc('%', f);
	    fseek(f, save_pos, SEEK_SET);
	} else
	    psw_print_bbox(f, pbbox);
    }
    if (!pdpc->ProduceEPS)
	fputs("%%EOF\n", f);
}

/* ---------------- Page level ---------------- */

/*
 * Write the page header.
 */
void
psw_write_page_header(stream *s, const gx_device *dev,
		      const gx_device_pswrite_common_t *pdpc,
		      bool do_scale)
{
    long page = dev->PageCount + 1;

    pprintld2(s, "%%%%Page: %ld %ld\n%%%%BeginPageSetup\n", page, page);
    /*
     * Adobe's documentation says that page setup must be placed outside the
     * save/restore that encapsulates the page contents, and that the
     * showpage must be placed after the restore.  This means that to
     * achieve page independence, *every* page's setup code must include a
     * setpagedevice that sets *every* page device parameter that is changed
     * on *any* page.  Currently, the only such parameter relevant to this
     * driver is page size, but there might be more in the future.
     */
    psw_put_procset_name(s, dev, pdpc);
    pputs(s, " begin\n");
    if (!pdpc->ProduceEPS) {
	int width = (int)(dev->width * 72.0 / dev->HWResolution[0] + 0.5);
	int height = (int)(dev->height * 72.0 / dev->HWResolution[1] + 0.5);
	typedef struct ps_ {
	    const char *size_name;
	    int width, height;
	} page_size;
	static const page_size sizes[] = {
	    {"/11x17", 792, 1224},
	    {"/a3", 842, 1190},
	    {"/a4", 595, 842},
	    {"/b5", 501, 709},
	    {"/ledger", 1224, 792},
	    {"/legal", 612, 1008},
	    {"/letter", 612, 792},
	    {"null", 0, 0}
	};
	const page_size *p = sizes;

	while (p->size_name[0] == '/' &&
	       (p->width != width || p->height != height))
	    ++p;
	pprintd2(s, "%d %d ", width, height);
	pprints1(s, "%s setpagesize\n", p->size_name);
    }
    pputs(s, "/pagesave save store 100 dict begin\n");
    if (do_scale)
	pprintg2(s, "%g %g scale\n",
		 72.0 / dev->HWResolution[0], 72.0 / dev->HWResolution[1]);
    pputs(s, "%%EndPageSetup\ngsave mark\n");
}

/*
 * Write the page trailer.  We do this directly to the file, rather than to
 * the stream, because we may have to do it during finalization.
 */
void
psw_write_page_trailer(FILE *f, int num_copies, int flush)
{
    if (num_copies != 1)
	fprintf(f, "userdict /#copies %d put\n", num_copies);
    fprintf(f, "cleartomark end end pagesave restore %s\n%%%%PageTrailer\n",
	    (flush ? "showpage" : "copypage"));
}

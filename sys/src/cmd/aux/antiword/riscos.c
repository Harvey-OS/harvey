/*
 * riscos.c
 * Copyright (C) 2001,2002 A.J. van Os; Released under GPL
 *
 * Description:
 * RISC OS only functions
 */

#include <string.h>
#include "kernel.h"
#include "swis.h"
#include "antiword.h"

#if !defined(DrawFile_Render)
#define DrawFile_Render		0x045540
#endif /* !DrawFile_Render */
#if !defined(JPEG_Info)
#define JPEG_Info		0x049980
#endif /* !JPEG_Info */


/*
 * iGetFiletype
 * This function will get the filetype of the given file.
 * returns the filetype.
 */
int
iGetFiletype(const char *szFilename)
{
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;

	fail(szFilename == NULL || szFilename[0] == '\0');

	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 23;
	regs.r[1] = (int)szFilename;
	e = _kernel_swi(OS_File, &regs, &regs);
	if (e == NULL) {
		return regs.r[6];
	}
	werr(0, "Get Filetype error %d: %s", e->errnum, e->errmess);
	return -1;
} /* end of iGetFiletype */

/*
 * vSetFiletype
 * This procedure will set the filetype of the given file to the given
 * type.
 */
void
vSetFiletype(const char *szFilename, int iFiletype)
{
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;

	fail(szFilename == NULL || szFilename[0] == '\0');

	if (iFiletype < 0x000 || iFiletype > 0xfff) {
		return;
	}
	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 18;
	regs.r[1] = (int)szFilename;
	regs.r[2] = iFiletype;
	e = _kernel_swi(OS_File, &regs, &regs);
	if (e != NULL) {
		switch (e->errnum) {
		case 0x000113:	/* ROM */
		case 0x0104e1:	/* Read-only floppy DOSFS */
		case 0x0108c9:	/* Read-only floppy ADFS */
		case 0x013803:	/* Read-only ArcFS */
		case 0x80344a:	/* CD-ROM */
			break;
		default:
			werr(0, "Set Filetype error %d: %s",
				e->errnum, e->errmess);
			break;
		}
	}
} /* end of vSetFileType */

/*
 * Check if the directory part of the given file exists, make the directory
 * if it does not exist yet.
 * Returns TRUE in case of success, otherwise FALSE.
 */
BOOL
bMakeDirectory(const char *szFilename)
{
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;
	char	*pcLastDot;
	char	szDirectory[PATH_MAX+1];

	DBG_MSG("bMakeDirectory");
	fail(szFilename == NULL || szFilename[0] == '\0');
	DBG_MSG(szFilename);

	if (strlen(szFilename) >= sizeof(szDirectory)) {
		DBG_DEC(strlen(szFilename));
		return FALSE;
	}
	strcpy(szDirectory, szFilename);
	pcLastDot = strrchr(szDirectory, '.');
	if (pcLastDot == NULL) {
		/* No directory equals current directory */
		DBG_MSG("No directory part given");
		return TRUE;
	}
	*pcLastDot = '\0';
	DBG_MSG(szDirectory);
	/* Check if the name exists */
	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 17;
	regs.r[1] = (int)szDirectory;
	e = _kernel_swi(OS_File, &regs, &regs);
	if (e != NULL) {
		werr(0, "Directory check %d: %s", e->errnum, e->errmess);
		return FALSE;
	}
	if (regs.r[0] == 2) {
		/* The name exists and it is a directory */
		DBG_MSG("The directory already exists");
		return TRUE;
	}
	if (regs.r[0] != 0) {
		/* The name exists and it is not a directory */
		DBG_DEC(regs.r[0]);
		return FALSE;
	}
	/* The name does not exist, make the directory */
	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 8;
	regs.r[1] = (int)szDirectory;
	regs.r[4] = 0;
	e = _kernel_swi(OS_File, &regs, &regs);
	if (e != NULL) {
		werr(0, "I can't make a directory %d: %s",
			e->errnum, e->errmess);
		return FALSE;
	}
	return TRUE;
} /* end of bMakeDirectory */

/*
 * iReadCurrentAlphabetNumber
 * This function reads the current Alphabet number.
 * Returns the current Alphabet number when successful, otherwise -1
 */
int
iReadCurrentAlphabetNumber(void)
{
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;

	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 71;
	regs.r[1] = 127;
	e = _kernel_swi(OS_Byte, &regs, &regs);
	if (e == NULL) {
		return regs.r[1];
	}
	werr(0, "Read alphabet error %d: %s", e->errnum, e->errmess);
	return -1;
} /* end of iReadCurrentAlphabetNumber */

/*
 * iGetRiscOsVersion - get the RISC OS version number
 *
 * returns the RISC OS version * 100
 */
int
iGetRiscOsVersion(void)
{
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;

	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 129;
	regs.r[1] = 0;
	regs.r[2] = 0xff;
	e = _kernel_swi(OS_Byte, &regs, &regs);
	if (e != NULL) {
		werr(0, "Read RISC OS version error %d: %s",
			e->errnum, e->errmess);
		return 0;
	}
	switch (regs.r[1]) {
	case 0xa0:	/* Arthur 1.20 */
		return 120;
	case 0xa1:	/* RISC OS 2.00 */
		return 200;
	case 0xa2:	/* RISC OS 2.01 */
		return 201;
	case 0xa3:	/* RISC OS 3.00 */
		return 300;
	case 0xa4:	/* RISC OS 3.1x */
		return 310;
	case 0xa5:	/* RISC OS 3.50 */
		return 350;
	case 0xa6:	/* RISC OS 3.60 */
		return 360;
	case 0xa7:	/* RISC OS 3.7x */
		return 370;
	case 0xa8:	/* RISC OS 4.0x */
		return 400;
	default:
		if (regs.r[1] >= 0xa9 && regs.r[1] <= 0xaf) {
			/* RISC OS 4.10 and up */
			return 410;
		}
		/* Unknown version */
		return 0;
	}
} /* end of iGetRiscOsVersion */

/*
 * Replaces the draw_render_diag function from RISC_OSLib when using
 * RISC OS version 3.60 or higher
 * This function calls a SWI that does not exist in earlier versions
 */
BOOL
bDrawRenderDiag360(draw_diag *pInfo,
	draw_redrawstr *pRedraw, double dScale, draw_error *pError)
{
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;
	int	aiTransform[6];

	fail(pInfo == NULL);
	fail(pRedraw == NULL);
	fail(dScale < 0.01);
	fail(pError == NULL);
	fail(iGetRiscOsVersion() < 360);

	aiTransform[0] = (int)(dScale * 0x10000);
	aiTransform[1] = 0;
	aiTransform[2] = 0;
	aiTransform[3] = (int)(dScale * 0x10000);
	aiTransform[4] = (pRedraw->box.x0 - pRedraw->scx) * 256;
	aiTransform[5] = (pRedraw->box.y1 - pRedraw->scy) * 256;

	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 0;
	regs.r[1] = (int)pInfo->data;
	regs.r[2] = pInfo->length;
	regs.r[3] = (int)aiTransform;
	regs.r[4] = (int)&pRedraw->box;
	regs.r[5] = 0;
	e = _kernel_swi(DrawFile_Render, &regs, &regs);
	if (e == NULL) {
		return TRUE;
	}
	werr(0, "DrawFile render error %d: %s", e->errnum, e->errmess);
	pError->type = DrawOSError;
	pError->err.os.errnum = e->errnum;
	strncpy(pError->err.os.errmess,
		e->errmess,
		sizeof(pError->err.os.errmess) - 1);
	pError->err.os.errmess[sizeof(pError->err.os.errmess) - 1] = '\0';
	return FALSE;
} /* end of bDrawRenderDiag360 */

#if defined(DEBUG)
BOOL
bGetJpegInfo(UCHAR *pucJpeg, size_t tJpegSize)
{
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;

	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 0x00;
	regs.r[1] = (int)pucJpeg;
	regs.r[2] = (int)tJpegSize;
	e = _kernel_swi(JPEG_Info, &regs, &regs);
	if (e == NULL) {
		if (regs.r[0] & BIT(2)) {
			DBG_MSG("Pixel density is a simple ratio");
		} else {
			DBG_MSG("Pixel density is in dpi");
		}
		DBG_DEC(regs.r[4]);
		DBG_DEC(regs.r[5]);
		return TRUE;
	}
	werr(0, "JPEG Info error %d: %s", e->errnum, e->errmess);
	return FALSE;
} /* end of bGetJpegInfo */
#endif /* DEBUG */

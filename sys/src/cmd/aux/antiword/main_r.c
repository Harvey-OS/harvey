/*
 * main_r.c
 *
 * Released under GPL
 *
 * Copyright (C) 1998-2003 A.J. van Os
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Description:
 * The main program of !Antiword (RISC OS version)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "baricon.h"
#include "dbox.h"
#include "event.h"
#include "flex.h"
#include "kernel.h"
#include "menu.h"
#include "res.h"
#include "resspr.h"
#include "wimp.h"
#include "template.h"
#include "wimpt.h"
#include "win.h"
#include "xferrecv.h"
#include "version.h"
#include "antiword.h"

/* The name of this program */
static char	*szTask = "!Antiword";

/* The window handle of the choices window */
static wimp_w	tChoicesWindow;

/* Info box fields */
#define PURPOSE_INFO_FIELD	2
#define AUTHOR_INFO_FIELD	3
#define VERSION_INFO_FIELD	4
#define STATUS_INFO_FIELD	5


static void
vBarInfo(void)
{
	dbox		d;

	d = dbox_new("ProgInfo");
	if (d != NULL) {
		dbox_setfield(d, PURPOSE_INFO_FIELD, PURPOSESTRING);
		dbox_setfield(d, AUTHOR_INFO_FIELD, AUTHORSTRING);
		dbox_setfield(d, VERSION_INFO_FIELD, VERSIONSTRING);
		dbox_setfield(d, STATUS_INFO_FIELD, STATUSSTRING);
		dbox_show(d);
		dbox_fillin(d);
		dbox_dispose(&d);
	}
} /* end of vBarInfo */

static void
vMouseButtonClick(wimp_mousestr *m)
{
	if (m->w == tChoicesWindow) {
		vChoicesMouseClick(m);
		return;
	}
	DBG_DEC(m->w);
} /* end of vMouseButtonClick */

static void
vKeyPressed(int chcode, wimp_caretstr *c)
{
	DBG_MSG("vKeyPressed");

	if (chcode != '\r') {
		wimpt_noerr(wimp_processkey(chcode));
		return;
	}
	if (c->w == tChoicesWindow) {
		vChoicesKeyPressed(c);
	}
} /* end of vKeyPressed */

/*
 * Move the given window to the top of the pile.
 */
static void
vWindowToFront(wimp_w tWindow)
{
	wimp_wstate	tWindowState;

	wimpt_noerr(wimp_get_wind_state(tWindow, &tWindowState));
	tWindowState.o.behind = -1;
	wimpt_noerr(wimp_open_wind(&tWindowState.o));
} /* end of vWindowToFront */

/*
 *
 */
static void
vIconclick(wimp_i tUnused)
{
} /* end of vIconclick */

static void
vSaveSelect(void *pvHandle, char *pcInput)
{
	diagram_type	*pDiag;

	fail(pvHandle == NULL || pcInput == NULL);

	pDiag = (diagram_type *)pvHandle;
	switch (pcInput[0]) {
	case 1:
		vScaleOpenAction(pDiag);
		break;
	case 2:
		vSaveDrawfile(pDiag);
		break;
	case 3:
		vSaveTextfile(pDiag);
		break;
	default:
		DBG_DEC(pcInput[0]);
		break;
	}
} /* end of vMenuSelect */

/*
 * Create the window for the text from the given file
 */
static diagram_type *
pCreateTextWindow(const char *szFilename)
{
	diagram_type	*pDiag;
	menu		pSaveMenu;

	DBG_MSG("pCreateTextWindow");

	fail(szFilename == NULL || szFilename[0] == '\0');

	pDiag = pCreateDiagram(szTask+1, szFilename);
	if (pDiag == NULL) {
		werr(0, "No new diagram object");
		return NULL;
	}
	win_register_event_handler(pDiag->tMainWindow,
				vMainEventHandler, pDiag);
	win_register_event_handler(pDiag->tScaleWindow,
				vScaleEventHandler, pDiag);
	pSaveMenu = menu_new(szTask+1,
		">Scale view,"
		">Save (Drawfile)   F3,"
		">Save (Text only) \213F3");
	if (pSaveMenu == NULL) {
		werr(0, "No new menu object");
		return NULL;
	}
	if (!event_attachmenu(pDiag->tMainWindow,
				pSaveMenu, vSaveSelect, pDiag)) {
		werr(0, "I can't attach to event");
		return NULL;
	}
	/* Set the window title */
	vSetTitle(pDiag);
	return pDiag;
} /* end of pCreateTextWindow */

static void
vProcessFile(const char *szFilename, int iFiletype)
{
	options_type	tOptions;
	FILE		*pFile;
	diagram_type	*pDiag;
	long		lFilesize;
	int		iWordVersion;

	fail(szFilename == NULL || szFilename[0] == '\0');

	DBG_MSG(szFilename);

	pFile = fopen(szFilename, "rb");
	if (pFile == NULL) {
		werr(0, "I can't open '%s' for reading", szFilename);
		return;
	}

	lFilesize = lGetFilesize(szFilename);
	if (lFilesize < 0) {
		(void)fclose(pFile);
		werr(0, "I can't get the size of '%s'", szFilename);
		return;
	}

	iWordVersion = iGuessVersionNumber(pFile, lFilesize);
	if (iWordVersion < 0 || iWordVersion == 3) {
		if (bIsRtfFile(pFile)) {
			werr(0, "%s is not a Word Document."
				" It is probably a Rich Text Format file",
				szFilename);
		} if (bIsWordPerfectFile(pFile)) {
			werr(0, "%s is not a Word Document."
				" It is probably a Word Perfect file",
				szFilename);
		} else {
			werr(0, "%s is not a Word Document.", szFilename);
		}
		(void)fclose(pFile);
		return;
	}
	/* Reset any reading done during file testing */
	rewind(pFile);

	if (iFiletype != FILETYPE_MSWORD) {
		vGetOptions(&tOptions);
		if (tOptions.bAutofiletypeAllowed) {
			vSetFiletype(szFilename, FILETYPE_MSWORD);
		}
	}

	pDiag = pCreateTextWindow(szFilename);
	if (pDiag == NULL) {
		(void)fclose(pFile);
		return;
	}

	(void)bWordDecryptor(pFile, lFilesize, pDiag);
	if (bVerifyDiagram(pDiag)) {
		vShowDiagram(pDiag);
	}

	(void)fclose(pFile);
} /* end of vProcessFile */

static void
vProcessDraggedFile(void)
{
	char	*szTmp;
	int	iFiletype;
	char	szFilename[PATH_MAX+1];

	iFiletype = xferrecv_checkinsert(&szTmp);
	if (iFiletype == -1) {
		werr(0, "I failed to import a file");
		return;
	}
	DBG_HEX(iFiletype);
	if (strlen(szTmp) >= sizeof(szFilename)) {
		werr(1, "Internal error: filename too long");
	}
	(void)strcpy(szFilename, szTmp);
	DBG_MSG(szFilename);
	vProcessFile(szFilename, iFiletype);
	xferrecv_insertfileok();
} /* end of vProcessDraggedFile */

static void
vEventHandler(wimp_eventstr *pEvent, void *pvUnused)
{
	switch (pEvent->e) {
	case wimp_ENULL:
		break;
	case wimp_EREDRAW:
		/* handled by the WIMP */
		break;
	case wimp_EOPEN:
		wimpt_noerr(wimp_open_wind(&pEvent->data.o));
		break;
	case wimp_ECLOSE:
		wimpt_noerr(wimp_close_wind(pEvent->data.o.w));
		break;
	case wimp_EBUT:
		vMouseButtonClick(&pEvent->data.but.m);
		break;
	case wimp_EKEY:
		vKeyPressed(pEvent->data.key.chcode, &pEvent->data.key.c);
		break;
	case wimp_ESEND:
	case wimp_ESENDWANTACK:
		switch (pEvent->data.msg.hdr.action) {
		case wimp_MCLOSEDOWN:
			exit(EXIT_SUCCESS);
			break;
		case wimp_MDATALOAD:
		case wimp_MDATAOPEN:
			vProcessDraggedFile();
			break;
		}
	}
} /* end of vEventHandler */

static void
vMenuSelect(void *pvUnused, char *Input)
{
	switch (*Input) {
	case 1:
		vBarInfo();
		break;
	case 2:
		vChoicesOpenAction(tChoicesWindow);
		vWindowToFront(tChoicesWindow);
		break;
	case 3:
		exit(EXIT_SUCCESS);
		break;
	default:
		break;
	}
} /* end of vMenuSelect */

static void
vTemplates(void)
{
	wimp_wind	*pw;

	pw = template_syshandle("Choices");
	if (pw == NULL) {
		werr(1, "Template 'Choices' can't be found");
	}
	wimpt_noerr(wimp_create_wind(pw, &tChoicesWindow));
	win_register_event_handler(tChoicesWindow, vEventHandler, NULL);
} /* end of vTemplates */

static void
vInitialise(void)
{
	menu	pBarMenu;

	(void)wimpt_init(szTask+1);
	res_init(szTask+1);
	template_init();
	dbox_init();
	flex_init();
	_kernel_register_slotextend(flex_budge);
	drawfobj_init();
	vTemplates();
	pBarMenu = menu_new(szTask+1, ">Info,Choices...,Quit");
	if (pBarMenu == NULL) {
		werr(1, "I can't initialise (menu_new)");
	}
	baricon(szTask, (int)resspr_area(), vIconclick);
	if (!event_attachmenu(win_ICONBAR, pBarMenu, vMenuSelect, NULL)) {
		werr(1, "I can't initialise (event_attachmenu)");
	}
	win_register_event_handler(win_ICONBARLOAD, vEventHandler, NULL);
} /* end of vInitialise */

int
main(int argc, char **argv)
{
	int	iFirst, iFiletype;

	vInitialise();
	iFirst = iReadOptions(argc, argv);
	if (iFirst != 1) {
		return EXIT_FAILURE;
	}

	if (argc > 1) {
		iFiletype = iGetFiletype(argv[1]);
		if (iFiletype < 0) {
			return EXIT_FAILURE;
		}
		vProcessFile(argv[1], iFiletype);
	}

	event_setmask(wimp_EMNULL|wimp_EMPTRENTER|wimp_EMPTRLEAVE);
	for (;;) {
		event_process();
	}
} /* end of main */
